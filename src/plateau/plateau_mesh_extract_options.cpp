#include "plateau_mesh_extract_options.h"

using namespace godot;
using namespace plateau::polygonMesh;
using namespace plateau::geometry;

PLATEAUMeshExtractOptions::PLATEAUMeshExtractOptions()
    : reference_point_(Vector3(0, 0, 0)),
      coordinate_system_(static_cast<int>(CoordinateSystem::EUN)),
      mesh_granularity_(static_cast<int>(MeshGranularity::PerPrimaryFeatureObject)),
      min_lod_(0),
      max_lod_(3),
      export_appearance_(true),
      grid_count_of_side_(10),
      unit_scale_(1.0f),
      coordinate_zone_id_(9),
      enable_texture_packing_(false),
      texture_packing_resolution_(2048),
      highest_lod_only_(false) {  // Default false: extract all LODs in range (Unity SDK compatible)
}

PLATEAUMeshExtractOptions::~PLATEAUMeshExtractOptions() {
}

void PLATEAUMeshExtractOptions::set_reference_point(const Vector3 &point) {
    reference_point_ = point;
}

Vector3 PLATEAUMeshExtractOptions::get_reference_point() const {
    return reference_point_;
}

void PLATEAUMeshExtractOptions::set_coordinate_system(int system) {
    coordinate_system_ = system;
}

int PLATEAUMeshExtractOptions::get_coordinate_system() const {
    return coordinate_system_;
}

void PLATEAUMeshExtractOptions::set_mesh_granularity(int granularity) {
    mesh_granularity_ = granularity;
}

int PLATEAUMeshExtractOptions::get_mesh_granularity() const {
    return mesh_granularity_;
}

void PLATEAUMeshExtractOptions::set_min_lod(int lod) {
    min_lod_ = lod;
}

int PLATEAUMeshExtractOptions::get_min_lod() const {
    return min_lod_;
}

void PLATEAUMeshExtractOptions::set_max_lod(int lod) {
    max_lod_ = lod;
}

int PLATEAUMeshExtractOptions::get_max_lod() const {
    return max_lod_;
}

void PLATEAUMeshExtractOptions::set_export_appearance(bool enable) {
    export_appearance_ = enable;
}

bool PLATEAUMeshExtractOptions::get_export_appearance() const {
    return export_appearance_;
}

void PLATEAUMeshExtractOptions::set_grid_count_of_side(int count) {
    grid_count_of_side_ = count;
}

int PLATEAUMeshExtractOptions::get_grid_count_of_side() const {
    return grid_count_of_side_;
}

void PLATEAUMeshExtractOptions::set_unit_scale(float scale) {
    unit_scale_ = scale;
}

float PLATEAUMeshExtractOptions::get_unit_scale() const {
    return unit_scale_;
}

void PLATEAUMeshExtractOptions::set_coordinate_zone_id(int zone_id) {
    coordinate_zone_id_ = zone_id;
}

int PLATEAUMeshExtractOptions::get_coordinate_zone_id() const {
    return coordinate_zone_id_;
}

void PLATEAUMeshExtractOptions::set_enable_texture_packing(bool enable) {
    enable_texture_packing_ = enable;
}

bool PLATEAUMeshExtractOptions::get_enable_texture_packing() const {
    return enable_texture_packing_;
}

void PLATEAUMeshExtractOptions::set_texture_packing_resolution(int resolution) {
    texture_packing_resolution_ = resolution;
}

int PLATEAUMeshExtractOptions::get_texture_packing_resolution() const {
    return texture_packing_resolution_;
}

void PLATEAUMeshExtractOptions::set_highest_lod_only(bool enable) {
    highest_lod_only_ = enable;
}

bool PLATEAUMeshExtractOptions::get_highest_lod_only() const {
    return highest_lod_only_;
}

MeshExtractOptions PLATEAUMeshExtractOptions::get_native() const {
    MeshExtractOptions options;
    options.reference_point = TVec3d(reference_point_.x, reference_point_.y, reference_point_.z);
    options.mesh_axes = static_cast<CoordinateSystem>(coordinate_system_);
    options.mesh_granularity = static_cast<MeshGranularity>(mesh_granularity_);
    options.min_lod = min_lod_;
    options.max_lod = max_lod_;
    options.export_appearance = export_appearance_;
    options.grid_count_of_side = grid_count_of_side_;
    options.unit_scale = unit_scale_;
    options.coordinate_zone_id = coordinate_zone_id_;
    options.enable_texture_packing = enable_texture_packing_;
    options.texture_packing_resolution = texture_packing_resolution_;
    options.highest_lod_only = highest_lod_only_;
    return options;
}

