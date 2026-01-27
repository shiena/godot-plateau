#include "plateau_instanced_city_model.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

using namespace godot;

PLATEAUInstancedCityModel::PLATEAUInstancedCityModel()
    : zone_id_(9), // Tokyo region default
      reference_point_(Vector3(0, 0, 0)),
      unit_scale_(1.0f),
      coordinate_system_(static_cast<int>(PLATEAUCoordinateSystem::EUN)),
      min_lod_(0),
      max_lod_(4),
      mesh_granularity_(1), // 1 = PRIMARY (per-building)
      lod_auto_switch_enabled_(true),
      lod2_distance_(DEFAULT_LOD2_DISTANCE),
      lod1_distance_(DEFAULT_LOD1_DISTANCE),
      lod_disable_in_editor_(true) {
}

PLATEAUInstancedCityModel::~PLATEAUInstancedCityModel() {
}

void PLATEAUInstancedCityModel::set_zone_id(int zone_id) {
    zone_id_ = zone_id;
    geo_reference_cache_.unref();
}

int PLATEAUInstancedCityModel::get_zone_id() const {
    return zone_id_;
}

void PLATEAUInstancedCityModel::set_reference_point(const Vector3 &point) {
    reference_point_ = point;
    geo_reference_cache_.unref();
}

Vector3 PLATEAUInstancedCityModel::get_reference_point() const {
    return reference_point_;
}

void PLATEAUInstancedCityModel::set_unit_scale(float scale) {
    unit_scale_ = scale;
    geo_reference_cache_.unref();
}

float PLATEAUInstancedCityModel::get_unit_scale() const {
    return unit_scale_;
}

void PLATEAUInstancedCityModel::set_coordinate_system(int system) {
    coordinate_system_ = system;
    geo_reference_cache_.unref();
}

int PLATEAUInstancedCityModel::get_coordinate_system() const {
    return coordinate_system_;
}

void PLATEAUInstancedCityModel::set_gml_path(const String &path) {
    gml_path_ = path;
}

String PLATEAUInstancedCityModel::get_gml_path() const {
    return gml_path_;
}

void PLATEAUInstancedCityModel::set_min_lod(int lod) {
    min_lod_ = lod;
}

int PLATEAUInstancedCityModel::get_min_lod() const {
    return min_lod_;
}

void PLATEAUInstancedCityModel::set_max_lod(int lod) {
    max_lod_ = lod;
}

int PLATEAUInstancedCityModel::get_max_lod() const {
    return max_lod_;
}

void PLATEAUInstancedCityModel::set_mesh_granularity(int granularity) {
    mesh_granularity_ = granularity;
}

int PLATEAUInstancedCityModel::get_mesh_granularity() const {
    return mesh_granularity_;
}

double PLATEAUInstancedCityModel::get_latitude() const {
    Ref<PLATEAUGeoReference> geo_ref = get_geo_reference();
    if (geo_ref.is_valid()) {
        Vector3 lat_lon = geo_ref->unproject(Vector3(0, 0, 0));
        return lat_lon.x;
    }
    return 0.0;
}

double PLATEAUInstancedCityModel::get_longitude() const {
    Ref<PLATEAUGeoReference> geo_ref = get_geo_reference();
    if (geo_ref.is_valid()) {
        Vector3 lat_lon = geo_ref->unproject(Vector3(0, 0, 0));
        return lat_lon.y;
    }
    return 0.0;
}

Ref<PLATEAUGeoReference> PLATEAUInstancedCityModel::get_geo_reference() const {
    if (!geo_reference_cache_.is_valid()) {
        geo_reference_cache_.instantiate();
        geo_reference_cache_->set_zone_id(zone_id_);
        geo_reference_cache_->set_reference_point(reference_point_);
        geo_reference_cache_->set_unit_scale(unit_scale_);
        geo_reference_cache_->set_coordinate_system(coordinate_system_);
    }
    return geo_reference_cache_;
}

TypedArray<Node3D> PLATEAUInstancedCityModel::get_gml_transforms() const {
    TypedArray<Node3D> result;
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++) {
        Node3D *child = Object::cast_to<Node3D>(children[i].operator Object*());
        if (child) {
            result.append(child);
        }
    }
    return result;
}

PackedInt32Array PLATEAUInstancedCityModel::get_lods(Node3D *gml_transform) const {
    PackedInt32Array result;
    if (!gml_transform) {
        return result;
    }

    TypedArray<Node> children = gml_transform->get_children();
    for (int i = 0; i < children.size(); i++) {
        Node3D *child = Object::cast_to<Node3D>(children[i].operator Object*());
        if (child) {
            int lod = parse_lod_from_name(child->get_name());
            if (lod >= 0) {
                // Check if already in result
                bool found = false;
                for (int j = 0; j < result.size(); j++) {
                    if (result[j] == lod) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    result.append(lod);
                }
            }
        }
    }

    // Sort ascending
    result.sort();
    return result;
}

