#include <gdal_priv.h>
#include <gdal_alg.h>
#include <gdalwarper.h>
#include <ogrsf_frmts.h>
#include <cpl_conv.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct Args {
    std::string dem;
    std::string parreg;
    std::string att;
    std::string parregIn;
    std::string shp;
    std::string shpAttName = "ID";

    std::string attTmin = "2.708";
    std::string attTmax = "2.708";
    std::string attCmin = "0.0";
    std::string attCmax = "0.25";
    std::string attPhimin = "30.0";
    std::string attPhimax = "45.0";
    std::string attSoildens = "2000.0";
};

static void PrintUsage(const char* prog) {
    std::cout
        << "Usage: " << prog << " -dem <dem.tif> -parreg <out_parreg.tif> -att <out_att.txt> [options]\n"
        << "Options:\n"
        << "  -parreg-in <region.tif>         Input region raster (mutually exclusive with -shp)\n"
        << "  -shp <regions.shp>              Input region polygon feature class\n"
        << "  -shp-att-name <field>           Field used during rasterization (default: ID)\n"
        << "  -att-tmin <v>                   Default: 2.708\n"
        << "  -att-tmax <v>                   Default: 2.708\n"
        << "  -att-cmin <v>                   Default: 0.0\n"
        << "  -att-cmax <v>                   Default: 0.25\n"
        << "  -att-phimin <v>                 Default: 30.0\n"
        << "  -att-phimax <v>                 Default: 45.0\n"
        << "  -att-soildens <v>               Default: 2000.0\n";
}

static bool StartsWith(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

static std::string RequireValue(int& i, int argc, char** argv, const std::string& opt) {
    if (i + 1 >= argc) {
        throw std::runtime_error("Missing value for option: " + opt);
    }
    ++i;
    return argv[i];
}

static Args ParseArgs(int argc, char** argv) {
    Args args;

    if (argc <= 1) {
        PrintUsage(argv[0]);
        throw std::runtime_error("No arguments provided.");
    }

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];

        if (a == "-h" || a == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        }

        auto parseLong = [&](const std::string& key, std::string& out) -> bool {
            if (a == key) {
                out = RequireValue(i, argc, argv, key);
                return true;
            }
            const std::string keyEq = key + "=";
            if (StartsWith(a, keyEq)) {
                out = a.substr(keyEq.size());
                return true;
            }
            return false;
        };

        if (parseLong("-dem", args.dem)) continue;
        if (parseLong("-parreg", args.parreg)) continue;
        if (parseLong("-att", args.att)) continue;
        if (parseLong("-parreg-in", args.parregIn)) continue;
        if (parseLong("-shp", args.shp)) continue;
        if (parseLong("-shp-att-name", args.shpAttName)) continue;

        if (parseLong("-att-tmin", args.attTmin)) continue;
        if (parseLong("-att-tmax", args.attTmax)) continue;
        if (parseLong("-att-cmin", args.attCmin)) continue;
        if (parseLong("-att-cmax", args.attCmax)) continue;
        if (parseLong("-att-phimin", args.attPhimin)) continue;
        if (parseLong("-att-phimax", args.attPhimax)) continue;
        if (parseLong("-att-soildens", args.attSoildens)) continue;

        throw std::runtime_error("Unknown argument: " + a);
    }

    if (args.dem.empty()) throw std::runtime_error("Required option missing: -dem");
    if (args.parreg.empty()) throw std::runtime_error("Required option missing: -parreg");
    if (args.att.empty()) throw std::runtime_error("Required option missing: -att");

    return args;
}

static bool PathParentExists(const std::string& p) {
    fs::path pp(p);
    fs::path parent = pp.parent_path();
    if (parent.empty()) {
        return true;
    }
    return fs::exists(parent);
}

static GDALDataset* OpenRasterOrThrow(const std::string& path) {
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_ReadOnly));
    if (!ds) {
        throw std::runtime_error("File open error. Not a valid file (" + path + ")");
    }
    return ds;
}

static bool GetBandNoDataValue(GDALDataset* ds, double& noData) {
    if (!ds) {
        return false;
    }

    GDALRasterBand* band = ds->GetRasterBand(1);
    int hasNoData = FALSE;
    noData = band ? band->GetNoDataValue(&hasNoData) : 0.0;
    return hasNoData != 0;
}

