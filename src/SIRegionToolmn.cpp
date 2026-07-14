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
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "commonLib.h"

namespace fs = std::filesystem;

static double DefaultNoDataForDataType(GDALDataType dt) {
    switch (dt) {
        case GDT_Byte:
            return 255.0;
        case GDT_Int16:
            return static_cast<double>(MISSINGSHORT);
        case GDT_UInt16:
            return static_cast<double>(std::numeric_limits<uint16_t>::max());
        case GDT_Int32:
            return static_cast<double>(MISSINGLONG);
        case GDT_UInt32:
            return static_cast<double>(std::numeric_limits<uint32_t>::max());
        case GDT_Float32:
            return static_cast<double>(MISSINGFLOAT);
        case GDT_Float64:
            return std::numeric_limits<double>::lowest();
        default:
            return 0.0;
    }
}

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

static GDALDataset* BuildTempFIDLayer(GDALDataset* srcVds, OGRLayer* srcLayer, const std::string& tempFieldName) {
    if (!srcVds || !srcLayer) {
        throw std::runtime_error("Invalid source datasource/layer.");
    }

    GDALDriver* memDrv = GetGDALDriverManager()->GetDriverByName("Memory");
    if (!memDrv) {
        throw std::runtime_error("GDAL Memory driver not available.");
    }

    GDALDataset* memDs = memDrv->Create("", 0, 0, 0, GDT_Unknown, nullptr);
    if (!memDs) {
        throw std::runtime_error("Failed to create in-memory datasource.");
    }

    const OGRSpatialReference* srs = srcLayer->GetSpatialRef();
    OGRLayer* memLayer = memDs->CreateLayer(
        srcLayer->GetName(),
        const_cast<OGRSpatialReference*>(srs),
        srcLayer->GetGeomType(),
        nullptr);
    if (!memLayer) {
        GDALClose(memDs);
        throw std::runtime_error("Failed to create in-memory layer.");
    }

    OGRFeatureDefn* srcDefn = srcLayer->GetLayerDefn();
    if (!srcDefn) {
        GDALClose(memDs);
        throw std::runtime_error("Source layer definition is missing.");
    }

    for (int i = 0; i < srcDefn->GetFieldCount(); ++i) {
        OGRFieldDefn fieldDefn(srcDefn->GetFieldDefn(i));
        if (memLayer->CreateField(&fieldDefn) != OGRERR_NONE) {
            GDALClose(memDs);
            throw std::runtime_error("Failed to copy fields into in-memory layer.");
        }
    }

    OGRFieldDefn fidField(tempFieldName.c_str(), OFTInteger64);
    if (memLayer->CreateField(&fidField) != OGRERR_NONE) {
        GDALClose(memDs);
        throw std::runtime_error("Failed to create temporary FID field.");
    }

    const int fidFieldIndex = memLayer->GetLayerDefn()->GetFieldIndex(tempFieldName.c_str());
    if (fidFieldIndex < 0) {
        GDALClose(memDs);
        throw std::runtime_error("Temporary FID field not found.");
    }

    srcLayer->ResetReading();
    OGRFeature* srcFeat = nullptr;
    while ((srcFeat = srcLayer->GetNextFeature()) != nullptr) {
        OGRFeature* outFeat = OGRFeature::CreateFeature(memLayer->GetLayerDefn());
        if (!outFeat) {
            OGRFeature::DestroyFeature(srcFeat);
            GDALClose(memDs);
            throw std::runtime_error("Failed to create in-memory feature.");
        }

        if (srcFeat->GetGeometryRef()) {
            if (outFeat->SetGeometry(srcFeat->GetGeometryRef()) != OGRERR_NONE) {
                OGRFeature::DestroyFeature(outFeat);
                OGRFeature::DestroyFeature(srcFeat);
                GDALClose(memDs);
                throw std::runtime_error("Failed to copy feature geometry.");
            }
        }

        outFeat->SetFrom(srcFeat, TRUE);
        outFeat->SetField(fidFieldIndex, static_cast<GIntBig>(srcFeat->GetFID()));

        if (memLayer->CreateFeature(outFeat) != OGRERR_NONE) {
            OGRFeature::DestroyFeature(outFeat);
            OGRFeature::DestroyFeature(srcFeat);
            GDALClose(memDs);
            throw std::runtime_error("Failed to create in-memory feature.");
        }

        OGRFeature::DestroyFeature(outFeat);
        OGRFeature::DestroyFeature(srcFeat);
    }

    return memDs;
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

        GDALDataset* vds = static_cast<GDALDataset*>(GDALOpenEx(args.shp.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
        if (!vds) {
            throw std::runtime_error("Not a valid shape file (" + args.shp + ") provided for '-shp'.");
        }

        OGRLayer* layer = vds->GetLayer(0);
        if (!layer) {
            GDALClose(vds);
            throw std::runtime_error("Invalid shapefile: no layer found.");
        }

        // If the user requested to rasterize using the feature FID, we accept it and
        // will synthesize an integer attribute later during rasterization. Otherwise
        // ensure the provided attribute exists on the input layer.
        if (args.shpAttName != "FID") {
            OGRFeatureDefn* defn = layer->GetLayerDefn();
            if (!defn || defn->GetFieldIndex(args.shpAttName.c_str()) < 0) {
                GDALClose(vds);
                throw std::runtime_error("Invalid shapefile. Attribute '" + args.shpAttName + "' is missing.");
            }
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

    const GDALDataType outDT = GDT_Int32;
    const double outNoData = DefaultNoDataForDataType(outDT);

    GDALRasterBand* band = out->GetRasterBand(1);
    if (band) {
        band->SetNoDataValue(outNoData);
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
    if (b) {
        const double noData = DefaultNoDataForDataType(dt);
        b->SetNoDataValue(noData);
        b->Fill(noData);
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

    // If the user asked to use the feature FID as the rasterized attribute,
    // we will rasterize geometries directly using their FID values as burn
    // values. This avoids creating a temporary layer or datasource and
    // sidesteps driver compatibility issues.
    std::string attrName = args.shpAttName;

        if (args.shpAttName == "FID") {
        const std::string tempFieldName = "__tau_dem_fid";
        GDALDataset* tempVds = BuildTempFIDLayer(vds, layer, tempFieldName);
        if (!tempVds) {
            GDALClose(vds);
            GDALClose(mem);
            throw std::runtime_error("Failed to prepare temporary FID layer.");
        }

        OGRLayer* tempLayer = tempVds->GetLayer(0);
        if (!tempLayer) {
            GDALClose(tempVds);
            GDALClose(vds);
            GDALClose(mem);
            throw std::runtime_error("Temporary FID layer has no layer.");
        }

        char* opt = CPLStrdup((std::string("ATTRIBUTE=") + tempFieldName).c_str());
        char** opts = nullptr;
        opts = CSLAddString(opts, opt);
        CPLFree(opt);

        int bands[1] = {1};
        OGRLayerH hLayer = reinterpret_cast<OGRLayerH>(tempLayer);
        const int err = GDALRasterizeLayers(mem, 1, bands, 1, &hLayer, nullptr, nullptr, nullptr, opts, nullptr, nullptr);
        CSLDestroy(opts);

        GDALClose(tempVds);

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
        return;
    }

    // Non-FID path: rasterize using an attribute field on the layer
    char* opt = CPLStrdup((std::string("ATTRIBUTE=") + attrName).c_str());
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

    const GDALDataType outDT = GDT_Int32;
    const double outNoData = DefaultNoDataForDataType(outDT);

    GDALRasterBand* dstBand = dst->GetRasterBand(1);
    if (dstBand) {
        dstBand->SetNoDataValue(outNoData);
        dstBand->Fill(outNoData);
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