TypedArray<Node3D> PLATEAUInstancedCityModel::get_lod_transforms(Node3D *gml_transform) const {
    TypedArray<Node3D> result;
    if (!gml_transform) {
        return result;
    }

    TypedArray<Node> children = gml_transform->get_children();
    for (int i = 0; i < children.size(); i++) {
        Node3D *child = Object::cast_to<Node3D>(children[i].operator Object*());
        if (child) {
            String name = child->get_name();
            if (name.begins_with("LOD")) {
                result.append(child);
            }
        }
    }
    return result;
}

TypedArray<Node3D> PLATEAUInstancedCityModel::get_city_objects(Node3D *gml_transform, int lod) const {
    TypedArray<Node3D> result;
    if (!gml_transform) {
        return result;
    }

    String lod_name = String("LOD") + String::num_int64(lod);

    TypedArray<Node> children = gml_transform->get_children();
    for (int i = 0; i < children.size(); i++) {
        Node3D *child = Object::cast_to<Node3D>(children[i].operator Object*());
        if (child && child->get_name() == lod_name) {
            // Found LOD node, get all MeshInstance3D children
            TypedArray<Node> lod_children = child->get_children();
            for (int j = 0; j < lod_children.size(); j++) {
                MeshInstance3D *mesh = Object::cast_to<MeshInstance3D>(lod_children[j].operator Object*());
                if (mesh) {
                    result.append(mesh);
                }
            }
            break;
        }
    }
    return result;
}

int PLATEAUInstancedCityModel::parse_lod_from_name(const String &name) {
    if (!name.begins_with("LOD")) {
        return -1;
    }
    String num_str = name.substr(3);
    if (num_str.is_valid_int()) {
        return num_str.to_int();
    }
    return -1;
}

// LOD Auto-Switch settings
void PLATEAUInstancedCityModel::set_lod_auto_switch_enabled(bool enabled) {
    lod_auto_switch_enabled_ = enabled;
    apply_lod_settings();
}

bool PLATEAUInstancedCityModel::get_lod_auto_switch_enabled() const {
    return lod_auto_switch_enabled_;
}

void PLATEAUInstancedCityModel::set_lod2_distance(float distance) {
    lod2_distance_ = distance;
    apply_lod_settings();
}

float PLATEAUInstancedCityModel::get_lod2_distance() const {
    return lod2_distance_;
}

void PLATEAUInstancedCityModel::set_lod1_distance(float distance) {
    lod1_distance_ = distance;
    apply_lod_settings();
}

float PLATEAUInstancedCityModel::get_lod1_distance() const {
    return lod1_distance_;
}

void PLATEAUInstancedCityModel::set_lod_disable_in_editor(bool disable) {
    lod_disable_in_editor_ = disable;
    apply_lod_settings();
}

bool PLATEAUInstancedCityModel::get_lod_disable_in_editor() const {
    return lod_disable_in_editor_;
}

void PLATEAUInstancedCityModel::apply_lod_to_mesh(MeshInstance3D *mesh, int lod, int max_lod, int min_lod, bool use_lod) {
    if (!mesh) {
        return;
    }

    if (!use_lod) {
        // Disable LOD: show all meshes
        mesh->set_visibility_range_begin(0);
        mesh->set_visibility_range_end(0);
        return;
    }

    // Apply visibility ranges based on LOD level
    if (lod == max_lod) {
        // Highest LOD: visible from 0 to lod2_distance
        mesh->set_visibility_range_begin(0);
        mesh->set_visibility_range_end(lod2_distance_);
    } else if (lod == min_lod) {
        // Lowest LOD: visible from lod1_distance to infinity (0)
        mesh->set_visibility_range_begin(lod1_distance_);
        mesh->set_visibility_range_end(0);
    } else {
        // Middle LODs: visible from lod2_distance to lod1_distance
        mesh->set_visibility_range_begin(lod2_distance_);
        mesh->set_visibility_range_end(lod1_distance_);
    }
}

