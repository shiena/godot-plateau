#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/aabb.hpp>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

namespace godot {

class PLATEAUDynamicTileManager;

/**
 * PLATEAUDynamicTile - Single tile data
 *
 * Represents a single tile that can be loaded/unloaded based on camera distance.
 */
class PLATEAUDynamicTile : public RefCounted {
    GDCLASS(PLATEAUDynamicTile, RefCounted)

public:
    enum LoadState {
        LOAD_STATE_NONE = 0,
        LOAD_STATE_LOADING = 1,
        LOAD_STATE_LOADED = 2,
        LOAD_STATE_UNLOADING = 3,
        LOAD_STATE_UNLOADED = 4,
        LOAD_STATE_ERROR = 5,
    };

    PLATEAUDynamicTile();
    ~PLATEAUDynamicTile();

    // Tile address (unique identifier / resource path)
    void set_address(const String &address);
    String get_address() const;

    // Zoom level (higher = more detail)
    void set_zoom_level(int level);
    int get_zoom_level() const;

    // 3D extent (bounding box)
    void set_extent(const AABB &extent);
    AABB get_extent() const;

    // Group name (package category)
    void set_group_name(const String &name);
    String get_group_name() const;

    // Load state
    LoadState get_load_state() const;

    // Distance from camera (updated by manager)
    float get_distance_from_camera() const;

    // Loaded instance (null if not loaded)
    Node3D *get_loaded_instance() const;

    // Parent tile (lower zoom level)
    void set_parent_tile(const Ref<PLATEAUDynamicTile> &parent);
    Ref<PLATEAUDynamicTile> get_parent_tile() const;

    // Child tiles (higher zoom level)
    void add_child_tile(const Ref<PLATEAUDynamicTile> &child);
    TypedArray<PLATEAUDynamicTile> get_child_tiles() const;
    int get_child_count() const;
    bool has_all_children_unloaded() const;
    bool has_any_children_loaded() const;

    // Calculate distance to camera position
    float calculate_distance(const Vector3 &camera_pos, bool ignore_y = true) const;

    // Check if tile is within load range for its zoom level
    bool is_within_range(float distance, const Dictionary &load_distances) const;

protected:
    static void _bind_methods();

private:
    friend class PLATEAUDynamicTileManager;

    String address_;
    int zoom_level_;
    AABB extent_;
    String group_name_;
    LoadState load_state_;
    LoadState next_load_state_;
    float distance_from_camera_;
    Node3D *loaded_instance_;
    Ref<PLATEAUDynamicTile> parent_tile_;
    TypedArray<PLATEAUDynamicTile> child_tiles_;

    void set_load_state(LoadState state);
    void set_next_load_state(LoadState state);
    void set_distance(float distance);
    void set_loaded_instance(Node3D *instance);
};

/**
 * PLATEAUDynamicTileMetaInfo - Metadata for a single tile
 */
class PLATEAUDynamicTileMetaInfo : public RefCounted {
    GDCLASS(PLATEAUDynamicTileMetaInfo, RefCounted)

public:
    PLATEAUDynamicTileMetaInfo();
    ~PLATEAUDynamicTileMetaInfo();

    void set_address(const String &address);
    String get_address() const;

    void set_extent(const AABB &extent);
    AABB get_extent() const;

    void set_zoom_level(int level);
    int get_zoom_level() const;

    void set_group_name(const String &name);
    String get_group_name() const;

    void set_lod(int lod);
    int get_lod() const;

protected:
    static void _bind_methods();

private:
    String address_;
    AABB extent_;
    int zoom_level_;
    String group_name_;
    int lod_;
};

/**
 * PLATEAUDynamicTileMetaStore - Container for tile metadata
 */
class PLATEAUDynamicTileMetaStore : public Resource {
    GDCLASS(PLATEAUDynamicTileMetaStore, Resource)

public:
    PLATEAUDynamicTileMetaStore();
    ~PLATEAUDynamicTileMetaStore();

    void set_tile_meta_infos(const TypedArray<PLATEAUDynamicTileMetaInfo> &infos);
    TypedArray<PLATEAUDynamicTileMetaInfo> get_tile_meta_infos() const;

    void set_reference_point(const Vector3 &point);
    Vector3 get_reference_point() const;

    void add_tile_meta_info(const Ref<PLATEAUDynamicTileMetaInfo> &info);
    int get_tile_count() const;
    Ref<PLATEAUDynamicTileMetaInfo> get_tile_meta_info(int index) const;

    // Create from JSON data
    static Ref<PLATEAUDynamicTileMetaStore> from_json(const Dictionary &json);

