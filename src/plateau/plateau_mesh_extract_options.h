#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <plateau/polygon_mesh/mesh_extract_options.h>

namespace godot {

// Mesh granularity enum
enum class PLATEAUMeshGranularity {
    PER_ATOMIC_FEATURE_OBJECT = 0, // Finest: each building part separately
    PER_PRIMARY_FEATURE_OBJECT = 1, // Medium: each building/road as whole
    PER_CITY_MODEL_AREA = 2 // Coarse: entire GML file merged
};

class PLATEAUMeshExtractOptions : public RefCounted {
    GDCLASS(PLATEAUMeshExtractOptions, RefCounted)

public:
    PLATEAUMeshExtractOptions();
    ~PLATEAUMeshExtractOptions();

    // Reference point
    void set_reference_point(const Vector3 &point);
    Vector3 get_reference_point() const;

    // Coordinate system (0=ENU, 1=WUN, 2=ESU, 3=EUN)
    void set_coordinate_system(int system);
    int get_coordinate_system() const;

    // Mesh granularity
    void set_mesh_granularity(int granularity);
    int get_mesh_granularity() const;

    // LOD range
    void set_min_lod(int lod);
    int get_min_lod() const;
    void set_max_lod(int lod);
    int get_max_lod() const;

    // Export appearance (textures)
    void set_export_appearance(bool enable);
    bool get_export_appearance() const;

    // Grid count
    void set_grid_count_of_side(int count);
    int get_grid_count_of_side() const;

    // Unit scale
    void set_unit_scale(float scale);
    float get_unit_scale() const;

    // Coordinate zone ID (Japan)
    void set_coordinate_zone_id(int zone_id);
    int get_coordinate_zone_id() const;

    // Texture packing
    void set_enable_texture_packing(bool enable);
    bool get_enable_texture_packing() const;
    void set_texture_packing_resolution(int resolution);
    int get_texture_packing_resolution() const;

    // Highest LOD only
    void set_highest_lod_only(bool enable);
    bool get_highest_lod_only() const;

    // Get native options struct
    plateau::polygonMesh::MeshExtractOptions get_native() const;

protected:
    static void _bind_methods();

private:
    Vector3 reference_point_;
    int coordinate_system_;
    int mesh_granularity_;
    int min_lod_;
    int max_lod_;
    bool export_appearance_;
    int grid_count_of_side_;
    float unit_scale_;
    int coordinate_zone_id_;
    bool enable_texture_packing_;
    int texture_packing_resolution_;
    bool highest_lod_only_;
};

} // namespace godot
