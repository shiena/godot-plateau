#include "plateau_dynamic_tile.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// ============================================================================
// PLATEAUDynamicTile
// ============================================================================

PLATEAUDynamicTile::PLATEAUDynamicTile() :
    zoom_level_(0),
    load_state_(LOAD_STATE_NONE),
    next_load_state_(LOAD_STATE_NONE),
    distance_from_camera_(0.0f),
    loaded_instance_(nullptr) {
}

PLATEAUDynamicTile::~PLATEAUDynamicTile() {
}

void PLATEAUDynamicTile::set_address(const String &address) {
    address_ = address;
}

String PLATEAUDynamicTile::get_address() const {
    return address_;
}

void PLATEAUDynamicTile::set_zoom_level(int level) {
    zoom_level_ = level;
}

int PLATEAUDynamicTile::get_zoom_level() const {
    return zoom_level_;
}

void PLATEAUDynamicTile::set_extent(const AABB &extent) {
    extent_ = extent;
}

AABB PLATEAUDynamicTile::get_extent() const {
    return extent_;
}

void PLATEAUDynamicTile::set_group_name(const String &name) {
    group_name_ = name;
}

String PLATEAUDynamicTile::get_group_name() const {
    return group_name_;
}

PLATEAUDynamicTile::LoadState PLATEAUDynamicTile::get_load_state() const {
    return load_state_;
}

void PLATEAUDynamicTile::set_load_state(LoadState state) {
    load_state_ = state;
}

void PLATEAUDynamicTile::set_next_load_state(LoadState state) {
    next_load_state_ = state;
}

float PLATEAUDynamicTile::get_distance_from_camera() const {
    return distance_from_camera_;
}

void PLATEAUDynamicTile::set_distance(float distance) {
    distance_from_camera_ = distance;
}

Node3D *PLATEAUDynamicTile::get_loaded_instance() const {
    return loaded_instance_;
}

void PLATEAUDynamicTile::set_loaded_instance(Node3D *instance) {
    loaded_instance_ = instance;
}

void PLATEAUDynamicTile::set_parent_tile(const Ref<PLATEAUDynamicTile> &parent) {
    parent_tile_ = parent;
}

Ref<PLATEAUDynamicTile> PLATEAUDynamicTile::get_parent_tile() const {
    return parent_tile_;
}

void PLATEAUDynamicTile::add_child_tile(const Ref<PLATEAUDynamicTile> &child) {
    if (child.is_valid()) {
        child_tiles_.append(child);
        child->set_parent_tile(Ref<PLATEAUDynamicTile>(this));
    }
}

TypedArray<PLATEAUDynamicTile> PLATEAUDynamicTile::get_child_tiles() const {
    return child_tiles_;
}

int PLATEAUDynamicTile::get_child_count() const {
    return child_tiles_.size();
}

bool PLATEAUDynamicTile::has_all_children_unloaded() const {
    for (int i = 0; i < child_tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> child = child_tiles_[i];
        if (child.is_valid() && child->get_load_state() != LOAD_STATE_UNLOADED &&
            child->get_load_state() != LOAD_STATE_NONE) {
            return false;
        }
    }
    return true;
}

bool PLATEAUDynamicTile::has_any_children_loaded() const {
    for (int i = 0; i < child_tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> child = child_tiles_[i];
        if (child.is_valid() && child->get_load_state() == LOAD_STATE_LOADED) {
            return true;
        }
    }
    return false;
}

float PLATEAUDynamicTile::calculate_distance(const Vector3 &camera_pos, bool ignore_y) const {
    Vector3 center = extent_.get_center();

    if (ignore_y) {
        // 2D distance (XZ plane)
        Vector2 tile_pos(center.x, center.z);
        Vector2 cam_pos(camera_pos.x, camera_pos.z);
        return tile_pos.distance_to(cam_pos);
    } else {
        // 3D distance to closest point on AABB
        Vector3 closest = extent_.get_center();
        closest.x = CLAMP(camera_pos.x, extent_.position.x, extent_.position.x + extent_.size.x);
        closest.y = CLAMP(camera_pos.y, extent_.position.y, extent_.position.y + extent_.size.y);
        closest.z = CLAMP(camera_pos.z, extent_.position.z, extent_.position.z + extent_.size.z);
        return camera_pos.distance_to(closest);
    }
}

bool PLATEAUDynamicTile::is_within_range(float distance, const Dictionary &load_distances) const {
    if (!load_distances.has(zoom_level_)) {
        return false;
    }

    Vector2 range = load_distances[zoom_level_];
    return distance >= range.x && distance < range.y;
}

