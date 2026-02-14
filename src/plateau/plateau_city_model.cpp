#include "plateau_city_model.h"
#include "plateau_platform.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/math_defs.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <citygml/citygmllogger.h>
#include <cmath>
#include <unordered_set>
#include <unordered_map>

using namespace godot;

// Simple CityGML logger that forwards to Godot's output
// Supports PLATEAULogLevel for controlling verbosity
class GodotCityGMLLogger : public citygml::CityGMLLogger {
public:
    // silent_mode: if true, suppress all log output (for LOG_LEVEL_NONE)
    GodotCityGMLLogger(LOGLEVEL level = LOGLEVEL::LL_WARNING, bool silent_mode = false)
        : CityGMLLogger(level), silent_mode_(silent_mode) {}

    void log(LOGLEVEL level, const std::string& message, const char* file, int line) const override {
        if (silent_mode_) {
            return;  // Suppress all log output
        }
        String godot_msg = String::utf8(message.c_str());
        switch (level) {
            case LOGLEVEL::LL_ERROR:
                UtilityFunctions::printerr("[CityGML ERROR] ", godot_msg);
                break;
            case LOGLEVEL::LL_WARNING:
                UtilityFunctions::print("[CityGML WARN] ", godot_msg);
                break;
            case LOGLEVEL::LL_INFO:
                UtilityFunctions::print("[CityGML INFO] ", godot_msg);
                break;
            case LOGLEVEL::LL_DEBUG:
            case LOGLEVEL::LL_TRACE:
                UtilityFunctions::print("[CityGML DEBUG] ", godot_msg);
                break;
        }
    }

private:
    bool silent_mode_;
};

#include "plateau_types.h"

// Type aliases for file-local types
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

// Texture path methods for export
void PLATEAUMeshData::set_texture_paths(const PackedStringArray &paths) {
    texture_paths_ = paths;
}

PackedStringArray PLATEAUMeshData::get_texture_paths() const {
    return texture_paths_;
}

void PLATEAUMeshData::add_texture_path(const String &path) {
    texture_paths_.push_back(path);
}

String PLATEAUMeshData::get_texture_path(int surface_index) const {
    if (surface_index >= 0 && surface_index < texture_paths_.size()) {
        return texture_paths_[surface_index];
    }
    return String();
}

int PLATEAUMeshData::get_texture_path_count() const {
    return texture_paths_.size();
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

    // Texture path methods for export
    ClassDB::bind_method(D_METHOD("set_texture_paths", "paths"), &PLATEAUMeshData::set_texture_paths);
    ClassDB::bind_method(D_METHOD("get_texture_paths"), &PLATEAUMeshData::get_texture_paths);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "texture_paths"), "set_texture_paths", "get_texture_paths");
    ClassDB::bind_method(D_METHOD("add_texture_path", "path"), &PLATEAUMeshData::add_texture_path);
    ClassDB::bind_method(D_METHOD("get_texture_path", "surface_index"), &PLATEAUMeshData::get_texture_path);
    ClassDB::bind_method(D_METHOD("get_texture_path_count"), &PLATEAUMeshData::get_texture_path_count);

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
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(false);
#endif
    // Convert Godot String to std::string
    std::string path = gml_path.utf8().get_data();

    // Parser parameters
    citygml::ParserParams params;
    params.tesselate = true;
    params.optimize = true;
    params.keepVertices = true;
    params.ignoreGeometries = false;

    // Create logger for CityGML parser based on log_level_ setting
    citygml::CityGMLLogger::LOGLEVEL citygml_level;
    bool silent_mode = false;
    switch (log_level_) {
        case LOG_LEVEL_NONE:
            citygml_level = citygml::CityGMLLogger::LOGLEVEL::LL_ERROR;
            silent_mode = true;
            break;
        case LOG_LEVEL_ERROR:
            citygml_level = citygml::CityGMLLogger::LOGLEVEL::LL_ERROR;
            break;
        case LOG_LEVEL_INFO:
            citygml_level = citygml::CityGMLLogger::LOGLEVEL::LL_INFO;
            break;
        case LOG_LEVEL_DEBUG:
            citygml_level = citygml::CityGMLLogger::LOGLEVEL::LL_DEBUG;
            break;
        case LOG_LEVEL_WARNING:
        default:
            citygml_level = citygml::CityGMLLogger::LOGLEVEL::LL_WARNING;
            break;
    }
    auto logger = std::make_shared<GodotCityGMLLogger>(citygml_level, silent_mode);

    try {
        city_model_ = citygml::load(path, params, logger);
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

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

    ERR_FAIL_COND_V_MSG(!is_loaded_ || city_model_ == nullptr, result, "CityModel not loaded.");
    ERR_FAIL_COND_V_MSG(options.is_null(), result, "MeshExtractOptions is null.");

    try {
        // Get native options
        plateau::polygonMesh::MeshExtractOptions native_options = options->get_native();

        // Extract meshes
        auto model = plateau::polygonMesh::MeshExtractor::extract(*city_model_, native_options);

        ERR_FAIL_COND_V_MSG(!model, result, "Failed to extract meshes.");

        // Convert each root node
        for (size_t i = 0; i < model->getRootNodeCount(); i++) {
            const PlateauNode &node = model->getRootNodeAt(i);
            Ref<PLATEAUMeshData> mesh_data = convert_node(node);
            if (mesh_data.is_valid()) {
                result.push_back(mesh_data);
            }
        }

        UtilityFunctions::print("Extracted ", result.size(), " root nodes");
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception extracting meshes: ", String(e.what()));
    }

    // Release caches after mesh conversion (resources are held by mesh surfaces)
    texture_cache_.clear();
    material_cache_.clear();

    return result;
}

