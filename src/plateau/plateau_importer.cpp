#include "plateau_importer.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

PLATEAUImporter::PLATEAUImporter()
    : is_imported_(false), generate_collision_(false) {
    // Note: extract_options_ and geo_reference_ are created lazily
    // to avoid "Instantiated ... used as default value" warning
}

PLATEAUImporter::~PLATEAUImporter() {
}

void PLATEAUImporter::set_gml_path(const String &path) {
    gml_path_ = path;
}

String PLATEAUImporter::get_gml_path() const {
    return gml_path_;
}

void PLATEAUImporter::set_extract_options(const Ref<PLATEAUMeshExtractOptions> &options) {
    extract_options_ = options;
}

Ref<PLATEAUMeshExtractOptions> PLATEAUImporter::get_extract_options() const {
    return extract_options_;
}

void PLATEAUImporter::set_geo_reference(const Ref<PLATEAUGeoReference> &geo_ref) {
    geo_reference_ = geo_ref;
}

Ref<PLATEAUGeoReference> PLATEAUImporter::get_geo_reference() const {
    return geo_reference_;
}

bool PLATEAUImporter::import_gml() {
    if (gml_path_.is_empty()) {
        UtilityFunctions::printerr("PLATEAUImporter: GML path is empty");
        return false;
    }

    return import_from_path(gml_path_);
}

bool PLATEAUImporter::import_from_path(const String &gml_path) {
    // Clear previous import
    clear_meshes();

    // Create city model and load GML
    city_model_.instantiate();
    if (!city_model_->load(gml_path)) {
        UtilityFunctions::printerr("PLATEAUImporter: Failed to load GML file: ", gml_path);
        is_imported_ = false;
        return false;
    }

    // Ensure options are instantiated
    if (extract_options_.is_null()) {
        extract_options_.instantiate();
    }
    if (geo_reference_.is_null()) {
        geo_reference_.instantiate();
    }

    // Update geo reference with center point
    Vector3 center = city_model_->get_center_point(geo_reference_->get_zone_id());
    geo_reference_->set_reference_point(center);

    // Also update extract options reference point
    extract_options_->set_reference_point(center);
    extract_options_->set_coordinate_zone_id(geo_reference_->get_zone_id());
    extract_options_->set_coordinate_system(geo_reference_->get_coordinate_system());

    TypedArray<PLATEAUMeshData> mesh_data_array = city_model_->extract_meshes(extract_options_);

    if (mesh_data_array.is_empty()) {
        UtilityFunctions::print("PLATEAUImporter: No meshes extracted");
        is_imported_ = true;
        return true;
    }

    // Build scene hierarchy
    build_scene_hierarchy(mesh_data_array, this);

    is_imported_ = true;
    gml_path_ = gml_path;

    UtilityFunctions::print("PLATEAUImporter: Successfully imported ", gml_path);
    return true;
}

void PLATEAUImporter::clear_meshes() {
    // Remove all children
    TypedArray<Node> children = get_children();
    for (int i = children.size() - 1; i >= 0; i--) {
        Node *child = Object::cast_to<Node>(children[i]);
        if (child) {
            remove_child(child);
            child->queue_free();
        }
    }

    is_imported_ = false;
}

Ref<PLATEAUCityModel> PLATEAUImporter::get_city_model() const {
    return city_model_;
}

bool PLATEAUImporter::is_imported() const {
    return is_imported_;
}

Node3D *PLATEAUImporter::import_to_scene(const TypedArray<PLATEAUMeshData> &mesh_data_array, const String &root_name) {
    if (mesh_data_array.is_empty()) {
        UtilityFunctions::printerr("PLATEAUImporter: mesh_data_array is empty");
        return nullptr;
    }

    // Create root node
    Node3D *root = memnew(Node3D);
    root->set_name(root_name.is_empty() ? "PLATEAU_Import" : root_name);

    // Build scene hierarchy under root
    build_scene_hierarchy(mesh_data_array, root);

    UtilityFunctions::print("PLATEAUImporter: Created scene with ", mesh_data_array.size(), " root meshes");
    return root;
}

void PLATEAUImporter::set_generate_collision(bool enable) {
    generate_collision_ = enable;
}

bool PLATEAUImporter::get_generate_collision() const {
    return generate_collision_;
}

void PLATEAUImporter::build_scene_hierarchy(const TypedArray<PLATEAUMeshData> &mesh_data_array, Node3D *parent) {
    for (int i = 0; i < mesh_data_array.size(); i++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];
        if (mesh_data.is_null()) {
            continue;
        }

        Node3D *node = create_node_from_mesh_data(mesh_data);
        if (node) {
            parent->add_child(node);
            node->set_owner(get_owner() ? get_owner() : this);

            // Recursively add children
            TypedArray<PLATEAUMeshData> children = mesh_data->get_children();
            if (!children.is_empty()) {
                build_scene_hierarchy(children, node);
            }
        }
    }
}