void PLATEAUDynamicTile::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_address", "address"), &PLATEAUDynamicTile::set_address);
    ClassDB::bind_method(D_METHOD("get_address"), &PLATEAUDynamicTile::get_address);
    ClassDB::bind_method(D_METHOD("set_zoom_level", "level"), &PLATEAUDynamicTile::set_zoom_level);
    ClassDB::bind_method(D_METHOD("get_zoom_level"), &PLATEAUDynamicTile::get_zoom_level);
    ClassDB::bind_method(D_METHOD("set_extent", "extent"), &PLATEAUDynamicTile::set_extent);
    ClassDB::bind_method(D_METHOD("get_extent"), &PLATEAUDynamicTile::get_extent);
    ClassDB::bind_method(D_METHOD("set_group_name", "name"), &PLATEAUDynamicTile::set_group_name);
    ClassDB::bind_method(D_METHOD("get_group_name"), &PLATEAUDynamicTile::get_group_name);
    ClassDB::bind_method(D_METHOD("get_load_state"), &PLATEAUDynamicTile::get_load_state);
    ClassDB::bind_method(D_METHOD("get_distance_from_camera"), &PLATEAUDynamicTile::get_distance_from_camera);
    ClassDB::bind_method(D_METHOD("get_loaded_instance"), &PLATEAUDynamicTile::get_loaded_instance);
    ClassDB::bind_method(D_METHOD("set_parent_tile", "parent"), &PLATEAUDynamicTile::set_parent_tile);
    ClassDB::bind_method(D_METHOD("get_parent_tile"), &PLATEAUDynamicTile::get_parent_tile);
    ClassDB::bind_method(D_METHOD("add_child_tile", "child"), &PLATEAUDynamicTile::add_child_tile);
    ClassDB::bind_method(D_METHOD("get_child_tiles"), &PLATEAUDynamicTile::get_child_tiles);
    ClassDB::bind_method(D_METHOD("get_child_count"), &PLATEAUDynamicTile::get_child_count);
    ClassDB::bind_method(D_METHOD("has_all_children_unloaded"), &PLATEAUDynamicTile::has_all_children_unloaded);
    ClassDB::bind_method(D_METHOD("has_any_children_loaded"), &PLATEAUDynamicTile::has_any_children_loaded);
    ClassDB::bind_method(D_METHOD("calculate_distance", "camera_pos", "ignore_y"), &PLATEAUDynamicTile::calculate_distance, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("is_within_range", "distance", "load_distances"), &PLATEAUDynamicTile::is_within_range);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "address"), "set_address", "get_address");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "zoom_level"), "set_zoom_level", "get_zoom_level");
    ADD_PROPERTY(PropertyInfo(Variant::AABB, "extent"), "set_extent", "get_extent");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "group_name"), "set_group_name", "get_group_name");

    BIND_ENUM_CONSTANT(LOAD_STATE_NONE);
    BIND_ENUM_CONSTANT(LOAD_STATE_LOADING);
    BIND_ENUM_CONSTANT(LOAD_STATE_LOADED);
    BIND_ENUM_CONSTANT(LOAD_STATE_UNLOADING);
    BIND_ENUM_CONSTANT(LOAD_STATE_UNLOADED);
    BIND_ENUM_CONSTANT(LOAD_STATE_ERROR);
}

// ============================================================================
// PLATEAUDynamicTileMetaInfo
// ============================================================================

PLATEAUDynamicTileMetaInfo::PLATEAUDynamicTileMetaInfo() :
    zoom_level_(0),
    lod_(0) {
}

PLATEAUDynamicTileMetaInfo::~PLATEAUDynamicTileMetaInfo() {
}

void PLATEAUDynamicTileMetaInfo::set_address(const String &address) {
    address_ = address;
}

String PLATEAUDynamicTileMetaInfo::get_address() const {
    return address_;
}

void PLATEAUDynamicTileMetaInfo::set_extent(const AABB &extent) {
    extent_ = extent;
}

AABB PLATEAUDynamicTileMetaInfo::get_extent() const {
    return extent_;
}

void PLATEAUDynamicTileMetaInfo::set_zoom_level(int level) {
    zoom_level_ = level;
}

int PLATEAUDynamicTileMetaInfo::get_zoom_level() const {
    return zoom_level_;
}

void PLATEAUDynamicTileMetaInfo::set_group_name(const String &name) {
    group_name_ = name;
}

String PLATEAUDynamicTileMetaInfo::get_group_name() const {
    return group_name_;
}

void PLATEAUDynamicTileMetaInfo::set_lod(int lod) {
    lod_ = lod;
}

int PLATEAUDynamicTileMetaInfo::get_lod() const {
    return lod_;
}

