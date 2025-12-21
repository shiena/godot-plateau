#include "plateau_road_network.h"
#include "plateau_city_model.h"

#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// ============================================================================
// PLATEAURnPoint
// ============================================================================

PLATEAURnPoint::PLATEAURnPoint() {
}

PLATEAURnPoint::~PLATEAURnPoint() {
}

void PLATEAURnPoint::set_position(const Vector3 &pos) {
    position_ = pos;
}

Vector3 PLATEAURnPoint::get_position() const {
    return position_;
}

Ref<PLATEAURnPoint> PLATEAURnPoint::create(const Vector3 &pos) {
    Ref<PLATEAURnPoint> point;
    point.instantiate();
    point->set_position(pos);
    return point;
}

void PLATEAURnPoint::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_position", "pos"), &PLATEAURnPoint::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &PLATEAURnPoint::get_position);
    ClassDB::bind_static_method("PLATEAURnPoint", D_METHOD("create", "pos"), &PLATEAURnPoint::create);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
}

// ============================================================================
// PLATEAURnLineString
// ============================================================================

PLATEAURnLineString::PLATEAURnLineString() {
}

PLATEAURnLineString::~PLATEAURnLineString() {
}

void PLATEAURnLineString::set_points(const PackedVector3Array &points) {
    points_ = points;
}

PackedVector3Array PLATEAURnLineString::get_points() const {
    return points_;
}

void PLATEAURnLineString::add_point(const Vector3 &point) {
    points_.append(point);
}

void PLATEAURnLineString::add_point_or_skip(const Vector3 &point, float epsilon) {
    if (points_.size() > 0) {
        Vector3 last = points_[points_.size() - 1];
        if (last.distance_to(point) < epsilon) {
            return;
        }
    }
    points_.append(point);
}

int PLATEAURnLineString::get_point_count() const {
    return points_.size();
}

Vector3 PLATEAURnLineString::get_point(int index) const {
    if (index < 0 || index >= points_.size()) {
        return Vector3();
    }
    return points_[index];
}

float PLATEAURnLineString::calc_length() const {
    float length = 0.0f;
    for (int i = 1; i < points_.size(); i++) {
        length += points_[i - 1].distance_to(points_[i]);
    }
    return length;
}

Vector3 PLATEAURnLineString::get_lerp_point(float t) const {
    if (points_.size() == 0) return Vector3();
    if (points_.size() == 1) return points_[0];

    t = CLAMP(t, 0.0f, 1.0f);
    float total_length = calc_length();
    float target_length = total_length * t;

    float current_length = 0.0f;
    for (int i = 1; i < points_.size(); i++) {
        float segment_length = points_[i - 1].distance_to(points_[i]);
        if (current_length + segment_length >= target_length) {
            float segment_t = (target_length - current_length) / segment_length;
            return points_[i - 1].lerp(points_[i], segment_t);
        }
        current_length += segment_length;
    }

    return points_[points_.size() - 1];
}

Vector3 PLATEAURnLineString::get_edge_normal(int index) const {
    if (index < 0 || index >= points_.size() - 1) {
        return Vector3(0, 1, 0);
    }

    Vector3 dir = (points_[index + 1] - points_[index]).normalized();
    // Assuming Y is up, calculate normal on XZ plane
    return Vector3(-dir.z, 0, dir.x).normalized();
}

Ref<PLATEAURnLineString> PLATEAURnLineString::refined(float interval) const {
    Ref<PLATEAURnLineString> result;
    result.instantiate();

    if (points_.size() == 0) return result;

    result->add_point(points_[0]);

    for (int i = 1; i < points_.size(); i++) {
        Vector3 start = points_[i - 1];
        Vector3 end = points_[i];
        float segment_length = start.distance_to(end);

        int num_segments = (int)Math::ceil(segment_length / interval);
        for (int j = 1; j < num_segments; j++) {
            float t = (float)j / num_segments;
            result->add_point(start.lerp(end, t));
        }

        result->add_point(end);
    }

    return result;
}

void PLATEAURnLineString::split_at_index(int index, Ref<PLATEAURnLineString> &front, Ref<PLATEAURnLineString> &back) const {
    front.instantiate();
    back.instantiate();

    for (int i = 0; i <= index && i < points_.size(); i++) {
        front->add_point(points_[i]);
    }

    for (int i = index; i < points_.size(); i++) {
        back->add_point(points_[i]);
    }
}

Ref<PLATEAURnLineString> PLATEAURnLineString::clone() const {
    Ref<PLATEAURnLineString> result;
    result.instantiate();
    result->set_points(points_);
    return result;
}

void PLATEAURnLineString::reverse() {
    points_.reverse();
}

Ref<PLATEAURnLineString> PLATEAURnLineString::create(const PackedVector3Array &points) {
    Ref<PLATEAURnLineString> line;
    line.instantiate();
    line->set_points(points);
    return line;
}

void PLATEAURnLineString::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_points", "points"), &PLATEAURnLineString::set_points);
    ClassDB::bind_method(D_METHOD("get_points"), &PLATEAURnLineString::get_points);
    ClassDB::bind_method(D_METHOD("add_point", "point"), &PLATEAURnLineString::add_point);
    ClassDB::bind_method(D_METHOD("add_point_or_skip", "point", "epsilon"), &PLATEAURnLineString::add_point_or_skip, DEFVAL(0.001f));
    ClassDB::bind_method(D_METHOD("get_point_count"), &PLATEAURnLineString::get_point_count);
    ClassDB::bind_method(D_METHOD("get_point", "index"), &PLATEAURnLineString::get_point);
    ClassDB::bind_method(D_METHOD("calc_length"), &PLATEAURnLineString::calc_length);
    ClassDB::bind_method(D_METHOD("get_lerp_point", "t"), &PLATEAURnLineString::get_lerp_point);
    ClassDB::bind_method(D_METHOD("get_edge_normal", "index"), &PLATEAURnLineString::get_edge_normal);
    ClassDB::bind_method(D_METHOD("refined", "interval"), &PLATEAURnLineString::refined);
    ClassDB::bind_method(D_METHOD("clone"), &PLATEAURnLineString::clone);
    ClassDB::bind_method(D_METHOD("reverse"), &PLATEAURnLineString::reverse);
    ClassDB::bind_static_method("PLATEAURnLineString", D_METHOD("create", "points"), &PLATEAURnLineString::create);

    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "points"), "set_points", "get_points");
}

// ============================================================================
// PLATEAURnWay
// ============================================================================

PLATEAURnWay::PLATEAURnWay() :
    is_reversed_(false),
    is_reverse_normal_(false) {
}

PLATEAURnWay::~PLATEAURnWay() {
}

void PLATEAURnWay::set_line_string(const Ref<PLATEAURnLineString> &line) {
    line_string_ = line;
}

Ref<PLATEAURnLineString> PLATEAURnWay::get_line_string() const {
    return line_string_;
}

void PLATEAURnWay::set_is_reversed(bool reversed) {
    is_reversed_ = reversed;
}

bool PLATEAURnWay::get_is_reversed() const {
    return is_reversed_;
}

void PLATEAURnWay::set_is_reverse_normal(bool reverse) {
    is_reverse_normal_ = reverse;
}

bool PLATEAURnWay::get_is_reverse_normal() const {
    return is_reverse_normal_;
}