static void ValidateArgs(const Args& args) {
    GDALDataset* dem = OpenRasterOrThrow(args.dem);
    GDALClose(dem);

    if (!args.parregIn.empty()) {
        GDALDataset* ds = OpenRasterOrThrow(args.parregIn);
        GDALRasterBand* b = ds->GetRasterBand(1);
        if (!b) {
            GDALClose(ds);
            throw std::runtime_error("Invalid '-parreg-in' file: missing band 1.");
        }

        const GDALDataType dt = b->GetRasterDataType();
        if (!(dt == GDT_Byte || dt == GDT_UInt16 || dt == GDT_UInt32 || dt == GDT_Int32)) {
            GDALClose(ds);
            throw std::runtime_error("Not a valid file (" + args.parregIn + ") provided for '-parreg-in'. Data type must be integer (Byte/UInt16/UInt32/Int32).");
        }
        GDALClose(ds);
    }

    if (!args.parregIn.empty() && !args.shp.empty()) {
        throw std::runtime_error("Either '-parreg-in' or '-shp' should be provided, but not both.");
    }

    if (!args.shp.empty()) {
        if (args.shpAttName.empty()) {
            throw std::runtime_error("'-shp-att-name' is required when '-shp' is provided.");
        }
        if (args.shpAttName == "FID") {
            throw std::runtime_error("'FID' is an invalid shape file attribute for calibration region calculation.");
        }

        GDALDataset* vds = static_cast<GDALDataset*>(GDALOpenEx(args.shp.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
        if (!vds) {
            throw std::runtime_error("Not a valid shape file (" + args.shp + ") provided for '-shp'.");
        }

        OGRLayer* layer = vds->GetLayer(0);
        if (!layer) {
            GDALClose(vds);
            throw std::runtime_error("Invalid shapefile: no layer found.");
        }

        OGRFeatureDefn* defn = layer->GetLayerDefn();
        if (!defn || defn->GetFieldIndex(args.shpAttName.c_str()) < 0) {
            GDALClose(vds);
            throw std::runtime_error("Invalid shapefile. Attribute '" + args.shpAttName + "' is missing.");
        }

        GDALClose(vds);
    }

    if (!PathParentExists(args.parreg)) {
        throw std::runtime_error("File path for '-parreg' does not exist.");
    }
    if (!PathParentExists(args.att)) {
        throw std::runtime_error("File path for '-att' does not exist.");
    }
}

static void CopyDEMGeometry(GDALDataset* dem, GDALDataset* out) {
    double gt[6] = {0, 1, 0, 0, 0, -1};
    if (dem->GetGeoTransform(gt) == CE_None) {
        out->SetGeoTransform(gt);
    }

    const char* proj = dem->GetProjectionRef();
    if (proj && *proj) {
        out->SetProjection(proj);
    }
}

static void CreateConstantRegionRaster(const std::string& demPath, const std::string& outPath) {
    GDALDataset* dem = OpenRasterOrThrow(demPath);

    double demNoData = 0.0;
    const bool hasDemNoData = GetBandNoDataValue(dem, demNoData);

    GDALDriver* drv = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!drv) {
        GDALClose(dem);
        throw std::runtime_error("GDAL GTiff driver not available.");
    }

    const int cols = dem->GetRasterXSize();
    const int rows = dem->GetRasterYSize();

    GDALDataset* out = drv->Create(outPath.c_str(), cols, rows, 1, GDT_Int32, nullptr);
    if (!out) {
        GDALClose(dem);
        throw std::runtime_error("Failed to create output raster: " + outPath);
    }

    CopyDEMGeometry(dem, out);

    GDALRasterBand* band = out->GetRasterBand(1);
    if (band && hasDemNoData) {
        band->SetNoDataValue(demNoData);
    }

    std::vector<int32_t> row(cols, 1);
    for (int y = 0; y < rows; ++y) {
        if (band->RasterIO(GF_Write, 0, y, cols, 1, row.data(), cols, 1, GDT_Int32, 0, 0, nullptr) != CE_None) {
            GDALClose(out);
            GDALClose(dem);
            throw std::runtime_error("Failed writing constant region raster.");
        }
    }

    out->FlushCache();
    GDALClose(out);
    GDALClose(dem);
}

static GDALDataset* CreateMemLikeDEM(const std::string& demPath, GDALDataType dt) {
    GDALDataset* dem = OpenRasterOrThrow(demPath);

    double demNoData = 0.0;
    const bool hasDemNoData = GetBandNoDataValue(dem, demNoData);

    GDALDriver* memDrv = GetGDALDriverManager()->GetDriverByName("MEM");
    if (!memDrv) {
        GDALClose(dem);
        throw std::runtime_error("GDAL MEM driver not available.");
    }

    GDALDataset* mem = memDrv->Create("", dem->GetRasterXSize(), dem->GetRasterYSize(), 1, dt, nullptr);
    if (!mem) {
        GDALClose(dem);
        throw std::runtime_error("Failed creating in-memory raster.");
    }

    CopyDEMGeometry(dem, mem);
    GDALRasterBand* b = mem->GetRasterBand(1);
    if (b && hasDemNoData) {
        b->SetNoDataValue(demNoData);
        b->Fill(demNoData);
    }

    GDALClose(dem);
    return mem;
}

static void RasterizeShapefileToRegion(const Args& args) {
    GDALDataset* mem = CreateMemLikeDEM(args.dem, GDT_Int32);

    GDALDataset* vds = static_cast<GDALDataset*>(GDALOpenEx(args.shp.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
    if (!vds) {
        GDALClose(mem);
        throw std::runtime_error("Failed opening shapefile for rasterization: " + args.shp);
    }

    OGRLayer* layer = vds->GetLayer(0);
    if (!layer) {
        GDALClose(vds);
        GDALClose(mem);
        throw std::runtime_error("Shapefile has no layers: " + args.shp);
    }

    char* opt = CPLStrdup((std::string("ATTRIBUTE=") + args.shpAttName).c_str());
    char** opts = nullptr;
    opts = CSLAddString(opts, opt);
    CPLFree(opt);

    int bands[1] = {1};
    OGRLayerH hLayer = reinterpret_cast<OGRLayerH>(layer);
    const int err = GDALRasterizeLayers(mem, 1, bands, 1, &hLayer, nullptr, nullptr, nullptr, opts, nullptr, nullptr);
    CSLDestroy(opts);

    if (err != CE_None) {
        GDALClose(vds);
        GDALClose(mem);
        throw std::runtime_error("Rasterization failed.");
    }

    GDALDriver* tifDrv = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!tifDrv) {
        GDALClose(vds);
        GDALClose(mem);
        throw std::runtime_error("GDAL GTiff driver not available.");
    }

    GDALDataset* out = tifDrv->CreateCopy(args.parreg.c_str(), mem, FALSE, nullptr, nullptr, nullptr);
    if (!out) {
        GDALClose(vds);
        GDALClose(mem);
        throw std::runtime_error("Failed writing output raster: " + args.parreg);
    }

    GDALClose(out);
    GDALClose(vds);
    GDALClose(mem);
}

static void ResampleParregInToDEM(const Args& args) {
    GDALDataset* dem = OpenRasterOrThrow(args.dem);
    GDALDataset* src = OpenRasterOrThrow(args.parregIn);

    GDALDriver* tifDrv = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!tifDrv) {
        GDALClose(src);
        GDALClose(dem);
        throw std::runtime_error("GDAL GTiff driver not available.");
    }

    char** co = nullptr;
    co = CSLAddString(co, "COMPRESS=LZW");
    co = CSLAddString(co, "TILED=YES");
    co = CSLAddString(co, "BLOCKXSIZE=256");
    co = CSLAddString(co, "BLOCKYSIZE=256");
    co = CSLAddString(co, "BIGTIFF=YES");

    GDALDataset* dst = tifDrv->Create(args.parreg.c_str(), dem->GetRasterXSize(), dem->GetRasterYSize(), 1, GDT_Int32, co);
    CSLDestroy(co);
    if (!dst) {
        GDALClose(src);
        GDALClose(dem);
        throw std::runtime_error("Failed creating output raster: " + args.parreg);
    }

    CopyDEMGeometry(dem, dst);

    double srcNoData = 0.0;
    bool hasSrcNoData = GetBandNoDataValue(src, srcNoData);
    if (!hasSrcNoData) {
        GDALDataset* demNoDataDs = dem;
        hasSrcNoData = GetBandNoDataValue(demNoDataDs, srcNoData);
    }

    GDALRasterBand* dstBand = dst->GetRasterBand(1);
    if (dstBand && hasSrcNoData) {
        dstBand->SetNoDataValue(srcNoData);
        dstBand->Fill(srcNoData);
    }

    CPLErr reprojectErr = GDALReprojectImage(
        src,
        src->GetProjectionRef(),
        dst,
        dst->GetProjectionRef(),
        GRA_NearestNeighbour,
        0.0,
        0.0,
        nullptr,
        nullptr,
        nullptr);

    if (reprojectErr != CE_None) {
        GDALClose(dst);
        GDALClose(src);
        GDALClose(dem);
        throw std::runtime_error("Failed to warp/resample input region grid to match DEM.");
    }

    dst->FlushCache();
    GDALClose(dst);
    GDALClose(src);
    GDALClose(dem);

    if (!fs::exists(args.parreg)) {
        throw std::runtime_error("Failed to save output raster file to disk: " + args.parreg);
    }
}

static void CreateParameterRegionGrid(const Args& args) {
    if (fs::exists(args.parreg)) {
        fs::remove(args.parreg);
    }

    if (args.shp.empty() && args.parregIn.empty()) {
        CreateConstantRegionRaster(args.dem, args.parreg);
    } else if (!args.shp.empty()) {
        RasterizeShapefileToRegion(args);
    } else {
        ResampleParregInToDEM(args);
    }
}

static std::set<int32_t> CollectParamIds(const std::string& parregPath) {
    std::set<int32_t> ids;
    GDALDataset* ds = OpenRasterOrThrow(parregPath);

    GDALRasterBand* band = ds->GetRasterBand(1);
    if (!band) {
        GDALClose(ds);
        throw std::runtime_error("Parameter region raster has no band 1.");
    }

    int hasNoData = FALSE;
    const double noData = band->GetNoDataValue(&hasNoData);

    const int cols = band->GetXSize();
    const int rows = band->GetYSize();
    if (band->GetRasterDataType() == GDT_Int32) {
        std::vector<int32_t> row(cols, 0);

        for (int y = 0; y < rows; ++y) {
            if (band->RasterIO(GF_Read, 0, y, cols, 1, row.data(), cols, 1, GDT_Int32, 0, 0, nullptr) != CE_None) {
                GDALClose(ds);
                throw std::runtime_error("Failed reading parameter region raster.");
            }

            for (int x = 0; x < cols; ++x) {
                const int32_t v = row[x];
                if (hasNoData && static_cast<double>(v) == noData) {
                    continue;
                }
                ids.insert(v);
            }
        }
    } else {
        std::vector<uint32_t> row(cols, 0);

        for (int y = 0; y < rows; ++y) {
            if (band->RasterIO(GF_Read, 0, y, cols, 1, row.data(), cols, 1, GDT_UInt32, 0, 0, nullptr) != CE_None) {
                GDALClose(ds);
                throw std::runtime_error("Failed reading parameter region raster.");
            }

            for (int x = 0; x < cols; ++x) {
                const uint32_t v = row[x];
                if (hasNoData && static_cast<double>(v) == noData) {
                    continue;
                }
                ids.insert(static_cast<int32_t>(v));
            }
        }
    }

    GDALClose(ds);
    return ids;
}

static void CreateAttributeTable(const Args& args) {
    std::set<int32_t> ids;
    if (!args.shp.empty() || !args.parregIn.empty()) {
        ids = CollectParamIds(args.parreg);
    } else {
        ids.insert(1);
    }

    std::ofstream out(args.att);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to create output attribute table file: " + args.att);
    }

    out << "SiID,tmin,tmax,cmin,cmax,phimin,phimax,SoilDens\n";
    for (uint32_t id : ids) {
        out << id << ','
            << args.attTmin << ','
            << args.attTmax << ','
            << args.attCmin << ','
            << args.attCmax << ','
            << args.attPhimin << ','
            << args.attPhimax << ','
            << args.attSoildens << '\n';
    }
}

int main(int argc, char** argv) {
    try {
        GDALAllRegister();
        OGRRegisterAll();

        Args args = ParseArgs(argc, argv);
        ValidateArgs(args);
        CreateParameterRegionGrid(args);
        CreateAttributeTable(args);

        std::cout << "Region computation successful." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Region computation failed." << std::endl;
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}