void PLATEAUDynamicTileMetaInfo::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_address", "address"), &PLATEAUDynamicTileMetaInfo::set_address);
    ClassDB::bind_method(D_METHOD("get_address"), &PLATEAUDynamicTileMetaInfo::get_address);
    ClassDB::bind_method(D_METHOD("set_extent", "extent"), &PLATEAUDynamicTileMetaInfo::set_extent);
    ClassDB::bind_method(D_METHOD("get_extent"), &PLATEAUDynamicTileMetaInfo::get_extent);
    ClassDB::bind_method(D_METHOD("set_zoom_level", "level"), &PLATEAUDynamicTileMetaInfo::set_zoom_level);
    ClassDB::bind_method(D_METHOD("get_zoom_level"), &PLATEAUDynamicTileMetaInfo::get_zoom_level);
    ClassDB::bind_method(D_METHOD("set_group_name", "name"), &PLATEAUDynamicTileMetaInfo::set_group_name);
    ClassDB::bind_method(D_METHOD("get_group_name"), &PLATEAUDynamicTileMetaInfo::get_group_name);
    ClassDB::bind_method(D_METHOD("set_lod", "lod"), &PLATEAUDynamicTileMetaInfo::set_lod);
    ClassDB::bind_method(D_METHOD("get_lod"), &PLATEAUDynamicTileMetaInfo::get_lod);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "address"), "set_address", "get_address");
    ADD_PROPERTY(PropertyInfo(Variant::AABB, "extent"), "set_extent", "get_extent");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "zoom_level"), "set_zoom_level", "get_zoom_level");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "group_name"), "set_group_name", "get_group_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lod"), "set_lod", "get_lod");
}

// ============================================================================
// PLATEAUDynamicTileMetaStore
// ============================================================================

PLATEAUDynamicTileMetaStore::PLATEAUDynamicTileMetaStore() {
}

PLATEAUDynamicTileMetaStore::~PLATEAUDynamicTileMetaStore() {
}

void PLATEAUDynamicTileMetaStore::set_tile_meta_infos(const TypedArray<PLATEAUDynamicTileMetaInfo> &infos) {
    tile_meta_infos_ = infos;
}

TypedArray<PLATEAUDynamicTileMetaInfo> PLATEAUDynamicTileMetaStore::get_tile_meta_infos() const {
    return tile_meta_infos_;
}

void PLATEAUDynamicTileMetaStore::set_reference_point(const Vector3 &point) {
    reference_point_ = point;
}

Vector3 PLATEAUDynamicTileMetaStore::get_reference_point() const {
    return reference_point_;
}

void PLATEAUDynamicTileMetaStore::add_tile_meta_info(const Ref<PLATEAUDynamicTileMetaInfo> &info) {
    if (info.is_valid()) {
        tile_meta_infos_.append(info);
    }
}

int PLATEAUDynamicTileMetaStore::get_tile_count() const {
    return tile_meta_infos_.size();
}

Ref<PLATEAUDynamicTileMetaInfo> PLATEAUDynamicTileMetaStore::get_tile_meta_info(int index) const {
    if (index < 0 || index >= tile_meta_infos_.size()) {
        return Ref<PLATEAUDynamicTileMetaInfo>();
    }
    return tile_meta_infos_[index];
}

Ref<PLATEAUDynamicTileMetaStore> PLATEAUDynamicTileMetaStore::from_json(const Dictionary &json) {
    Ref<PLATEAUDynamicTileMetaStore> store;
    store.instantiate();

    if (json.has("reference_point")) {
        Dictionary rp = json["reference_point"];
        store->set_reference_point(Vector3(rp["x"], rp["y"], rp["z"]));
    }

    if (json.has("tiles")) {
        Array tiles = json["tiles"];
        for (int i = 0; i < tiles.size(); i++) {
            Dictionary tile_data = tiles[i];
            Ref<PLATEAUDynamicTileMetaInfo> info;
            info.instantiate();

            if (tile_data.has("address")) info->set_address(tile_data["address"]);
            if (tile_data.has("zoom_level")) info->set_zoom_level(tile_data["zoom_level"]);
            if (tile_data.has("group_name")) info->set_group_name(tile_data["group_name"]);
            if (tile_data.has("lod")) info->set_lod(tile_data["lod"]);

            if (tile_data.has("extent")) {
                Dictionary ext = tile_data["extent"];
                AABB extent;
                extent.position = Vector3(ext["x"], ext["y"], ext["z"]);
                extent.size = Vector3(ext["width"], ext["height"], ext["depth"]);
                info->set_extent(extent);
            }

            store->add_tile_meta_info(info);
        }
    }

    return store;
}

