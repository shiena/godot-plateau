#include "plateau_city_model_scene.h"
#include "plateau_city_object_type.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// ============================================================================
// PLATEAUFilterCondition
// ============================================================================

PLATEAUFilterCondition::PLATEAUFilterCondition() :
    city_object_types_(0xFFFFFFFFFFFFFFFFLL),
    min_lod_(0),
    max_lod_(4),
    packages_(PLATEAUCityModelPackage::PACKAGE_ALL) {
}

PLATEAUFilterCondition::~PLATEAUFilterCondition() {
}

void PLATEAUFilterCondition::set_city_object_types(int64_t types) {
    city_object_types_ = types;
}

int64_t PLATEAUFilterCondition::get_city_object_types() const {
    return city_object_types_;
}

void PLATEAUFilterCondition::set_min_lod(int lod) {
    min_lod_ = lod;
}

int PLATEAUFilterCondition::get_min_lod() const {
    return min_lod_;
}

void PLATEAUFilterCondition::set_max_lod(int lod) {
    max_lod_ = lod;
}

int PLATEAUFilterCondition::get_max_lod() const {
    return max_lod_;
}

void PLATEAUFilterCondition::set_packages(int64_t packages) {
    packages_ = packages;
}

int64_t PLATEAUFilterCondition::get_packages() const {
    return packages_;
}

bool PLATEAUFilterCondition::matches(const Ref<PLATEAUMeshData> &mesh_data) const {
    if (mesh_data.is_null()) {
        return false;
    }
    return matches_type(mesh_data->get_city_object_type());
}

bool PLATEAUFilterCondition::matches_type(int64_t city_object_type) const {
    if (city_object_type == 0) {
        return true; // Unknown type, allow
    }
    return (city_object_types_ & city_object_type) != 0;
}

bool PLATEAUFilterCondition::matches_lod(int lod) const {
    return lod >= min_lod_ && lod <= max_lod_;
}

Ref<PLATEAUFilterCondition> PLATEAUFilterCondition::create_all() {
    Ref<PLATEAUFilterCondition> filter;
    filter.instantiate();
    return filter;
}

Ref<PLATEAUFilterCondition> PLATEAUFilterCondition::create_buildings() {
    Ref<PLATEAUFilterCondition> filter;
    filter.instantiate();
    filter->set_city_object_types(
        COT_Building | COT_BuildingPart | COT_BuildingInstallation |
        COT_RoofSurface | COT_WallSurface | COT_GroundSurface |
        COT_ClosureSurface | COT_FloorSurface | COT_Door | COT_Window
    );
    filter->set_packages(PLATEAUCityModelPackage::PACKAGE_BUILDING);
    return filter;
}

Ref<PLATEAUFilterCondition> PLATEAUFilterCondition::create_terrain() {
    Ref<PLATEAUFilterCondition> filter;
    filter.instantiate();
    filter->set_city_object_types(
        COT_ReliefFeature | COT_ReliefComponent | COT_TINRelief |
        COT_MassPointRelief | COT_BreaklineRelief | COT_RasterRelief
    );
    filter->set_packages(PLATEAUCityModelPackage::PACKAGE_RELIEF);
    return filter;
}

void PLATEAUFilterCondition::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_city_object_types", "types"), &PLATEAUFilterCondition::set_city_object_types);
    ClassDB::bind_method(D_METHOD("get_city_object_types"), &PLATEAUFilterCondition::get_city_object_types);
    ClassDB::bind_method(D_METHOD("set_min_lod", "lod"), &PLATEAUFilterCondition::set_min_lod);
    ClassDB::bind_method(D_METHOD("get_min_lod"), &PLATEAUFilterCondition::get_min_lod);
    ClassDB::bind_method(D_METHOD("set_max_lod", "lod"), &PLATEAUFilterCondition::set_max_lod);
    ClassDB::bind_method(D_METHOD("get_max_lod"), &PLATEAUFilterCondition::get_max_lod);
    ClassDB::bind_method(D_METHOD("set_packages", "packages"), &PLATEAUFilterCondition::set_packages);
    ClassDB::bind_method(D_METHOD("get_packages"), &PLATEAUFilterCondition::get_packages);
    ClassDB::bind_method(D_METHOD("matches", "mesh_data"), &PLATEAUFilterCondition::matches);
    ClassDB::bind_method(D_METHOD("matches_type", "city_object_type"), &PLATEAUFilterCondition::matches_type);
    ClassDB::bind_method(D_METHOD("matches_lod", "lod"), &PLATEAUFilterCondition::matches_lod);
    ClassDB::bind_static_method("PLATEAUFilterCondition", D_METHOD("create_all"), &PLATEAUFilterCondition::create_all);
    ClassDB::bind_static_method("PLATEAUFilterCondition", D_METHOD("create_buildings"), &PLATEAUFilterCondition::create_buildings);
    ClassDB::bind_static_method("PLATEAUFilterCondition", D_METHOD("create_terrain"), &PLATEAUFilterCondition::create_terrain);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "city_object_types"), "set_city_object_types", "get_city_object_types");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "min_lod"), "set_min_lod", "get_min_lod");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lod"), "set_max_lod", "get_max_lod");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "packages"), "set_packages", "get_packages");
}