    // Export to JSON
    Dictionary to_json() const;

protected:
    static void _bind_methods();

private:
    TypedArray<PLATEAUDynamicTileMetaInfo> tile_meta_infos_;
    Vector3 reference_point_;
};

/**
 * PLATEAUDynamicTileManager - Dynamic tile loading manager
 *
 * Manages automatic loading/unloading of tiles based on camera position.
 * Equivalent to Unity's PLATEAUTileManager.
 *
 * Usage:
 * ```gdscript
 * var manager = PLATEAUDynamicTileManager.new()
 * add_child(manager)
 *
 * # Set load distances per zoom level
 * manager.set_load_distance(11, Vector2(-10000, 500))   # High detail, close range
 * manager.set_load_distance(10, Vector2(500, 1500))    # Medium detail, medium range
 * manager.set_load_distance(9, Vector2(1500, 10000))   # Low detail, far range
 *
 * # Initialize with metadata store
 * await manager.initialize(meta_store, "res://tiles/")
 *
 * # Set camera (auto-updated if null, uses current camera)
 * manager.camera = $Camera3D
 *
 * # Enable auto-update (called every frame)
 * manager.auto_update = true
 *
 * # Or manually update
 * await manager.update_by_camera_position(camera.global_position)
 *
 * # Signals
 * manager.tile_loaded.connect(_on_tile_loaded)
 * manager.tile_unloaded.connect(_on_tile_unloaded)
 * ```
 */
class PLATEAUDynamicTileManager : public Node3D {
    GDCLASS(PLATEAUDynamicTileManager, Node3D)

public:
    enum ManagerState {
        STATE_NONE = 0,
        STATE_INITIALIZING = 1,
        STATE_OPERATING = 2,
        STATE_CLEANING_UP = 3,
    };

    PLATEAUDynamicTileManager();
    ~PLATEAUDynamicTileManager();

    // Lifecycle
    void _ready() override;
    void _process(double delta) override;

    // State
    ManagerState get_state() const;

    // Camera to track
    void set_camera(Camera3D *camera);
    Camera3D *get_camera() const;

    // Auto update flag
    void set_auto_update(bool enable);
    bool get_auto_update() const;

    // Ignore Y axis for distance calculation
    void set_ignore_y(bool ignore);
    bool get_ignore_y() const;

    // Load distances per zoom level (key: zoom level, value: Vector2(min, max))
    void set_load_distance(int zoom_level, const Vector2 &range);
    Vector2 get_load_distance(int zoom_level) const;
    Dictionary get_load_distances() const;
    void set_load_distances(const Dictionary &distances);

    // Tile base path (directory containing tile scenes/resources)
    void set_tile_base_path(const String &path);
    String get_tile_base_path() const;

    // Force high resolution tile addresses
    void set_force_high_resolution_addresses(const PackedStringArray &addresses);
    PackedStringArray get_force_high_resolution_addresses() const;

    // Initialize with metadata store
    void initialize(const Ref<PLATEAUDynamicTileMetaStore> &meta_store);

    // Update tiles by camera position (async)
    void update_by_camera_position(const Vector3 &position);

    // Force load specific tiles by address
    void force_load_tiles(const PackedStringArray &addresses);

    // Cancel current load operation
    void cancel_load();

    // Get all tiles
    TypedArray<PLATEAUDynamicTile> get_tiles() const;

    // Get tiles by zoom level
    TypedArray<PLATEAUDynamicTile> get_tiles_by_zoom_level(int zoom_level) const;

    // Get loaded tiles
    TypedArray<PLATEAUDynamicTile> get_loaded_tiles() const;

    // Check if camera position has changed significantly
    bool check_camera_position_changed(const Vector3 &position, float threshold = 1.0f) const;

    // Get last camera position
    Vector3 get_last_camera_position() const;

    // Cleanup all tiles
    void cleanup();

protected:
    static void _bind_methods();

private:
    ManagerState state_;
    Camera3D *camera_;
    bool auto_update_;
    bool ignore_y_;
    Dictionary load_distances_;
    String tile_base_path_;
    PackedStringArray force_high_resolution_addresses_;
    Vector3 last_camera_position_;

    TypedArray<PLATEAUDynamicTile> tiles_;
    Dictionary address_to_tile_;

    // Processing state
    std::atomic<bool> is_processing_;
    std::mutex load_mutex_;
    std::queue<Ref<PLATEAUDynamicTile>> load_queue_;
    std::queue<Ref<PLATEAUDynamicTile>> unload_queue_;

    // Frame-based instantiation
    int tiles_per_frame_;
    void process_queues();

    // Internal methods
    void build_tile_hierarchy();
    void calculate_distances(const Vector3 &camera_pos);
    void determine_load_states();
    void fill_tile_holes();
    void apply_force_high_resolution();
    void execute_load_unload();

    void load_tile(const Ref<PLATEAUDynamicTile> &tile);
    void unload_tile(const Ref<PLATEAUDynamicTile> &tile);
    Node3D *instantiate_tile(const Ref<PLATEAUDynamicTile> &tile);
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PLATEAUDynamicTile::LoadState);
VARIANT_ENUM_CAST(godot::PLATEAUDynamicTileManager::ManagerState);