Dictionary PLATEAUDynamicTileMetaStore::to_json() const {
    Dictionary result;

    Dictionary rp;
    rp["x"] = reference_point_.x;
    rp["y"] = reference_point_.y;
    rp["z"] = reference_point_.z;
    result["reference_point"] = rp;

    Array tiles;
    for (int i = 0; i < tile_meta_infos_.size(); i++) {
        Ref<PLATEAUDynamicTileMetaInfo> info = tile_meta_infos_[i];
        if (info.is_null()) continue;

        Dictionary tile_data;
        tile_data["address"] = info->get_address();
        tile_data["zoom_level"] = info->get_zoom_level();
        tile_data["group_name"] = info->get_group_name();
        tile_data["lod"] = info->get_lod();

        Dictionary ext;
        AABB extent = info->get_extent();
        ext["x"] = extent.position.x;
        ext["y"] = extent.position.y;
        ext["z"] = extent.position.z;
        ext["width"] = extent.size.x;
        ext["height"] = extent.size.y;
        ext["depth"] = extent.size.z;
        tile_data["extent"] = ext;

        tiles.append(tile_data);
    }
    result["tiles"] = tiles;

    return result;
}

void PLATEAUDynamicTileMetaStore::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_tile_meta_infos", "infos"), &PLATEAUDynamicTileMetaStore::set_tile_meta_infos);
    ClassDB::bind_method(D_METHOD("get_tile_meta_infos"), &PLATEAUDynamicTileMetaStore::get_tile_meta_infos);
    ClassDB::bind_method(D_METHOD("set_reference_point", "point"), &PLATEAUDynamicTileMetaStore::set_reference_point);
    ClassDB::bind_method(D_METHOD("get_reference_point"), &PLATEAUDynamicTileMetaStore::get_reference_point);
    ClassDB::bind_method(D_METHOD("add_tile_meta_info", "info"), &PLATEAUDynamicTileMetaStore::add_tile_meta_info);
    ClassDB::bind_method(D_METHOD("get_tile_count"), &PLATEAUDynamicTileMetaStore::get_tile_count);
    ClassDB::bind_method(D_METHOD("get_tile_meta_info", "index"), &PLATEAUDynamicTileMetaStore::get_tile_meta_info);
    ClassDB::bind_static_method("PLATEAUDynamicTileMetaStore", D_METHOD("from_json", "json"), &PLATEAUDynamicTileMetaStore::from_json);
    ClassDB::bind_method(D_METHOD("to_json"), &PLATEAUDynamicTileMetaStore::to_json);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tile_meta_infos", PROPERTY_HINT_ARRAY_TYPE, "PLATEAUDynamicTileMetaInfo"), "set_tile_meta_infos", "get_tile_meta_infos");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "reference_point"), "set_reference_point", "get_reference_point");
}

// ============================================================================
// PLATEAUDynamicTileManager
// ============================================================================

PLATEAUDynamicTileManager::PLATEAUDynamicTileManager() :
    state_(STATE_NONE),
    camera_(nullptr),
    auto_update_(true),
    ignore_y_(true),
    is_processing_(false),
    tiles_per_frame_(1) {

    // Default load distances
    load_distances_[11] = Vector2(-10000.0f, 500.0f);
    load_distances_[10] = Vector2(500.0f, 1500.0f);
    load_distances_[9] = Vector2(1500.0f, 10000.0f);
}

PLATEAUDynamicTileManager::~PLATEAUDynamicTileManager() {
}

void PLATEAUDynamicTileManager::_ready() {
    set_process(auto_update_);
}

void PLATEAUDynamicTileManager::_process(double delta) {
    if (!auto_update_ || state_ != STATE_OPERATING) {
        return;
    }

    // Get camera position
    Vector3 camera_pos;
    if (camera_) {
        camera_pos = camera_->get_global_position();
    } else {
        // Try to find current camera
        Viewport *viewport = get_viewport();
        if (viewport) {
            Camera3D *current_camera = viewport->get_camera_3d();
            if (current_camera) {
                camera_pos = current_camera->get_global_position();
            }
        }
    }

    // Check if camera moved significantly
    if (check_camera_position_changed(camera_pos, 10.0f)) {
        update_by_camera_position(camera_pos);
    }

    // Process load/unload queues
    process_queues();
}

PLATEAUDynamicTileManager::ManagerState PLATEAUDynamicTileManager::get_state() const {
    return state_;
}

void PLATEAUDynamicTileManager::set_camera(Camera3D *camera) {
    camera_ = camera;
}

Camera3D *PLATEAUDynamicTileManager::get_camera() const {
    return camera_;
}

void PLATEAUDynamicTileManager::set_auto_update(bool enable) {
    auto_update_ = enable;
    if (is_inside_tree()) {
        set_process(auto_update_);
    }
}

bool PLATEAUDynamicTileManager::get_auto_update() const {
    return auto_update_;
}

void PLATEAUDynamicTileManager::set_ignore_y(bool ignore) {
    ignore_y_ = ignore;
}

bool PLATEAUDynamicTileManager::get_ignore_y() const {
    return ignore_y_;
}

void PLATEAUDynamicTileManager::set_load_distance(int zoom_level, const Vector2 &range) {
    load_distances_[zoom_level] = range;
}

