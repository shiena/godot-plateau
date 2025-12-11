#include "plateau_city_model.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>
#include <unordered_set>
#include <unordered_map>

using namespace godot;

// Type aliases to avoid name collision with godot::Node and godot::Mesh
using PlateauNode = plateau::polygonMesh::Node;
using PlateauMesh = plateau::polygonMesh::Mesh;
using PlateauModel = plateau::polygonMesh::Model;
using PlateauSubMesh = plateau::polygonMesh::SubMesh;
using PlateauCityObjectList = plateau::polygonMesh::CityObjectList;
using PlateauCityObjectIndex = plateau::polygonMesh::CityObjectIndex;

// Helper function to convert CityObjectType to string
static String city_object_type_to_string(int64_t type) {
    if (type & PLATEAUCityObjectType::COT_Building) return "Building";
    if (type & PLATEAUCityObjectType::COT_BuildingPart) return "BuildingPart";
    if (type & PLATEAUCityObjectType::COT_Road) return "Road";
    if (type & PLATEAUCityObjectType::COT_Railway) return "Railway";
    if (type & PLATEAUCityObjectType::COT_Track) return "Track";
    if (type & PLATEAUCityObjectType::COT_Square) return "Square";
    if (type & PLATEAUCityObjectType::COT_Bridge) return "Bridge";
    if (type & PLATEAUCityObjectType::COT_Tunnel) return "Tunnel";
    if (type & PLATEAUCityObjectType::COT_LandUse) return "LandUse";
    if (type & PLATEAUCityObjectType::COT_WaterBody) return "WaterBody";
    if (type & PLATEAUCityObjectType::COT_PlantCover) return "PlantCover";
    if (type & PLATEAUCityObjectType::COT_SolitaryVegetationObject) return "SolitaryVegetationObject";
    if (type & PLATEAUCityObjectType::COT_CityFurniture) return "CityFurniture";
    if (type & PLATEAUCityObjectType::COT_ReliefFeature) return "ReliefFeature";
    if (type & PLATEAUCityObjectType::COT_TINRelief) return "TINRelief";
    if (type & PLATEAUCityObjectType::COT_WallSurface) return "WallSurface";
    if (type & PLATEAUCityObjectType::COT_RoofSurface) return "RoofSurface";
    if (type & PLATEAUCityObjectType::COT_GroundSurface) return "GroundSurface";
    if (type & PLATEAUCityObjectType::COT_GenericCityObject) return "GenericCityObject";
    if (type & PLATEAUCityObjectType::COT_Unknown) return "Unknown";
    return "Unknown";
}

// PLATEAUMeshData implementation
PLATEAUMeshData::PLATEAUMeshData()
    : city_object_type_(0) {
}

PLATEAUMeshData::~PLATEAUMeshData() {
}

void PLATEAUMeshData::set_name(const String &name) {
    name_ = name;
}

String PLATEAUMeshData::get_name() const {
    return name_;
}

void PLATEAUMeshData::set_mesh(const Ref<ArrayMesh> &mesh) {
    mesh_ = mesh;
}

Ref<ArrayMesh> PLATEAUMeshData::get_mesh() const {
    return mesh_;
}

void PLATEAUMeshData::set_transform(const Transform3D &transform) {
    transform_ = transform;
}

Transform3D PLATEAUMeshData::get_transform() const {
    return transform_;
}

void PLATEAUMeshData::add_child(const Ref<PLATEAUMeshData> &child) {
    children_.push_back(child);
}

TypedArray<PLATEAUMeshData> PLATEAUMeshData::get_children() const {
    return children_;
}

int PLATEAUMeshData::get_child_count() const {
    return children_.size();
}

Ref<PLATEAUMeshData> PLATEAUMeshData::get_child(int index) const {
    if (index >= 0 && index < children_.size()) {
        return children_[index];
    }
    return Ref<PLATEAUMeshData>();
}

