#include "plateau_dataset_source.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// ============================================================================
// PLATEAUDatasetMetadata
// ============================================================================

PLATEAUDatasetMetadata::PLATEAUDatasetMetadata() {
}

PLATEAUDatasetMetadata::~PLATEAUDatasetMetadata() {
}

void PLATEAUDatasetMetadata::set_id(const String &id) {
    id_ = id;
}

String PLATEAUDatasetMetadata::get_id() const {
    return id_;
}

void PLATEAUDatasetMetadata::set_title(const String &title) {
    title_ = title;
}

String PLATEAUDatasetMetadata::get_title() const {
    return title_;
}

void PLATEAUDatasetMetadata::set_description(const String &description) {
    description_ = description;
}

String PLATEAUDatasetMetadata::get_description() const {
    return description_;
}

void PLATEAUDatasetMetadata::set_feature_types(const PackedStringArray &types) {
    feature_types_ = types;
}

PackedStringArray PLATEAUDatasetMetadata::get_feature_types() const {
    return feature_types_;
}

void PLATEAUDatasetMetadata::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_id", "id"), &PLATEAUDatasetMetadata::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &PLATEAUDatasetMetadata::get_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");

    ClassDB::bind_method(D_METHOD("set_title", "title"), &PLATEAUDatasetMetadata::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &PLATEAUDatasetMetadata::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &PLATEAUDatasetMetadata::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &PLATEAUDatasetMetadata::get_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description"), "set_description", "get_description");

    ClassDB::bind_method(D_METHOD("set_feature_types", "types"), &PLATEAUDatasetMetadata::set_feature_types);
    ClassDB::bind_method(D_METHOD("get_feature_types"), &PLATEAUDatasetMetadata::get_feature_types);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "feature_types"), "set_feature_types", "get_feature_types");
}

// ============================================================================
// PLATEAUDatasetGroup
// ============================================================================

PLATEAUDatasetGroup::PLATEAUDatasetGroup() {
}

PLATEAUDatasetGroup::~PLATEAUDatasetGroup() {
}

void PLATEAUDatasetGroup::set_id(const String &id) {
    id_ = id;
}

String PLATEAUDatasetGroup::get_id() const {
    return id_;
}

void PLATEAUDatasetGroup::set_title(const String &title) {
    title_ = title;
}

String PLATEAUDatasetGroup::get_title() const {
    return title_;
}

void PLATEAUDatasetGroup::set_datasets(const TypedArray<PLATEAUDatasetMetadata> &datasets) {
    datasets_ = datasets;
}

TypedArray<PLATEAUDatasetMetadata> PLATEAUDatasetGroup::get_datasets() const {
    return datasets_;
}

int PLATEAUDatasetGroup::get_dataset_count() const {
    return datasets_.size();
}

Ref<PLATEAUDatasetMetadata> PLATEAUDatasetGroup::get_dataset(int index) const {
    if (index < 0 || index >= datasets_.size()) {
        return Ref<PLATEAUDatasetMetadata>();
    }
    return datasets_[index];
}

void PLATEAUDatasetGroup::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_id", "id"), &PLATEAUDatasetGroup::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &PLATEAUDatasetGroup::get_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");

    ClassDB::bind_method(D_METHOD("set_title", "title"), &PLATEAUDatasetGroup::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &PLATEAUDatasetGroup::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_datasets", "datasets"), &PLATEAUDatasetGroup::set_datasets);
    ClassDB::bind_method(D_METHOD("get_datasets"), &PLATEAUDatasetGroup::get_datasets);

    ClassDB::bind_method(D_METHOD("get_dataset_count"), &PLATEAUDatasetGroup::get_dataset_count);
    ClassDB::bind_method(D_METHOD("get_dataset", "index"), &PLATEAUDatasetGroup::get_dataset);
}

// ============================================================================
// PLATEAUGmlFileInfo
// ============================================================================

PLATEAUGmlFileInfo::PLATEAUGmlFileInfo()
    : max_lod_(0), package_type_(0) {
}

PLATEAUGmlFileInfo::~PLATEAUGmlFileInfo() {
}

void PLATEAUGmlFileInfo::set_path(const String &path) {
    path_ = path;
}

String PLATEAUGmlFileInfo::get_path() const {
    return path_;
}

void PLATEAUGmlFileInfo::set_mesh_code(const String &code) {
    mesh_code_ = code;
}

String PLATEAUGmlFileInfo::get_mesh_code() const {
    return mesh_code_;
}

void PLATEAUGmlFileInfo::set_max_lod(int lod) {
    max_lod_ = lod;
}

int PLATEAUGmlFileInfo::get_max_lod() const {
    return max_lod_;
}

void PLATEAUGmlFileInfo::set_package_type(int64_t type) {
    package_type_ = type;
}

int64_t PLATEAUGmlFileInfo::get_package_type() const {
    return package_type_;
}