// ============================================================================
// PLATEAUCityModelScene
// ============================================================================

PLATEAUCityModelScene::PLATEAUCityModelScene() {
}

PLATEAUCityModelScene::~PLATEAUCityModelScene() {
}

void PLATEAUCityModelScene::set_geo_reference(const Ref<PLATEAUGeoReference> &geo_ref) {
    geo_reference_ = geo_ref;
}

Ref<PLATEAUGeoReference> PLATEAUCityModelScene::get_geo_reference() const {
    return geo_reference_;
}

double PLATEAUCityModelScene::get_latitude() const {
    if (geo_reference_.is_null()) {
        return 0.0;
    }
    Vector3 origin = geo_reference_->get_reference_point();
    Vector3 lat_lon = geo_reference_->unproject(origin);
    return lat_lon.x;
}

double PLATEAUCityModelScene::get_longitude() const {
    if (geo_reference_.is_null()) {
        return 0.0;
    }
    Vector3 origin = geo_reference_->get_reference_point();
    Vector3 lat_lon = geo_reference_->unproject(origin);
    return lat_lon.y;
}

TypedArray<Node3D> PLATEAUCityModelScene::get_gml_transforms() const {
    TypedArray<Node3D> result;
    for (int i = 0; i < get_child_count(); i++) {
        Node3D *child = Object::cast_to<Node3D>(get_child(i));
        if (child && gml_path_to_node_.has(child->get_name())) {
            result.append(child);
        }
    }
    return result;
}

Node3D *PLATEAUCityModelScene::get_gml_transform(const String &gml_path) const {
    if (!gml_path_to_node_.has(gml_path)) {
        return nullptr;
    }
    String node_name = gml_path_to_node_[gml_path];
    return Object::cast_to<Node3D>(find_child(node_name, false, false));
}

Node3D *PLATEAUCityModelScene::import_gml(const String &gml_path, const Ref<PLATEAUMeshExtractOptions> &options) {
    // Create CityModel and load
    Ref<PLATEAUCityModel> city_model;
    city_model.instantiate();

    if (!city_model->load(gml_path)) {
        UtilityFunctions::push_error("Failed to load GML: " + gml_path);
        return nullptr;
    }

    // Set geo reference if provided
    if (geo_reference_.is_valid() && options.is_valid()) {
        // Options should already have coordinate zone set
    }

    // Extract meshes
    TypedArray<PLATEAUMeshData> mesh_data_array = city_model->extract_meshes(options);
    if (mesh_data_array.is_empty()) {
        UtilityFunctions::push_warning("No meshes extracted from GML: " + gml_path);
        return nullptr;
    }

    // Create root node for this GML
    Node3D *gml_root = memnew(Node3D);
    String base_name = gml_path.get_file().get_basename();
    gml_root->set_name(base_name);
    add_child(gml_root);
    gml_root->set_owner(get_owner() ? get_owner() : this);

    // Build scene hierarchy from mesh data
    for (int i = 0; i < mesh_data_array.size(); i++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];
        if (mesh_data.is_null()) continue;

        // Create MeshInstance3D
        if (mesh_data->get_mesh().is_valid()) {
            MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
            mesh_instance->set_name(mesh_data->get_name().is_empty() ? String("Mesh_") + String::num_int64(i) : mesh_data->get_name());
            mesh_instance->set_mesh(mesh_data->get_mesh());
            mesh_instance->set_transform(mesh_data->get_transform());

            // Store mesh data as metadata for later retrieval
            mesh_instance->set_meta("plateau_mesh_data", mesh_data);
            mesh_instance->set_meta("gml_id", mesh_data->get_gml_id());
            mesh_instance->set_meta("city_object_type", mesh_data->get_city_object_type());

            gml_root->add_child(mesh_instance);
            mesh_instance->set_owner(get_owner() ? get_owner() : this);
        }

        // Recursively add children
        for (int j = 0; j < mesh_data->get_child_count(); j++) {
            Ref<PLATEAUMeshData> child_data = mesh_data->get_child(j);
            if (child_data.is_valid() && child_data->get_mesh().is_valid()) {
                MeshInstance3D *child_instance = memnew(MeshInstance3D);
                child_instance->set_name(child_data->get_name().is_empty() ? String("Mesh_") + String::num_int64(j) : child_data->get_name());
                child_instance->set_mesh(child_data->get_mesh());
                child_instance->set_transform(child_data->get_transform());
                child_instance->set_meta("plateau_mesh_data", child_data);
                child_instance->set_meta("gml_id", child_data->get_gml_id());
                child_instance->set_meta("city_object_type", child_data->get_city_object_type());

                gml_root->add_child(child_instance);
                child_instance->set_owner(get_owner() ? get_owner() : this);
            }
        }
    }

    // Store mappings
    gml_path_to_node_[gml_path] = gml_root->get_name();
    node_to_options_[gml_root->get_name()] = options;

    return gml_root;
}

