#include <gdal_priv.h>
#include <cpl_conv.h>

#include <array>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#define POPEN _popen
#define PCLOSE _pclose
#else
#include <sys/wait.h>
#define POPEN popen
#define PCLOSE pclose
#endif

namespace fs = std::filesystem;

namespace Keys {
static const char* slp = "slp";
static const char* ang = "ang";
static const char* sca = "sca";
static const char* calpar = "calpar";
static const char* cal = "cal";
static const char* si = "si";
static const char* sat = "sat";
static const char* minRecharge = "minimumterrainrecharge";
static const char* maxRecharge = "maximumterrainrecharge";
static const char* gravity = "g";
static const char* rhow = "rhow";
static const char* tempDir = "temporary_output_files_directory";
static const char* deleteIntermediates = "is_delete_intermediate_output_files";
}  // namespace Keys

namespace Intermediates {
static const char* weightMin = "weightmin.tif";
static const char* weightMax = "weightmax.tif";
static const char* scaMin = "scamin.tif";
static const char* scaMax = "scamax.tif";
static const char* control = "Si_Control.txt";
}  // namespace Intermediates

static std::string Trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::string Quote(const std::string& s) {
    return "\"" + s + "\"";
}

static bool IsNumeric(const std::string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    errno = 0;
    std::strtod(s.c_str(), &end);
    return errno == 0 && end && *end == '\0';
}

static std::string ResolveInputPath(const std::string& p) {
    if (p.empty()) return p;
    fs::path path(p);
    if (path.has_parent_path()) return p;
    return (fs::current_path() / path).string();
}

static std::map<std::string, std::string> DefaultParams() {
    return {
        {Keys::slp, ""},
        {Keys::ang, ""},
        {Keys::sca, ""},
        {Keys::calpar, ""},
        {Keys::cal, ""},
        {Keys::si, ""},
        {Keys::sat, ""},
        {Keys::minRecharge, ""},
        {Keys::maxRecharge, ""},
        {Keys::gravity, "9.81"},
        {Keys::rhow, "1000"},
        {Keys::tempDir, ""},
        {Keys::deleteIntermediates, "True"},
    };
}

static void ParseControlFile(const std::string& controlFile, std::map<std::string, std::string>& p) {
    std::ifstream in(controlFile);
    if (!in.is_open()) {
        throw std::runtime_error("Input control file cannot be opened: " + controlFile);
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid control file format at line " + std::to_string(lineNo));
        }

        const std::string key = Trim(line.substr(0, pos));
        const std::string value = Trim(line.substr(pos + 1));

        if (p.find(key) == p.end()) {
            throw std::runtime_error("Invalid parameter name in control file: " + key);
        }
        p[key] = value;
    }
}

static void EnsureDatasetOpenable(const std::string& path) {
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_ReadOnly));
    if (!ds) {
        throw std::runtime_error("Unable to open raster file: " + path);
    }
    GDALClose(ds);
}

