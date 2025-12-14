#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <plateau/geometry/geo_reference.h>

namespace godot {

// Godot-compatible coordinate system enum
enum class PLATEAUCoordinateSystem {
    ENU = 0, // PLATEAU default (East, North, Up)
    WUN = 1, // West, Up, North
    ESU = 2, // Unreal Engine (East, South, Up)
    EUN = 3  // Unity/Godot (East, Up, North)
};

class PLATEAUGeoReference : public RefCounted {
    GDCLASS(PLATEAUGeoReference, RefCounted)

public:
    PLATEAUGeoReference();
    ~PLATEAUGeoReference();

    // Zone ID (Japanese coordinate zone 1-19)
    void set_zone_id(int zone_id);
    int get_zone_id() const;

    // Reference point (origin for local coordinates)
    void set_reference_point(const Vector3 &point);
    Vector3 get_reference_point() const;

    // Unit scale
    void set_unit_scale(float scale);
    float get_unit_scale() const;

    // Coordinate system
    void set_coordinate_system(int system);
    int get_coordinate_system() const;

    // Coordinate conversion methods
    Vector3 project(const Vector3 &lat_lon_height) const;
    Vector3 unproject(const Vector3 &xyz) const;

    // Internal access for other PLATEAU classes
    plateau::geometry::GeoReference *get_native() const { return geo_reference_.get(); }

protected:
    static void _bind_methods();

private:
    std::unique_ptr<plateau::geometry::GeoReference> geo_reference_;
    int zone_id_;
    Vector3 reference_point_;
    float unit_scale_;
    int coordinate_system_;

    void update_native();
};

} // namespace godot