Vector2 PLATEAUDynamicTileManager::get_load_distance(int zoom_level) const {
    if (load_distances_.has(zoom_level)) {
        return load_distances_[zoom_level];
    }
    return Vector2(0, 0);
}

Dictionary PLATEAUDynamicTileManager::get_load_distances() const {
    return load_distances_;
}

void PLATEAUDynamicTileManager::set_load_distances(const Dictionary &distances) {
    load_distances_ = distances;
}

void PLATEAUDynamicTileManager::set_tile_base_path(const String &path) {
    tile_base_path_ = path;
}

String PLATEAUDynamicTileManager::get_tile_base_path() const {
    return tile_base_path_;
}

void PLATEAUDynamicTileManager::set_force_high_resolution_addresses(const PackedStringArray &addresses) {
    force_high_resolution_addresses_ = addresses;
}

PackedStringArray PLATEAUDynamicTileManager::get_force_high_resolution_addresses() const {
    return force_high_resolution_addresses_;
}

void PLATEAUDynamicTileManager::initialize(const Ref<PLATEAUDynamicTileMetaStore> &meta_store) {
    if (meta_store.is_null()) {
        UtilityFunctions::push_error("PLATEAUDynamicTileManager: meta_store is null");
        return;
    }

    state_ = STATE_INITIALIZING;
    tiles_.clear();
    address_to_tile_.clear();

    // Create tiles from meta info
    for (int i = 0; i < meta_store->get_tile_count(); i++) {
        Ref<PLATEAUDynamicTileMetaInfo> info = meta_store->get_tile_meta_info(i);
        if (info.is_null()) continue;

        Ref<PLATEAUDynamicTile> tile;
        tile.instantiate();
        tile->set_address(info->get_address());
        tile->set_zoom_level(info->get_zoom_level());
        tile->set_extent(info->get_extent());
        tile->set_group_name(info->get_group_name());

        tiles_.append(tile);
        address_to_tile_[info->get_address()] = tile;
    }

    // Build hierarchy
    build_tile_hierarchy();

    state_ = STATE_OPERATING;
    emit_signal("initialized");
}

void PLATEAUDynamicTileManager::update_by_camera_position(const Vector3 &position) {
    if (state_ != STATE_OPERATING || is_processing_.load()) {
        return;
    }

    is_processing_.store(true);
    last_camera_position_ = position;

    // Calculate distances
    calculate_distances(position);

    // Determine load states
    determine_load_states();

    // Fill holes
    fill_tile_holes();

    // Apply force high resolution
    if (force_high_resolution_addresses_.size() > 0) {
        apply_force_high_resolution();
    }

    // Execute load/unload
    execute_load_unload();

    is_processing_.store(false);
}

void PLATEAUDynamicTileManager::force_load_tiles(const PackedStringArray &addresses) {
    for (int i = 0; i < addresses.size(); i++) {
        if (address_to_tile_.has(addresses[i])) {
            Ref<PLATEAUDynamicTile> tile = address_to_tile_[addresses[i]];
            if (tile.is_valid() && tile->get_load_state() != PLATEAUDynamicTile::LOAD_STATE_LOADED) {
                load_tile(tile);
            }
        }
    }
}

void PLATEAUDynamicTileManager::cancel_load() {
    std::lock_guard<std::mutex> lock(load_mutex_);
    while (!load_queue_.empty()) {
        load_queue_.pop();
    }
}

TypedArray<PLATEAUDynamicTile> PLATEAUDynamicTileManager::get_tiles() const {
    return tiles_;
}

TypedArray<PLATEAUDynamicTile> PLATEAUDynamicTileManager::get_tiles_by_zoom_level(int zoom_level) const {
    TypedArray<PLATEAUDynamicTile> result;
    for (int i = 0; i < tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> tile = tiles_[i];
        if (tile.is_valid() && tile->get_zoom_level() == zoom_level) {
            result.append(tile);
        }
    }
    return result;
}

TypedArray<PLATEAUDynamicTile> PLATEAUDynamicTileManager::get_loaded_tiles() const {
    TypedArray<PLATEAUDynamicTile> result;
    for (int i = 0; i < tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> tile = tiles_[i];
        if (tile.is_valid() && tile->get_load_state() == PLATEAUDynamicTile::LOAD_STATE_LOADED) {
            result.append(tile);
        }
    }
    return result;
}

bool PLATEAUDynamicTileManager::check_camera_position_changed(const Vector3 &position, float threshold) const {
    if (ignore_y_) {
        Vector2 last_pos(last_camera_position_.x, last_camera_position_.z);
        Vector2 new_pos(position.x, position.z);
        return last_pos.distance_to(new_pos) > threshold;
    } else {
        return last_camera_position_.distance_to(position) > threshold;
    }
}