Node3D *PLATEAUImporter::create_node_from_mesh_data(const Ref<PLATEAUMeshData> &mesh_data) {
    if (mesh_data.is_null()) {
        return nullptr;
    }

    Ref<ArrayMesh> mesh = mesh_data->get_mesh();

    if (mesh.is_valid() && mesh->get_surface_count() > 0) {
        // Create MeshInstance3D for nodes with meshes
        MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
        mesh_instance->set_name(mesh_data->get_name());
        mesh_instance->set_mesh(mesh);
        mesh_instance->set_transform(mesh_data->get_transform());

        // Generate collision if enabled
        if (generate_collision_) {
            create_collision_for_mesh(mesh_instance);
        }

        return mesh_instance;
    } else {
        // Create Node3D for nodes without meshes (containers)
        Node3D *node = memnew(Node3D);
        node->set_name(mesh_data->get_name());
        node->set_transform(mesh_data->get_transform());
        return node;
    }
}

void PLATEAUImporter::create_collision_for_mesh(MeshInstance3D *mesh_instance) {
    if (!mesh_instance) {
        return;
    }

    Ref<Mesh> mesh = mesh_instance->get_mesh();
    if (mesh.is_null()) {
        return;
    }

    // Create trimesh collision shape from mesh
    Ref<ConcavePolygonShape3D> shape;
    shape.instantiate();

    // Get faces from all surfaces
    PackedVector3Array faces;
    for (int surface_idx = 0; surface_idx < mesh->get_surface_count(); surface_idx++) {
        Array arrays = mesh->surface_get_arrays(surface_idx);
        if (arrays.size() == 0) {
            continue;
        }

        PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
        PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX];

        if (indices.size() > 0) {
            // Indexed mesh
            for (int i = 0; i < indices.size(); i++) {
                faces.push_back(vertices[indices[i]]);
            }
        } else {
            // Non-indexed mesh
            for (int i = 0; i < vertices.size(); i++) {
                faces.push_back(vertices[i]);
            }
        }
    }

    if (faces.size() < 3) {
        return;
    }

    shape->set_faces(faces);

    // Create StaticBody3D and CollisionShape3D
    StaticBody3D *static_body = memnew(StaticBody3D);
    static_body->set_name(String(mesh_instance->get_name()) + "_collision");

    CollisionShape3D *collision_shape = memnew(CollisionShape3D);
    collision_shape->set_name("CollisionShape3D");
    collision_shape->set_shape(shape);

    static_body->add_child(collision_shape);
    mesh_instance->add_child(static_body);
}

void PLATEAUImporter::_bind_methods() {
    // GML path
    ClassDB::bind_method(D_METHOD("set_gml_path", "path"), &PLATEAUImporter::set_gml_path);
    ClassDB::bind_method(D_METHOD("get_gml_path"), &PLATEAUImporter::get_gml_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "gml_path", PROPERTY_HINT_FILE, "*.gml"), "set_gml_path", "get_gml_path");

    // Extract options
    ClassDB::bind_method(D_METHOD("set_extract_options", "options"), &PLATEAUImporter::set_extract_options);
    ClassDB::bind_method(D_METHOD("get_extract_options"), &PLATEAUImporter::get_extract_options);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "extract_options", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUMeshExtractOptions"), "set_extract_options", "get_extract_options");

    // Geo reference
    ClassDB::bind_method(D_METHOD("set_geo_reference", "geo_ref"), &PLATEAUImporter::set_geo_reference);
    ClassDB::bind_method(D_METHOD("get_geo_reference"), &PLATEAUImporter::get_geo_reference);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "geo_reference", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUGeoReference"), "set_geo_reference", "get_geo_reference");

    // Collision generation
    ClassDB::bind_method(D_METHOD("set_generate_collision", "enable"), &PLATEAUImporter::set_generate_collision);
    ClassDB::bind_method(D_METHOD("get_generate_collision"), &PLATEAUImporter::get_generate_collision);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "generate_collision"), "set_generate_collision", "get_generate_collision");

    // Methods
    ClassDB::bind_method(D_METHOD("import_gml"), &PLATEAUImporter::import_gml);
    ClassDB::bind_method(D_METHOD("import_from_path", "gml_path"), &PLATEAUImporter::import_from_path);
    ClassDB::bind_method(D_METHOD("import_to_scene", "mesh_data_array", "root_name"), &PLATEAUImporter::import_to_scene);
    ClassDB::bind_method(D_METHOD("clear_meshes"), &PLATEAUImporter::clear_meshes);
    ClassDB::bind_method(D_METHOD("get_city_model"), &PLATEAUImporter::get_city_model);
    ClassDB::bind_method(D_METHOD("is_imported"), &PLATEAUImporter::is_imported);
}