static void ValidateParams(const std::string& controlFile, const std::map<std::string, std::string>& p) {
    const std::array<const char*, 9> required = {
        Keys::slp, Keys::sca, Keys::calpar, Keys::cal, Keys::si,
        Keys::sat, Keys::minRecharge, Keys::maxRecharge, Keys::tempDir};

    for (const char* k : required) {
        if (p.at(k).empty()) {
            throw std::runtime_error("Invalid input control file (" + controlFile + "): missing value for " + k);
        }
    }

    for (const char* k : {Keys::minRecharge, Keys::maxRecharge, Keys::gravity, Keys::rhow}) {
        if (!IsNumeric(p.at(k))) {
            throw std::runtime_error("Invalid input control file (" + controlFile + "): parameter " + std::string(k) + " must be numeric.");
        }
    }

    if (!(p.at(Keys::deleteIntermediates) == "True" || p.at(Keys::deleteIntermediates) == "False")) {
        throw std::runtime_error("Invalid value for is_delete_intermediate_output_files. Must be True or False.");
    }

    fs::path tempDir(p.at(Keys::tempDir));
    if (!fs::is_directory(tempDir)) {
        throw std::runtime_error("Temporary output directory is not a directory: " + p.at(Keys::tempDir));
    }

    for (const char* k : {Keys::ang, Keys::calpar, Keys::sca, Keys::slp, Keys::cal}) {
        if (std::string(k) == Keys::ang && p.at(Keys::ang).empty()) continue;
        const std::string resolved = ResolveInputPath(p.at(k));
        if (!fs::exists(resolved)) {
            throw std::runtime_error("Input file not found for " + std::string(k) + ": " + p.at(k));
        }
        if (std::string(k) == Keys::ang || std::string(k) == Keys::sca || std::string(k) == Keys::slp || std::string(k) == Keys::cal) {
            EnsureDatasetOpenable(resolved);
        }
    }

    for (const char* k : {Keys::si, Keys::sat}) {
        fs::path outPath(p.at(k));
        fs::path parent = outPath.parent_path();
        if (!parent.empty() && !fs::exists(parent)) {
            throw std::runtime_error("Output directory does not exist for " + std::string(k) + ": " + parent.string());
        }
    }
}

static void CreateZeroRasterLike(const std::string& baseRasterPath, const std::string& outPath) {
    GDALDataset* base = static_cast<GDALDataset*>(GDALOpen(baseRasterPath.c_str(), GA_ReadOnly));
    if (!base) throw std::runtime_error("Unable to open base raster: " + baseRasterPath);

    GDALDriver* drv = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!drv) {
        GDALClose(base);
        throw std::runtime_error("GTiff driver not available.");
    }

    if (fs::exists(outPath)) fs::remove(outPath);

    GDALDataset* out = drv->Create(outPath.c_str(), base->GetRasterXSize(), base->GetRasterYSize(), 1, GDT_Float32, nullptr);
    if (!out) {
        GDALClose(base);
        throw std::runtime_error("Unable to create raster: " + outPath);
    }

    double gt[6] = {0};
    if (base->GetGeoTransform(gt) == CE_None) {
        out->SetGeoTransform(gt);
    }
    const char* proj = base->GetProjectionRef();
    if (proj && *proj) out->SetProjection(proj);

    GDALRasterBand* b = out->GetRasterBand(1);
    b->SetNoDataValue(-9999);

    const int cols = out->GetRasterXSize();
    const int rows = out->GetRasterYSize();
    std::vector<float> row(cols, 0.0f);
    for (int y = 0; y < rows; ++y) {
        if (b->RasterIO(GF_Write, 0, y, cols, 1, row.data(), cols, 1, GDT_Float32, 0, 0, nullptr) != CE_None) {
            GDALClose(out);
            GDALClose(base);
            throw std::runtime_error("Failed writing raster: " + outPath);
        }
    }

    out->FlushCache();
    GDALClose(out);
    GDALClose(base);
}

static int RunCommandAndLog(const std::string& cmd) {
    std::cout << cmd << std::endl;
    std::string shellCmd = cmd + " 2>&1";
    FILE* pipe = POPEN(shellCmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to start command: " + cmd);
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::cout << buffer;
    }

    int status = PCLOSE(pipe);
#ifdef _WIN32
    return status;
#else
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return status;
#endif
}

static void DeleteIntermediates(const std::map<std::string, std::string>& p) {
    const fs::path dir(p.at(Keys::tempDir));
    for (const char* f : {Intermediates::weightMin, Intermediates::weightMax, Intermediates::scaMin, Intermediates::scaMax, Intermediates::control}) {
        fs::path target = dir / f;
        if (fs::is_regular_file(target)) {
            std::error_code ec;
            fs::remove(target, ec);
        }
    }
}

static std::string ResolveSiblingTool(const fs::path& exeDir, const std::string& toolBaseName) {
    fs::path p = exeDir / toolBaseName;
    if (fs::exists(p)) return p.string();
#ifdef _WIN32
    fs::path pe = exeDir / (toolBaseName + ".exe");
    if (fs::exists(pe)) return pe.string();
#endif
    return toolBaseName;
}

