#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

namespace godot {

class PLATEAURnRoad;
class PLATEAURnIntersection;
class PLATEAURnLane;
class PLATEAURnSideWalk;

/**
 * Turn types for intersection tracks
 */
enum PLATEAURnTurnType {
    TURN_LEFT_BACK = 0,
    TURN_LEFT = 1,
    TURN_LEFT_FRONT = 2,
    TURN_STRAIGHT = 3,
    TURN_RIGHT_FRONT = 4,
    TURN_RIGHT = 5,
    TURN_RIGHT_BACK = 6,
    TURN_U_TURN = 7,
};

/**
 * Flow type mask for intersection edges
 */
enum PLATEAURnFlowType {
    FLOW_EMPTY = 0,
    FLOW_INBOUND = 1 << 0,
    FLOW_OUTBOUND = 1 << 1,
    FLOW_BOTH = FLOW_INBOUND | FLOW_OUTBOUND,
};

/**
 * Lane attributes
 */
enum PLATEAURnLaneAttribute {
    LANE_ATTR_NONE = 0,
    LANE_ATTR_LEFT_TURN = 1 << 0,
    LANE_ATTR_RIGHT_TURN = 1 << 1,
    LANE_ATTR_STRAIGHT = 1 << 2,
    LANE_ATTR_MEDIAN = 1 << 3,
};

/**
 * Sidewalk lane type
 */
enum PLATEAURnSideWalkLaneType {
    SIDEWALK_UNDEFINED = 0,
    SIDEWALK_LEFT_LANE = 1,
    SIDEWALK_RIGHT_LANE = 2,
};

/**
 * PLATEAURnPoint - 3D point in road network
 */
class PLATEAURnPoint : public RefCounted {
    GDCLASS(PLATEAURnPoint, RefCounted)

public:
    PLATEAURnPoint();
    ~PLATEAURnPoint();

    void set_position(const Vector3 &pos);
    Vector3 get_position() const;

    static Ref<PLATEAURnPoint> create(const Vector3 &pos);

protected:
    static void _bind_methods();

private:
    Vector3 position_;
};

/**
 * PLATEAURnLineString - Sequence of points forming a line
 */
class PLATEAURnLineString : public RefCounted {
    GDCLASS(PLATEAURnLineString, RefCounted)

public:
    PLATEAURnLineString();
    ~PLATEAURnLineString();

    void set_points(const PackedVector3Array &points);
    PackedVector3Array get_points() const;

    void add_point(const Vector3 &point);
    void add_point_or_skip(const Vector3 &point, float epsilon = 0.001f);
    int get_point_count() const;
    Vector3 get_point(int index) const;

    // Calculate total length
    float calc_length() const;

    // Get interpolated point at t (0.0 - 1.0)
    Vector3 get_lerp_point(float t) const;

    // Get edge normal at index
    Vector3 get_edge_normal(int index) const;

    // Refine with specified interval
    Ref<PLATEAURnLineString> refined(float interval) const;

    // Split at index
    void split_at_index(int index, Ref<PLATEAURnLineString> &front, Ref<PLATEAURnLineString> &back) const;

    // Clone
    Ref<PLATEAURnLineString> clone() const;

    // Reverse order
    void reverse();

    static Ref<PLATEAURnLineString> create(const PackedVector3Array &points);

protected:
    static void _bind_methods();

private:
    PackedVector3Array points_;
};

/**
 * PLATEAURnWay - Wrapper for LineString with direction and normal flags
 */
class PLATEAURnWay : public RefCounted {
    GDCLASS(PLATEAURnWay, RefCounted)

public:
    PLATEAURnWay();
    ~PLATEAURnWay();

    void set_line_string(const Ref<PLATEAURnLineString> &line);
    Ref<PLATEAURnLineString> get_line_string() const;

    void set_is_reversed(bool reversed);
    bool get_is_reversed() const;

    void set_is_reverse_normal(bool reverse);
    bool get_is_reverse_normal() const;

    // Get point considering reverse flag
    Vector3 get_point(int index) const;
    int get_point_count() const;

    // Get interpolated point
    Vector3 get_lerp_point(float t) const;

    // Get edge normal (considering reverse_normal flag)
    Vector3 get_edge_normal(int index) const;

    // Calculate length
    float calc_length() const;

    // Check if same line reference
    bool is_same_line_reference(const Ref<PLATEAURnWay> &other) const;

    // Create reversed way
    Ref<PLATEAURnWay> reversed_way() const;

    // Clone
    Ref<PLATEAURnWay> clone(bool clone_vertex = true) const;