void PLATEAUGmlFileInfo::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_path", "path"), &PLATEAUGmlFileInfo::set_path);
    ClassDB::bind_method(D_METHOD("get_path"), &PLATEAUGmlFileInfo::get_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "path"), "set_path", "get_path");

    ClassDB::bind_method(D_METHOD("set_mesh_code", "code"), &PLATEAUGmlFileInfo::set_mesh_code);
    ClassDB::bind_method(D_METHOD("get_mesh_code"), &PLATEAUGmlFileInfo::get_mesh_code);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "mesh_code"), "set_mesh_code", "get_mesh_code");

    ClassDB::bind_method(D_METHOD("set_max_lod", "lod"), &PLATEAUGmlFileInfo::set_max_lod);
    ClassDB::bind_method(D_METHOD("get_max_lod"), &PLATEAUGmlFileInfo::get_max_lod);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lod"), "set_max_lod", "get_max_lod");

    ClassDB::bind_method(D_METHOD("set_package_type", "type"), &PLATEAUGmlFileInfo::set_package_type);
    ClassDB::bind_method(D_METHOD("get_package_type"), &PLATEAUGmlFileInfo::get_package_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "package_type"), "set_package_type", "get_package_type");
}

// ============================================================================
// PLATEAUDatasetSource
// ============================================================================

PLATEAUDatasetSource::PLATEAUDatasetSource()
    : is_valid_(false) {
}

PLATEAUDatasetSource::~PLATEAUDatasetSource() {
}

Ref<PLATEAUDatasetSource> PLATEAUDatasetSource::create_local(const String &local_path) {
    Ref<PLATEAUDatasetSource> source;
    source.instantiate();

    try {
        auto dataset_source = plateau::dataset::DatasetSource::createLocal(local_path.utf8().get_data());
        source->accessor_ = dataset_source.getAccessor();
        source->is_valid_ = (source->accessor_ != nullptr);

        if (source->is_valid_) {
            UtilityFunctions::print("PLATEAUDatasetSource: Created local source from ", local_path);
        } else {
            UtilityFunctions::printerr("PLATEAUDatasetSource: Failed to create local source");
        }

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUDatasetSource exception: ", String(e.what()));
        source->is_valid_ = false;
    }

    return source;
}

String PLATEAUDatasetSource::get_default_server_url() {
    // Same value as libplateau's getDefaultServerUrl() in src/network/client.cpp
    return "https://api.plateau.reearth.io";
}

String PLATEAUDatasetSource::get_mock_server_url() {
    // Same value as libplateau's Client::getMockServerUrl() in src/network/client.cpp
    return "https://plateauapimockv3-1-w3921743.deta.app";
}

PackedStringArray PLATEAUDatasetSource::build_auth_headers(const String &custom_token, bool use_default_token) {
    PackedStringArray headers;
    headers.push_back("Content-Type: application/json");

    String token;
    if (!custom_token.is_empty()) {
        // Use user-provided custom token
        token = custom_token;
    } else if (use_default_token) {
        // Use built-in default token (not exposed to GDScript)
        // Same value as libplateau's getDefaultApiToken() in src/network/client.cpp
        token = "secret-56c66bcac0ab4724b86fc48309fe517a";
    }

    if (!token.is_empty()) {
        headers.push_back("Authorization: Bearer " + token);
    }

    return headers;
}

bool PLATEAUDatasetSource::is_valid() const {
    return is_valid_ && accessor_ != nullptr;
}

int64_t PLATEAUDatasetSource::get_available_packages() const {
    if (!is_valid()) {
        return PACKAGE_NONE;
    }

    try {
        auto packages = accessor_->getPackages();
        return static_cast<int64_t>(packages);
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUDatasetSource exception: ", String(e.what()));
        return PACKAGE_NONE;
    }
}

TypedArray<PLATEAUGmlFileInfo> PLATEAUDatasetSource::get_gml_files(int64_t package_flags) {
    TypedArray<PLATEAUGmlFileInfo> result;

    if (!is_valid()) {
        UtilityFunctions::printerr("PLATEAUDatasetSource: Source is not valid");
        return result;
    }

    try {
        auto package = static_cast<plateau::dataset::PredefinedCityModelPackage>(package_flags);
        auto gml_files = accessor_->getGmlFiles(package);

        if (gml_files) {
            for (auto &gml_file : *gml_files) {
                Ref<PLATEAUGmlFileInfo> info;
                info.instantiate();
                info->set_path(String(gml_file.getPath().c_str()));
                auto grid_code = gml_file.getGridCode();
                if (grid_code) {
                    info->set_mesh_code(String(grid_code->get().c_str()));
                }
                info->set_max_lod(gml_file.getMaxLod());
                info->set_package_type(static_cast<int64_t>(gml_file.getPackage()));
                result.push_back(info);
            }
        }

        UtilityFunctions::print("PLATEAUDatasetSource: Found ", result.size(), " GML files");

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUDatasetSource exception: ", String(e.what()));
    }

    return result;
}