void PLATEAUMeshExtractOptions::_bind_methods() {
    // Reference point
    ClassDB::bind_method(D_METHOD("set_reference_point", "point"), &PLATEAUMeshExtractOptions::set_reference_point);
    ClassDB::bind_method(D_METHOD("get_reference_point"), &PLATEAUMeshExtractOptions::get_reference_point);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "reference_point"), "set_reference_point", "get_reference_point");

    // Coordinate system
    ClassDB::bind_method(D_METHOD("set_coordinate_system", "system"), &PLATEAUMeshExtractOptions::set_coordinate_system);
    ClassDB::bind_method(D_METHOD("get_coordinate_system"), &PLATEAUMeshExtractOptions::get_coordinate_system);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "coordinate_system", PROPERTY_HINT_ENUM, "ENU,WUN,ESU,EUN"), "set_coordinate_system", "get_coordinate_system");

    // Mesh granularity
    ClassDB::bind_method(D_METHOD("set_mesh_granularity", "granularity"), &PLATEAUMeshExtractOptions::set_mesh_granularity);
    ClassDB::bind_method(D_METHOD("get_mesh_granularity"), &PLATEAUMeshExtractOptions::get_mesh_granularity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mesh_granularity", PROPERTY_HINT_ENUM, "PerAtomicFeatureObject,PerPrimaryFeatureObject,PerCityModelArea"), "set_mesh_granularity", "get_mesh_granularity");

    // LOD range
    ClassDB::bind_method(D_METHOD("set_min_lod", "lod"), &PLATEAUMeshExtractOptions::set_min_lod);
    ClassDB::bind_method(D_METHOD("get_min_lod"), &PLATEAUMeshExtractOptions::get_min_lod);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "min_lod", PROPERTY_HINT_RANGE, "0,4"), "set_min_lod", "get_min_lod");

    ClassDB::bind_method(D_METHOD("set_max_lod", "lod"), &PLATEAUMeshExtractOptions::set_max_lod);
    ClassDB::bind_method(D_METHOD("get_max_lod"), &PLATEAUMeshExtractOptions::get_max_lod);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lod", PROPERTY_HINT_RANGE, "0,4"), "set_max_lod", "get_max_lod");

    // Export appearance
    ClassDB::bind_method(D_METHOD("set_export_appearance", "enable"), &PLATEAUMeshExtractOptions::set_export_appearance);
    ClassDB::bind_method(D_METHOD("get_export_appearance"), &PLATEAUMeshExtractOptions::get_export_appearance);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "export_appearance"), "set_export_appearance", "get_export_appearance");

    // Grid count
    ClassDB::bind_method(D_METHOD("set_grid_count_of_side", "count"), &PLATEAUMeshExtractOptions::set_grid_count_of_side);
    ClassDB::bind_method(D_METHOD("get_grid_count_of_side"), &PLATEAUMeshExtractOptions::get_grid_count_of_side);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_count_of_side", PROPERTY_HINT_RANGE, "1,100"), "set_grid_count_of_side", "get_grid_count_of_side");

    // Unit scale
    ClassDB::bind_method(D_METHOD("set_unit_scale", "scale"), &PLATEAUMeshExtractOptions::set_unit_scale);
    ClassDB::bind_method(D_METHOD("get_unit_scale"), &PLATEAUMeshExtractOptions::get_unit_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_scale"), "set_unit_scale", "get_unit_scale");

    // Coordinate zone ID
    ClassDB::bind_method(D_METHOD("set_coordinate_zone_id", "zone_id"), &PLATEAUMeshExtractOptions::set_coordinate_zone_id);
    ClassDB::bind_method(D_METHOD("get_coordinate_zone_id"), &PLATEAUMeshExtractOptions::get_coordinate_zone_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "coordinate_zone_id", PROPERTY_HINT_RANGE, "1,19"), "set_coordinate_zone_id", "get_coordinate_zone_id");

    // Texture packing
    ClassDB::bind_method(D_METHOD("set_enable_texture_packing", "enable"), &PLATEAUMeshExtractOptions::set_enable_texture_packing);
    ClassDB::bind_method(D_METHOD("get_enable_texture_packing"), &PLATEAUMeshExtractOptions::get_enable_texture_packing);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_texture_packing"), "set_enable_texture_packing", "get_enable_texture_packing");

    ClassDB::bind_method(D_METHOD("set_texture_packing_resolution", "resolution"), &PLATEAUMeshExtractOptions::set_texture_packing_resolution);
    ClassDB::bind_method(D_METHOD("get_texture_packing_resolution"), &PLATEAUMeshExtractOptions::get_texture_packing_resolution);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "texture_packing_resolution", PROPERTY_HINT_ENUM, "512,1024,2048,4096"), "set_texture_packing_resolution", "get_texture_packing_resolution");

    // Highest LOD only
    ClassDB::bind_method(D_METHOD("set_highest_lod_only", "enable"), &PLATEAUMeshExtractOptions::set_highest_lod_only);
    ClassDB::bind_method(D_METHOD("get_highest_lod_only"), &PLATEAUMeshExtractOptions::get_highest_lod_only);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "highest_lod_only"), "set_highest_lod_only", "get_highest_lod_only");
}
