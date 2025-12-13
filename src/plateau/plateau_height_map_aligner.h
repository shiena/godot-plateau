#pragma once

// Platform detection for mobile exclusions
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_OS_VISION) && TARGET_OS_VISION)
#define PLATEAU_MOBILE_PLATFORM 1
#endif

#ifndef PLATEAU_MOBILE_PLATFORM

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <plateau/height_map_alighner/height_map_aligner.h>
#include <plateau/polygon_mesh/model.h>

#include "plateau_terrain.h"
#include "plateau_city_model.h"

namespace godot {

/**
 * PLATEAUHeightMapAligner - Aligns model heights to terrain heightmaps or vice versa
 *
 * Usage for aligning buildings to terrain:
 * ```gdscript
 * var aligner = PLATEAUHeightMapAligner.new()
 * aligner.height_offset = 0.0
 * aligner.max_edge_length = 4.0  # meters
 * aligner.add_heightmap(terrain_heightmap_data)
 * var aligned_meshes = aligner.align(building_mesh_data_array)
 * ```
 *
 * Usage for aligning terrain to roads (inverse):
 * ```gdscript
 * var aligner = PLATEAUHeightMapAligner.new()
 * aligner.add_heightmap(terrain_heightmap_data)
 * var updated_heightmaps = aligner.align_invert(road_mesh_data_array)
 * ```
 */
class PLATEAUHeightMapAligner : public RefCounted {
    GDCLASS(PLATEAUHeightMapAligner, RefCounted)

public:
    PLATEAUHeightMapAligner();
    ~PLATEAUHeightMapAligner();

    // Height offset when aligning models to terrain
    // Use small negative values (e.g., -0.15) to sink buildings slightly into terrain
    void set_height_offset(double offset);
    double get_height_offset() const;

    // Maximum edge length for mesh subdivision during alignment
    // Smaller values = more accurate alignment but slower processing
    // Recommended: ~4.0 meters
    void set_max_edge_length(float length);
    float get_max_edge_length() const;

    // Parameters for inverse alignment (terrain to model)
    void set_alpha_expand_width(int width);
    int get_alpha_expand_width() const;

    void set_alpha_averaging_width(int width);
    int get_alpha_averaging_width() const;

    void set_invert_height_offset(double offset);
    double get_invert_height_offset() const;

    void set_skip_threshold_distance(float distance);
    float get_skip_threshold_distance() const;

    // Add a heightmap to use for alignment
    // Multiple heightmaps can be added for areas with multiple terrain pieces
    void add_heightmap(const Ref<PLATEAUHeightMapData> &heightmap_data);

    // Clear all heightmaps
    void clear_heightmaps();

    // Get number of registered heightmaps
    int get_heightmap_count() const;

    // Align model heights to the registered heightmaps
    // Returns array of aligned PLATEAUMeshData
    // Input meshes are modified in place when possible
    TypedArray<PLATEAUMeshData> align(const TypedArray<PLATEAUMeshData> &mesh_data_array);

    // Align heightmaps to model heights (inverse)
    // Used for aligning terrain to roads, etc.
    // Returns array of updated PLATEAUHeightMapData
    TypedArray<PLATEAUHeightMapData> align_invert(const TypedArray<PLATEAUMeshData> &mesh_data_array);

    // Get height at a specific XZ position from the heightmaps
    // Returns NaN if position is outside all heightmaps
    double get_height_at(const Vector2 &xz_position) const;

protected:
    static void _bind_methods();

private:
    double height_offset_;
    float max_edge_length_;
    int alpha_expand_width_;
    int alpha_averaging_width_;
    double invert_height_offset_;
    float skip_threshold_distance_;

    std::vector<Ref<PLATEAUHeightMapData>> heightmap_refs_;

    // Internal helper: Convert PLATEAUMeshData array to plateau::polygonMesh::Model
    std::shared_ptr<plateau::polygonMesh::Model> create_model_from_mesh_data(
        const TypedArray<PLATEAUMeshData> &mesh_data_array);

    // Internal helper: Update PLATEAUMeshData from aligned Model
    void update_mesh_data_from_model(
        TypedArray<PLATEAUMeshData> &mesh_data_array,
        const plateau::polygonMesh::Model &model);

    // Internal helper: Convert single MeshData to Node
    void add_mesh_data_to_node(
        plateau::polygonMesh::Node &parent_node,
        const Ref<PLATEAUMeshData> &mesh_data);

    // Internal helper: Update single mesh from node
    void update_single_mesh_from_node(
        Ref<PLATEAUMeshData> &mesh_data,
        const plateau::polygonMesh::Node &node);

    // Internal helper: Recursively update mesh data from node hierarchy
    void update_node_recursive(
        Ref<PLATEAUMeshData> &mesh_data,
        const plateau::polygonMesh::Node &node);
};

} // namespace godot

#endif // !PLATEAU_MOBILE_PLATFORM