Vector3 PLATEAUDynamicTileManager::get_last_camera_position() const {
    return last_camera_position_;
}

void PLATEAUDynamicTileManager::cleanup() {
    state_ = STATE_CLEANING_UP;

    for (int i = 0; i < tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> tile = tiles_[i];
        if (tile.is_valid()) {
            unload_tile(tile);
        }
    }

    tiles_.clear();
    address_to_tile_.clear();
    state_ = STATE_NONE;
}

void PLATEAUDynamicTileManager::build_tile_hierarchy() {
    // Group tiles by zoom level
    TypedArray<PLATEAUDynamicTile> z11 = get_tiles_by_zoom_level(11);
    TypedArray<PLATEAUDynamicTile> z10 = get_tiles_by_zoom_level(10);
    TypedArray<PLATEAUDynamicTile> z9 = get_tiles_by_zoom_level(9);

    // Assign parents: z11 -> z10
    for (int i = 0; i < z11.size(); i++) {
        Ref<PLATEAUDynamicTile> inner = z11[i];
        if (inner.is_null()) continue;

        Vector3 inner_center = inner->get_extent().get_center();

        for (int j = 0; j < z10.size(); j++) {
            Ref<PLATEAUDynamicTile> outer = z10[j];
            if (outer.is_null()) continue;

            // Check if inner center is within outer extent and same group
            if (outer->get_extent().has_point(inner_center) &&
                outer->get_group_name() == inner->get_group_name()) {
                outer->add_child_tile(inner);
                break;
            }
        }
    }

    // Assign parents: z10 -> z9
    for (int i = 0; i < z10.size(); i++) {
        Ref<PLATEAUDynamicTile> inner = z10[i];
        if (inner.is_null()) continue;

        Vector3 inner_center = inner->get_extent().get_center();

        for (int j = 0; j < z9.size(); j++) {
            Ref<PLATEAUDynamicTile> outer = z9[j];
            if (outer.is_null()) continue;

            if (outer->get_extent().has_point(inner_center) &&
                outer->get_group_name() == inner->get_group_name()) {
                outer->add_child_tile(inner);
                break;
            }
        }
    }
}

void PLATEAUDynamicTileManager::calculate_distances(const Vector3 &camera_pos) {
    for (int i = 0; i < tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> tile = tiles_[i];
        if (tile.is_valid()) {
            float distance = tile->calculate_distance(camera_pos, ignore_y_);
            tile->set_distance(distance);
        }
    }
}

void PLATEAUDynamicTileManager::determine_load_states() {
    for (int i = 0; i < tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> tile = tiles_[i];
        if (tile.is_null()) continue;

        float distance = tile->get_distance_from_camera();
        bool in_range = tile->is_within_range(distance, load_distances_);

        if (in_range) {
            tile->set_next_load_state(PLATEAUDynamicTile::LOAD_STATE_LOADED);
        } else {
            tile->set_next_load_state(PLATEAUDynamicTile::LOAD_STATE_UNLOADED);
        }
    }
}

void PLATEAUDynamicTileManager::fill_tile_holes() {
    // For each tile marked as unload, check if we need to fill holes
    for (int i = 0; i < tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> tile = tiles_[i];
        if (tile.is_null()) continue;

        if (tile->next_load_state_ == PLATEAUDynamicTile::LOAD_STATE_UNLOADED) {
            // If parent is unloaded and has any children that should be loaded,
            // load all children to avoid holes
            if (tile->has_any_children_loaded() ||
                tile->next_load_state_ == PLATEAUDynamicTile::LOAD_STATE_LOADED) {

                TypedArray<PLATEAUDynamicTile> children = tile->get_child_tiles();
                for (int j = 0; j < children.size(); j++) {
                    Ref<PLATEAUDynamicTile> child = children[j];
                    if (child.is_valid() &&
                        child->next_load_state_ == PLATEAUDynamicTile::LOAD_STATE_LOADED) {
                        // Mark parent as needed to prevent holes
                        tile->set_next_load_state(PLATEAUDynamicTile::LOAD_STATE_LOADED);
                        break;
                    }
                }
            }
        }
    }
}

void PLATEAUDynamicTileManager::apply_force_high_resolution() {
    for (int i = 0; i < force_high_resolution_addresses_.size(); i++) {
        if (address_to_tile_.has(force_high_resolution_addresses_[i])) {
            Ref<PLATEAUDynamicTile> tile = address_to_tile_[force_high_resolution_addresses_[i]];
            if (tile.is_valid()) {
                tile->set_next_load_state(PLATEAUDynamicTile::LOAD_STATE_LOADED);

                // Unload parent tiles
                Ref<PLATEAUDynamicTile> parent = tile->get_parent_tile();
                while (parent.is_valid()) {
                    parent->set_next_load_state(PLATEAUDynamicTile::LOAD_STATE_UNLOADED);
                    parent = parent->get_parent_tile();
                }
            }
        }
    }
}