void PLATEAUCityModelScene::import_gml_async(const String &gml_path, const Ref<PLATEAUMeshExtractOptions> &options) {
    // For now, just call sync version
    // TODO: Implement proper async with signals
    Node3D *result = import_gml(gml_path, options);
    emit_signal("gml_imported", gml_path, result != nullptr);
}

PackedInt32Array PLATEAUCityModelScene::get_lods(Node3D *gml_transform) const {
    PackedInt32Array result;
    if (!gml_transform) return result;

    // Collect unique LODs from mesh metadata
    for (int i = 0; i < gml_transform->get_child_count(); i++) {
        MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(gml_transform->get_child(i));
        if (mesh_instance && mesh_instance->has_meta("lod")) {
            int lod = mesh_instance->get_meta("lod");
            if (result.find(lod) == -1) {
                result.append(lod);
            }
        }
    }

    result.sort();
    return result;
}

TypedArray<Node3D> PLATEAUCityModelScene::get_lod_transforms(Node3D *gml_transform) const {
    TypedArray<Node3D> result;
    if (!gml_transform) return result;

    for (int i = 0; i < gml_transform->get_child_count(); i++) {
        Node3D *child = Object::cast_to<Node3D>(gml_transform->get_child(i));
        if (child && child->get_name().begins_with("LOD")) {
            result.append(child);
        }
    }

    return result;
}

TypedArray<Node3D> PLATEAUCityModelScene::get_city_objects(Node3D *gml_transform, int lod) const {
    TypedArray<Node3D> result;
    if (!gml_transform) return result;

    // Find LOD node
    Node3D *lod_node = Object::cast_to<Node3D>(gml_transform->find_child("LOD" + String::num_int64(lod), false, false));
    if (!lod_node) {
        lod_node = gml_transform; // Fall back to gml_transform itself
    }

    // Collect all mesh instances recursively
    TypedArray<MeshInstance3D> mesh_instances;
    collect_mesh_instances(lod_node, mesh_instances);

    for (int i = 0; i < mesh_instances.size(); i++) {
        result.append(mesh_instances[i]);
    }

    return result;
}

int64_t PLATEAUCityModelScene::get_package(const Ref<PLATEAUMeshData> &mesh_data) const {
    if (mesh_data.is_null()) {
        return PLATEAUCityModelPackage::PACKAGE_UNKNOWN;
    }

    int64_t type = mesh_data->get_city_object_type();
    return PLATEAUCityObjectTypeHierarchy::type_to_package(type);
}

TypedArray<MeshInstance3D> PLATEAUCityModelScene::get_all_mesh_instances() const {
    TypedArray<MeshInstance3D> result;
    collect_mesh_instances(const_cast<PLATEAUCityModelScene*>(this), result);
    return result;
}

Ref<PLATEAUMeshData> PLATEAUCityModelScene::find_mesh_data_by_gml_id(const String &gml_id) const {
    TypedArray<MeshInstance3D> mesh_instances = get_all_mesh_instances();

    for (int i = 0; i < mesh_instances.size(); i++) {
        MeshInstance3D *instance = Object::cast_to<MeshInstance3D>(mesh_instances[i]);
        if (instance && instance->has_meta("gml_id")) {
            String instance_gml_id = instance->get_meta("gml_id");
            if (instance_gml_id == gml_id) {
                return get_mesh_data_from_instance(instance);
            }
        }
    }

    return Ref<PLATEAUMeshData>();
}

void PLATEAUCityModelScene::set_filter_condition(const Ref<PLATEAUFilterCondition> &condition) {
    filter_condition_ = condition;
}

Ref<PLATEAUFilterCondition> PLATEAUCityModelScene::get_filter_condition() const {
    return filter_condition_;
}

void PLATEAUCityModelScene::copy_from(PLATEAUCityModelScene *other) {
    if (!other) return;
    geo_reference_ = other->geo_reference_;
}