Vector3 PLATEAURnWay::get_point(int index) const {
    if (line_string_.is_null()) return Vector3();

    int count = line_string_->get_point_count();
    if (is_reversed_) {
        index = count - 1 - index;
    }
    return line_string_->get_point(index);
}

int PLATEAURnWay::get_point_count() const {
    if (line_string_.is_null()) return 0;
    return line_string_->get_point_count();
}

Vector3 PLATEAURnWay::get_lerp_point(float t) const {
    if (line_string_.is_null()) return Vector3();

    if (is_reversed_) {
        t = 1.0f - t;
    }
    return line_string_->get_lerp_point(t);
}

Vector3 PLATEAURnWay::get_edge_normal(int index) const {
    if (line_string_.is_null()) return Vector3(0, 1, 0);

    int count = line_string_->get_point_count();
    if (is_reversed_) {
        index = count - 2 - index;
    }

    Vector3 normal = line_string_->get_edge_normal(index);

    if (is_reversed_ != is_reverse_normal_) {
        normal = -normal;
    }

    return normal;
}

float PLATEAURnWay::calc_length() const {
    if (line_string_.is_null()) return 0.0f;
    return line_string_->calc_length();
}

bool PLATEAURnWay::is_same_line_reference(const Ref<PLATEAURnWay> &other) const {
    if (other.is_null() || line_string_.is_null()) return false;
    return line_string_ == other->get_line_string();
}

Ref<PLATEAURnWay> PLATEAURnWay::reversed_way() const {
    Ref<PLATEAURnWay> result;
    result.instantiate();
    result->set_line_string(line_string_);
    result->set_is_reversed(!is_reversed_);
    result->set_is_reverse_normal(is_reverse_normal_);
    return result;
}

Ref<PLATEAURnWay> PLATEAURnWay::clone(bool clone_vertex) const {
    Ref<PLATEAURnWay> result;
    result.instantiate();

    if (clone_vertex && line_string_.is_valid()) {
        result->set_line_string(line_string_->clone());
    } else {
        result->set_line_string(line_string_);
    }

    result->set_is_reversed(is_reversed_);
    result->set_is_reverse_normal(is_reverse_normal_);
    return result;
}

Ref<PLATEAURnWay> PLATEAURnWay::create(const Ref<PLATEAURnLineString> &line, bool reversed, bool reverse_normal) {
    Ref<PLATEAURnWay> way;
    way.instantiate();
    way->set_line_string(line);
    way->set_is_reversed(reversed);
    way->set_is_reverse_normal(reverse_normal);
    return way;
}

void PLATEAURnWay::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_line_string", "line"), &PLATEAURnWay::set_line_string);
    ClassDB::bind_method(D_METHOD("get_line_string"), &PLATEAURnWay::get_line_string);
    ClassDB::bind_method(D_METHOD("set_is_reversed", "reversed"), &PLATEAURnWay::set_is_reversed);
    ClassDB::bind_method(D_METHOD("get_is_reversed"), &PLATEAURnWay::get_is_reversed);
    ClassDB::bind_method(D_METHOD("set_is_reverse_normal", "reverse"), &PLATEAURnWay::set_is_reverse_normal);
    ClassDB::bind_method(D_METHOD("get_is_reverse_normal"), &PLATEAURnWay::get_is_reverse_normal);
    ClassDB::bind_method(D_METHOD("get_point", "index"), &PLATEAURnWay::get_point);
    ClassDB::bind_method(D_METHOD("get_point_count"), &PLATEAURnWay::get_point_count);
    ClassDB::bind_method(D_METHOD("get_lerp_point", "t"), &PLATEAURnWay::get_lerp_point);
    ClassDB::bind_method(D_METHOD("get_edge_normal", "index"), &PLATEAURnWay::get_edge_normal);
    ClassDB::bind_method(D_METHOD("calc_length"), &PLATEAURnWay::calc_length);
    ClassDB::bind_method(D_METHOD("is_same_line_reference", "other"), &PLATEAURnWay::is_same_line_reference);
    ClassDB::bind_method(D_METHOD("reversed_way"), &PLATEAURnWay::reversed_way);
    ClassDB::bind_method(D_METHOD("clone", "clone_vertex"), &PLATEAURnWay::clone, DEFVAL(true));
    ClassDB::bind_static_method("PLATEAURnWay", D_METHOD("create", "line", "reversed", "reverse_normal"), &PLATEAURnWay::create, DEFVAL(false), DEFVAL(false));

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "line_string", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAURnLineString"), "set_line_string", "get_line_string");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_reversed"), "set_is_reversed", "get_is_reversed");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_reverse_normal"), "set_is_reverse_normal", "get_is_reverse_normal");
}

// ============================================================================
// PLATEAURnTrack
// ============================================================================

PLATEAURnTrack::PLATEAURnTrack() :
    turn_type_(TURN_STRAIGHT) {
}

PLATEAURnTrack::~PLATEAURnTrack() {
}

void PLATEAURnTrack::set_from_border(const Ref<PLATEAURnWay> &border) {
    from_border_ = border;
}

Ref<PLATEAURnWay> PLATEAURnTrack::get_from_border() const {
    return from_border_;
}

void PLATEAURnTrack::set_to_border(const Ref<PLATEAURnWay> &border) {
    to_border_ = border;
}

Ref<PLATEAURnWay> PLATEAURnTrack::get_to_border() const {
    return to_border_;
}

void PLATEAURnTrack::set_spline(const Ref<Curve3D> &spline) {
    spline_ = spline;
}

Ref<Curve3D> PLATEAURnTrack::get_spline() const {
    return spline_;
}

void PLATEAURnTrack::set_turn_type(PLATEAURnTurnType type) {
    turn_type_ = type;
}

PLATEAURnTurnType PLATEAURnTrack::get_turn_type() const {
    return turn_type_;
}

void PLATEAURnTrack::build_spline() {
    if (from_border_.is_null() || to_border_.is_null()) return;

    spline_.instantiate();

    Vector3 from_pos = from_border_->get_lerp_point(0.5f);
    Vector3 to_pos = to_border_->get_lerp_point(0.5f);

    // Simple bezier curve
    Vector3 from_dir = (from_border_->get_point(from_border_->get_point_count() - 1) -
                        from_border_->get_point(0)).normalized();
    Vector3 to_dir = (to_border_->get_point(to_border_->get_point_count() - 1) -
                      to_border_->get_point(0)).normalized();

    float distance = from_pos.distance_to(to_pos);

    spline_->add_point(from_pos, Vector3(), from_dir * distance * 0.3f);
    spline_->add_point(to_pos, -to_dir * distance * 0.3f, Vector3());
}

Vector3 PLATEAURnTrack::get_point(float t) const {
    if (spline_.is_null()) return Vector3();
    return spline_->sample_baked(t * spline_->get_baked_length());
}

Vector3 PLATEAURnTrack::get_tangent(float t) const {
    if (spline_.is_null()) return Vector3(0, 0, 1);

    float length = spline_->get_baked_length();
    float offset = t * length;
    float delta = 0.01f * length;

    Vector3 p0 = spline_->sample_baked(MAX(0, offset - delta));
    Vector3 p1 = spline_->sample_baked(MIN(length, offset + delta));

    return (p1 - p0).normalized();
}

