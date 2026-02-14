#include "plateau_gml_file.h"
#include "plateau_dataset_source.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

PLATEAUGmlFile::PLATEAUGmlFile() {
}

PLATEAUGmlFile::~PLATEAUGmlFile() {
}

Ref<PLATEAUGmlFile> PLATEAUGmlFile::create(const String &path) {
    Ref<PLATEAUGmlFile> gml_file;
    gml_file.instantiate();

    gml_file->path_ = path;

    try {
        std::string path_str = path.utf8().get_data();
        gml_file->gml_file_ = std::make_shared<plateau::dataset::GmlFile>(path_str);
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGmlFile: Failed to create GmlFile: ", e.what());
        gml_file->gml_file_ = nullptr;
    }

    return gml_file;
}

String PLATEAUGmlFile::get_path() const {
    return path_;
}

String PLATEAUGmlFile::get_grid_code() const {
    if (!gml_file_ || !gml_file_->isValid()) {
        return String();
    }

    try {
        auto grid_code = gml_file_->getGridCode();
        if (grid_code && grid_code->isValid()) {
            return String::utf8(grid_code->get().c_str());
        }
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGmlFile: Failed to get grid code: ", e.what());
    }

    return String();
}

int PLATEAUGmlFile::get_epsg() const {
    if (!gml_file_) {
        return 6697; // Default EPSG for Japan
    }

    return static_cast<int>(gml_file_->getEpsg());
}

String PLATEAUGmlFile::get_feature_type() const {
    if (!gml_file_) {
        return String();
    }

    return String::utf8(gml_file_->getFeatureType().c_str());
}

int64_t PLATEAUGmlFile::get_package_type() const {
    if (!gml_file_) {
        return PLATEAUCityModelPackage::PACKAGE_UNKNOWN;
    }

    auto package = gml_file_->getPackage();

    // Convert libplateau package to Godot enum
    switch (package) {
        case plateau::dataset::PredefinedCityModelPackage::Building:
            return PLATEAUCityModelPackage::PACKAGE_BUILDING;
        case plateau::dataset::PredefinedCityModelPackage::Road:
            return PLATEAUCityModelPackage::PACKAGE_ROAD;
        case plateau::dataset::PredefinedCityModelPackage::UrbanPlanningDecision:
            return PLATEAUCityModelPackage::PACKAGE_URBAN_PLANNING;
        case plateau::dataset::PredefinedCityModelPackage::LandUse:
            return PLATEAUCityModelPackage::PACKAGE_LAND_USE;
        case plateau::dataset::PredefinedCityModelPackage::CityFurniture:
            return PLATEAUCityModelPackage::PACKAGE_CITY_FURNITURE;
        case plateau::dataset::PredefinedCityModelPackage::Vegetation:
            return PLATEAUCityModelPackage::PACKAGE_VEGETATION;
        case plateau::dataset::PredefinedCityModelPackage::Relief:
            return PLATEAUCityModelPackage::PACKAGE_RELIEF;
        case plateau::dataset::PredefinedCityModelPackage::DisasterRisk:
            return PLATEAUCityModelPackage::PACKAGE_FLOOD;
        case plateau::dataset::PredefinedCityModelPackage::Railway:
            return PLATEAUCityModelPackage::PACKAGE_RAILWAY;
        case plateau::dataset::PredefinedCityModelPackage::Waterway:
            return PLATEAUCityModelPackage::PACKAGE_WATERWAY;
        case plateau::dataset::PredefinedCityModelPackage::WaterBody:
            return PLATEAUCityModelPackage::PACKAGE_WATER_BODY;
        case plateau::dataset::PredefinedCityModelPackage::Bridge:
            return PLATEAUCityModelPackage::PACKAGE_BRIDGE;
        case plateau::dataset::PredefinedCityModelPackage::Track:
            return PLATEAUCityModelPackage::PACKAGE_TRACK;
        case plateau::dataset::PredefinedCityModelPackage::Square:
            return PLATEAUCityModelPackage::PACKAGE_SQUARE;
        case plateau::dataset::PredefinedCityModelPackage::Tunnel:
            return PLATEAUCityModelPackage::PACKAGE_TUNNEL;
        case plateau::dataset::PredefinedCityModelPackage::UndergroundFacility:
            return PLATEAUCityModelPackage::PACKAGE_UNDERGROUND_FACILITY;
        case plateau::dataset::PredefinedCityModelPackage::UndergroundBuilding:
            return PLATEAUCityModelPackage::PACKAGE_UNDERGROUND_BUILDING;
        case plateau::dataset::PredefinedCityModelPackage::Area:
            return PLATEAUCityModelPackage::PACKAGE_AREA;
        case plateau::dataset::PredefinedCityModelPackage::OtherConstruction:
            return PLATEAUCityModelPackage::PACKAGE_OTHER_CONSTRUCTION;
        case plateau::dataset::PredefinedCityModelPackage::Generic:
            return PLATEAUCityModelPackage::PACKAGE_GENERIC;
        default:
            return PLATEAUCityModelPackage::PACKAGE_UNKNOWN;
    }
}

