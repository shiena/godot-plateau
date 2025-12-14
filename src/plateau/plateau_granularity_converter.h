#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <plateau/granularity_convert/granularity_converter.h>
#include <plateau/polygon_mesh/model.h>

#include "plateau_city_model.h"

namespace godot {

// Granularity types for conversion
enum PLATEAUConvertGranularity : int {
    // Atomic feature (LOD2/3 components)
    CONVERT_GRANULARITY_ATOMIC = 0,
    // Primary feature (building, road, etc.)
    CONVERT_GRANULARITY_PRIMARY = 1,
    // City model area (entire GML file)
    CONVERT_GRANULARITY_AREA = 2,
    // Material-based within primary feature
    CONVERT_GRANULARITY_MATERIAL_IN_PRIMARY = 3,
};

/**
 * PLATEAUGranularityConverter - Convert mesh granularity
 *
 * Converts mesh data between different granularity levels:
 * - Atomic: Individual components (walls, roofs, etc.)
 * - Primary: Whole features (buildings, roads)
 * - Area: Entire model area
 * - MaterialInPrimary: Material-based grouping within primary features
 *
 * Usage:
 * ```gdscript
 * var converter = PLATEAUGranularityConverter.new()
 *
 * # Convert atomic meshes to primary (combine building parts)
 * var primary_meshes = converter.convert(
 *     atomic_mesh_array,
 *     PLATEAUMeshGranularity.GRANULARITY_PRIMARY
 * )
 *
 * # Convert to area with grid subdivision
 * converter.grid_count = 4  # 4x4 grid
 * var area_meshes = converter.convert(
 *     mesh_array,
 *     PLATEAUMeshGranularity.GRANULARITY_AREA
 * )
 * ```
 */
class PLATEAUGranularityConverter : public RefCounted {
    GDCLASS(PLATEAUGranularityConverter, RefCounted)

public:
    PLATEAUGranularityConverter();
    ~PLATEAUGranularityConverter();

    // Grid count for area granularity (creates NxN grid)
    void set_grid_count(int count);
    int get_grid_count() const;

    // Convert mesh data to specified granularity
    // Returns new array of converted PLATEAUMeshData
    TypedArray<PLATEAUMeshData> convert(
        const TypedArray<PLATEAUMeshData> &mesh_data_array,
        int target_granularity);

    // Get current granularity of mesh data (best guess based on structure)
    static int detect_granularity(const TypedArray<PLATEAUMeshData> &mesh_data_array);

protected:
    static void _bind_methods();

private:
    int grid_count_;

    // Internal helper: Convert mesh data to plateau model
    std::shared_ptr<plateau::polygonMesh::Model> create_model_from_mesh_data(
        const TypedArray<PLATEAUMeshData> &mesh_data_array);

    // Internal helper: Convert plateau model to mesh data
    TypedArray<PLATEAUMeshData> create_mesh_data_from_model(
        const plateau::polygonMesh::Model &model);

    // Internal helper: Add mesh data to node recursively
    void add_mesh_data_to_node(
        plateau::polygonMesh::Node &parent_node,
        const Ref<PLATEAUMeshData> &mesh_data);

    // Internal helper: Create mesh data from node
    Ref<PLATEAUMeshData> create_mesh_data_from_node(
        const plateau::polygonMesh::Node &node);
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PLATEAUConvertGranularity);