// Phase 1: Attribute access methods
void PLATEAUMeshData::set_gml_id(const String &gml_id) {
    gml_id_ = gml_id;
}

String PLATEAUMeshData::get_gml_id() const {
    return gml_id_;
}

void PLATEAUMeshData::set_city_object_type(int64_t type) {
    city_object_type_ = type;
}

int64_t PLATEAUMeshData::get_city_object_type() const {
    return city_object_type_;
}

void PLATEAUMeshData::set_attributes(const Dictionary &attributes) {
    attributes_ = attributes;
}

Dictionary PLATEAUMeshData::get_attributes() const {
    return attributes_;
}

Variant PLATEAUMeshData::get_attribute(const String &key) const {
    // Support nested keys with "/" separator
    PackedStringArray keys = key.split("/");

    if (keys.size() == 0) {
        return Variant();
    }

    Dictionary current = attributes_;
    for (int i = 0; i < keys.size() - 1; i++) {
        if (!current.has(keys[i])) {
            return Variant();
        }
        Variant val = current[keys[i]];
        if (val.get_type() != Variant::DICTIONARY) {
            return Variant();
        }
        current = val;
    }

    String final_key = keys[keys.size() - 1];
    if (current.has(final_key)) {
        return current[final_key];
    }
    return Variant();
}

bool PLATEAUMeshData::has_city_object_info() const {
    return !gml_id_.is_empty();
}

String PLATEAUMeshData::get_city_object_type_name() const {
    return city_object_type_to_string(city_object_type_);
}

void PLATEAUMeshData::set_city_object_list(const PlateauCityObjectList &list) {
    city_object_list_ = list;
}

const PlateauCityObjectList& PLATEAUMeshData::get_city_object_list_internal() const {
    return city_object_list_;
}

String PLATEAUMeshData::get_gml_id_from_uv(const Vector2 &uv) const {
    PlateauCityObjectIndex index = PlateauCityObjectIndex::fromUV({uv.x, uv.y});
    std::string gml_id;
    if (city_object_list_.tryGetAtomicGmlID(index, gml_id)) {
        return String::utf8(gml_id.c_str());
    }
    // Try primary index if atomic not found
    if (city_object_list_.tryGetPrimaryGmlID(index.primary_index, gml_id)) {
        return String::utf8(gml_id.c_str());
    }
    return String();
}

