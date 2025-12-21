#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "plateau_geo_reference.h"
#include "plateau_city_model.h"
#include "plateau_dataset_source.h"

namespace godot {

class PLATEAUFilterCondition;

/**
 * PLATEAUCityModelScene - Root node for imported PLATEAU city model
 *
 * Equivalent to Unity's PLATEAUInstancedCityModel.
 * Holds geo reference, imported GML transforms, and city model metadata.
 *
 * Usage:
 * ```gdscript
 * var scene = PLATEAUCityModelScene.new()
 * scene.geo_reference = geo_ref
 * add_child(scene)
 * scene.import_gml("path/to/file.gml", import_options)
 *
 * # Get latitude/longitude of origin
 * print("Origin: ", scene.latitude, ", ", scene.longitude)
 *
 * # Get all GML transforms
 * for gml_transform in scene.get_gml_transforms():
 *     print(gml_transform.name)
 * ```
 */
class PLATEAUCityModelScene : public Node3D {
    GDCLASS(PLATEAUCityModelScene, Node3D)

public:
    PLATEAUCityModelScene();
    ~PLATEAUCityModelScene();

    // GeoReference
    void set_geo_reference(const Ref<PLATEAUGeoReference> &geo_ref);
    Ref<PLATEAUGeoReference> get_geo_reference() const;

    // Origin coordinates (read-only, derived from geo_reference)
    double get_latitude() const;
    double get_longitude() const;

    // Get all GML Transform nodes (children that represent imported GML files)
    TypedArray<Node3D> get_gml_transforms() const;

    // Get Transform for specific GML file
    Node3D *get_gml_transform(const String &gml_path) const;

    // Import GML file and add as child
    // Returns the created transform node, or null on failure
    Node3D *import_gml(const String &gml_path, const Ref<PLATEAUMeshExtractOptions> &options);

    // Async version - emits "gml_imported" signal when done
    void import_gml_async(const String &gml_path, const Ref<PLATEAUMeshExtractOptions> &options);

    // Get available LODs for a GML transform
    PackedInt32Array get_lods(Node3D *gml_transform) const;

    // Get LOD transforms for a GML transform
    TypedArray<Node3D> get_lod_transforms(Node3D *gml_transform) const;

    // Get all CityObject nodes for a GML transform at specified LOD
    TypedArray<Node3D> get_city_objects(Node3D *gml_transform, int lod) const;

    // Get package type for a mesh data
    int64_t get_package(const Ref<PLATEAUMeshData> &mesh_data) const;

    // Get all mesh instances with PLATEAUMeshData in this scene
    TypedArray<MeshInstance3D> get_all_mesh_instances() const;

    // Find mesh data by GML ID
    Ref<PLATEAUMeshData> find_mesh_data_by_gml_id(const String &gml_id) const;

    // Filter condition (for UI display)
    void set_filter_condition(const Ref<PLATEAUFilterCondition> &condition);
    Ref<PLATEAUFilterCondition> get_filter_condition() const;

    // Copy geo reference from another scene
    void copy_from(PLATEAUCityModelScene *other);

    // Reload GML for a transform (re-import with same options)
    bool reload_gml(Node3D *gml_transform);

    // Remove GML transform and its children
    void remove_gml(Node3D *gml_transform);

protected:
    static void _bind_methods();

private:
    Ref<PLATEAUGeoReference> geo_reference_;
    Ref<PLATEAUFilterCondition> filter_condition_;

    // Mapping from GML path to transform node name
    Dictionary gml_path_to_node_;

    // Mapping from node name to import options (for reload)
    Dictionary node_to_options_;

    // Helper to recursively find mesh instances
    void collect_mesh_instances(Node *node, TypedArray<MeshInstance3D> &result) const;

    // Helper to get PLATEAUMeshData from MeshInstance3D metadata
    Ref<PLATEAUMeshData> get_mesh_data_from_instance(MeshInstance3D *instance) const;
};

/**
 * PLATEAUFilterCondition - Filter condition for city objects
 *
 * Used to filter which city objects to display/process based on type and LOD.
 *
 * Usage:
 * ```gdscript
 * var filter = PLATEAUFilterCondition.new()
 * filter.city_object_types = COT_Building | COT_Road
 * filter.min_lod = 1
 * filter.max_lod = 2
 *
 * if filter.matches(mesh_data):
 *     # Process this mesh
 * ```
 */
class PLATEAUFilterCondition : public Resource {
    GDCLASS(PLATEAUFilterCondition, Resource)

public:
    PLATEAUFilterCondition();
    ~PLATEAUFilterCondition();

    // City object type filter (bitmask of PLATEAUCityObjectType)
    void set_city_object_types(int64_t types);
    int64_t get_city_object_types() const;

    // LOD range
    void set_min_lod(int lod);
    int get_min_lod() const;

    void set_max_lod(int lod);
    int get_max_lod() const;

    // Package filter (bitmask of PLATEAUCityModelPackage)
    void set_packages(int64_t packages);
    int64_t get_packages() const;

    // Check if a mesh data matches this filter
    bool matches(const Ref<PLATEAUMeshData> &mesh_data) const;

    // Check if a city object type matches
    bool matches_type(int64_t city_object_type) const;

    // Check if LOD is in range
    bool matches_lod(int lod) const;

    // Create a filter for all types
    static Ref<PLATEAUFilterCondition> create_all();

    // Create a filter for buildings only
    static Ref<PLATEAUFilterCondition> create_buildings();

    // Create a filter for terrain only
    static Ref<PLATEAUFilterCondition> create_terrain();

protected:
    static void _bind_methods();

private:
    int64_t city_object_types_;
    int min_lod_;
    int max_lod_;
    int64_t packages_;
};

} // namespace godot
