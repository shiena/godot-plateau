#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "plateau_types.h"
#include "plateau_city_model.h"

#include <vector>
#include <memory>

namespace godot {
namespace plateau_utils {

/**
 * Compute smooth normals for a mesh using area-weighted face normals.
 *
 * @param vertices Vertex positions
 * @param indices Triangle indices (must be divisible by 3)
 * @return Computed normals array (same size as vertices)
 */
PackedVector3Array compute_smooth_normals(
    const PackedVector3Array &vertices,
    const PackedInt32Array &indices);

/**
 * Extract all vertices, indices, and UVs from a Godot ArrayMesh.
 * Supports multi-surface meshes by merging all surfaces with proper index offsets.
 *
 * @param godot_mesh Input Godot ArrayMesh
 * @param out_vertices Output vertices (TVec3d format for libplateau)
 * @param out_indices Output indices (unsigned int format for libplateau)
 * @param out_uvs Output UVs (TVec2f format for libplateau, Y-flipped)
 */
void extract_mesh_arrays(
    const Ref<ArrayMesh> &godot_mesh,
    std::vector<TVec3d> &out_vertices,
    std::vector<unsigned int> &out_indices,
    std::vector<TVec2f> &out_uvs);

/**
 * Convert an array of PLATEAUMeshData to a libplateau Model.
 *
 * @param mesh_data_array Array of PLATEAUMeshData objects
 * @param assign_hierarchy If true, calls model->assignNodeHierarchy() (required for HeightMapAligner)
 * @return Shared pointer to the created Model
 */
std::shared_ptr<PlateauModel> create_model_from_mesh_data(
    const TypedArray<PLATEAUMeshData> &mesh_data_array,
    bool assign_hierarchy = false);

/**
 * Convert a libplateau Model to an array of PLATEAUMeshData.
 *
 * @param model Input libplateau Model
 * @return Array of PLATEAUMeshData objects
 */
TypedArray<PLATEAUMeshData> create_mesh_data_from_model(
    const PlateauModel &model);

/**
 * Helper: Add mesh data to a PlateauNode (recursive for children).
 *
 * @param parent_node Node to add mesh to
 * @param mesh_data Source PLATEAUMeshData
 * @param merge_surfaces If true, merge all surfaces into one (for export/conversion)
 * @param include_uv4 If true, also set UV4 (required for HeightMapAligner)
 */
void add_mesh_data_to_node(
    PlateauNode &parent_node,
    const Ref<PLATEAUMeshData> &mesh_data,
    bool merge_surfaces = true,
    bool include_uv4 = false);

/**
 * Helper: Create PLATEAUMeshData from a PlateauNode (recursive for children).
 *
 * @param node Source PlateauNode
 * @return Created PLATEAUMeshData
 */
Ref<PLATEAUMeshData> create_mesh_data_from_node(
    const PlateauNode &node);

/**
 * Create a Godot ArrayMesh from libplateau mesh data with computed normals.
 *
 * @param vertices Vertex positions
 * @param indices Triangle indices
 * @param uvs Optional UV coordinates (Y will be flipped)
 * @return Created ArrayMesh
 */
Ref<ArrayMesh> create_array_mesh(
    const std::vector<TVec3d> &vertices,
    const std::vector<unsigned int> &indices,
    const std::vector<TVec2f> &uvs = {});

} // namespace plateau_utils
} // namespace godot