void PLATEAUMeshData::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_name", "name"), &PLATEAUMeshData::set_name);
    ClassDB::bind_method(D_METHOD("get_name"), &PLATEAUMeshData::get_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");

    ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &PLATEAUMeshData::set_mesh);
    ClassDB::bind_method(D_METHOD("get_mesh"), &PLATEAUMeshData::get_mesh);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "ArrayMesh"), "set_mesh", "get_mesh");

    ClassDB::bind_method(D_METHOD("set_transform", "transform"), &PLATEAUMeshData::set_transform);
    ClassDB::bind_method(D_METHOD("get_transform"), &PLATEAUMeshData::get_transform);
    ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "transform"), "set_transform", "get_transform");

    ClassDB::bind_method(D_METHOD("add_child", "child"), &PLATEAUMeshData::add_child);
    ClassDB::bind_method(D_METHOD("get_children"), &PLATEAUMeshData::get_children);
    ClassDB::bind_method(D_METHOD("get_child_count"), &PLATEAUMeshData::get_child_count);
    ClassDB::bind_method(D_METHOD("get_child", "index"), &PLATEAUMeshData::get_child);

    // Phase 1: Attribute access bindings
    ClassDB::bind_method(D_METHOD("set_gml_id", "gml_id"), &PLATEAUMeshData::set_gml_id);
    ClassDB::bind_method(D_METHOD("get_gml_id"), &PLATEAUMeshData::get_gml_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "gml_id"), "set_gml_id", "get_gml_id");

    ClassDB::bind_method(D_METHOD("set_city_object_type", "type"), &PLATEAUMeshData::set_city_object_type);
    ClassDB::bind_method(D_METHOD("get_city_object_type"), &PLATEAUMeshData::get_city_object_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "city_object_type"), "set_city_object_type", "get_city_object_type");

    ClassDB::bind_method(D_METHOD("set_attributes", "attributes"), &PLATEAUMeshData::set_attributes);
    ClassDB::bind_method(D_METHOD("get_attributes"), &PLATEAUMeshData::get_attributes);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "attributes"), "set_attributes", "get_attributes");

    ClassDB::bind_method(D_METHOD("get_attribute", "key"), &PLATEAUMeshData::get_attribute);
    ClassDB::bind_method(D_METHOD("has_city_object_info"), &PLATEAUMeshData::has_city_object_info);
    ClassDB::bind_method(D_METHOD("get_city_object_type_name"), &PLATEAUMeshData::get_city_object_type_name);
    ClassDB::bind_method(D_METHOD("get_gml_id_from_uv", "uv"), &PLATEAUMeshData::get_gml_id_from_uv);

    // CityObjectType enum constants
    BIND_CONSTANT(COT_GenericCityObject);
    BIND_CONSTANT(COT_Building);
    BIND_CONSTANT(COT_Room);
    BIND_CONSTANT(COT_BuildingInstallation);
    BIND_CONSTANT(COT_BuildingFurniture);
    BIND_CONSTANT(COT_Door);
    BIND_CONSTANT(COT_Window);
    BIND_CONSTANT(COT_CityFurniture);
    BIND_CONSTANT(COT_Track);
    BIND_CONSTANT(COT_Road);
    BIND_CONSTANT(COT_Railway);
    BIND_CONSTANT(COT_Square);
    BIND_CONSTANT(COT_PlantCover);
    BIND_CONSTANT(COT_SolitaryVegetationObject);
    BIND_CONSTANT(COT_WaterBody);
    BIND_CONSTANT(COT_ReliefFeature);
    BIND_CONSTANT(COT_LandUse);
    BIND_CONSTANT(COT_Tunnel);
    BIND_CONSTANT(COT_Bridge);
    BIND_CONSTANT(COT_WallSurface);
    BIND_CONSTANT(COT_RoofSurface);
    BIND_CONSTANT(COT_GroundSurface);
    BIND_CONSTANT(COT_Unknown);
}

// PLATEAUCityModel implementation
PLATEAUCityModel::PLATEAUCityModel()
    : is_loaded_(false) {
}

PLATEAUCityModel::~PLATEAUCityModel() {
}

bool PLATEAUCityModel::load(const String &gml_path) {
    // Convert Godot String to std::string
    std::string path = gml_path.utf8().get_data();

    // Parser parameters
    citygml::ParserParams params;
    params.tesselate = true;
    params.optimize = true;
    params.keepVertices = true;
    params.ignoreGeometries = false;

    try {
        city_model_ = citygml::load(path, params);
        if (city_model_ == nullptr) {
            UtilityFunctions::printerr("Failed to load CityGML file: ", gml_path);
            is_loaded_ = false;
            return false;
        }

        gml_path_ = gml_path;
        is_loaded_ = true;
        UtilityFunctions::print("Successfully loaded CityGML: ", gml_path);
        return true;
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception loading CityGML: ", String(e.what()));
        is_loaded_ = false;
        return false;
    }
}

bool PLATEAUCityModel::is_loaded() const {
    return is_loaded_;
}

String PLATEAUCityModel::get_gml_path() const {
    return gml_path_;
}