static std::string MaybeQuoteExe(const std::string& exe) {
    // For bare PATH commands (e.g., SinmapSI), do not quote; cmd.exe lookup
    // is more reliable this way on Windows. Quote only explicit paths.
    if (exe.find(' ') != std::string::npos ||
        exe.find('/') != std::string::npos ||
        exe.find('\\') != std::string::npos) {
        return Quote(exe);
    }
    return exe;
}

int main(int argc, char** argv) {
    try {
        if (argc < 3 || std::string(argv[1]) != "--params") {
            std::cerr << "Usage: " << argv[0] << " --params <Si_Control.txt>" << std::endl;
            return 1;
        }

        GDALAllRegister();

        const std::string controlFile = argv[2];
        auto p = DefaultParams();
        ParseControlFile(controlFile, p);
        ValidateParams(controlFile, p);

        const fs::path exeDir = fs::absolute(argv[0]).parent_path();
#ifdef _WIN32
        const std::string areadinfExe = ResolveSiblingTool(exeDir, "Areadinf");
        const std::string sinmapsiExe = ResolveSiblingTool(exeDir, "SinmapSI");
#else
        const std::string areadinfExe = ResolveSiblingTool(exeDir, "areadinf");
        const std::string sinmapsiExe = ResolveSiblingTool(exeDir, "sinmapsi");
#endif

        const bool hasAng = !p[Keys::ang].empty();
        const fs::path tempDir(p[Keys::tempDir]);

        if (hasAng) {
            const std::string wMin = (tempDir / Intermediates::weightMin).string();
            const std::string wMax = (tempDir / Intermediates::weightMax).string();
            const std::string scaMin = (tempDir / Intermediates::scaMin).string();
            const std::string scaMax = (tempDir / Intermediates::scaMax).string();

            CreateZeroRasterLike(p[Keys::slp], wMin);
            CreateZeroRasterLike(p[Keys::slp], wMax);

            std::cout << "Areadinf started:" << std::endl;
            int rc = RunCommandAndLog(MaybeQuoteExe(areadinfExe) + " -ang " + Quote(p[Keys::ang]) + " -wg " + Quote(wMin) + " -sca " + Quote(scaMin));
            if (rc != 0) throw std::runtime_error("Areadinf failed with return code: " + std::to_string(rc));

            std::cout << "Areadinf started:" << std::endl;
            rc = RunCommandAndLog(MaybeQuoteExe(areadinfExe) + " -ang " + Quote(p[Keys::ang]) + " -wg " + Quote(wMax) + " -sca " + Quote(scaMax));
            if (rc != 0) throw std::runtime_error("Areadinf failed with return code: " + std::to_string(rc));
        }

        std::string sinmapCmd =
            MaybeQuoteExe(sinmapsiExe) +
            " -slp " + Quote(p[Keys::slp]) +
            " -sca " + Quote(p[Keys::sca]) +
            " -calpar " + Quote(p[Keys::calpar]) +
            " -cal " + Quote(p[Keys::cal]) +
            " -si " + Quote(p[Keys::si]) +
            " -sat " + Quote(p[Keys::sat]) +
            " -par " + p[Keys::minRecharge] + " " + p[Keys::maxRecharge] + " " + p[Keys::gravity] + " " + p[Keys::rhow];

        if (hasAng) {
            sinmapCmd += " -scamin " + Quote((tempDir / Intermediates::scaMin).string()) +
                         " -scamax " + Quote((tempDir / Intermediates::scaMax).string());
        }

        std::cout << "SinmapSI started:" << std::endl;
        int rc = RunCommandAndLog(sinmapCmd);
        if (rc != 0) throw std::runtime_error("SinmapSI failed with return code: " + std::to_string(rc));

        if (p[Keys::deleteIntermediates] == "True") {
            DeleteIntermediates(p);
        }

        std::cout << "done...." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Combined stability index computation failed." << std::endl;
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}