Vector3 PLATEAUCityModel::get_center_point(int coordinate_zone_id) const {
    ERR_FAIL_COND_V_MSG(!is_loaded_ || city_model_ == nullptr, Vector3(), "CityModel not loaded.");

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

Ref<PLATEAUMeshData> PLATEAUCityModel::convert_node(const plateau::polygonMesh::Node &node) {
    Ref<PLATEAUMeshData> mesh_data;
    mesh_data.instantiate();

    // Set name (often the GML ID)
    std::string node_name = node.getName();
    mesh_data->set_name(String::utf8(node_name.c_str()));

    // Convert mesh if present
    const PlateauMesh *mesh = node.getMesh();
    PlateauCityObjectList city_object_list;
    PackedStringArray texture_paths;
    if (mesh != nullptr && mesh->hasVertices()) {
        Ref<ArrayMesh> godot_mesh = convert_mesh(*mesh, city_object_list, texture_paths);
        if (godot_mesh.is_valid()) {
            mesh_data->set_mesh(godot_mesh);
            mesh_data->set_city_object_list(city_object_list);
            mesh_data->set_texture_paths(texture_paths);
        }
    }

    // Phase 1: Try to get attributes from CityModel using node name as GML ID
    if (city_model_ != nullptr) {
        // First try to find city object by node name
        const citygml::CityObject* city_obj = city_model_->getCityObjectById(node_name);

        // If not found directly, try looking in the CityObjectList
        auto keys = city_object_list.getAllKeys();
        if (city_obj == nullptr && keys != nullptr && !keys->empty()) {
            // Get first GML ID from the mesh's city object list
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

    // Set local transform (position, rotation, scale)
    TVec3d pos = node.getLocalPosition();
    TVec3d scale = node.getLocalScale();
    plateau::polygonMesh::Quaternion rot = node.getLocalRotation();

    // Create Godot Quaternion from libplateau Quaternion (both use xyzw order)
    godot::Quaternion godot_quat(rot.getX(), rot.getY(), rot.getZ(), rot.getW());

    // Create basis from quaternion, then apply scale
    Basis basis(godot_quat);
    basis.scale_local(Vector3(scale.x, scale.y, scale.z));

    Transform3D transform;
    transform.basis = basis;
    transform.origin = Vector3(pos.x, pos.y, pos.z);
    mesh_data->set_transform(transform);

    // Convert children recursively
    for (size_t i = 0; i < node.getChildCount(); i++) {
        const PlateauNode &child = node.getChildAt(i);
        Ref<PLATEAUMeshData> child_data = convert_node(child);
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
            WARN_PRINT_ONCE("compute_normals: index out of bounds, skipping face.");
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
        if (n.length_squared() > CMP_EPSILON) {
            normals.set(i, n.normalized());
        } else {
            normals.set(i, Vector3(0, 1, 0)); // Default up normal
        }
    }

    return normals;
}

// Load texture with caching
Ref<ImageTexture> PLATEAUCityModel::load_texture_cached(const String &texture_path) const {
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

Ref<ArrayMesh> PLATEAUCityModel::convert_mesh(const plateau::polygonMesh::Mesh &mesh, PlateauCityObjectList &out_city_object_list, PackedStringArray &out_texture_paths) {
    Ref<ArrayMesh> array_mesh;
    array_mesh.instantiate();

    const auto &vertices = mesh.getVertices();
    const auto &indices = mesh.getIndices();
    const auto &uv1 = mesh.getUV1();
    const auto &uv4 = mesh.getUV4(); // CityObjectIndex stored here
    const auto &sub_meshes = mesh.getSubMeshes();

    // Copy CityObjectList for attribute lookup
    out_city_object_list = mesh.getCityObjectList();

    // Clear output texture paths
    out_texture_paths.clear();

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

    // Convert all indices with winding order inversion for correct normals
    // libplateau outputs CW winding, but Godot expects CCW for front faces
    PackedInt32Array all_indices;
    all_indices.resize(indices.size());
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Swap first and third vertex of each triangle to invert winding
        all_indices.set(i, indices[i + 2]);
        all_indices.set(i + 1, indices[i + 1]);
        all_indices.set(i + 2, indices[i]);
    }

    // Compute normals from winding-inverted indices
    PackedVector3Array godot_normals = compute_normals(godot_vertices, all_indices);

    // Process each submesh with compact per-surface arrays to avoid memory bloat
    for (size_t sub_idx = 0; sub_idx < sub_meshes.size(); sub_idx++) {
        const auto &sub_mesh = sub_meshes[sub_idx];
        size_t start_index = sub_mesh.getStartIndex();
        size_t end_index = sub_mesh.getEndIndex();

        if (end_index >= indices.size() || end_index < start_index) {
            WARN_PRINT_ONCE("convert_mesh: invalid submesh range, skipping submesh.");
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
        // Invert winding order (swap indices 0 and 2 of each triangle) to fix normal direction
        // libplateau outputs CW winding, but Godot expects CCW for front faces
        size_t index_count = end_index - start_index + 1;
        PackedInt32Array submesh_indices;
        submesh_indices.resize(index_count);
        for (size_t i = start_index; i <= end_index; i += 3) {
            // Swap first and third vertex of each triangle to invert winding
            submesh_indices.set(i - start_index, global_to_compact[indices[i + 2]]);
            submesh_indices.set(i - start_index + 1, global_to_compact[indices[i + 1]]);
            submesh_indices.set(i - start_index + 2, global_to_compact[indices[i]]);
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

        // Collect texture path for this submesh (for export)
        // Keep original path format for libplateau export compatibility
        std::string texture_path_str = sub_mesh.getTexturePath();
        out_texture_paths.push_back(String::utf8(texture_path_str.c_str()));

        // Create and set material
        Ref<StandardMaterial3D> material = create_material(sub_mesh);
        if (material.is_valid()) {
            array_mesh->surface_set_material(array_mesh->get_surface_count() - 1, material);
        }
    }

    return array_mesh;
}

Ref<StandardMaterial3D> PLATEAUCityModel::create_material(const plateau::polygonMesh::SubMesh &sub_mesh) {
    // Build cache key from texture path + material properties
    String cache_key;
    std::string texture_path_str = sub_mesh.getTexturePath();
    String normalized_texture_path;

    if (!texture_path_str.empty()) {
        normalized_texture_path = String::utf8(texture_path_str.c_str()).replace("\\", "/").simplify_path();
        cache_key = normalized_texture_path;
    }

    auto mat = sub_mesh.getMaterial();
    if (mat) {
        auto d = mat->getDiffuse();
        auto s = mat->getSpecular();
        auto e = mat->getEmissive();
        cache_key += "|d:" + String::num(d.x, 4) + "," + String::num(d.y, 4) + "," + String::num(d.z, 4)
            + "|s:" + String::num(s.x, 4) + "," + String::num(s.y, 4) + "," + String::num(s.z, 4)
            + "|e:" + String::num(e.x, 4) + "," + String::num(e.y, 4) + "," + String::num(e.z, 4)
            + "|sh:" + String::num(mat->getShininess(), 4)
            + "|tr:" + String::num(mat->getTransparency(), 4)
            + "|am:" + String::num(mat->getAmbientIntensity(), 4);
    } else {
        cache_key += "|default";
    }

    // Return cached material if available
    if (material_cache_.has(cache_key)) {
        return material_cache_[cache_key];
    }

    Ref<StandardMaterial3D> material;
    material.instantiate();

    // Set default material properties
    material->set_cull_mode(StandardMaterial3D::CULL_BACK);

    bool has_texture = false;
    if (!normalized_texture_path.is_empty()) {
        Ref<ImageTexture> texture = load_texture_cached(normalized_texture_path);
        if (texture.is_valid()) {
            material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
            has_texture = true;
        }
    }

    if (mat) {
        auto diffuse = mat->getDiffuse();
        auto specular = mat->getSpecular();
        auto emissive = mat->getEmissive();
        float shininess = mat->getShininess();
        float transparency = mat->getTransparency();

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
            material->set_albedo(Color(1.0f, 1.0f, 1.0f, albedo_color.a));
        }

        // Set emissive color
        if (emissive.x > 0.01f || emissive.y > 0.01f || emissive.z > 0.01f) {
            material->set_feature(StandardMaterial3D::FEATURE_EMISSION, true);
            material->set_emission(Color(emissive.x, emissive.y, emissive.z));
            material->set_emission_energy_multiplier(1.0f);
        }

        // Calculate metallic from specular (Unreal SDK approach)
        bool diffuse_is_gray = std::abs(diffuse.x - diffuse.y) < 0.01f && std::abs(diffuse.x - diffuse.z) < 0.01f;
        bool specular_is_gray = std::abs(specular.x - specular.y) < 0.01f && std::abs(specular.x - specular.z) < 0.01f;

        if (diffuse_is_gray && specular_is_gray && diffuse.x > 0.01f) {
            float metallic = specular.x / diffuse.x;
            metallic = std::min(1.0f, std::max(0.0f, metallic));
            material->set_metallic(metallic);
            material->set_specular(0.5f);
        } else {
            float spec_avg = (specular.x + specular.y + specular.z) / 3.0f;
            material->set_metallic(0.0f);
            material->set_specular(std::min(1.0f, spec_avg));
        }

        // Set roughness from shininess (inverse relationship)
        float roughness = 1.0f - (shininess / 128.0f);
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

    material_cache_[cache_key] = material;
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
    ERR_FAIL_COND_V_MSG(!is_loaded_ || city_model_ == nullptr, Dictionary(), "CityModel not loaded.");

    std::string id = gml_id.utf8().get_data();
    const citygml::CityObject* city_obj = city_model_->getCityObjectById(id);

    if (city_obj == nullptr) {
        return Dictionary();
    }

    return convert_attributes(city_obj->getAttributes());
}

int64_t PLATEAUCityModel::get_city_object_type(const String &gml_id) const {
    ERR_FAIL_COND_V_MSG(!is_loaded_ || city_model_ == nullptr, 0, "CityModel not loaded.");

    std::string id = gml_id.utf8().get_data();
    const citygml::CityObject* city_obj = city_model_->getCityObjectById(id);

    if (city_obj == nullptr) {
        return 0;
    }

    return static_cast<int64_t>(city_obj->getType());
}

void PLATEAUCityModel::set_log_level(int level) {
    log_level_ = level;
}

int PLATEAUCityModel::get_log_level() const {
    return log_level_;
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

    // Async API
    ClassDB::bind_method(D_METHOD("load_async", "gml_path"), &PLATEAUCityModel::load_async);
    ClassDB::bind_method(D_METHOD("extract_meshes_async", "options"), &PLATEAUCityModel::extract_meshes_async);
    ClassDB::bind_method(D_METHOD("is_processing"), &PLATEAUCityModel::is_processing);

    // Log level control
    ClassDB::bind_method(D_METHOD("set_log_level", "level"), &PLATEAUCityModel::set_log_level);
    ClassDB::bind_method(D_METHOD("get_log_level"), &PLATEAUCityModel::get_log_level);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "log_level", PROPERTY_HINT_ENUM, "None,Error,Warning,Info,Debug"), "set_log_level", "get_log_level");

    // Log level enum constants
    BIND_CONSTANT(LOG_LEVEL_NONE);
    BIND_CONSTANT(LOG_LEVEL_ERROR);
    BIND_CONSTANT(LOG_LEVEL_WARNING);
    BIND_CONSTANT(LOG_LEVEL_INFO);
    BIND_CONSTANT(LOG_LEVEL_DEBUG);

    // Internal method for deferred call from worker thread (not for GDScript use)
    ClassDB::bind_method(D_METHOD("_finalize_meshes_on_main_thread"), &PLATEAUCityModel::_finalize_meshes_on_main_thread);

    // Signals for async completion
    ADD_SIGNAL(MethodInfo("load_completed", PropertyInfo(Variant::BOOL, "success")));
    ADD_SIGNAL(MethodInfo("extract_completed", PropertyInfo(Variant::ARRAY, "meshes")));

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "gml_path"), "", "get_gml_path");
}

// Async API implementation
void PLATEAUCityModel::load_async(const String &gml_path) {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED();
#endif
    if (is_processing_.load()) {
        UtilityFunctions::printerr("PLATEAUCityModel: Already processing, cannot start new load");
        return;
    }

    is_processing_.store(true);
    pending_gml_path_ = gml_path;

    // Use WorkerThreadPool to run load in background
    WorkerThreadPool::get_singleton()->add_task(
        callable_mp(this, &PLATEAUCityModel::_load_thread_func)
    );
}

void PLATEAUCityModel::_load_thread_func() {
    bool success = load(pending_gml_path_);
    is_processing_.store(false);
    pending_gml_path_ = "";

    // Emit signal on main thread via call_deferred
    call_deferred("emit_signal", "load_completed", success);
}

void PLATEAUCityModel::extract_meshes_async(const Ref<PLATEAUMeshExtractOptions> &options) {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED();
#endif
    if (is_processing_.load()) {
        UtilityFunctions::printerr("PLATEAUCityModel: Already processing, cannot start new extraction");
        return;
    }

    if (!is_loaded_) {
        UtilityFunctions::printerr("PLATEAUCityModel: Model not loaded, cannot extract meshes");
        TypedArray<PLATEAUMeshData> empty_result;
        call_deferred("emit_signal", "extract_completed", empty_result);
        return;
    }

    is_processing_.store(true);
    pending_options_ = options;

    // Use WorkerThreadPool to run extraction in background
    // Stage 1: Extract libplateau Model only (thread-safe)
    WorkerThreadPool::get_singleton()->add_task(
        callable_mp(this, &PLATEAUCityModel::_extract_model_thread_func)
    );
}

void PLATEAUCityModel::_extract_model_thread_func() {
    // Stage 1: Extract libplateau Model only (worker thread - thread-safe)
    // This does NOT create any Godot objects (ArrayMesh, Material, etc.)
    try {
        if (pending_options_.is_valid() && city_model_ != nullptr) {
            plateau::polygonMesh::MeshExtractOptions native_options = pending_options_->get_native();
            pending_model_ = plateau::polygonMesh::MeshExtractor::extract(*city_model_, native_options);
        }
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception extracting model: ", String(e.what()));
        pending_model_.reset();
    }

    // Stage 2: Finalize on main thread (create Godot resources)
    call_deferred("_finalize_meshes_on_main_thread");
}

void PLATEAUCityModel::_finalize_meshes_on_main_thread() {
    // Stage 2: Create Godot resources (main thread only!)
    // This creates ArrayMesh, StandardMaterial3D, ImageTexture, etc.
    TypedArray<PLATEAUMeshData> result;

    if (pending_model_) {
        for (size_t i = 0; i < pending_model_->getRootNodeCount(); i++) {
            const PlateauNode &node = pending_model_->getRootNodeAt(i);
            Ref<PLATEAUMeshData> mesh_data = convert_node(node);
            if (mesh_data.is_valid()) {
                result.push_back(mesh_data);
            }
        }

        UtilityFunctions::print("Extracted ", result.size(), " root nodes (async)");
    }

    // Cleanup
    texture_cache_.clear();
    material_cache_.clear();
    pending_model_.reset();
    pending_options_.unref();
    is_processing_.store(false);

    // Emit signal
    emit_signal("extract_completed", result);
}

bool PLATEAUCityModel::is_processing() const {
    return is_processing_.load();
}