TypedArray<PLATEAUMeshData> PLATEAUCityModel::extract_meshes(const Ref<PLATEAUMeshExtractOptions> &options) {
    TypedArray<PLATEAUMeshData> result;

    if (!is_loaded_ || city_model_ == nullptr) {
        UtilityFunctions::printerr("CityModel not loaded");
        return result;
    }

    if (options.is_null()) {
        UtilityFunctions::printerr("MeshExtractOptions is null");
        return result;
    }

    try {
        // Get native options
        plateau::polygonMesh::MeshExtractOptions native_options = options->get_native();

        // Extract meshes
        auto model = plateau::polygonMesh::MeshExtractor::extract(*city_model_, native_options);

        if (!model) {
            UtilityFunctions::printerr("Failed to extract meshes");
            return result;
        }

        // Get base texture path (directory of GML file)
        String base_path = gml_path_.get_base_dir();

        // Convert each root node
        for (size_t i = 0; i < model->getRootNodeCount(); i++) {
            const PlateauNode &node = model->getRootNodeAt(i);
            Ref<PLATEAUMeshData> mesh_data = convert_node(node, base_path);
            if (mesh_data.is_valid()) {
                result.push_back(mesh_data);
            }
        }

        UtilityFunctions::print("Extracted ", result.size(), " root nodes");
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception extracting meshes: ", String(e.what()));
    }

    return result;
}

Vector3 PLATEAUCityModel::get_center_point(int coordinate_zone_id) const {
    if (!is_loaded_ || city_model_ == nullptr) {
        return Vector3();
    }

    // Get envelope from city model
    const auto &envelope = city_model_->getEnvelope();
    TVec3d lower = envelope.getLowerBound();
    TVec3d upper = envelope.getUpperBound();

    // Calculate center
    TVec3d center;
    center.x = (lower.x + upper.x) / 2.0;
    center.y = (lower.y + upper.y) / 2.0;
    center.z = (lower.z + upper.z) / 2.0;

    return Vector3(center.x, center.y, center.z);
}

Ref<PLATEAUMeshData> PLATEAUCityModel::convert_node(const plateau::polygonMesh::Node &node, const String &base_texture_path) {
    Ref<PLATEAUMeshData> mesh_data;
    mesh_data.instantiate();

    // Set name (often the GML ID)
    std::string node_name = node.getName();
    mesh_data->set_name(String::utf8(node_name.c_str()));

    // Convert mesh if present
    const PlateauMesh *mesh = node.getMesh();
    PlateauCityObjectList city_object_list;
    if (mesh != nullptr && mesh->hasVertices()) {
        Ref<ArrayMesh> godot_mesh = convert_mesh(*mesh, base_texture_path, city_object_list);
        if (godot_mesh.is_valid()) {
            mesh_data->set_mesh(godot_mesh);
            mesh_data->set_city_object_list(city_object_list);
        }
    }

    // Phase 1: Try to get attributes from CityModel using node name as GML ID
    if (city_model_ != nullptr) {
        // First try to find city object by node name
        const citygml::CityObject* city_obj = city_model_->getCityObjectById(node_name);

        // If not found directly, try looking in the CityObjectList
        if (city_obj == nullptr && !city_object_list.getAllKeys()->empty()) {
            // Get first GML ID from the mesh's city object list
            auto keys = city_object_list.getAllKeys();
            if (!keys->empty()) {
                std::string gml_id;
                if (city_object_list.tryGetPrimaryGmlID((*keys)[0].primary_index, gml_id)) {
                    city_obj = city_model_->getCityObjectById(gml_id);
                    node_name = gml_id;
                }
            }
        }

        if (city_obj != nullptr) {
            // Set GML ID
            mesh_data->set_gml_id(String::utf8(city_obj->getId().c_str()));

            // Set city object type
            mesh_data->set_city_object_type(static_cast<int64_t>(city_obj->getType()));

            // Set attributes
            Dictionary attrs = convert_attributes(city_obj->getAttributes());
            mesh_data->set_attributes(attrs);
        }
    }

    // Set local transform
    TVec3d pos = node.getLocalPosition();
    Transform3D transform;
    transform.origin = Vector3(pos.x, pos.y, pos.z);
    mesh_data->set_transform(transform);

    // Convert children recursively
    for (size_t i = 0; i < node.getChildCount(); i++) {
        const PlateauNode &child = node.getChildAt(i);
        Ref<PLATEAUMeshData> child_data = convert_node(child, base_texture_path);
        if (child_data.is_valid()) {
            mesh_data->add_child(child_data);
        }
    }

    return mesh_data;
}

