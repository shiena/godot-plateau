#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "plateau_city_model.h"
#include "plateau_mesh_extract_options.h"
#include "plateau_geo_reference.h"

namespace godot {

class PLATEAUImporter : public Node3D {
    GDCLASS(PLATEAUImporter, Node3D)

public:
    PLATEAUImporter();
    ~PLATEAUImporter();

    // GML file path
    void set_gml_path(const String &path);
    String get_gml_path() const;

    // Mesh extract options
    void set_extract_options(const Ref<PLATEAUMeshExtractOptions> &options);
    Ref<PLATEAUMeshExtractOptions> get_extract_options() const;

    // Geo reference
    void set_geo_reference(const Ref<PLATEAUGeoReference> &geo_ref);
    Ref<PLATEAUGeoReference> get_geo_reference() const;

    // Import the CityGML file and create scene hierarchy
    bool import_gml();

    // Import from file path
    bool import_from_path(const String &gml_path);

    // Clear all imported meshes
    void clear_meshes();

    // Get the city model (for advanced usage)
    Ref<PLATEAUCityModel> get_city_model() const;

    // Check if import was successful
    bool is_imported() const;

    // Collision generation
    void set_generate_collision(bool enable);
    bool get_generate_collision() const;

    // Import mesh data array to scene and return root node
    // This creates a Node3D hierarchy from pre-extracted mesh data
    Node3D *import_to_scene(const TypedArray<PLATEAUMeshData> &mesh_data_array, const String &root_name);

protected:
    static void _bind_methods();

private:
    String gml_path_;
    Ref<PLATEAUMeshExtractOptions> extract_options_;
    Ref<PLATEAUGeoReference> geo_reference_;
    Ref<PLATEAUCityModel> city_model_;
    bool is_imported_;
    bool generate_collision_;

    // Helper methods
    void build_scene_hierarchy(const TypedArray<PLATEAUMeshData> &mesh_data_array, Node3D *parent);
    Node3D *create_node_from_mesh_data(const Ref<PLATEAUMeshData> &mesh_data);
    void create_collision_for_mesh(MeshInstance3D *mesh_instance);
};

} // namespace godot