    static Ref<PLATEAURnWay> create(const Ref<PLATEAURnLineString> &line, bool reversed = false, bool reverse_normal = false);

protected:
    static void _bind_methods();

private:
    Ref<PLATEAURnLineString> line_string_;
    bool is_reversed_;
    bool is_reverse_normal_;
};

/**
 * PLATEAURnTrack - Track through intersection (bezier curve)
 */
class PLATEAURnTrack : public RefCounted {
    GDCLASS(PLATEAURnTrack, RefCounted)

public:
    PLATEAURnTrack();
    ~PLATEAURnTrack();

    void set_from_border(const Ref<PLATEAURnWay> &border);
    Ref<PLATEAURnWay> get_from_border() const;

    void set_to_border(const Ref<PLATEAURnWay> &border);
    Ref<PLATEAURnWay> get_to_border() const;

    void set_spline(const Ref<Curve3D> &spline);
    Ref<Curve3D> get_spline() const;

    void set_turn_type(PLATEAURnTurnType type);
    PLATEAURnTurnType get_turn_type() const;

    // Build spline from borders
    void build_spline();

    // Get point on track at t (0.0 - 1.0)
    Vector3 get_point(float t) const;

    // Get tangent at t
    Vector3 get_tangent(float t) const;

protected:
    static void _bind_methods();

private:
    Ref<PLATEAURnWay> from_border_;
    Ref<PLATEAURnWay> to_border_;
    Ref<Curve3D> spline_;
    PLATEAURnTurnType turn_type_;
};

/**
 * PLATEAURnIntersectionEdge - Edge of intersection
 */
class PLATEAURnIntersectionEdge : public RefCounted {
    GDCLASS(PLATEAURnIntersectionEdge, RefCounted)

public:
    PLATEAURnIntersectionEdge();
    ~PLATEAURnIntersectionEdge();

    void set_border(const Ref<PLATEAURnWay> &border);
    Ref<PLATEAURnWay> get_border() const;

    void set_road(const Ref<PLATEAURnRoad> &road);
    Ref<PLATEAURnRoad> get_road() const;

    bool is_border() const;
    bool is_valid() const;
    bool is_median_border() const;

    PLATEAURnFlowType get_flow_type() const;
    Vector3 calc_center() const;

    // Get connected lanes
    TypedArray<PLATEAURnLane> get_connected_lanes() const;

protected:
    static void _bind_methods();

private:
    Ref<PLATEAURnWay> border_;
    Ref<PLATEAURnRoad> road_;
};

/**
 * PLATEAURnLane - Single lane in a road
 */
class PLATEAURnLane : public RefCounted {
    GDCLASS(PLATEAURnLane, RefCounted)

public:
    PLATEAURnLane();
    ~PLATEAURnLane();

    void set_parent_road(const Ref<PLATEAURnRoad> &road);
    Ref<PLATEAURnRoad> get_parent_road() const;

    void set_left_way(const Ref<PLATEAURnWay> &way);
    Ref<PLATEAURnWay> get_left_way() const;

    void set_right_way(const Ref<PLATEAURnWay> &way);
    Ref<PLATEAURnWay> get_right_way() const;

    void set_prev_border(const Ref<PLATEAURnWay> &border);
    Ref<PLATEAURnWay> get_prev_border() const;

    void set_next_border(const Ref<PLATEAURnWay> &border);
    Ref<PLATEAURnWay> get_next_border() const;

    void set_is_reversed(bool reversed);
    bool get_is_reversed() const;

    void set_attributes(int64_t attrs);
    int64_t get_attributes() const;

    // Validation
    bool is_valid_way() const;
    bool has_both_border() const;
    bool is_both_connected_lane() const;
    bool is_empty_lane() const;
    bool is_median_lane() const;

    // Width calculation
    float calc_width() const;
    float calc_min_width() const;

    // Create center way
    Ref<PLATEAURnWay> create_center_way() const;

    // Get connected lanes
    TypedArray<PLATEAURnLane> get_prev_lanes() const;
    TypedArray<PLATEAURnLane> get_next_lanes() const;

    // Reverse direction
    void reverse();

    // Split into multiple lanes
    TypedArray<PLATEAURnLane> split(int split_num) const;

protected:
    static void _bind_methods();

private:
    Ref<PLATEAURnRoad> parent_road_;
    Ref<PLATEAURnWay> left_way_;
    Ref<PLATEAURnWay> right_way_;
    Ref<PLATEAURnWay> prev_border_;
    Ref<PLATEAURnWay> next_border_;
    bool is_reversed_;
    int64_t attributes_;
};

/**
 * PLATEAURnSideWalk - Sidewalk adjacent to road
 */
class PLATEAURnSideWalk : public RefCounted {
    GDCLASS(PLATEAURnSideWalk, RefCounted)

public:
    PLATEAURnSideWalk();
    ~PLATEAURnSideWalk();