void PLATEAURnTrack::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_from_border", "border"), &PLATEAURnTrack::set_from_border);
    ClassDB::bind_method(D_METHOD("get_from_border"), &PLATEAURnTrack::get_from_border);
    ClassDB::bind_method(D_METHOD("set_to_border", "border"), &PLATEAURnTrack::set_to_border);
    ClassDB::bind_method(D_METHOD("get_to_border"), &PLATEAURnTrack::get_to_border);
    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &PLATEAURnTrack::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &PLATEAURnTrack::get_spline);
    ClassDB::bind_method(D_METHOD("set_turn_type", "type"), &PLATEAURnTrack::set_turn_type);
    ClassDB::bind_method(D_METHOD("get_turn_type"), &PLATEAURnTrack::get_turn_type);
    ClassDB::bind_method(D_METHOD("build_spline"), &PLATEAURnTrack::build_spline);
    ClassDB::bind_method(D_METHOD("get_point", "t"), &PLATEAURnTrack::get_point);
    ClassDB::bind_method(D_METHOD("get_tangent", "t"), &PLATEAURnTrack::get_tangent);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "from_border", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAURnWay"), "set_from_border", "get_from_border");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "to_border", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAURnWay"), "set_to_border", "get_to_border");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Curve3D"), "set_spline", "get_spline");

    BIND_ENUM_CONSTANT(TURN_LEFT_BACK);
    BIND_ENUM_CONSTANT(TURN_LEFT);
    BIND_ENUM_CONSTANT(TURN_LEFT_FRONT);
    BIND_ENUM_CONSTANT(TURN_STRAIGHT);
    BIND_ENUM_CONSTANT(TURN_RIGHT_FRONT);
    BIND_ENUM_CONSTANT(TURN_RIGHT);
    BIND_ENUM_CONSTANT(TURN_RIGHT_BACK);
    BIND_ENUM_CONSTANT(TURN_U_TURN);
}

// ============================================================================
// PLATEAURnIntersectionEdge
// ============================================================================

PLATEAURnIntersectionEdge::PLATEAURnIntersectionEdge() {
}

PLATEAURnIntersectionEdge::~PLATEAURnIntersectionEdge() {
}

void PLATEAURnIntersectionEdge::set_border(const Ref<PLATEAURnWay> &border) {
    border_ = border;
}

Ref<PLATEAURnWay> PLATEAURnIntersectionEdge::get_border() const {
    return border_;
}

void PLATEAURnIntersectionEdge::set_road(const Ref<PLATEAURnRoad> &road) {
    road_ = road;
}

Ref<PLATEAURnRoad> PLATEAURnIntersectionEdge::get_road() const {
    return road_;
}

bool PLATEAURnIntersectionEdge::is_border() const {
    return road_.is_valid();
}

bool PLATEAURnIntersectionEdge::is_valid() const {
    return border_.is_valid();
}

bool PLATEAURnIntersectionEdge::is_median_border() const {
    if (road_.is_null()) return false;
    Ref<PLATEAURnLane> median = road_->get_median_lane();
    return median.is_valid();
}

PLATEAURnFlowType PLATEAURnIntersectionEdge::get_flow_type() const {
    // Simplified implementation
    return FLOW_BOTH;
}

Vector3 PLATEAURnIntersectionEdge::calc_center() const {
    if (border_.is_null()) return Vector3();
    return border_->get_lerp_point(0.5f);
}

TypedArray<PLATEAURnLane> PLATEAURnIntersectionEdge::get_connected_lanes() const {
    TypedArray<PLATEAURnLane> result;
    if (road_.is_null()) return result;
    return road_->get_main_lanes();
}

void PLATEAURnIntersectionEdge::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_border", "border"), &PLATEAURnIntersectionEdge::set_border);
    ClassDB::bind_method(D_METHOD("get_border"), &PLATEAURnIntersectionEdge::get_border);
    ClassDB::bind_method(D_METHOD("set_road", "road"), &PLATEAURnIntersectionEdge::set_road);
    ClassDB::bind_method(D_METHOD("get_road"), &PLATEAURnIntersectionEdge::get_road);
    ClassDB::bind_method(D_METHOD("is_border"), &PLATEAURnIntersectionEdge::is_border);
    ClassDB::bind_method(D_METHOD("is_valid"), &PLATEAURnIntersectionEdge::is_valid);
    ClassDB::bind_method(D_METHOD("is_median_border"), &PLATEAURnIntersectionEdge::is_median_border);
    ClassDB::bind_method(D_METHOD("get_flow_type"), &PLATEAURnIntersectionEdge::get_flow_type);
    ClassDB::bind_method(D_METHOD("calc_center"), &PLATEAURnIntersectionEdge::calc_center);
    ClassDB::bind_method(D_METHOD("get_connected_lanes"), &PLATEAURnIntersectionEdge::get_connected_lanes);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "border", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAURnWay"), "set_border", "get_border");

    BIND_ENUM_CONSTANT(FLOW_EMPTY);
    BIND_ENUM_CONSTANT(FLOW_INBOUND);
    BIND_ENUM_CONSTANT(FLOW_OUTBOUND);
    BIND_ENUM_CONSTANT(FLOW_BOTH);
}

// ============================================================================
// PLATEAURnLane
// ============================================================================

PLATEAURnLane::PLATEAURnLane() :
    is_reversed_(false),
    attributes_(0) {
}

PLATEAURnLane::~PLATEAURnLane() {
}

void PLATEAURnLane::set_parent_road(const Ref<PLATEAURnRoad> &road) {
    parent_road_ = road;
}

Ref<PLATEAURnRoad> PLATEAURnLane::get_parent_road() const {
    return parent_road_;
}

void PLATEAURnLane::set_left_way(const Ref<PLATEAURnWay> &way) {
    left_way_ = way;
}

Ref<PLATEAURnWay> PLATEAURnLane::get_left_way() const {
    return left_way_;
}

void PLATEAURnLane::set_right_way(const Ref<PLATEAURnWay> &way) {
    right_way_ = way;
}

Ref<PLATEAURnWay> PLATEAURnLane::get_right_way() const {
    return right_way_;
}

void PLATEAURnLane::set_prev_border(const Ref<PLATEAURnWay> &border) {
    prev_border_ = border;
}

Ref<PLATEAURnWay> PLATEAURnLane::get_prev_border() const {
    return prev_border_;
}

void PLATEAURnLane::set_next_border(const Ref<PLATEAURnWay> &border) {
    next_border_ = border;
}

Ref<PLATEAURnWay> PLATEAURnLane::get_next_border() const {
    return next_border_;
}

void PLATEAURnLane::set_is_reversed(bool reversed) {
    is_reversed_ = reversed;
}

bool PLATEAURnLane::get_is_reversed() const {
    return is_reversed_;
}

void PLATEAURnLane::set_attributes(int64_t attrs) {
    attributes_ = attrs;
}

int64_t PLATEAURnLane::get_attributes() const {
    return attributes_;
}

bool PLATEAURnLane::is_valid_way() const {
    return left_way_.is_valid() && right_way_.is_valid();
}

bool PLATEAURnLane::has_both_border() const {
    return prev_border_.is_valid() && next_border_.is_valid();
}