String PLATEAUGmlFile::get_appearance_directory_path() const {
    if (!gml_file_) {
        return String();
    }

    return String::utf8(gml_file_->getAppearanceDirectoryPath().c_str());
}

String PLATEAUGmlFile::get_dataset_root_path() const {
    if (path_.is_empty()) {
        return String();
    }

    // Find /udx/ in path and return everything before it
    String normalized_path = path_.replace("\\", "/");
    int udx_idx = normalized_path.rfind("/udx/");
    if (udx_idx < 0) {
        // Try without trailing slash
        udx_idx = normalized_path.rfind("/udx");
        if (udx_idx < 0) {
            return String();
        }
    }

    return normalized_path.substr(0, udx_idx);
}

int PLATEAUGmlFile::get_max_lod() {
    if (!gml_file_) {
        return -1;
    }

    return gml_file_->getMaxLod();
}

bool PLATEAUGmlFile::is_valid() const {
    return gml_file_ && gml_file_->isValid();
}

PackedStringArray PLATEAUGmlFile::search_image_paths() const {
    PackedStringArray result;

    if (!gml_file_ || !gml_file_->isValid()) {
        return result;
    }

    try {
        auto paths = gml_file_->searchAllImagePathsInGML();
        for (const auto &path : paths) {
            result.push_back(String::utf8(path.c_str()));
        }
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGmlFile: Failed to search image paths: ", e.what());
    }

    return result;
}

PackedStringArray PLATEAUGmlFile::search_codelist_paths() const {
    PackedStringArray result;

    if (!gml_file_ || !gml_file_->isValid()) {
        return result;
    }

    try {
        auto paths = gml_file_->searchAllCodelistPathsInGML();
        for (const auto &path : paths) {
            result.push_back(String::utf8(path.c_str()));
        }
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGmlFile: Failed to search codelist paths: ", e.what());
    }

    return result;
}

Dictionary PLATEAUGmlFile::get_grid_extent() const {
    Dictionary result;

    if (!gml_file_ || !gml_file_->isValid()) {
        return result;
    }

    try {
        auto grid_code = gml_file_->getGridCode();
        if (grid_code && grid_code->isValid()) {
            auto extent = grid_code->getExtent();
            result["min_lat"] = extent.min.latitude;
            result["max_lat"] = extent.max.latitude;
            result["min_lon"] = extent.min.longitude;
            result["max_lon"] = extent.max.longitude;
        }
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGmlFile: Failed to get grid extent: ", e.what());
    }

    return result;
}

void PLATEAUGmlFile::_bind_methods() {
    // Static methods
    ClassDB::bind_static_method("PLATEAUGmlFile", D_METHOD("create", "path"), &PLATEAUGmlFile::create);

    // Instance methods
    ClassDB::bind_method(D_METHOD("get_path"), &PLATEAUGmlFile::get_path);
    ClassDB::bind_method(D_METHOD("get_grid_code"), &PLATEAUGmlFile::get_grid_code);
    ClassDB::bind_method(D_METHOD("get_epsg"), &PLATEAUGmlFile::get_epsg);
    ClassDB::bind_method(D_METHOD("get_feature_type"), &PLATEAUGmlFile::get_feature_type);
    ClassDB::bind_method(D_METHOD("get_package_type"), &PLATEAUGmlFile::get_package_type);
    ClassDB::bind_method(D_METHOD("get_appearance_directory_path"), &PLATEAUGmlFile::get_appearance_directory_path);
    ClassDB::bind_method(D_METHOD("get_dataset_root_path"), &PLATEAUGmlFile::get_dataset_root_path);
    ClassDB::bind_method(D_METHOD("get_max_lod"), &PLATEAUGmlFile::get_max_lod);
    ClassDB::bind_method(D_METHOD("is_valid"), &PLATEAUGmlFile::is_valid);
    ClassDB::bind_method(D_METHOD("search_image_paths"), &PLATEAUGmlFile::search_image_paths);
    ClassDB::bind_method(D_METHOD("search_codelist_paths"), &PLATEAUGmlFile::search_codelist_paths);
    ClassDB::bind_method(D_METHOD("get_grid_extent"), &PLATEAUGmlFile::get_grid_extent);
}
