#include "plateau_importer.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

PLATEAUImporter::PLATEAUImporter()
    : is_imported_(false), generate_collision_(false), show_only_max_lod_(true) {
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
    ERR_FAIL_COND_V_MSG(gml_path_.is_empty(), false, "PLATEAUImporter: GML path is empty.");

    return import_from_path(gml_path_);
}

bool PLATEAUImporter::import_from_path(const String &gml_path) {
    // Clear previous import
    clear_meshes();

    // Create city model and load GML
    city_model_.instantiate();
    if (!city_model_->load(gml_path)) {
        is_imported_ = false;
        ERR_FAIL_V_MSG(false, "PLATEAUImporter: Failed to load GML file: " + gml_path);
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

    // Build scene hierarchy (this is in scene tree, so set proper owner)
    build_scene_hierarchy(mesh_data_array, this, get_owner() ? get_owner() : this);

    // Apply LOD visibility if enabled
    if (show_only_max_lod_) {
        apply_lod_visibility(this);
    }

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

PLATEAUInstancedCityModel *PLATEAUImporter::import_to_scene(
    const TypedArray<PLATEAUMeshData> &mesh_data_array,
    const String &root_name,
    const Ref<PLATEAUGeoReference> &geo_reference,
    const Ref<PLATEAUMeshExtractOptions> &options,
    const String &gml_path) {

    ERR_FAIL_COND_V_MSG(mesh_data_array.is_empty(), nullptr, "PLATEAUImporter: mesh_data_array is empty.");

    // Create PLATEAUInstancedCityModel as root node
    PLATEAUInstancedCityModel *root = memnew(PLATEAUInstancedCityModel);
    root->set_name(root_name.is_empty() ? "PLATEAU_Import" : root_name);

    // Set GeoReference data if provided
    if (geo_reference.is_valid()) {
        root->set_zone_id(geo_reference->get_zone_id());
        root->set_reference_point(geo_reference->get_reference_point());
        root->set_unit_scale(geo_reference->get_unit_scale());
        root->set_coordinate_system(geo_reference->get_coordinate_system());
    }

    // Set import options if provided
    if (options.is_valid()) {
        root->set_min_lod(options->get_min_lod());
        root->set_max_lod(options->get_max_lod());
        root->set_mesh_granularity(static_cast<int>(options->get_mesh_granularity()));
    }

    // Set GML path if provided
    if (!gml_path.is_empty()) {
        root->set_gml_path(gml_path);
    }

    // Build scene hierarchy under root
    // Pass nullptr as owner since root is standalone (not yet in scene tree)
    build_scene_hierarchy(mesh_data_array, root, nullptr);

    // Apply LOD visibility if enabled
    if (show_only_max_lod_) {
        apply_lod_visibility(root);
    }

    UtilityFunctions::print("PLATEAUImporter: Created scene with ", mesh_data_array.size(), " root meshes");
    return root;
}

void PLATEAUImporter::set_generate_collision(bool enable) {
    generate_collision_ = enable;
}

bool PLATEAUImporter::get_generate_collision() const {
    return generate_collision_;
}

void PLATEAUImporter::set_show_only_max_lod(bool enable) {
    show_only_max_lod_ = enable;
}

bool PLATEAUImporter::get_show_only_max_lod() const {
    return show_only_max_lod_;
}

int PLATEAUImporter::parse_lod_from_name(const String &name) {
    // Parse LOD number from node name (e.g., "LOD0", "LOD1", "LOD2")
    // Returns -1 if not an LOD node
    if (!name.begins_with("LOD")) {
        return -1;
    }
    String num_str = name.substr(3);
    if (num_str.is_valid_int()) {
        return num_str.to_int();
    }
    return -1;
}

void PLATEAUImporter::apply_lod_visibility(Node3D *root) {
    if (!root) {
        return;
    }

    // Collect all LOD nodes at the root level and find max LOD
    TypedArray<Node> children = root->get_children();
    int max_lod = -1;

    for (int i = 0; i < children.size(); i++) {
        Node3D *child = Object::cast_to<Node3D>(children[i].operator Object*());
        if (!child) {
            continue;
        }

        int lod = parse_lod_from_name(child->get_name());
        if (lod > max_lod) {
            max_lod = lod;
        }
    }

    // If no LOD nodes found, nothing to do
    if (max_lod < 0) {
        return;
    }

    // Set visibility: only max LOD is visible
    for (int i = 0; i < children.size(); i++) {
        Node3D *child = Object::cast_to<Node3D>(children[i].operator Object*());
        if (!child) {
            continue;
        }

        int lod = parse_lod_from_name(child->get_name());
        if (lod >= 0) {
            child->set_visible(lod == max_lod);
        }
    }

    UtilityFunctions::print("PLATEAUImporter: Applied LOD visibility (max LOD: ", max_lod, ")");
}

void PLATEAUImporter::build_scene_hierarchy(const TypedArray<PLATEAUMeshData> &mesh_data_array, Node3D *parent, Node *owner) {
    for (int i = 0; i < mesh_data_array.size(); i++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];
        if (mesh_data.is_null()) {
            continue;
        }

        Node3D *node = create_node_from_mesh_data(mesh_data);
        if (node) {
            parent->add_child(node);
            // Only set owner if provided (standalone scenes don't need owner set)
            if (owner != nullptr) {
                node->set_owner(owner);
            }

            // Recursively add children
            TypedArray<PLATEAUMeshData> children = mesh_data->get_children();
            if (!children.is_empty()) {
                build_scene_hierarchy(children, node, owner);
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

    // Set owner for scene serialization (nodes without owner are lost when saving)
    Node *owner = mesh_instance->get_owner();
    if (owner != nullptr) {
        static_body->set_owner(owner);
        collision_shape->set_owner(owner);
    }
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

    // LOD visibility control
    ClassDB::bind_method(D_METHOD("set_show_only_max_lod", "enable"), &PLATEAUImporter::set_show_only_max_lod);
    ClassDB::bind_method(D_METHOD("get_show_only_max_lod"), &PLATEAUImporter::get_show_only_max_lod);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_only_max_lod"), "set_show_only_max_lod", "get_show_only_max_lod");

    // Methods
    ClassDB::bind_method(D_METHOD("import_gml"), &PLATEAUImporter::import_gml);
    ClassDB::bind_method(D_METHOD("import_from_path", "gml_path"), &PLATEAUImporter::import_from_path);
    ClassDB::bind_method(D_METHOD("import_to_scene", "mesh_data_array", "root_name", "geo_reference", "options", "gml_path"), &PLATEAUImporter::import_to_scene, DEFVAL(Ref<PLATEAUGeoReference>()), DEFVAL(Ref<PLATEAUMeshExtractOptions>()), DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("clear_meshes"), &PLATEAUImporter::clear_meshes);
    ClassDB::bind_method(D_METHOD("get_city_model"), &PLATEAUImporter::get_city_model);
    ClassDB::bind_method(D_METHOD("is_imported"), &PLATEAUImporter::is_imported);
}