bool PLATEAURnLane::is_both_connected_lane() const {
    return prev_border_.is_valid() && next_border_.is_valid();
}

bool PLATEAURnLane::is_empty_lane() const {
    return !left_way_.is_valid() && !right_way_.is_valid();
}

bool PLATEAURnLane::is_median_lane() const {
    return (attributes_ & LANE_ATTR_MEDIAN) != 0;
}

float PLATEAURnLane::calc_width() const {
    if (!is_valid_way()) return 0.0f;

    // Calculate average width
    float total_width = 0.0f;
    int samples = 10;

    for (int i = 0; i < samples; i++) {
        float t = (float)i / (samples - 1);
        Vector3 left = left_way_->get_lerp_point(t);
        Vector3 right = right_way_->get_lerp_point(t);
        total_width += left.distance_to(right);
    }

    return total_width / samples;
}

float PLATEAURnLane::calc_min_width() const {
    if (!is_valid_way()) return 0.0f;

    float min_width = 1e10f;
    int samples = 10;

    for (int i = 0; i < samples; i++) {
        float t = (float)i / (samples - 1);
        Vector3 left = left_way_->get_lerp_point(t);
        Vector3 right = right_way_->get_lerp_point(t);
        min_width = MIN(min_width, left.distance_to(right));
    }

    return min_width;
}

Ref<PLATEAURnWay> PLATEAURnLane::create_center_way() const {
    if (!is_valid_way()) return Ref<PLATEAURnWay>();

    Ref<PLATEAURnLineString> line;
    line.instantiate();

    int count = MAX(left_way_->get_point_count(), right_way_->get_point_count());
    for (int i = 0; i < count; i++) {
        float t = (float)i / (count - 1);
        Vector3 left = left_way_->get_lerp_point(t);
        Vector3 right = right_way_->get_lerp_point(t);
        line->add_point((left + right) * 0.5f);
    }

    return PLATEAURnWay::create(line);
}

TypedArray<PLATEAURnLane> PLATEAURnLane::get_prev_lanes() const {
    // Would need connection tracking
    return TypedArray<PLATEAURnLane>();
}

TypedArray<PLATEAURnLane> PLATEAURnLane::get_next_lanes() const {
    return TypedArray<PLATEAURnLane>();
}

void PLATEAURnLane::reverse() {
    is_reversed_ = !is_reversed_;

    // Swap left/right
    Ref<PLATEAURnWay> temp = left_way_;
    left_way_ = right_way_;
    right_way_ = temp;

    // Swap prev/next borders
    temp = prev_border_;
    prev_border_ = next_border_;
    next_border_ = temp;
}

TypedArray<PLATEAURnLane> PLATEAURnLane::split(int split_num) const {
    TypedArray<PLATEAURnLane> result;
    // Implementation would split lane into multiple lanes
    return result;
}

void PLATEAURnLane::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_parent_road", "road"), &PLATEAURnLane::set_parent_road);
    ClassDB::bind_method(D_METHOD("get_parent_road"), &PLATEAURnLane::get_parent_road);
    ClassDB::bind_method(D_METHOD("set_left_way", "way"), &PLATEAURnLane::set_left_way);
    ClassDB::bind_method(D_METHOD("get_left_way"), &PLATEAURnLane::get_left_way);
    ClassDB::bind_method(D_METHOD("set_right_way", "way"), &PLATEAURnLane::set_right_way);
    ClassDB::bind_method(D_METHOD("get_right_way"), &PLATEAURnLane::get_right_way);
    ClassDB::bind_method(D_METHOD("set_prev_border", "border"), &PLATEAURnLane::set_prev_border);
    ClassDB::bind_method(D_METHOD("get_prev_border"), &PLATEAURnLane::get_prev_border);
    ClassDB::bind_method(D_METHOD("set_next_border", "border"), &PLATEAURnLane::set_next_border);
    ClassDB::bind_method(D_METHOD("get_next_border"), &PLATEAURnLane::get_next_border);
    ClassDB::bind_method(D_METHOD("set_is_reversed", "reversed"), &PLATEAURnLane::set_is_reversed);
    ClassDB::bind_method(D_METHOD("get_is_reversed"), &PLATEAURnLane::get_is_reversed);
    ClassDB::bind_method(D_METHOD("set_attributes", "attrs"), &PLATEAURnLane::set_attributes);
    ClassDB::bind_method(D_METHOD("get_attributes"), &PLATEAURnLane::get_attributes);
    ClassDB::bind_method(D_METHOD("is_valid_way"), &PLATEAURnLane::is_valid_way);
    ClassDB::bind_method(D_METHOD("has_both_border"), &PLATEAURnLane::has_both_border);
    ClassDB::bind_method(D_METHOD("is_both_connected_lane"), &PLATEAURnLane::is_both_connected_lane);
    ClassDB::bind_method(D_METHOD("is_empty_lane"), &PLATEAURnLane::is_empty_lane);
    ClassDB::bind_method(D_METHOD("is_median_lane"), &PLATEAURnLane::is_median_lane);
    ClassDB::bind_method(D_METHOD("calc_width"), &PLATEAURnLane::calc_width);
    ClassDB::bind_method(D_METHOD("calc_min_width"), &PLATEAURnLane::calc_min_width);
    ClassDB::bind_method(D_METHOD("create_center_way"), &PLATEAURnLane::create_center_way);
    ClassDB::bind_method(D_METHOD("get_prev_lanes"), &PLATEAURnLane::get_prev_lanes);
    ClassDB::bind_method(D_METHOD("get_next_lanes"), &PLATEAURnLane::get_next_lanes);
    ClassDB::bind_method(D_METHOD("reverse"), &PLATEAURnLane::reverse);
    ClassDB::bind_method(D_METHOD("split", "split_num"), &PLATEAURnLane::split);

    BIND_ENUM_CONSTANT(LANE_ATTR_NONE);
    BIND_ENUM_CONSTANT(LANE_ATTR_LEFT_TURN);
    BIND_ENUM_CONSTANT(LANE_ATTR_RIGHT_TURN);
    BIND_ENUM_CONSTANT(LANE_ATTR_STRAIGHT);
    BIND_ENUM_CONSTANT(LANE_ATTR_MEDIAN);
}

// ============================================================================
// PLATEAURnSideWalk
// ============================================================================

PLATEAURnSideWalk::PLATEAURnSideWalk() :
    lane_type_(SIDEWALK_UNDEFINED) {
}

PLATEAURnSideWalk::~PLATEAURnSideWalk() {
}

void PLATEAURnSideWalk::set_outside_way(const Ref<PLATEAURnWay> &way) {
    outside_way_ = way;
}

Ref<PLATEAURnWay> PLATEAURnSideWalk::get_outside_way() const {
    return outside_way_;
}

void PLATEAURnSideWalk::set_inside_way(const Ref<PLATEAURnWay> &way) {
    inside_way_ = way;
}

Ref<PLATEAURnWay> PLATEAURnSideWalk::get_inside_way() const {
    return inside_way_;
}

void PLATEAURnSideWalk::set_start_edge_way(const Ref<PLATEAURnWay> &way) {
    start_edge_way_ = way;
}

Ref<PLATEAURnWay> PLATEAURnSideWalk::get_start_edge_way() const {
    return start_edge_way_;
}