// Compute smooth normals for mesh (area-weighted vertex normals)
PackedVector3Array PLATEAUCityModel::compute_normals(const PackedVector3Array &vertices, const PackedInt32Array &indices) {
    PackedVector3Array normals;
    normals.resize(vertices.size());

    // Initialize all normals to zero
    for (int i = 0; i < normals.size(); i++) {
        normals.set(i, Vector3(0, 0, 0));
    }

    // Calculate face normals and accumulate to vertices
    int face_count = indices.size() / 3;
    for (int face = 0; face < face_count; face++) {
        int i0 = indices[face * 3];
        int i1 = indices[face * 3 + 1];
        int i2 = indices[face * 3 + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }

        Vector3 v0 = vertices[i0];
        Vector3 v1 = vertices[i1];
        Vector3 v2 = vertices[i2];

        // Calculate face normal (cross product)
        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 face_normal = edge1.cross(edge2);

        // Don't normalize yet - larger faces contribute more (area weighting)
        normals.set(i0, normals[i0] + face_normal);
        normals.set(i1, normals[i1] + face_normal);
        normals.set(i2, normals[i2] + face_normal);
    }

    // Normalize all normals
    for (int i = 0; i < normals.size(); i++) {
        Vector3 n = normals[i];
        if (n.length_squared() > 0.0001f) {
            normals.set(i, n.normalized());
        } else {
            normals.set(i, Vector3(0, 1, 0)); // Default up normal
        }
    }

    return normals;
}

// Load texture with caching
Ref<ImageTexture> PLATEAUCityModel::load_texture_cached(const String &texture_path) {
    // Check cache first
    if (texture_cache_.has(texture_path)) {
        return texture_cache_[texture_path];
    }

    // Load texture
    if (!FileAccess::file_exists(texture_path)) {
        texture_cache_[texture_path] = Ref<ImageTexture>();
        return Ref<ImageTexture>();
    }

    Ref<Image> image;
    image.instantiate();
    Error err = image->load(texture_path);
    if (err != OK) {
        texture_cache_[texture_path] = Ref<ImageTexture>();
        return Ref<ImageTexture>();
    }

    Ref<ImageTexture> texture = ImageTexture::create_from_image(image);
    texture_cache_[texture_path] = texture;
    return texture;
}