PackedStringArray PLATEAUDatasetSource::get_mesh_codes() {
    PackedStringArray result;

    if (!is_valid()) {
        return result;
    }

    try {
        const auto &grid_codes = accessor_->getGridCodes();
        for (const auto &code : grid_codes) {
            if (code) {
                result.push_back(String(code->get().c_str()));
            }
        }
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUDatasetSource exception: ", String(e.what()));
    }

    return result;
}

Ref<PLATEAUDatasetSource> PLATEAUDatasetSource::filter_by_mesh_codes(const PackedStringArray &codes) {
    Ref<PLATEAUDatasetSource> filtered;
    filtered.instantiate();

    if (!is_valid()) {
        UtilityFunctions::printerr("PLATEAUDatasetSource: Source is not valid");
        return filtered;
    }

    try {
        std::vector<std::shared_ptr<plateau::dataset::GridCode>> grid_codes;
        for (int i = 0; i < codes.size(); i++) {
            auto code = plateau::dataset::GridCode::create(codes[i].utf8().get_data());
            if (code) {
                grid_codes.push_back(code);
            }
        }

        filtered->accessor_ = accessor_->filterByGridCodes(grid_codes);
        filtered->is_valid_ = (filtered->accessor_ != nullptr);

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUDatasetSource exception: ", String(e.what()));
    }

    return filtered;
}

void PLATEAUDatasetSource::_bind_methods() {
    ClassDB::bind_static_method("PLATEAUDatasetSource", D_METHOD("create_local", "local_path"), &PLATEAUDatasetSource::create_local);
    ClassDB::bind_static_method("PLATEAUDatasetSource", D_METHOD("get_default_server_url"), &PLATEAUDatasetSource::get_default_server_url);
    ClassDB::bind_static_method("PLATEAUDatasetSource", D_METHOD("get_mock_server_url"), &PLATEAUDatasetSource::get_mock_server_url);
    ClassDB::bind_static_method("PLATEAUDatasetSource", D_METHOD("build_auth_headers", "custom_token", "use_default_token"), &PLATEAUDatasetSource::build_auth_headers);

    ClassDB::bind_method(D_METHOD("is_valid"), &PLATEAUDatasetSource::is_valid);
    ClassDB::bind_method(D_METHOD("get_available_packages"), &PLATEAUDatasetSource::get_available_packages);
    ClassDB::bind_method(D_METHOD("get_gml_files", "package_flags"), &PLATEAUDatasetSource::get_gml_files);
    ClassDB::bind_method(D_METHOD("get_mesh_codes"), &PLATEAUDatasetSource::get_mesh_codes);
    ClassDB::bind_method(D_METHOD("filter_by_mesh_codes", "codes"), &PLATEAUDatasetSource::filter_by_mesh_codes);

    // Package type enum
    BIND_ENUM_CONSTANT(PACKAGE_NONE);
    BIND_ENUM_CONSTANT(PACKAGE_BUILDING);
    BIND_ENUM_CONSTANT(PACKAGE_ROAD);
    BIND_ENUM_CONSTANT(PACKAGE_URBAN_PLANNING);
    BIND_ENUM_CONSTANT(PACKAGE_LAND_USE);
    BIND_ENUM_CONSTANT(PACKAGE_CITY_FURNITURE);
    BIND_ENUM_CONSTANT(PACKAGE_VEGETATION);
    BIND_ENUM_CONSTANT(PACKAGE_RELIEF);
    BIND_ENUM_CONSTANT(PACKAGE_FLOOD);
    BIND_ENUM_CONSTANT(PACKAGE_TSUNAMI);
    BIND_ENUM_CONSTANT(PACKAGE_LANDSLIDE);
    BIND_ENUM_CONSTANT(PACKAGE_STORM_SURGE);
    BIND_ENUM_CONSTANT(PACKAGE_INLAND_FLOOD);
    BIND_ENUM_CONSTANT(PACKAGE_RAILWAY);
    BIND_ENUM_CONSTANT(PACKAGE_WATERWAY);
    BIND_ENUM_CONSTANT(PACKAGE_WATER_BODY);
    BIND_ENUM_CONSTANT(PACKAGE_BRIDGE);
    BIND_ENUM_CONSTANT(PACKAGE_TRACK);
    BIND_ENUM_CONSTANT(PACKAGE_SQUARE);
    BIND_ENUM_CONSTANT(PACKAGE_TUNNEL);
    BIND_ENUM_CONSTANT(PACKAGE_UNDERGROUND_FACILITY);
    BIND_ENUM_CONSTANT(PACKAGE_UNDERGROUND_BUILDING);
    BIND_ENUM_CONSTANT(PACKAGE_AREA);
    BIND_ENUM_CONSTANT(PACKAGE_OTHER_CONSTRUCTION);
    BIND_ENUM_CONSTANT(PACKAGE_GENERIC);
    BIND_ENUM_CONSTANT(PACKAGE_UNKNOWN);
    BIND_ENUM_CONSTANT(PACKAGE_ALL);
}