void PLATEAURnSideWalk::set_end_edge_way(const Ref<PLATEAURnWay> &way) {
    end_edge_way_ = way;
}

Ref<PLATEAURnWay> PLATEAURnSideWalk::get_end_edge_way() const {
    return end_edge_way_;
}

void PLATEAURnSideWalk::set_lane_type(PLATEAURnSideWalkLaneType type) {
    lane_type_ = type;
}

PLATEAURnSideWalkLaneType PLATEAURnSideWalk::get_lane_type() const {
    return lane_type_;
}

bool PLATEAURnSideWalk::is_valid() const {
    return inside_way_.is_valid() && outside_way_.is_valid();
}

bool PLATEAURnSideWalk::is_all_way_valid() const {
    return inside_way_.is_valid() && outside_way_.is_valid() &&
           start_edge_way_.is_valid() && end_edge_way_.is_valid();
}

void PLATEAURnSideWalk::align() {
    // Align way directions to edge ways
}

void PLATEAURnSideWalk::reverse() {
    Ref<PLATEAURnWay> temp = start_edge_way_;
    start_edge_way_ = end_edge_way_;
    end_edge_way_ = temp;
}

bool PLATEAURnSideWalk::try_merge_neighbor(const Ref<PLATEAURnSideWalk> &other) {
    // Implementation would merge adjacent sidewalks
    return false;
}

void PLATEAURnSideWalk::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_outside_way", "way"), &PLATEAURnSideWalk::set_outside_way);
    ClassDB::bind_method(D_METHOD("get_outside_way"), &PLATEAURnSideWalk::get_outside_way);
    ClassDB::bind_method(D_METHOD("set_inside_way", "way"), &PLATEAURnSideWalk::set_inside_way);
    ClassDB::bind_method(D_METHOD("get_inside_way"), &PLATEAURnSideWalk::get_inside_way);
    ClassDB::bind_method(D_METHOD("set_start_edge_way", "way"), &PLATEAURnSideWalk::set_start_edge_way);
    ClassDB::bind_method(D_METHOD("get_start_edge_way"), &PLATEAURnSideWalk::get_start_edge_way);
    ClassDB::bind_method(D_METHOD("set_end_edge_way", "way"), &PLATEAURnSideWalk::set_end_edge_way);
    ClassDB::bind_method(D_METHOD("get_end_edge_way"), &PLATEAURnSideWalk::get_end_edge_way);
    ClassDB::bind_method(D_METHOD("set_lane_type", "type"), &PLATEAURnSideWalk::set_lane_type);
    ClassDB::bind_method(D_METHOD("get_lane_type"), &PLATEAURnSideWalk::get_lane_type);
    ClassDB::bind_method(D_METHOD("is_valid"), &PLATEAURnSideWalk::is_valid);
    ClassDB::bind_method(D_METHOD("is_all_way_valid"), &PLATEAURnSideWalk::is_all_way_valid);
    ClassDB::bind_method(D_METHOD("align"), &PLATEAURnSideWalk::align);
    ClassDB::bind_method(D_METHOD("reverse"), &PLATEAURnSideWalk::reverse);
    ClassDB::bind_method(D_METHOD("try_merge_neighbor", "other"), &PLATEAURnSideWalk::try_merge_neighbor);

    BIND_ENUM_CONSTANT(SIDEWALK_UNDEFINED);
    BIND_ENUM_CONSTANT(SIDEWALK_LEFT_LANE);
    BIND_ENUM_CONSTANT(SIDEWALK_RIGHT_LANE);
}

// ============================================================================
// PLATEAURnRoadBase
// ============================================================================

PLATEAURnRoadBase::PLATEAURnRoadBase() :
    id_(0) {
}

PLATEAURnRoadBase::~PLATEAURnRoadBase() {
}

void PLATEAURnRoadBase::set_id(int64_t id) {
    id_ = id;
}

int64_t PLATEAURnRoadBase::get_id() const {
    return id_;
}

void PLATEAURnRoadBase::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_id", "id"), &PLATEAURnRoadBase::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &PLATEAURnRoadBase::get_id);
    ClassDB::bind_method(D_METHOD("is_road"), &PLATEAURnRoadBase::is_road);
    ClassDB::bind_method(D_METHOD("is_intersection"), &PLATEAURnRoadBase::is_intersection);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "id"), "set_id", "get_id");
}

// ============================================================================
// PLATEAURnRoad
// ============================================================================

PLATEAURnRoad::PLATEAURnRoad() {
}

PLATEAURnRoad::~PLATEAURnRoad() {
}

void PLATEAURnRoad::set_prev(const Ref<PLATEAURnRoadBase> &prev) {
    prev_ = prev;
}

Ref<PLATEAURnRoadBase> PLATEAURnRoad::get_prev() const {
    return prev_;
}

void PLATEAURnRoad::set_next(const Ref<PLATEAURnRoadBase> &next) {
    next_ = next;
}

Ref<PLATEAURnRoadBase> PLATEAURnRoad::get_next() const {
    return next_;
}

void PLATEAURnRoad::add_main_lane(const Ref<PLATEAURnLane> &lane) {
    if (lane.is_valid()) {
        lane->set_parent_road(Ref<PLATEAURnRoad>(this));
        main_lanes_.append(lane);
    }
}

void PLATEAURnRoad::remove_main_lane(const Ref<PLATEAURnLane> &lane) {
    int idx = main_lanes_.find(lane);
    if (idx >= 0) {
        main_lanes_.remove_at(idx);
    }
}

TypedArray<PLATEAURnLane> PLATEAURnRoad::get_main_lanes() const {
    return main_lanes_;
}

int PLATEAURnRoad::get_main_lane_count() const {
    return main_lanes_.size();
}

void PLATEAURnRoad::set_median_lane(const Ref<PLATEAURnLane> &lane) {
    median_lane_ = lane;
    if (lane.is_valid()) {
        lane->set_attributes(lane->get_attributes() | LANE_ATTR_MEDIAN);
    }
}

Ref<PLATEAURnLane> PLATEAURnRoad::get_median_lane() const {
    return median_lane_;
}

float PLATEAURnRoad::get_median_width() const {
    if (median_lane_.is_null()) return 0.0f;
    return median_lane_->calc_width();
}

void PLATEAURnRoad::add_sidewalk(const Ref<PLATEAURnSideWalk> &sidewalk) {
    if (sidewalk.is_valid()) {
        sidewalks_.append(sidewalk);
    }
}

TypedArray<PLATEAURnSideWalk> PLATEAURnRoad::get_sidewalks() const {
    return sidewalks_;
}

TypedArray<PLATEAURnLane> PLATEAURnRoad::get_left_lanes() const {
    TypedArray<PLATEAURnLane> result;
    for (int i = 0; i < main_lanes_.size(); i++) {
        Ref<PLATEAURnLane> lane = main_lanes_[i];
        if (lane.is_valid() && !lane->get_is_reversed()) {
            result.append(lane);
        }
    }
    return result;
}

TypedArray<PLATEAURnLane> PLATEAURnRoad::get_right_lanes() const {
    TypedArray<PLATEAURnLane> result;
    for (int i = 0; i < main_lanes_.size(); i++) {
        Ref<PLATEAURnLane> lane = main_lanes_[i];
        if (lane.is_valid() && lane->get_is_reversed()) {
            result.append(lane);
        }
    }
    return result;
}