Ref<ArrayMesh> PLATEAUCityModel::convert_mesh(const plateau::polygonMesh::Mesh &mesh, const String &base_texture_path, PlateauCityObjectList &out_city_object_list) {
    Ref<ArrayMesh> array_mesh;
    array_mesh.instantiate();

    const auto &vertices = mesh.getVertices();
    const auto &indices = mesh.getIndices();
    const auto &uv1 = mesh.getUV1();
    const auto &uv4 = mesh.getUV4(); // CityObjectIndex stored here
    const auto &sub_meshes = mesh.getSubMeshes();

    // Copy CityObjectList for attribute lookup
    out_city_object_list = mesh.getCityObjectList();

    if (vertices.empty() || indices.empty()) {
        return array_mesh;
    }

    // Convert vertices
    PackedVector3Array godot_vertices;
    godot_vertices.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        const auto &v = vertices[i];
        godot_vertices.set(i, Vector3(v.x, v.y, v.z));
    }

    // Convert UV1 (texture coordinates) with Y-axis flip
    PackedVector2Array godot_uvs;
    godot_uvs.resize(uv1.size());
    for (size_t i = 0; i < uv1.size(); i++) {
        const auto &uv = uv1[i];
        // Flip Y coordinate for correct texture orientation
        godot_uvs.set(i, Vector2(uv.x, 1.0f - uv.y));
    }

    // Convert UV4 (CityObjectIndex) - store in ARRAY_TEX_UV2 for later attribute lookup
    PackedVector2Array godot_uv4;
    godot_uv4.resize(uv4.size());
    for (size_t i = 0; i < uv4.size(); i++) {
        const auto &uv = uv4[i];
        godot_uv4.set(i, Vector2(uv.x, uv.y));
    }

    // Convert all indices for normal calculation
    PackedInt32Array all_indices;
    all_indices.resize(indices.size());
    for (size_t i = 0; i < indices.size(); i++) {
        all_indices.set(i, indices[i]);
    }

    // Compute normals
    PackedVector3Array godot_normals = compute_normals(godot_vertices, all_indices);

    // Process each submesh with compact per-surface arrays to avoid memory bloat
    for (size_t sub_idx = 0; sub_idx < sub_meshes.size(); sub_idx++) {
        const auto &sub_mesh = sub_meshes[sub_idx];
        size_t start_index = sub_mesh.getStartIndex();
        size_t end_index = sub_mesh.getEndIndex();

        if (end_index < start_index) {
            continue;
        }

        // Collect unique vertex indices used by this submesh
        std::unordered_set<unsigned int> used_indices_set;
        for (size_t i = start_index; i <= end_index; i++) {
            used_indices_set.insert(indices[i]);
        }

        // Create mapping from global index to compact index
        std::unordered_map<unsigned int, int> global_to_compact;
        std::vector<unsigned int> compact_to_global;
        compact_to_global.reserve(used_indices_set.size());

        for (unsigned int idx : used_indices_set) {
            global_to_compact[idx] = static_cast<int>(compact_to_global.size());
            compact_to_global.push_back(idx);
        }

        // Build compact arrays for this submesh only
        int compact_vertex_count = static_cast<int>(compact_to_global.size());

        PackedVector3Array submesh_vertices;
        submesh_vertices.resize(compact_vertex_count);
        PackedVector3Array submesh_normals;
        submesh_normals.resize(compact_vertex_count);
        PackedVector2Array submesh_uvs;
        PackedVector2Array submesh_uv4;

        bool has_uvs = !godot_uvs.is_empty() && godot_uvs.size() >= static_cast<int64_t>(vertices.size());
        bool has_uv4 = !godot_uv4.is_empty() && godot_uv4.size() >= static_cast<int64_t>(vertices.size());

        if (has_uvs) {
            submesh_uvs.resize(compact_vertex_count);
        }
        if (has_uv4) {
            submesh_uv4.resize(compact_vertex_count);
        }

        // Copy only the vertex data needed for this submesh
        for (int compact_idx = 0; compact_idx < compact_vertex_count; compact_idx++) {
            unsigned int global_idx = compact_to_global[compact_idx];
            submesh_vertices.set(compact_idx, godot_vertices[global_idx]);
            submesh_normals.set(compact_idx, godot_normals[global_idx]);
            if (has_uvs) {
                submesh_uvs.set(compact_idx, godot_uvs[global_idx]);
            }
            if (has_uv4) {
                submesh_uv4.set(compact_idx, godot_uv4[global_idx]);
            }
        }

        // Build remapped indices for this submesh (0-based for compact arrays)
        size_t index_count = end_index - start_index + 1;
        PackedInt32Array submesh_indices;
        submesh_indices.resize(index_count);
        for (size_t i = start_index; i <= end_index; i++) {
            submesh_indices.set(i - start_index, global_to_compact[indices[i]]);
        }

        // Create surface arrays with compact data (only referenced vertices)
        Array arrays;
        arrays.resize(godot::Mesh::ARRAY_MAX);
        arrays[godot::Mesh::ARRAY_VERTEX] = submesh_vertices;
        arrays[godot::Mesh::ARRAY_INDEX] = submesh_indices;
        arrays[godot::Mesh::ARRAY_NORMAL] = submesh_normals;

        if (!submesh_uvs.is_empty()) {
            arrays[godot::Mesh::ARRAY_TEX_UV] = submesh_uvs;
        }

        // Store UV4 (CityObjectIndex) in second UV channel for raycast lookup
        if (!submesh_uv4.is_empty()) {
            arrays[godot::Mesh::ARRAY_TEX_UV2] = submesh_uv4;
        }

        // Add surface
        array_mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES, arrays);

        // Create and set material
        Ref<StandardMaterial3D> material = create_material(sub_mesh, base_texture_path);
        if (material.is_valid()) {
            array_mesh->surface_set_material(array_mesh->get_surface_count() - 1, material);
        }
    }

    return array_mesh;
}