    void set_outside_way(const Ref<PLATEAURnWay> &way);
    Ref<PLATEAURnWay> get_outside_way() const;

    void set_inside_way(const Ref<PLATEAURnWay> &way);
    Ref<PLATEAURnWay> get_inside_way() const;

    void set_start_edge_way(const Ref<PLATEAURnWay> &way);
    Ref<PLATEAURnWay> get_start_edge_way() const;

    void set_end_edge_way(const Ref<PLATEAURnWay> &way);
    Ref<PLATEAURnWay> get_end_edge_way() const;

    void set_lane_type(PLATEAURnSideWalkLaneType type);
    PLATEAURnSideWalkLaneType get_lane_type() const;

    bool is_valid() const;
    bool is_all_way_valid() const;

    // Align way directions to edge ways
    void align();

    // Reverse start/end
    void reverse();

    // Try merge with neighbor sidewalk
    bool try_merge_neighbor(const Ref<PLATEAURnSideWalk> &other);

protected:
    static void _bind_methods();

private:
    Ref<PLATEAURnWay> outside_way_;
    Ref<PLATEAURnWay> inside_way_;
    Ref<PLATEAURnWay> start_edge_way_;
    Ref<PLATEAURnWay> end_edge_way_;
    PLATEAURnSideWalkLaneType lane_type_;
};

/**
 * PLATEAURnRoadBase - Base class for road and intersection
 */
class PLATEAURnRoadBase : public RefCounted {
    GDCLASS(PLATEAURnRoadBase, RefCounted)

public:
    PLATEAURnRoadBase();
    virtual ~PLATEAURnRoadBase();

    // Get unique ID
    void set_id(int64_t id);
    int64_t get_id() const;

    // Check if this is a road or intersection
    virtual bool is_road() const { return false; }
    virtual bool is_intersection() const { return false; }

protected:
    static void _bind_methods();

private:
    int64_t id_;
};

/**
 * PLATEAURnRoad - Road segment with lanes
 */
class PLATEAURnRoad : public PLATEAURnRoadBase {
    GDCLASS(PLATEAURnRoad, PLATEAURnRoadBase)

public:
    PLATEAURnRoad();
    ~PLATEAURnRoad();

    bool is_road() const override { return true; }

    // Previous/Next connections
    void set_prev(const Ref<PLATEAURnRoadBase> &prev);
    Ref<PLATEAURnRoadBase> get_prev() const;

    void set_next(const Ref<PLATEAURnRoadBase> &next);
    Ref<PLATEAURnRoadBase> get_next() const;

    // Main lanes
    void add_main_lane(const Ref<PLATEAURnLane> &lane);
    void remove_main_lane(const Ref<PLATEAURnLane> &lane);
    TypedArray<PLATEAURnLane> get_main_lanes() const;
    int get_main_lane_count() const;

    // Median lane
    void set_median_lane(const Ref<PLATEAURnLane> &lane);
    Ref<PLATEAURnLane> get_median_lane() const;
    float get_median_width() const;

    // Sidewalks
    void add_sidewalk(const Ref<PLATEAURnSideWalk> &sidewalk);
    TypedArray<PLATEAURnSideWalk> get_sidewalks() const;

    // Lane queries
    TypedArray<PLATEAURnLane> get_left_lanes() const;
    TypedArray<PLATEAURnLane> get_right_lanes() const;
    int get_left_lane_count() const;
    int get_right_lane_count() const;

    // Validation
    bool is_valid() const;
    bool is_all_both_connected_lane() const;
    bool is_all_lane_valid() const;
    bool has_both_lane() const;
    bool is_empty_road() const;

    // Merged border ways
    Ref<PLATEAURnWay> get_merged_side_way(bool left) const;

    // Reverse prev/next and all lanes
    void reverse();

    // Align lane border directions
    void align_lane_borders();

protected:
    static void _bind_methods();

private:
    Ref<PLATEAURnRoadBase> prev_;
    Ref<PLATEAURnRoadBase> next_;
    TypedArray<PLATEAURnLane> main_lanes_;
    Ref<PLATEAURnLane> median_lane_;
    TypedArray<PLATEAURnSideWalk> sidewalks_;
};

/**
 * PLATEAURnIntersection - Intersection node
 */
class PLATEAURnIntersection : public PLATEAURnRoadBase {
    GDCLASS(PLATEAURnIntersection, PLATEAURnRoadBase)

public:
    PLATEAURnIntersection();
    ~PLATEAURnIntersection();