int PLATEAURnRoad::get_left_lane_count() const {
    return get_left_lanes().size();
}

int PLATEAURnRoad::get_right_lane_count() const {
    return get_right_lanes().size();
}

bool PLATEAURnRoad::is_valid() const {
    for (int i = 0; i < main_lanes_.size(); i++) {
        Ref<PLATEAURnLane> lane = main_lanes_[i];
        if (lane.is_valid() && !lane->has_both_border()) {
            return false;
        }
    }
    return true;
}

bool PLATEAURnRoad::is_all_both_connected_lane() const {
    for (int i = 0; i < main_lanes_.size(); i++) {
        Ref<PLATEAURnLane> lane = main_lanes_[i];
        if (lane.is_valid() && !lane->is_both_connected_lane()) {
            return false;
        }
    }
    return true;
}

bool PLATEAURnRoad::is_all_lane_valid() const {
    for (int i = 0; i < main_lanes_.size(); i++) {
        Ref<PLATEAURnLane> lane = main_lanes_[i];
        if (lane.is_valid() && !lane->is_valid_way()) {
            return false;
        }
    }
    return true;
}

bool PLATEAURnRoad::has_both_lane() const {
    return get_left_lane_count() > 0 && get_right_lane_count() > 0;
}

bool PLATEAURnRoad::is_empty_road() const {
    return main_lanes_.size() == 0;
}

Ref<PLATEAURnWay> PLATEAURnRoad::get_merged_side_way(bool left) const {
    TypedArray<PLATEAURnLane> lanes = left ? get_left_lanes() : get_right_lanes();
    if (lanes.size() == 0) return Ref<PLATEAURnWay>();

    Ref<PLATEAURnLane> outermost = lanes[left ? 0 : lanes.size() - 1];
    return left ? outermost->get_left_way() : outermost->get_right_way();
}

void PLATEAURnRoad::reverse() {
    // Swap prev/next
    Ref<PLATEAURnRoadBase> temp = prev_;
    prev_ = next_;
    next_ = temp;

    // Reverse all lanes
    for (int i = 0; i < main_lanes_.size(); i++) {
        Ref<PLATEAURnLane> lane = main_lanes_[i];
        if (lane.is_valid()) {
            lane->reverse();
        }
    }

    // Reverse lane order
    TypedArray<PLATEAURnLane> reversed;
    for (int i = main_lanes_.size() - 1; i >= 0; i--) {
        reversed.append(main_lanes_[i]);
    }
    main_lanes_ = reversed;
}

void PLATEAURnRoad::align_lane_borders() {
    // Implementation would align border directions
}

void PLATEAURnRoad::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_prev", "prev"), &PLATEAURnRoad::set_prev);
    ClassDB::bind_method(D_METHOD("get_prev"), &PLATEAURnRoad::get_prev);
    ClassDB::bind_method(D_METHOD("set_next", "next"), &PLATEAURnRoad::set_next);
    ClassDB::bind_method(D_METHOD("get_next"), &PLATEAURnRoad::get_next);
    ClassDB::bind_method(D_METHOD("add_main_lane", "lane"), &PLATEAURnRoad::add_main_lane);
    ClassDB::bind_method(D_METHOD("remove_main_lane", "lane"), &PLATEAURnRoad::remove_main_lane);
    ClassDB::bind_method(D_METHOD("get_main_lanes"), &PLATEAURnRoad::get_main_lanes);
    ClassDB::bind_method(D_METHOD("get_main_lane_count"), &PLATEAURnRoad::get_main_lane_count);
    ClassDB::bind_method(D_METHOD("set_median_lane", "lane"), &PLATEAURnRoad::set_median_lane);
    ClassDB::bind_method(D_METHOD("get_median_lane"), &PLATEAURnRoad::get_median_lane);
    ClassDB::bind_method(D_METHOD("get_median_width"), &PLATEAURnRoad::get_median_width);
    ClassDB::bind_method(D_METHOD("add_sidewalk", "sidewalk"), &PLATEAURnRoad::add_sidewalk);
    ClassDB::bind_method(D_METHOD("get_sidewalks"), &PLATEAURnRoad::get_sidewalks);
    ClassDB::bind_method(D_METHOD("get_left_lanes"), &PLATEAURnRoad::get_left_lanes);
    ClassDB::bind_method(D_METHOD("get_right_lanes"), &PLATEAURnRoad::get_right_lanes);
    ClassDB::bind_method(D_METHOD("get_left_lane_count"), &PLATEAURnRoad::get_left_lane_count);
    ClassDB::bind_method(D_METHOD("get_right_lane_count"), &PLATEAURnRoad::get_right_lane_count);
    ClassDB::bind_method(D_METHOD("is_valid"), &PLATEAURnRoad::is_valid);
    ClassDB::bind_method(D_METHOD("is_all_both_connected_lane"), &PLATEAURnRoad::is_all_both_connected_lane);
    ClassDB::bind_method(D_METHOD("is_all_lane_valid"), &PLATEAURnRoad::is_all_lane_valid);
    ClassDB::bind_method(D_METHOD("has_both_lane"), &PLATEAURnRoad::has_both_lane);
    ClassDB::bind_method(D_METHOD("is_empty_road"), &PLATEAURnRoad::is_empty_road);
    ClassDB::bind_method(D_METHOD("get_merged_side_way", "left"), &PLATEAURnRoad::get_merged_side_way);
    ClassDB::bind_method(D_METHOD("reverse"), &PLATEAURnRoad::reverse);
    ClassDB::bind_method(D_METHOD("align_lane_borders"), &PLATEAURnRoad::align_lane_borders);
}

// ============================================================================
// PLATEAURnIntersection
// ============================================================================

PLATEAURnIntersection::PLATEAURnIntersection() {
}

PLATEAURnIntersection::~PLATEAURnIntersection() {
}

void PLATEAURnIntersection::add_edge(const Ref<PLATEAURnIntersectionEdge> &edge) {
    if (edge.is_valid()) {
        edges_.append(edge);
    }
}

void PLATEAURnIntersection::remove_edge(const Ref<PLATEAURnIntersectionEdge> &edge) {
    int idx = edges_.find(edge);
    if (idx >= 0) {
        edges_.remove_at(idx);
    }
}

TypedArray<PLATEAURnIntersectionEdge> PLATEAURnIntersection::get_edges() const {
    return edges_;
}

TypedArray<PLATEAURnIntersectionEdge> PLATEAURnIntersection::get_borders() const {
    TypedArray<PLATEAURnIntersectionEdge> result;
    for (int i = 0; i < edges_.size(); i++) {
        Ref<PLATEAURnIntersectionEdge> edge = edges_[i];
        if (edge.is_valid() && edge->is_border()) {
            result.append(edge);
        }
    }
    return result;
}

void PLATEAURnIntersection::add_track(const Ref<PLATEAURnTrack> &track) {
    if (track.is_valid()) {
        tracks_.append(track);
    }
}

void PLATEAURnIntersection::remove_track(const Ref<PLATEAURnTrack> &track) {
    int idx = tracks_.find(track);
    if (idx >= 0) {
        tracks_.remove_at(idx);
    }
}

TypedArray<PLATEAURnTrack> PLATEAURnIntersection::get_tracks() const {
    return tracks_;
}

