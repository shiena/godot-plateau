#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "plateau_geo_reference.h"

namespace godot {

// PLATEAUInstancedCityModel - Root node for imported PLATEAU city model
// This node holds metadata about the imported city model and provides
// methods to access child GML nodes and LOD transforms.
// Similar to Unity SDK's PLATEAUInstancedCityModel.
class PLATEAUInstancedCityModel : public Node3D {
    GDCLASS(PLATEAUInstancedCityModel, Node3D)

public:
    // Default LOD distance constants
    static constexpr float DEFAULT_LOD2_DISTANCE = 200.0f;
    static constexpr float DEFAULT_LOD1_DISTANCE = 500.0f;

    PLATEAUInstancedCityModel();
    ~PLATEAUInstancedCityModel();

    // GeoReference data (serialized)
    void set_zone_id(int zone_id);
    int get_zone_id() const;

    void set_reference_point(const Vector3 &point);
    Vector3 get_reference_point() const;

    void set_unit_scale(float scale);
    float get_unit_scale() const;

    void set_coordinate_system(int system);
    int get_coordinate_system() const;

    // GML file path (for reference)
    void set_gml_path(const String &path);
    String get_gml_path() const;

    // Import settings (for reference)
    void set_min_lod(int lod);
    int get_min_lod() const;

    void set_max_lod(int lod);
    int get_max_lod() const;

    void set_mesh_granularity(int granularity);
    int get_mesh_granularity() const;

    // Computed properties (read-only in Inspector)
    double get_latitude() const;
    double get_longitude() const;

    // GeoReference object (for coordinate conversion)
    Ref<PLATEAUGeoReference> get_geo_reference() const;

    // Access child transforms
    // Returns all direct child Node3D (GML file nodes)
    TypedArray<Node3D> get_gml_transforms() const;

    // Returns available LOD numbers for a GML transform
    PackedInt32Array get_lods(Node3D *gml_transform) const;

    // Returns LOD transforms (children starting with "LOD")
    TypedArray<Node3D> get_lod_transforms(Node3D *gml_transform) const;

    // Returns all MeshInstance3D under a specific LOD
    TypedArray<Node3D> get_city_objects(Node3D *gml_transform, int lod) const;

    // Parse LOD number from node name (e.g., "LOD2" -> 2)
    static int parse_lod_from_name(const String &name);

    // LOD Auto-Switch settings
    void set_lod_auto_switch_enabled(bool enabled);
    bool get_lod_auto_switch_enabled() const;

    void set_lod2_distance(float distance);
    float get_lod2_distance() const;

    void set_lod1_distance(float distance);
    float get_lod1_distance() const;

    void set_lod_disable_in_editor(bool disable);
    bool get_lod_disable_in_editor() const;

    // Apply LOD settings to all MeshInstance3D children
    void apply_lod_settings();

    // Reset LOD settings (disable visibility_range)
    void reset_lod_settings();

    // Godot lifecycle
    void _ready();

protected:
    static void _bind_methods();

private:
    // Helper to recursively apply LOD settings
    void apply_lod_to_mesh(class MeshInstance3D *mesh, int lod, int max_lod, int min_lod, bool use_lod);

    // GeoReference data
    int zone_id_;
    Vector3 reference_point_;
    float unit_scale_;
    int coordinate_system_;

    // GML info
    String gml_path_;

    // Import settings
    int min_lod_;
    int max_lod_;
    int mesh_granularity_;

    // LOD Auto-Switch settings
    bool lod_auto_switch_enabled_;
    float lod2_distance_;
    float lod1_distance_;
    bool lod_disable_in_editor_;

    // Cached GeoReference
    mutable Ref<PLATEAUGeoReference> geo_reference_cache_;
};

} // namespace godot
