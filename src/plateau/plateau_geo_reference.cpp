#include "plateau_geo_reference.h"

using namespace godot;
using namespace plateau::geometry;

PLATEAUGeoReference::PLATEAUGeoReference()
    : zone_id_(9), // Tokyo region default
      reference_point_(Vector3(0, 0, 0)),
      unit_scale_(1.0f),
      coordinate_system_(static_cast<int>(PLATEAUCoordinateSystem::EUN)) {
    update_native();
}

PLATEAUGeoReference::~PLATEAUGeoReference() {
}

void PLATEAUGeoReference::set_zone_id(int zone_id) {
    zone_id_ = zone_id;
    update_native();
}

int PLATEAUGeoReference::get_zone_id() const {
    return zone_id_;
}

void PLATEAUGeoReference::set_reference_point(const Vector3 &point) {
    reference_point_ = point;
    update_native();
}

Vector3 PLATEAUGeoReference::get_reference_point() const {
    return reference_point_;
}

void PLATEAUGeoReference::set_unit_scale(float scale) {
    unit_scale_ = scale;
    update_native();
}

float PLATEAUGeoReference::get_unit_scale() const {
    return unit_scale_;
}

void PLATEAUGeoReference::set_coordinate_system(int system) {
    coordinate_system_ = system;
    update_native();
}

int PLATEAUGeoReference::get_coordinate_system() const {
    return coordinate_system_;
}

Vector3 PLATEAUGeoReference::project(const Vector3 &lat_lon_height) const {
    if (!geo_reference_) {
        return Vector3();
    }

    GeoCoordinate geo_coord(lat_lon_height.x, lat_lon_height.y, lat_lon_height.z);
    TVec3d result = geo_reference_->project(geo_coord);
    return Vector3(result.x, result.y, result.z);
}

Vector3 PLATEAUGeoReference::unproject(const Vector3 &xyz) const {
    if (!geo_reference_) {
        return Vector3();
    }

    TVec3d point(xyz.x, xyz.y, xyz.z);
    GeoCoordinate result = geo_reference_->unproject(point);
    return Vector3(result.latitude, result.longitude, result.height);
}

void PLATEAUGeoReference::update_native() {
    TVec3d ref_point(reference_point_.x, reference_point_.y, reference_point_.z);
    CoordinateSystem coord_sys = static_cast<CoordinateSystem>(coordinate_system_);

    geo_reference_ = std::make_unique<GeoReference>(
        zone_id_,
        ref_point,
        unit_scale_,
        coord_sys
    );
}

void PLATEAUGeoReference::_bind_methods() {
    // Zone ID
    ClassDB::bind_method(D_METHOD("set_zone_id", "zone_id"), &PLATEAUGeoReference::set_zone_id);
    ClassDB::bind_method(D_METHOD("get_zone_id"), &PLATEAUGeoReference::get_zone_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "zone_id", PROPERTY_HINT_RANGE, "1,19"), "set_zone_id", "get_zone_id");

    // Reference point
    ClassDB::bind_method(D_METHOD("set_reference_point", "point"), &PLATEAUGeoReference::set_reference_point);
    ClassDB::bind_method(D_METHOD("get_reference_point"), &PLATEAUGeoReference::get_reference_point);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "reference_point"), "set_reference_point", "get_reference_point");

    // Unit scale
    ClassDB::bind_method(D_METHOD("set_unit_scale", "scale"), &PLATEAUGeoReference::set_unit_scale);
    ClassDB::bind_method(D_METHOD("get_unit_scale"), &PLATEAUGeoReference::get_unit_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_scale"), "set_unit_scale", "get_unit_scale");

    // Coordinate system
    ClassDB::bind_method(D_METHOD("set_coordinate_system", "system"), &PLATEAUGeoReference::set_coordinate_system);
    ClassDB::bind_method(D_METHOD("get_coordinate_system"), &PLATEAUGeoReference::get_coordinate_system);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "coordinate_system", PROPERTY_HINT_ENUM, "ENU,WUN,ESU,EUN"), "set_coordinate_system", "get_coordinate_system");

    // Conversion methods
    ClassDB::bind_method(D_METHOD("project", "lat_lon_height"), &PLATEAUGeoReference::project);
    ClassDB::bind_method(D_METHOD("unproject", "xyz"), &PLATEAUGeoReference::unproject);

}