    bool is_intersection() const override { return true; }

    // Edges
    void add_edge(const Ref<PLATEAURnIntersectionEdge> &edge);
    void remove_edge(const Ref<PLATEAURnIntersectionEdge> &edge);
    TypedArray<PLATEAURnIntersectionEdge> get_edges() const;
    TypedArray<PLATEAURnIntersectionEdge> get_borders() const;

    // Tracks
    void add_track(const Ref<PLATEAURnTrack> &track);
    void remove_track(const Ref<PLATEAURnTrack> &track);
    TypedArray<PLATEAURnTrack> get_tracks() const;
    Ref<PLATEAURnTrack> find_track(const Ref<PLATEAURnWay> &from, const Ref<PLATEAURnWay> &to) const;

    // Build all tracks automatically
    void build_tracks();

    // Validation
    bool is_empty_intersection() const;

    // Align edges clockwise
    void align();

    // 2D inside check (XZ plane)
    bool is_inside_2d(const Vector3 &pos) const;

    // Get center point
    Vector3 get_center() const;

protected:
    static void _bind_methods();

private:
    TypedArray<PLATEAURnIntersectionEdge> edges_;
    TypedArray<PLATEAURnTrack> tracks_;
};

/**
 * PLATEAURnModel - Root container for road network
 *
 * Usage:
 * ```gdscript
 * var model = PLATEAURnModel.new()
 *
 * # Add roads and intersections
 * var road = PLATEAURnRoad.new()
 * model.add_road(road)
 *
 * var intersection = PLATEAURnIntersection.new()
 * model.add_intersection(intersection)
 *
 * # Build from mesh data
 * var model2 = PLATEAURnModel.create_from_mesh_data(road_mesh_data_array)
 *
 * # Generate mesh
 * var mesh = model.generate_mesh()
 * ```
 */
class PLATEAURnModel : public RefCounted {
    GDCLASS(PLATEAURnModel, RefCounted)

public:
    PLATEAURnModel();
    ~PLATEAURnModel();

    // Roads
    void add_road(const Ref<PLATEAURnRoad> &road);
    void remove_road(const Ref<PLATEAURnRoad> &road);
    TypedArray<PLATEAURnRoad> get_roads() const;
    int get_road_count() const;

    // Intersections
    void add_intersection(const Ref<PLATEAURnIntersection> &intersection);
    void remove_intersection(const Ref<PLATEAURnIntersection> &intersection);
    TypedArray<PLATEAURnIntersection> get_intersections() const;
    int get_intersection_count() const;

    // Sidewalks (standalone, not attached to roads)
    void add_sidewalk(const Ref<PLATEAURnSideWalk> &sidewalk);
    TypedArray<PLATEAURnSideWalk> get_sidewalks() const;

    // Factory version
    void set_factory_version(const String &version);
    String get_factory_version() const;

    // Collect all elements
    TypedArray<PLATEAURnLane> collect_all_lanes() const;
    TypedArray<PLATEAURnWay> collect_all_ways() const;
    TypedArray<PLATEAURnIntersectionEdge> collect_all_edges() const;

    // Conversion
    Ref<PLATEAURnIntersection> convert_road_to_intersection(const Ref<PLATEAURnRoad> &road, bool build_tracks = true);
    Ref<PLATEAURnRoad> convert_intersection_to_road(const Ref<PLATEAURnIntersection> &intersection);

    // Merge road groups
    void merge_road_groups();

    // Split lanes by width
    void split_lanes_by_width(float width, bool rebuild_tracks = true);

    // Validation
    bool check() const;

    // Create from PLATEAUMeshData array (road package)
    static Ref<PLATEAURnModel> create_from_mesh_data(const TypedArray<class PLATEAUMeshData> &mesh_data_array);

    // Generate mesh representation
    Ref<ArrayMesh> generate_mesh() const;

    // Serialize to dictionary
    Dictionary serialize() const;

    // Deserialize from dictionary
    static Ref<PLATEAURnModel> deserialize(const Dictionary &data);

protected:
    static void _bind_methods();

private:
    TypedArray<PLATEAURnRoad> roads_;
    TypedArray<PLATEAURnIntersection> intersections_;
    TypedArray<PLATEAURnSideWalk> sidewalks_;
    String factory_version_;
    int64_t next_id_;

    int64_t allocate_id();
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PLATEAURnTurnType);
VARIANT_ENUM_CAST(godot::PLATEAURnFlowType);
VARIANT_ENUM_CAST(godot::PLATEAURnLaneAttribute);
VARIANT_ENUM_CAST(godot::PLATEAURnSideWalkLaneType);