Ref<StandardMaterial3D> PLATEAUCityModel::create_material(const plateau::polygonMesh::SubMesh &sub_mesh, const String &base_texture_path) {
    Ref<StandardMaterial3D> material;
    material.instantiate();

    // Set default material properties
    material->set_cull_mode(StandardMaterial3D::CULL_BACK);

    // Get texture path
    std::string texture_path_str = sub_mesh.getTexturePath();
    bool has_texture = false;

    if (!texture_path_str.empty()) {
        // Construct full texture path
        String full_path = base_texture_path.path_join(String::utf8(texture_path_str.c_str()));

        // Try to load texture with caching
        Ref<ImageTexture> texture = const_cast<PLATEAUCityModel*>(this)->load_texture_cached(full_path);
        if (texture.is_valid()) {
            material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
            has_texture = true;
        }
    }

    // Get material properties from libplateau if available
    auto mat = sub_mesh.getMaterial();
    if (mat) {
        // Get material properties
        auto diffuse = mat->getDiffuse();
        auto specular = mat->getSpecular();
        auto emissive = mat->getEmissive();
        float shininess = mat->getShininess();
        float transparency = mat->getTransparency();
        float ambient = mat->getAmbientIntensity();

        // Set base color (diffuse)
        Color albedo_color(diffuse.x, diffuse.y, diffuse.z, 1.0f);

        // Set transparency
        if (transparency > 0.0f) {
            material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
            albedo_color.a = 1.0f - transparency;
        }

        // Set albedo color if no texture, or modulate with texture
        if (!has_texture) {
            material->set_albedo(albedo_color);
        } else if (transparency > 0.0f) {
            // If has texture but also transparency, set alpha
            material->set_albedo(Color(1.0f, 1.0f, 1.0f, albedo_color.a));
        }

        // Set emissive color
        if (emissive.x > 0.01f || emissive.y > 0.01f || emissive.z > 0.01f) {
            material->set_feature(StandardMaterial3D::FEATURE_EMISSION, true);
            material->set_emission(Color(emissive.x, emissive.y, emissive.z));
            material->set_emission_energy_multiplier(1.0f);
        }

        // Calculate metallic from specular (Unreal SDK approach)
        // If base color and specular RGB ratios are similar, derive metallic
        bool diffuse_is_gray = std::abs(diffuse.x - diffuse.y) < 0.01f && std::abs(diffuse.x - diffuse.z) < 0.01f;
        bool specular_is_gray = std::abs(specular.x - specular.y) < 0.01f && std::abs(specular.x - specular.z) < 0.01f;

        if (diffuse_is_gray && specular_is_gray && diffuse.x > 0.01f) {
            // metallic = specular / diffuse
            float metallic = specular.x / diffuse.x;
            metallic = std::min(1.0f, std::max(0.0f, metallic));
            material->set_metallic(metallic);
            material->set_specular(0.5f);
        } else {
            // Use average specular as specular value
            float spec_avg = (specular.x + specular.y + specular.z) / 3.0f;
            material->set_metallic(0.0f);
            material->set_specular(std::min(1.0f, spec_avg));
        }

        // Set roughness from shininess (inverse relationship)
        // Higher shininess = lower roughness
        float roughness = 1.0f - (shininess / 128.0f); // Assuming shininess 0-128 range
        roughness = std::min(1.0f, std::max(0.0f, roughness));
        material->set_roughness(roughness);

    } else {
        // Default material if no material info
        if (!has_texture) {
            material->set_albedo(Color(0.8f, 0.8f, 0.8f, 1.0f));
        }
        material->set_metallic(0.0f);
        material->set_roughness(0.8f);
        material->set_specular(0.5f);
    }

    return material;
}