Ref<PLATEAURnTrack> PLATEAURnIntersection::find_track(const Ref<PLATEAURnWay> &from, const Ref<PLATEAURnWay> &to) const {
    for (int i = 0; i < tracks_.size(); i++) {
        Ref<PLATEAURnTrack> track = tracks_[i];
        if (track.is_valid() &&
            track->get_from_border()->is_same_line_reference(from) &&
            track->get_to_border()->is_same_line_reference(to)) {
            return track;
        }
    }
    return Ref<PLATEAURnTrack>();
}

void PLATEAURnIntersection::build_tracks() {
    tracks_.clear();

    TypedArray<PLATEAURnIntersectionEdge> borders = get_borders();

    // Create track for each pair of borders
    for (int i = 0; i < borders.size(); i++) {
        Ref<PLATEAURnIntersectionEdge> from_edge = borders[i];
        for (int j = 0; j < borders.size(); j++) {
            if (i == j) continue;

            Ref<PLATEAURnIntersectionEdge> to_edge = borders[j];

            Ref<PLATEAURnTrack> track;
            track.instantiate();
            track->set_from_border(from_edge->get_border());
            track->set_to_border(to_edge->get_border());
            track->build_spline();

            add_track(track);
        }
    }
}

bool PLATEAURnIntersection::is_empty_intersection() const {
    return edges_.size() <= 2;
}

void PLATEAURnIntersection::align() {
    // Would sort edges clockwise
}

bool PLATEAURnIntersection::is_inside_2d(const Vector3 &pos) const {
    // Simple point-in-polygon test on XZ plane
    int count = 0;
    for (int i = 0; i < edges_.size(); i++) {
        Ref<PLATEAURnIntersectionEdge> edge = edges_[i];
        if (edge.is_null() || edge->get_border().is_null()) continue;

        Ref<PLATEAURnWay> border = edge->get_border();
        for (int j = 0; j < border->get_point_count() - 1; j++) {
            Vector3 p1 = border->get_point(j);
            Vector3 p2 = border->get_point(j + 1);

            if (((p1.z <= pos.z && pos.z < p2.z) || (p2.z <= pos.z && pos.z < p1.z)) &&
                pos.x < (p2.x - p1.x) * (pos.z - p1.z) / (p2.z - p1.z) + p1.x) {
                count++;
            }
        }
    }
    return (count % 2) == 1;
}

Vector3 PLATEAURnIntersection::get_center() const {
    Vector3 sum;
    int count = 0;

    for (int i = 0; i < edges_.size(); i++) {
        Ref<PLATEAURnIntersectionEdge> edge = edges_[i];
        if (edge.is_valid()) {
            sum += edge->calc_center();
            count++;
        }
    }

    return count > 0 ? sum / count : Vector3();
}

void PLATEAURnIntersection::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_edge", "edge"), &PLATEAURnIntersection::add_edge);
    ClassDB::bind_method(D_METHOD("remove_edge", "edge"), &PLATEAURnIntersection::remove_edge);
    ClassDB::bind_method(D_METHOD("get_edges"), &PLATEAURnIntersection::get_edges);
    ClassDB::bind_method(D_METHOD("get_borders"), &PLATEAURnIntersection::get_borders);
    ClassDB::bind_method(D_METHOD("add_track", "track"), &PLATEAURnIntersection::add_track);
    ClassDB::bind_method(D_METHOD("remove_track", "track"), &PLATEAURnIntersection::remove_track);
    ClassDB::bind_method(D_METHOD("get_tracks"), &PLATEAURnIntersection::get_tracks);
    ClassDB::bind_method(D_METHOD("find_track", "from", "to"), &PLATEAURnIntersection::find_track);
    ClassDB::bind_method(D_METHOD("build_tracks"), &PLATEAURnIntersection::build_tracks);
    ClassDB::bind_method(D_METHOD("is_empty_intersection"), &PLATEAURnIntersection::is_empty_intersection);
    ClassDB::bind_method(D_METHOD("align"), &PLATEAURnIntersection::align);
    ClassDB::bind_method(D_METHOD("is_inside_2d", "pos"), &PLATEAURnIntersection::is_inside_2d);
    ClassDB::bind_method(D_METHOD("get_center"), &PLATEAURnIntersection::get_center);
}

// ============================================================================
// PLATEAURnModel
// ============================================================================

PLATEAURnModel::PLATEAURnModel() :
    factory_version_("1.0"),
    next_id_(1) {
}

PLATEAURnModel::~PLATEAURnModel() {
}

void PLATEAURnModel::add_road(const Ref<PLATEAURnRoad> &road) {
    if (road.is_valid()) {
        road->set_id(allocate_id());
        roads_.append(road);
    }
}

void PLATEAURnModel::remove_road(const Ref<PLATEAURnRoad> &road) {
    int idx = roads_.find(road);
    if (idx >= 0) {
        roads_.remove_at(idx);
    }
}

TypedArray<PLATEAURnRoad> PLATEAURnModel::get_roads() const {
    return roads_;
}

int PLATEAURnModel::get_road_count() const {
    return roads_.size();
}

void PLATEAURnModel::add_intersection(const Ref<PLATEAURnIntersection> &intersection) {
    if (intersection.is_valid()) {
        intersection->set_id(allocate_id());
        intersections_.append(intersection);
    }
}

void PLATEAURnModel::remove_intersection(const Ref<PLATEAURnIntersection> &intersection) {
    int idx = intersections_.find(intersection);
    if (idx >= 0) {
        intersections_.remove_at(idx);
    }
}

TypedArray<PLATEAURnIntersection> PLATEAURnModel::get_intersections() const {
    return intersections_;
}

int PLATEAURnModel::get_intersection_count() const {
    return intersections_.size();
}

void PLATEAURnModel::add_sidewalk(const Ref<PLATEAURnSideWalk> &sidewalk) {
    if (sidewalk.is_valid()) {
        sidewalks_.append(sidewalk);
    }
}

TypedArray<PLATEAURnSideWalk> PLATEAURnModel::get_sidewalks() const {
    return sidewalks_;
}

void PLATEAURnModel::set_factory_version(const String &version) {
    factory_version_ = version;
}

String PLATEAURnModel::get_factory_version() const {
    return factory_version_;
}

TypedArray<PLATEAURnLane> PLATEAURnModel::collect_all_lanes() const {
    TypedArray<PLATEAURnLane> result;
    for (int i = 0; i < roads_.size(); i++) {
        Ref<PLATEAURnRoad> road = roads_[i];
        if (road.is_valid()) {
            TypedArray<PLATEAURnLane> lanes = road->get_main_lanes();
            for (int j = 0; j < lanes.size(); j++) {
                result.append(lanes[j]);
            }
        }
    }
    return result;
}

TypedArray<PLATEAURnWay> PLATEAURnModel::collect_all_ways() const {
    TypedArray<PLATEAURnWay> result;
    TypedArray<PLATEAURnLane> lanes = collect_all_lanes();

    for (int i = 0; i < lanes.size(); i++) {
        Ref<PLATEAURnLane> lane = lanes[i];
        if (lane.is_valid()) {
            if (lane->get_left_way().is_valid()) result.append(lane->get_left_way());
            if (lane->get_right_way().is_valid()) result.append(lane->get_right_way());
            if (lane->get_prev_border().is_valid()) result.append(lane->get_prev_border());
            if (lane->get_next_border().is_valid()) result.append(lane->get_next_border());
        }
    }

    return result;
}

