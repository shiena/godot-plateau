#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <plateau/height_map_generator/heightmap_generator.h>
#include <plateau/height_map_generator/heightmap_mesh_generator.h>
#include <plateau/height_map_generator/heightmap_types.h>
#include <plateau/polygon_mesh/mesh.h>
#include <plateau/geometry/geo_reference.h>

#include "plateau_city_model.h"

namespace godot {

// Forward declaration
class PLATEAUHeightMapAligner;

/**
 * PLATEAUHeightMapData - Stores heightmap data and metadata
 * Used for terrain conversion and height alignment
 */
class PLATEAUHeightMapData : public RefCounted {
    GDCLASS(PLATEAUHeightMapData, RefCounted)

public:
    PLATEAUHeightMapData();
    ~PLATEAUHeightMapData();

    // Name/identifier for this heightmap
    void set_name(const String &name);
    String get_name() const;

    // Heightmap dimensions
    int get_width() const;
    int get_height() const;

    // Bounding box in local coordinates
    Vector3 get_min_bounds() const;
    Vector3 get_max_bounds() const;

    // UV coordinates for texture mapping
    Vector2 get_min_uv() const;
    Vector2 get_max_uv() const;

    // Texture path for diffuse texture
    void set_texture_path(const String &path);
    String get_texture_path() const;

    // Get heightmap data as raw bytes (uint16 array)
    PackedByteArray get_heightmap_raw() const;

    // Get heightmap as normalized float array [0.0 - 1.0]
    PackedFloat32Array get_heightmap_normalized() const;

    // Save heightmap to file
    bool save_png(const String &path) const;
    bool save_raw(const String &path) const;

    // Generate mesh from heightmap
    Ref<ArrayMesh> generate_mesh() const;

    // Internal: Set data from generator
    void set_data(const plateau::heightMapGenerator::HeightMapT &heightmap,
                  int width, int height,
                  const TVec3d &min, const TVec3d &max,
                  const TVec2f &uv_min, const TVec2f &uv_max);

    // Internal: Get raw heightmap for HeightMapAligner
    const plateau::heightMapGenerator::HeightMapT& get_heightmap_internal() const;
    TVec3d get_min_internal() const;
    TVec3d get_max_internal() const;

protected:
    static void _bind_methods();

private:
    String name_;
    String texture_path_;
    int width_;
    int height_;
    TVec3d min_;
    TVec3d max_;
    TVec2f uv_min_;
    TVec2f uv_max_;
    plateau::heightMapGenerator::HeightMapT heightmap_data_;
};

/**
 * PLATEAUTerrain - Converts terrain meshes to heightmaps and smoothed meshes
 *
 * Usage:
 * ```gdscript
 * var terrain = PLATEAUTerrain.new()
 * terrain.texture_width = 257
 * terrain.texture_height = 257
 * var heightmap_data = terrain.generate_from_mesh(mesh_data)
 * var smoothed_mesh = heightmap_data.generate_mesh()
 * ```
 */
class PLATEAUTerrain : public RefCounted {
    GDCLASS(PLATEAUTerrain, RefCounted)

public:
    PLATEAUTerrain();
    ~PLATEAUTerrain();

    // Heightmap resolution
    void set_texture_width(int width);
    int get_texture_width() const;

    void set_texture_height(int height);
    int get_texture_height() const;

    // Offset margin for heightmap generation
    void set_offset(const Vector2 &offset);
    Vector2 get_offset() const;

    // Processing options
    void set_fill_edges(bool fill);
    bool get_fill_edges() const;

    void set_apply_blur_filter(bool apply);
    bool get_apply_blur_filter() const;

    // Generate heightmap from PLATEAUMeshData
    // Returns PLATEAUHeightMapData containing the generated heightmap
    Ref<PLATEAUHeightMapData> generate_from_mesh(const Ref<PLATEAUMeshData> &mesh_data);

    // Generate heightmap from array of PLATEAUMeshData (meshes are merged)
    // Returns PLATEAUHeightMapData containing the generated heightmap
    Ref<PLATEAUHeightMapData> generate_from_meshes(const TypedArray<PLATEAUMeshData> &mesh_data_array);

    // Generate heightmap from raw plateau::polygonMesh::Mesh
    // (Internal use, for direct mesh processing)
    Ref<PLATEAUHeightMapData> generate_from_plateau_mesh(const plateau::polygonMesh::Mesh &mesh, const String &name);

protected:
    static void _bind_methods();

private:
    int texture_width_;
    int texture_height_;
    Vector2 offset_;
    bool fill_edges_;
    bool apply_blur_filter_;
};

} // namespace godot
