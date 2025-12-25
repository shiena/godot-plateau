#include "plateau_instanced_city_model.h"
#include <godot_cpp/classes/mesh_instance3d.hpp>

using namespace godot;

PLATEAUInstancedCityModel::PLATEAUInstancedCityModel()
    : zone_id_(9), // Tokyo region default
      reference_point_(Vector3(0, 0, 0)),
      unit_scale_(1.0f),
      coordinate_system_(static_cast<int>(PLATEAUCoordinateSystem::EUN)),
      min_lod_(0),
      max_lod_(4),
      mesh_granularity_(1) { // 1 = PRIMARY (per-building)
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
}