void PLATEAUInstancedCityModel::apply_lod_settings() {
    // Determine if LOD should be used
    bool use_lod = lod_auto_switch_enabled_;
    if (lod_disable_in_editor_ && Engine::get_singleton()->is_editor_hint()) {
        use_lod = false;
    }

    // Iterate all GML transforms
    TypedArray<Node3D> gml_transforms = get_gml_transforms();
    for (int i = 0; i < gml_transforms.size(); i++) {
        Node3D *gml_transform = Object::cast_to<Node3D>(gml_transforms[i].operator Object*());
        if (!gml_transform) {
            continue;
        }

        // Get available LODs for this GML
        PackedInt32Array lods = get_lods(gml_transform);
        if (lods.size() <= 1) {
            // Single LOD or no LOD: disable visibility range
            TypedArray<Node3D> lod_transforms = get_lod_transforms(gml_transform);
            for (int j = 0; j < lod_transforms.size(); j++) {
                Node3D *lod_node = Object::cast_to<Node3D>(lod_transforms[j].operator Object*());
                if (!lod_node) {
                    continue;
                }
                TypedArray<Node> children = lod_node->get_children();
                for (int k = 0; k < children.size(); k++) {
                    MeshInstance3D *mesh = Object::cast_to<MeshInstance3D>(children[k].operator Object*());
                    if (mesh) {
                        mesh->set_visibility_range_begin(0);
                        mesh->set_visibility_range_end(0);
                    }
                }
            }
            continue;
        }

        // Multiple LODs: apply visibility ranges
        int max_lod = lods[lods.size() - 1];
        int min_lod = lods[0];

        TypedArray<Node3D> lod_transforms = get_lod_transforms(gml_transform);
        for (int j = 0; j < lod_transforms.size(); j++) {
            Node3D *lod_node = Object::cast_to<Node3D>(lod_transforms[j].operator Object*());
            if (!lod_node) {
                continue;
            }

            int lod = parse_lod_from_name(lod_node->get_name());
            if (lod < 0) {
                continue;
            }

            TypedArray<Node> children = lod_node->get_children();
            for (int k = 0; k < children.size(); k++) {
                MeshInstance3D *mesh = Object::cast_to<MeshInstance3D>(children[k].operator Object*());
                if (mesh) {
                    apply_lod_to_mesh(mesh, lod, max_lod, min_lod, use_lod);
                }
            }
        }
    }
}

void PLATEAUInstancedCityModel::_ready() {
    // Apply LOD settings when scene is loaded
    // This ensures editor settings take effect immediately
    apply_lod_settings();
}

void PLATEAUInstancedCityModel::reset_lod_settings() {
    // Disable all visibility ranges
    TypedArray<Node3D> gml_transforms = get_gml_transforms();
    for (int i = 0; i < gml_transforms.size(); i++) {
        Node3D *gml_transform = Object::cast_to<Node3D>(gml_transforms[i].operator Object*());
        if (!gml_transform) {
            continue;
        }

        TypedArray<Node3D> lod_transforms = get_lod_transforms(gml_transform);
        for (int j = 0; j < lod_transforms.size(); j++) {
            Node3D *lod_node = Object::cast_to<Node3D>(lod_transforms[j].operator Object*());
            if (!lod_node) {
                continue;
            }

            TypedArray<Node> children = lod_node->get_children();
            for (int k = 0; k < children.size(); k++) {
                MeshInstance3D *mesh = Object::cast_to<MeshInstance3D>(children[k].operator Object*());
                if (mesh) {
                    mesh->set_visibility_range_begin(0);
                    mesh->set_visibility_range_end(0);
                }
            }
        }
    }
}