// Phase 1: Attribute conversion helpers
Dictionary PLATEAUCityModel::convert_attributes(const citygml::AttributesMap &attrs) {
    Dictionary result;

    for (const auto &pair : attrs) {
        // Use utf8() for proper UTF-8 to Godot String conversion
        String key = String::utf8(pair.first.c_str());
        Variant value = convert_attribute_value(pair.second);
        result[key] = value;
    }

    return result;
}

Variant PLATEAUCityModel::convert_attribute_value(const citygml::AttributeValue &value) {
    switch (value.getType()) {
        case citygml::AttributeType::String:
        case citygml::AttributeType::Date:
        case citygml::AttributeType::Uri:
        case citygml::AttributeType::Measure:
        case citygml::AttributeType::CodeList:
            // Use utf8() for proper UTF-8 to Godot String conversion
            return String::utf8(value.asString().c_str());

        case citygml::AttributeType::Integer:
            return value.asInteger();

        case citygml::AttributeType::Double:
            return value.asDouble();

        case citygml::AttributeType::Boolean:
            return value.asBoolean();

        case citygml::AttributeType::AttributeSet: {
            // Recursive conversion for nested attributes
            const citygml::AttributesMap &nested = value.asAttributeSet();
            return convert_attributes(nested);
        }

        default:
            return String::utf8(value.asString().c_str());
    }
}

Dictionary PLATEAUCityModel::get_city_object_attributes(const String &gml_id) const {
    if (!is_loaded_ || city_model_ == nullptr) {
        return Dictionary();
    }

    std::string id = gml_id.utf8().get_data();
    const citygml::CityObject* city_obj = city_model_->getCityObjectById(id);

    if (city_obj == nullptr) {
        return Dictionary();
    }

    return convert_attributes(city_obj->getAttributes());
}

int64_t PLATEAUCityModel::get_city_object_type(const String &gml_id) const {
    if (!is_loaded_ || city_model_ == nullptr) {
        return 0;
    }

    std::string id = gml_id.utf8().get_data();
    const citygml::CityObject* city_obj = city_model_->getCityObjectById(id);

    if (city_obj == nullptr) {
        return 0;
    }

    return static_cast<int64_t>(city_obj->getType());
}

void PLATEAUCityModel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load", "gml_path"), &PLATEAUCityModel::load);
    ClassDB::bind_method(D_METHOD("is_loaded"), &PLATEAUCityModel::is_loaded);
    ClassDB::bind_method(D_METHOD("get_gml_path"), &PLATEAUCityModel::get_gml_path);
    ClassDB::bind_method(D_METHOD("extract_meshes", "options"), &PLATEAUCityModel::extract_meshes);
    ClassDB::bind_method(D_METHOD("get_center_point", "coordinate_zone_id"), &PLATEAUCityModel::get_center_point);

    // Phase 1: Attribute access methods
    ClassDB::bind_method(D_METHOD("get_city_object_attributes", "gml_id"), &PLATEAUCityModel::get_city_object_attributes);
    ClassDB::bind_method(D_METHOD("get_city_object_type", "gml_id"), &PLATEAUCityModel::get_city_object_type);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "gml_path"), "", "get_gml_path");
}