TypedArray<PLATEAURnIntersectionEdge> PLATEAURnModel::collect_all_edges() const {
    TypedArray<PLATEAURnIntersectionEdge> result;
    for (int i = 0; i < intersections_.size(); i++) {
        Ref<PLATEAURnIntersection> intersection = intersections_[i];
        if (intersection.is_valid()) {
            TypedArray<PLATEAURnIntersectionEdge> edges = intersection->get_edges();
            for (int j = 0; j < edges.size(); j++) {
                result.append(edges[j]);
            }
        }
    }
    return result;
}

Ref<PLATEAURnIntersection> PLATEAURnModel::convert_road_to_intersection(const Ref<PLATEAURnRoad> &road, bool build_tracks) {
    // Implementation would convert a road segment to an intersection
    return Ref<PLATEAURnIntersection>();
}

Ref<PLATEAURnRoad> PLATEAURnModel::convert_intersection_to_road(const Ref<PLATEAURnIntersection> &intersection) {
    // Implementation would convert an intersection to a road
    return Ref<PLATEAURnRoad>();
}

void PLATEAURnModel::merge_road_groups() {
    // Implementation would merge consecutive roads
}

void PLATEAURnModel::split_lanes_by_width(float width, bool rebuild_tracks) {
    // Implementation would split lanes wider than specified width
}

bool PLATEAURnModel::check() const {
    // Validate the network
    for (int i = 0; i < roads_.size(); i++) {
        Ref<PLATEAURnRoad> road = roads_[i];
        if (road.is_valid() && !road->is_valid()) {
            return false;
        }
    }
    return true;
}

Ref<PLATEAURnModel> PLATEAURnModel::create_from_mesh_data(const TypedArray<PLATEAUMeshData> &mesh_data_array) {
    Ref<PLATEAURnModel> model;
    model.instantiate();

    // This would parse road mesh data to extract network structure
    // For now, just create an empty model
    UtilityFunctions::push_warning("PLATEAURnModel::create_from_mesh_data not fully implemented");

    return model;
}

Ref<ArrayMesh> PLATEAURnModel::generate_mesh() const {
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    // Generate mesh for all lanes
    for (int i = 0; i < roads_.size(); i++) {
        Ref<PLATEAURnRoad> road = roads_[i];
        if (road.is_null()) continue;

        TypedArray<PLATEAURnLane> lanes = road->get_main_lanes();
        for (int j = 0; j < lanes.size(); j++) {
            Ref<PLATEAURnLane> lane = lanes[j];
            if (lane.is_null() || !lane->is_valid_way()) continue;

            Ref<PLATEAURnWay> left = lane->get_left_way();
            Ref<PLATEAURnWay> right = lane->get_right_way();

            int count = MAX(left->get_point_count(), right->get_point_count());
            for (int k = 0; k < count - 1; k++) {
                float t0 = (float)k / (count - 1);
                float t1 = (float)(k + 1) / (count - 1);

                Vector3 l0 = left->get_lerp_point(t0);
                Vector3 l1 = left->get_lerp_point(t1);
                Vector3 r0 = right->get_lerp_point(t0);
                Vector3 r1 = right->get_lerp_point(t1);

                st->set_normal(Vector3(0, 1, 0));

                // First triangle
                st->add_vertex(l0);
                st->add_vertex(r0);
                st->add_vertex(l1);

                // Second triangle
                st->add_vertex(l1);
                st->add_vertex(r0);
                st->add_vertex(r1);
            }
        }
    }

    return st->commit();
}

Dictionary PLATEAURnModel::serialize() const {
    Dictionary result;
    result["factory_version"] = factory_version_;

    // Serialize roads, intersections, sidewalks
    // Implementation would be more complete

    return result;
}

Ref<PLATEAURnModel> PLATEAURnModel::deserialize(const Dictionary &data) {
    Ref<PLATEAURnModel> model;
    model.instantiate();

    if (data.has("factory_version")) {
        model->set_factory_version(data["factory_version"]);
    }

    // Deserialize roads, intersections, sidewalks
    // Implementation would be more complete

    return model;
}

int64_t PLATEAURnModel::allocate_id() {
    return next_id_++;
}

void PLATEAURnModel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_road", "road"), &PLATEAURnModel::add_road);
    ClassDB::bind_method(D_METHOD("remove_road", "road"), &PLATEAURnModel::remove_road);
    ClassDB::bind_method(D_METHOD("get_roads"), &PLATEAURnModel::get_roads);
    ClassDB::bind_method(D_METHOD("get_road_count"), &PLATEAURnModel::get_road_count);
    ClassDB::bind_method(D_METHOD("add_intersection", "intersection"), &PLATEAURnModel::add_intersection);
    ClassDB::bind_method(D_METHOD("remove_intersection", "intersection"), &PLATEAURnModel::remove_intersection);
    ClassDB::bind_method(D_METHOD("get_intersections"), &PLATEAURnModel::get_intersections);
    ClassDB::bind_method(D_METHOD("get_intersection_count"), &PLATEAURnModel::get_intersection_count);
    ClassDB::bind_method(D_METHOD("add_sidewalk", "sidewalk"), &PLATEAURnModel::add_sidewalk);
    ClassDB::bind_method(D_METHOD("get_sidewalks"), &PLATEAURnModel::get_sidewalks);
    ClassDB::bind_method(D_METHOD("set_factory_version", "version"), &PLATEAURnModel::set_factory_version);
    ClassDB::bind_method(D_METHOD("get_factory_version"), &PLATEAURnModel::get_factory_version);
    ClassDB::bind_method(D_METHOD("collect_all_lanes"), &PLATEAURnModel::collect_all_lanes);
    ClassDB::bind_method(D_METHOD("collect_all_ways"), &PLATEAURnModel::collect_all_ways);
    ClassDB::bind_method(D_METHOD("collect_all_edges"), &PLATEAURnModel::collect_all_edges);
    ClassDB::bind_method(D_METHOD("convert_road_to_intersection", "road", "build_tracks"), &PLATEAURnModel::convert_road_to_intersection, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("convert_intersection_to_road", "intersection"), &PLATEAURnModel::convert_intersection_to_road);
    ClassDB::bind_method(D_METHOD("merge_road_groups"), &PLATEAURnModel::merge_road_groups);
    ClassDB::bind_method(D_METHOD("split_lanes_by_width", "width", "rebuild_tracks"), &PLATEAURnModel::split_lanes_by_width, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("check"), &PLATEAURnModel::check);
    ClassDB::bind_static_method("PLATEAURnModel", D_METHOD("create_from_mesh_data", "mesh_data_array"), &PLATEAURnModel::create_from_mesh_data);
    ClassDB::bind_method(D_METHOD("generate_mesh"), &PLATEAURnModel::generate_mesh);
    ClassDB::bind_method(D_METHOD("serialize"), &PLATEAURnModel::serialize);
    ClassDB::bind_static_method("PLATEAURnModel", D_METHOD("deserialize", "data"), &PLATEAURnModel::deserialize);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "factory_version"), "set_factory_version", "get_factory_version");
}

} // namespace godot