void PLATEAUInstancedCityModel::_bind_methods() {
    // GeoReference data
    ClassDB::bind_method(D_METHOD("set_zone_id", "zone_id"), &PLATEAUInstancedCityModel::set_zone_id);
    ClassDB::bind_method(D_METHOD("get_zone_id"), &PLATEAUInstancedCityModel::get_zone_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "zone_id", PROPERTY_HINT_RANGE, "1,19"), "set_zone_id", "get_zone_id");

    ClassDB::bind_method(D_METHOD("set_reference_point", "point"), &PLATEAUInstancedCityModel::set_reference_point);
    ClassDB::bind_method(D_METHOD("get_reference_point"), &PLATEAUInstancedCityModel::get_reference_point);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "reference_point"), "set_reference_point", "get_reference_point");

    ClassDB::bind_method(D_METHOD("set_unit_scale", "scale"), &PLATEAUInstancedCityModel::set_unit_scale);
    ClassDB::bind_method(D_METHOD("get_unit_scale"), &PLATEAUInstancedCityModel::get_unit_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_scale"), "set_unit_scale", "get_unit_scale");

    ClassDB::bind_method(D_METHOD("set_coordinate_system", "system"), &PLATEAUInstancedCityModel::set_coordinate_system);
    ClassDB::bind_method(D_METHOD("get_coordinate_system"), &PLATEAUInstancedCityModel::get_coordinate_system);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "coordinate_system", PROPERTY_HINT_ENUM, "ENU,WUN,ESU,EUN"), "set_coordinate_system", "get_coordinate_system");

    // GML path
    ClassDB::bind_method(D_METHOD("set_gml_path", "path"), &PLATEAUInstancedCityModel::set_gml_path);
    ClassDB::bind_method(D_METHOD("get_gml_path"), &PLATEAUInstancedCityModel::get_gml_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "gml_path", PROPERTY_HINT_FILE, "*.gml"), "set_gml_path", "get_gml_path");

    // Import settings
    ClassDB::bind_method(D_METHOD("set_min_lod", "lod"), &PLATEAUInstancedCityModel::set_min_lod);
    ClassDB::bind_method(D_METHOD("get_min_lod"), &PLATEAUInstancedCityModel::get_min_lod);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "min_lod", PROPERTY_HINT_RANGE, "0,4"), "set_min_lod", "get_min_lod");

    ClassDB::bind_method(D_METHOD("set_max_lod", "lod"), &PLATEAUInstancedCityModel::set_max_lod);
    ClassDB::bind_method(D_METHOD("get_max_lod"), &PLATEAUInstancedCityModel::get_max_lod);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lod", PROPERTY_HINT_RANGE, "0,4"), "set_max_lod", "get_max_lod");

    ClassDB::bind_method(D_METHOD("set_mesh_granularity", "granularity"), &PLATEAUInstancedCityModel::set_mesh_granularity);
    ClassDB::bind_method(D_METHOD("get_mesh_granularity"), &PLATEAUInstancedCityModel::get_mesh_granularity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mesh_granularity", PROPERTY_HINT_ENUM, "Area,Primary,Atomic"), "set_mesh_granularity", "get_mesh_granularity");

    // Computed properties (read-only)
    ClassDB::bind_method(D_METHOD("get_latitude"), &PLATEAUInstancedCityModel::get_latitude);
    ClassDB::bind_method(D_METHOD("get_longitude"), &PLATEAUInstancedCityModel::get_longitude);

    // GeoReference object
    ClassDB::bind_method(D_METHOD("get_geo_reference"), &PLATEAUInstancedCityModel::get_geo_reference);

    // Child access methods
    ClassDB::bind_method(D_METHOD("get_gml_transforms"), &PLATEAUInstancedCityModel::get_gml_transforms);
    ClassDB::bind_method(D_METHOD("get_lods", "gml_transform"), &PLATEAUInstancedCityModel::get_lods);
    ClassDB::bind_method(D_METHOD("get_lod_transforms", "gml_transform"), &PLATEAUInstancedCityModel::get_lod_transforms);
    ClassDB::bind_method(D_METHOD("get_city_objects", "gml_transform", "lod"), &PLATEAUInstancedCityModel::get_city_objects);

    // Static methods
    ClassDB::bind_static_method("PLATEAUInstancedCityModel", D_METHOD("parse_lod_from_name", "name"), &PLATEAUInstancedCityModel::parse_lod_from_name);

    // LOD Auto-Switch settings
    ADD_GROUP("LOD Auto-Switch", "lod_");

    ClassDB::bind_method(D_METHOD("set_lod_auto_switch_enabled", "enabled"), &PLATEAUInstancedCityModel::set_lod_auto_switch_enabled);
    ClassDB::bind_method(D_METHOD("get_lod_auto_switch_enabled"), &PLATEAUInstancedCityModel::get_lod_auto_switch_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lod_auto_switch_enabled"), "set_lod_auto_switch_enabled", "get_lod_auto_switch_enabled");

    ClassDB::bind_method(D_METHOD("set_lod2_distance", "distance"), &PLATEAUInstancedCityModel::set_lod2_distance);
    ClassDB::bind_method(D_METHOD("get_lod2_distance"), &PLATEAUInstancedCityModel::get_lod2_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod2_distance", PROPERTY_HINT_RANGE, "10,2000,10,suffix:m"), "set_lod2_distance", "get_lod2_distance");

    ClassDB::bind_method(D_METHOD("set_lod1_distance", "distance"), &PLATEAUInstancedCityModel::set_lod1_distance);
    ClassDB::bind_method(D_METHOD("get_lod1_distance"), &PLATEAUInstancedCityModel::get_lod1_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lod1_distance", PROPERTY_HINT_RANGE, "50,5000,10,suffix:m"), "set_lod1_distance", "get_lod1_distance");

    ClassDB::bind_method(D_METHOD("set_lod_disable_in_editor", "disable"), &PLATEAUInstancedCityModel::set_lod_disable_in_editor);
    ClassDB::bind_method(D_METHOD("get_lod_disable_in_editor"), &PLATEAUInstancedCityModel::get_lod_disable_in_editor);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lod_disable_in_editor"), "set_lod_disable_in_editor", "get_lod_disable_in_editor");

    ClassDB::bind_method(D_METHOD("apply_lod_settings"), &PLATEAUInstancedCityModel::apply_lod_settings);
    ClassDB::bind_method(D_METHOD("reset_lod_settings"), &PLATEAUInstancedCityModel::reset_lod_settings);

    // Constants
    BIND_CONSTANT(DEFAULT_LOD2_DISTANCE);
    BIND_CONSTANT(DEFAULT_LOD1_DISTANCE);
}