void PLATEAUDynamicTileManager::execute_load_unload() {
    // Sort by distance (closer tiles first for loading)
    std::vector<std::pair<float, Ref<PLATEAUDynamicTile>>> sorted_tiles;
    for (int i = 0; i < tiles_.size(); i++) {
        Ref<PLATEAUDynamicTile> tile = tiles_[i];
        if (tile.is_valid()) {
            sorted_tiles.push_back({tile->get_distance_from_camera(), tile});
        }
    }

    std::sort(sorted_tiles.begin(), sorted_tiles.end(),
        [](const auto &a, const auto &b) { return a.first < b.first; });

    std::lock_guard<std::mutex> lock(load_mutex_);

    for (const auto &pair : sorted_tiles) {
        Ref<PLATEAUDynamicTile> tile = pair.second;
        if (tile.is_null()) continue;

        PLATEAUDynamicTile::LoadState current = tile->get_load_state();
        PLATEAUDynamicTile::LoadState next = tile->next_load_state_;

        if (next == PLATEAUDynamicTile::LOAD_STATE_LOADED &&
            current != PLATEAUDynamicTile::LOAD_STATE_LOADED &&
            current != PLATEAUDynamicTile::LOAD_STATE_LOADING) {
            load_queue_.push(tile);
        } else if (next == PLATEAUDynamicTile::LOAD_STATE_UNLOADED &&
                   current == PLATEAUDynamicTile::LOAD_STATE_LOADED) {
            unload_queue_.push(tile);
        }
    }
}

void PLATEAUDynamicTileManager::process_queues() {
    std::lock_guard<std::mutex> lock(load_mutex_);

    int processed = 0;

    // Process unload queue first (to free resources)
    while (!unload_queue_.empty() && processed < tiles_per_frame_) {
        Ref<PLATEAUDynamicTile> tile = unload_queue_.front();
        unload_queue_.pop();
        unload_tile(tile);
        processed++;
    }

    // Process load queue
    while (!load_queue_.empty() && processed < tiles_per_frame_) {
        Ref<PLATEAUDynamicTile> tile = load_queue_.front();
        load_queue_.pop();
        load_tile(tile);
        processed++;
    }
}

void PLATEAUDynamicTileManager::load_tile(const Ref<PLATEAUDynamicTile> &tile) {
    if (tile.is_null()) return;

    tile->set_load_state(PLATEAUDynamicTile::LOAD_STATE_LOADING);

    // Load the scene
    String resource_path = tile_base_path_.path_join(tile->get_address() + ".tscn");

    if (!ResourceLoader::get_singleton()->exists(resource_path)) {
        resource_path = tile_base_path_.path_join(tile->get_address() + ".scn");
    }

    if (!ResourceLoader::get_singleton()->exists(resource_path)) {
        UtilityFunctions::push_warning("Tile resource not found: " + resource_path);
        tile->set_load_state(PLATEAUDynamicTile::LOAD_STATE_ERROR);
        return;
    }

    Node3D *instance = instantiate_tile(tile);
    if (instance) {
        tile->set_load_state(PLATEAUDynamicTile::LOAD_STATE_LOADED);
        emit_signal("tile_loaded", tile);
    } else {
        tile->set_load_state(PLATEAUDynamicTile::LOAD_STATE_ERROR);
    }
}

void PLATEAUDynamicTileManager::unload_tile(const Ref<PLATEAUDynamicTile> &tile) {
    if (tile.is_null()) return;

    tile->set_load_state(PLATEAUDynamicTile::LOAD_STATE_UNLOADING);

    Node3D *instance = tile->get_loaded_instance();
    if (instance) {
        emit_signal("tile_unloading", tile);
        instance->queue_free();
        tile->set_loaded_instance(nullptr);
    }

    tile->set_load_state(PLATEAUDynamicTile::LOAD_STATE_UNLOADED);
    emit_signal("tile_unloaded", tile);
}

Node3D *PLATEAUDynamicTileManager::instantiate_tile(const Ref<PLATEAUDynamicTile> &tile) {
    if (tile.is_null()) return nullptr;

    String resource_path = tile_base_path_.path_join(tile->get_address() + ".tscn");
    if (!ResourceLoader::get_singleton()->exists(resource_path)) {
        resource_path = tile_base_path_.path_join(tile->get_address() + ".scn");
    }

    Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(resource_path);
    if (scene.is_null()) {
        return nullptr;
    }

    Node *instance = scene->instantiate();
    Node3D *instance_3d = Object::cast_to<Node3D>(instance);

    if (instance_3d) {
        add_child(instance_3d);
        tile->set_loaded_instance(instance_3d);
    } else if (instance) {
        instance->queue_free();
    }

    return instance_3d;
}

void PLATEAUDynamicTileManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_state"), &PLATEAUDynamicTileManager::get_state);
    ClassDB::bind_method(D_METHOD("set_camera", "camera"), &PLATEAUDynamicTileManager::set_camera);
    ClassDB::bind_method(D_METHOD("get_camera"), &PLATEAUDynamicTileManager::get_camera);
    ClassDB::bind_method(D_METHOD("set_auto_update", "enable"), &PLATEAUDynamicTileManager::set_auto_update);
    ClassDB::bind_method(D_METHOD("get_auto_update"), &PLATEAUDynamicTileManager::get_auto_update);
    ClassDB::bind_method(D_METHOD("set_ignore_y", "ignore"), &PLATEAUDynamicTileManager::set_ignore_y);
    ClassDB::bind_method(D_METHOD("get_ignore_y"), &PLATEAUDynamicTileManager::get_ignore_y);
    ClassDB::bind_method(D_METHOD("set_load_distance", "zoom_level", "range"), &PLATEAUDynamicTileManager::set_load_distance);
    ClassDB::bind_method(D_METHOD("get_load_distance", "zoom_level"), &PLATEAUDynamicTileManager::get_load_distance);
    ClassDB::bind_method(D_METHOD("get_load_distances"), &PLATEAUDynamicTileManager::get_load_distances);
    ClassDB::bind_method(D_METHOD("set_load_distances", "distances"), &PLATEAUDynamicTileManager::set_load_distances);
    ClassDB::bind_method(D_METHOD("set_tile_base_path", "path"), &PLATEAUDynamicTileManager::set_tile_base_path);
    ClassDB::bind_method(D_METHOD("get_tile_base_path"), &PLATEAUDynamicTileManager::get_tile_base_path);
    ClassDB::bind_method(D_METHOD("set_force_high_resolution_addresses", "addresses"), &PLATEAUDynamicTileManager::set_force_high_resolution_addresses);
    ClassDB::bind_method(D_METHOD("get_force_high_resolution_addresses"), &PLATEAUDynamicTileManager::get_force_high_resolution_addresses);
    ClassDB::bind_method(D_METHOD("initialize", "meta_store"), &PLATEAUDynamicTileManager::initialize);
    ClassDB::bind_method(D_METHOD("update_by_camera_position", "position"), &PLATEAUDynamicTileManager::update_by_camera_position);
    ClassDB::bind_method(D_METHOD("force_load_tiles", "addresses"), &PLATEAUDynamicTileManager::force_load_tiles);
    ClassDB::bind_method(D_METHOD("cancel_load"), &PLATEAUDynamicTileManager::cancel_load);
    ClassDB::bind_method(D_METHOD("get_tiles"), &PLATEAUDynamicTileManager::get_tiles);
    ClassDB::bind_method(D_METHOD("get_tiles_by_zoom_level", "zoom_level"), &PLATEAUDynamicTileManager::get_tiles_by_zoom_level);
    ClassDB::bind_method(D_METHOD("get_loaded_tiles"), &PLATEAUDynamicTileManager::get_loaded_tiles);
    ClassDB::bind_method(D_METHOD("check_camera_position_changed", "position", "threshold"), &PLATEAUDynamicTileManager::check_camera_position_changed, DEFVAL(1.0f));
    ClassDB::bind_method(D_METHOD("get_last_camera_position"), &PLATEAUDynamicTileManager::get_last_camera_position);
    ClassDB::bind_method(D_METHOD("cleanup"), &PLATEAUDynamicTileManager::cleanup);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_NODE_TYPE, "Camera3D"), "set_camera", "get_camera");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_update"), "set_auto_update", "get_auto_update");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ignore_y"), "set_ignore_y", "get_ignore_y");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tile_base_path"), "set_tile_base_path", "get_tile_base_path");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "load_distances"), "set_load_distances", "get_load_distances");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "force_high_resolution_addresses"), "set_force_high_resolution_addresses", "get_force_high_resolution_addresses");

    ADD_SIGNAL(MethodInfo("initialized"));
    ADD_SIGNAL(MethodInfo("tile_loaded", PropertyInfo(Variant::OBJECT, "tile", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUDynamicTile")));
    ADD_SIGNAL(MethodInfo("tile_unloading", PropertyInfo(Variant::OBJECT, "tile", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUDynamicTile")));
    ADD_SIGNAL(MethodInfo("tile_unloaded", PropertyInfo(Variant::OBJECT, "tile", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUDynamicTile")));

    BIND_ENUM_CONSTANT(STATE_NONE);
    BIND_ENUM_CONSTANT(STATE_INITIALIZING);
    BIND_ENUM_CONSTANT(STATE_OPERATING);
    BIND_ENUM_CONSTANT(STATE_CLEANING_UP);
}

} // namespace godot