bool PLATEAUCityModelScene::reload_gml(Node3D *gml_transform) {
    if (!gml_transform) return false;

    String node_name = gml_transform->get_name();

    // Find original GML path
    String gml_path;
    Array keys = gml_path_to_node_.keys();
    for (int i = 0; i < keys.size(); i++) {
        if (gml_path_to_node_[keys[i]] == node_name) {
            gml_path = keys[i];
            break;
        }
    }

    if (gml_path.is_empty()) return false;

    // Get original options
    Ref<PLATEAUMeshExtractOptions> options;
    if (node_to_options_.has(node_name)) {
        options = node_to_options_[node_name];
    }

    // Remove old transform
    remove_gml(gml_transform);

    // Re-import
    return import_gml(gml_path, options) != nullptr;
}

void PLATEAUCityModelScene::remove_gml(Node3D *gml_transform) {
    if (!gml_transform) return;

    String node_name = gml_transform->get_name();

    // Remove from mappings
    Array keys = gml_path_to_node_.keys();
    for (int i = 0; i < keys.size(); i++) {
        if (gml_path_to_node_[keys[i]] == node_name) {
            gml_path_to_node_.erase(keys[i]);
            break;
        }
    }
    node_to_options_.erase(node_name);

    // Remove node
    remove_child(gml_transform);
    gml_transform->queue_free();
}

void PLATEAUCityModelScene::collect_mesh_instances(Node *node, TypedArray<MeshInstance3D> &result) const {
    if (!node) return;

    MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(node);
    if (mesh_instance) {
        result.append(mesh_instance);
    }

    for (int i = 0; i < node->get_child_count(); i++) {
        collect_mesh_instances(node->get_child(i), result);
    }
}

Ref<PLATEAUMeshData> PLATEAUCityModelScene::get_mesh_data_from_instance(MeshInstance3D *instance) const {
    if (!instance || !instance->has_meta("plateau_mesh_data")) {
        return Ref<PLATEAUMeshData>();
    }
    return instance->get_meta("plateau_mesh_data");
}

void PLATEAUCityModelScene::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_geo_reference", "geo_ref"), &PLATEAUCityModelScene::set_geo_reference);
    ClassDB::bind_method(D_METHOD("get_geo_reference"), &PLATEAUCityModelScene::get_geo_reference);
    ClassDB::bind_method(D_METHOD("get_latitude"), &PLATEAUCityModelScene::get_latitude);
    ClassDB::bind_method(D_METHOD("get_longitude"), &PLATEAUCityModelScene::get_longitude);
    ClassDB::bind_method(D_METHOD("get_gml_transforms"), &PLATEAUCityModelScene::get_gml_transforms);
    ClassDB::bind_method(D_METHOD("get_gml_transform", "gml_path"), &PLATEAUCityModelScene::get_gml_transform);
    ClassDB::bind_method(D_METHOD("import_gml", "gml_path", "options"), &PLATEAUCityModelScene::import_gml);
    ClassDB::bind_method(D_METHOD("import_gml_async", "gml_path", "options"), &PLATEAUCityModelScene::import_gml_async);
    ClassDB::bind_method(D_METHOD("get_lods", "gml_transform"), &PLATEAUCityModelScene::get_lods);
    ClassDB::bind_method(D_METHOD("get_lod_transforms", "gml_transform"), &PLATEAUCityModelScene::get_lod_transforms);
    ClassDB::bind_method(D_METHOD("get_city_objects", "gml_transform", "lod"), &PLATEAUCityModelScene::get_city_objects);
    ClassDB::bind_method(D_METHOD("get_package", "mesh_data"), &PLATEAUCityModelScene::get_package);
    ClassDB::bind_method(D_METHOD("get_all_mesh_instances"), &PLATEAUCityModelScene::get_all_mesh_instances);
    ClassDB::bind_method(D_METHOD("find_mesh_data_by_gml_id", "gml_id"), &PLATEAUCityModelScene::find_mesh_data_by_gml_id);
    ClassDB::bind_method(D_METHOD("set_filter_condition", "condition"), &PLATEAUCityModelScene::set_filter_condition);
    ClassDB::bind_method(D_METHOD("get_filter_condition"), &PLATEAUCityModelScene::get_filter_condition);
    ClassDB::bind_method(D_METHOD("copy_from", "other"), &PLATEAUCityModelScene::copy_from);
    ClassDB::bind_method(D_METHOD("reload_gml", "gml_transform"), &PLATEAUCityModelScene::reload_gml);
    ClassDB::bind_method(D_METHOD("remove_gml", "gml_transform"), &PLATEAUCityModelScene::remove_gml);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "geo_reference", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUGeoReference"), "set_geo_reference", "get_geo_reference");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "latitude", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_EDITOR), "", "get_latitude");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "longitude", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_EDITOR), "", "get_longitude");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "filter_condition", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUFilterCondition"), "set_filter_condition", "get_filter_condition");

    ADD_SIGNAL(MethodInfo("gml_imported", PropertyInfo(Variant::STRING, "gml_path"), PropertyInfo(Variant::BOOL, "success")));
}

} // namespace godot
