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

protected:
    static void _bind_methods();

private:
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

    // Cached GeoReference
    mutable Ref<PLATEAUGeoReference> geo_reference_cache_;
};

} // namespace godot
