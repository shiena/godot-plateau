#include "plateau_city_object_type.h"

namespace godot {

// ============================================================================
// PLATEAUCityObjectTypeNode
// ============================================================================

PLATEAUCityObjectTypeNode::PLATEAUCityObjectTypeNode() :
    type_(0),
    package_(0) {
}

PLATEAUCityObjectTypeNode::~PLATEAUCityObjectTypeNode() {
}

void PLATEAUCityObjectTypeNode::set_name(const String &name) {
    name_ = name;
}

String PLATEAUCityObjectTypeNode::get_name() const {
    return name_;
}

void PLATEAUCityObjectTypeNode::set_display_name(const String &name) {
    display_name_ = name;
}

String PLATEAUCityObjectTypeNode::get_display_name() const {
    return display_name_.is_empty() ? name_ : display_name_;
}

void PLATEAUCityObjectTypeNode::set_type(int64_t type) {
    type_ = type;
}

int64_t PLATEAUCityObjectTypeNode::get_type() const {
    return type_;
}

void PLATEAUCityObjectTypeNode::set_package(int64_t package) {
    package_ = package;
}

int64_t PLATEAUCityObjectTypeNode::get_package() const {
    return package_;
}

void PLATEAUCityObjectTypeNode::set_parent(const Ref<PLATEAUCityObjectTypeNode> &parent) {
    parent_ = parent;
}

Ref<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeNode::get_parent() const {
    return parent_;
}

void PLATEAUCityObjectTypeNode::add_child(const Ref<PLATEAUCityObjectTypeNode> &child) {
    if (child.is_valid()) {
        children_.append(child);
        child->set_parent(Ref<PLATEAUCityObjectTypeNode>(this));
    }
}

TypedArray<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeNode::get_children() const {
    return children_;
}

int PLATEAUCityObjectTypeNode::get_child_count() const {
    return children_.size();
}

Ref<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeNode::get_child(int index) const {
    if (index < 0 || index >= children_.size()) {
        return Ref<PLATEAUCityObjectTypeNode>();
    }
    return children_[index];
}

bool PLATEAUCityObjectTypeNode::is_leaf() const {
    return type_ != 0;
}

bool PLATEAUCityObjectTypeNode::contains_type(int64_t type) const {
    if (type_ == type) return true;

    for (int i = 0; i < children_.size(); i++) {
        Ref<PLATEAUCityObjectTypeNode> child = children_[i];
        if (child.is_valid() && child->contains_type(type)) {
            return true;
        }
    }

    return false;
}

PackedInt64Array PLATEAUCityObjectTypeNode::get_all_types() const {
    PackedInt64Array result;
    collect_types(result);
    return result;
}

void PLATEAUCityObjectTypeNode::collect_types(PackedInt64Array &result) const {
    if (type_ != 0) {
        result.append(type_);
    }

    for (int i = 0; i < children_.size(); i++) {
        Ref<PLATEAUCityObjectTypeNode> child = children_[i];
        if (child.is_valid()) {
            child->collect_types(result);
        }
    }
}

void PLATEAUCityObjectTypeNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_name", "name"), &PLATEAUCityObjectTypeNode::set_name);
    ClassDB::bind_method(D_METHOD("get_name"), &PLATEAUCityObjectTypeNode::get_name);
    ClassDB::bind_method(D_METHOD("set_display_name", "name"), &PLATEAUCityObjectTypeNode::set_display_name);
    ClassDB::bind_method(D_METHOD("get_display_name"), &PLATEAUCityObjectTypeNode::get_display_name);
    ClassDB::bind_method(D_METHOD("set_type", "type"), &PLATEAUCityObjectTypeNode::set_type);
    ClassDB::bind_method(D_METHOD("get_type"), &PLATEAUCityObjectTypeNode::get_type);
    ClassDB::bind_method(D_METHOD("set_package", "package"), &PLATEAUCityObjectTypeNode::set_package);
    ClassDB::bind_method(D_METHOD("get_package"), &PLATEAUCityObjectTypeNode::get_package);
    ClassDB::bind_method(D_METHOD("get_parent"), &PLATEAUCityObjectTypeNode::get_parent);
    ClassDB::bind_method(D_METHOD("add_child", "child"), &PLATEAUCityObjectTypeNode::add_child);
    ClassDB::bind_method(D_METHOD("get_children"), &PLATEAUCityObjectTypeNode::get_children);
    ClassDB::bind_method(D_METHOD("get_child_count"), &PLATEAUCityObjectTypeNode::get_child_count);
    ClassDB::bind_method(D_METHOD("get_child", "index"), &PLATEAUCityObjectTypeNode::get_child);
    ClassDB::bind_method(D_METHOD("is_leaf"), &PLATEAUCityObjectTypeNode::is_leaf);
    ClassDB::bind_method(D_METHOD("contains_type", "type"), &PLATEAUCityObjectTypeNode::contains_type);
    ClassDB::bind_method(D_METHOD("get_all_types"), &PLATEAUCityObjectTypeNode::get_all_types);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type"), "set_type", "get_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "package"), "set_package", "get_package");
}

// ============================================================================
// PLATEAUCityObjectTypeHierarchy
// ============================================================================

PLATEAUCityObjectTypeHierarchy::PLATEAUCityObjectTypeHierarchy() {
    build_hierarchy();
}

PLATEAUCityObjectTypeHierarchy::~PLATEAUCityObjectTypeHierarchy() {
}

Ref<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeHierarchy::get_root() const {
    return root_;
}

Ref<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeHierarchy::get_node_by_type(int64_t type) const {
    if (type_to_node_.has(type)) {
        return type_to_node_[type];
    }
    return Ref<PLATEAUCityObjectTypeNode>();
}

Ref<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeHierarchy::get_node_by_package(int64_t package) const {
    if (package_to_node_.has(package)) {
        return package_to_node_[package];
    }
    return Ref<PLATEAUCityObjectTypeNode>();
}

TypedArray<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeHierarchy::get_all_types() const {
    TypedArray<PLATEAUCityObjectTypeNode> leaves;
    TypedArray<PLATEAUCityObjectTypeNode> categories;
    collect_nodes(root_, leaves, categories);
    return leaves;
}

TypedArray<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeHierarchy::get_all_categories() const {
    TypedArray<PLATEAUCityObjectTypeNode> leaves;
    TypedArray<PLATEAUCityObjectTypeNode> categories;
    collect_nodes(root_, leaves, categories);
    return categories;
}

int64_t PLATEAUCityObjectTypeHierarchy::type_to_package(int64_t type) {
    // Building types
    if (type & (COT_Building | COT_BuildingPart | COT_BuildingInstallation |
                COT_BuildingFurniture | COT_Room | COT_Door | COT_Window |
                COT_RoofSurface | COT_WallSurface | COT_GroundSurface |
                COT_ClosureSurface | COT_FloorSurface | COT_InteriorWallSurface |
                COT_CeilingSurface | COT_OuterCeilingSurface | COT_OuterFloorSurface |
                COT_IntBuildingInstallation)) {
        return PACKAGE_BUILDING;
    }

    // Transportation types
    if (type & (COT_Road | COT_TransportationObject)) {
        return PACKAGE_ROAD;
    }

    if (type & COT_Railway) {
        return PACKAGE_RAILWAY;
    }

    if (type & COT_Track) {
        return PACKAGE_TRACK;
    }

    if (type & COT_Square) {
        return PACKAGE_SQUARE;
    }

    // Relief types
    if (type & (COT_ReliefFeature | COT_ReliefComponent | COT_TINRelief |
                COT_MassPointRelief | COT_BreaklineRelief | COT_RasterRelief)) {
        return PACKAGE_RELIEF;
    }

    // Vegetation types
    if (type & (COT_PlantCover | COT_SolitaryVegetationObject)) {
        return PACKAGE_VEGETATION;
    }

    // Water types
    if (type & (COT_WaterBody | COT_WaterSurface)) {
        return PACKAGE_WATER_BODY;
    }

    // Other types
    if (type & COT_CityFurniture) {
        return PACKAGE_CITY_FURNITURE;
    }

    if (type & COT_LandUse) {
        return PACKAGE_LAND_USE;
    }

    if (type & (COT_Bridge | COT_BridgePart | COT_BridgeConstructionElement | COT_BridgeInstallation)) {
        return PACKAGE_BRIDGE;
    }

    if (type & COT_Tunnel) {
        return PACKAGE_TUNNEL;
    }

    if (type & COT_GenericCityObject) {
        return PACKAGE_GENERIC;
    }

    if (type & COT_CityObjectGroup) {
        return PACKAGE_AREA;
    }

    return PACKAGE_UNKNOWN;
}

String PLATEAUCityObjectTypeHierarchy::get_type_name(int64_t type) {
    switch (type) {
        case COT_GenericCityObject: return "GenericCityObject";
        case COT_Building: return "Building";
        case COT_Room: return "Room";
        case COT_BuildingInstallation: return "BuildingInstallation";
        case COT_BuildingFurniture: return "BuildingFurniture";
        case COT_Door: return "Door";
        case COT_Window: return "Window";
        case COT_CityFurniture: return "CityFurniture";
        case COT_Track: return "Track";
        case COT_Road: return "Road";
        case COT_Railway: return "Railway";
        case COT_Square: return "Square";
        case COT_PlantCover: return "PlantCover";
        case COT_SolitaryVegetationObject: return "SolitaryVegetationObject";
        case COT_WaterBody: return "WaterBody";
        case COT_ReliefFeature: return "ReliefFeature";
        case COT_LandUse: return "LandUse";
        case COT_Tunnel: return "Tunnel";
        case COT_Bridge: return "Bridge";
        case COT_BridgeConstructionElement: return "BridgeConstructionElement";
        case COT_BridgeInstallation: return "BridgeInstallation";
        case COT_BridgePart: return "BridgePart";
        case COT_BuildingPart: return "BuildingPart";
        case COT_WallSurface: return "WallSurface";
        case COT_RoofSurface: return "RoofSurface";
        case COT_GroundSurface: return "GroundSurface";
        case COT_ClosureSurface: return "ClosureSurface";
        case COT_FloorSurface: return "FloorSurface";
        case COT_InteriorWallSurface: return "InteriorWallSurface";
        case COT_CeilingSurface: return "CeilingSurface";
        case COT_CityObjectGroup: return "CityObjectGroup";
        case COT_OuterCeilingSurface: return "OuterCeilingSurface";
        case COT_OuterFloorSurface: return "OuterFloorSurface";
        case COT_TransportationObject: return "TransportationObject";
        case COT_IntBuildingInstallation: return "IntBuildingInstallation";
        case COT_WaterSurface: return "WaterSurface";
        case COT_ReliefComponent: return "ReliefComponent";
        case COT_TINRelief: return "TINRelief";
        case COT_MassPointRelief: return "MassPointRelief";
        case COT_BreaklineRelief: return "BreaklineRelief";
        case COT_RasterRelief: return "RasterRelief";
        case COT_Unknown: return "Unknown";
        default: return "Unknown";
    }
}

String PLATEAUCityObjectTypeHierarchy::get_type_display_name(int64_t type) {
    switch (type) {
        case COT_GenericCityObject: return String::utf8("汎用都市オブジェクト");
        case COT_Building: return String::utf8("建築物");
        case COT_Room: return String::utf8("部屋");
        case COT_BuildingInstallation: return String::utf8("建築物付属設備");
        case COT_BuildingFurniture: return String::utf8("建築物内家具");
        case COT_Door: return String::utf8("ドア");
        case COT_Window: return String::utf8("窓");
        case COT_CityFurniture: return String::utf8("都市設備");
        case COT_Track: return String::utf8("徒歩道");
        case COT_Road: return String::utf8("道路");
        case COT_Railway: return String::utf8("鉄道");
        case COT_Square: return String::utf8("広場");
        case COT_PlantCover: return String::utf8("植被");
        case COT_SolitaryVegetationObject: return String::utf8("単独植生");
        case COT_WaterBody: return String::utf8("水部");
        case COT_ReliefFeature: return String::utf8("起伏");
        case COT_LandUse: return String::utf8("土地利用");
        case COT_Tunnel: return String::utf8("トンネル");
        case COT_Bridge: return String::utf8("橋梁");
        case COT_BridgeConstructionElement: return String::utf8("橋梁構造要素");
        case COT_BridgeInstallation: return String::utf8("橋梁付属物");
        case COT_BridgePart: return String::utf8("橋梁部分");
        case COT_BuildingPart: return String::utf8("建築物部分");
        case COT_WallSurface: return String::utf8("壁面");
        case COT_RoofSurface: return String::utf8("屋根面");
        case COT_GroundSurface: return String::utf8("底面");
        case COT_ClosureSurface: return String::utf8("閉鎖面");
        case COT_FloorSurface: return String::utf8("床面");
        case COT_InteriorWallSurface: return String::utf8("内壁面");
        case COT_CeilingSurface: return String::utf8("天井面");
        case COT_CityObjectGroup: return String::utf8("都市オブジェクトグループ");
        case COT_OuterCeilingSurface: return String::utf8("外部天井面");
        case COT_OuterFloorSurface: return String::utf8("外部床面");
        case COT_TransportationObject: return String::utf8("交通オブジェクト");
        case COT_IntBuildingInstallation: return String::utf8("屋内建築物付属設備");
        case COT_WaterSurface: return String::utf8("水面");
        case COT_ReliefComponent: return String::utf8("起伏構成要素");
        case COT_TINRelief: return String::utf8("TIN起伏");
        case COT_MassPointRelief: return String::utf8("点群起伏");
        case COT_BreaklineRelief: return String::utf8("ブレークライン起伏");
        case COT_RasterRelief: return String::utf8("ラスタ起伏");
        case COT_Unknown: return String::utf8("不明");
        default: return String::utf8("不明");
    }
}

String PLATEAUCityObjectTypeHierarchy::get_package_name(int64_t package) {
    switch (package) {
        case PACKAGE_BUILDING: return "bldg";
        case PACKAGE_ROAD: return "tran";
        case PACKAGE_URBAN_PLANNING: return "urf";
        case PACKAGE_LAND_USE: return "luse";
        case PACKAGE_CITY_FURNITURE: return "frn";
        case PACKAGE_VEGETATION: return "veg";
        case PACKAGE_RELIEF: return "dem";
        case PACKAGE_FLOOD: return "fld";
        case PACKAGE_TSUNAMI: return "tnm";
        case PACKAGE_LANDSLIDE: return "lsld";
        case PACKAGE_STORM_SURGE: return "htd";
        case PACKAGE_INLAND_FLOOD: return "ifld";
        case PACKAGE_RAILWAY: return "rwy";
        case PACKAGE_WATERWAY: return "wwy";
        case PACKAGE_WATER_BODY: return "wtr";
        case PACKAGE_BRIDGE: return "brid";
        case PACKAGE_TRACK: return "trk";
        case PACKAGE_SQUARE: return "squr";
        case PACKAGE_TUNNEL: return "tun";
        case PACKAGE_UNDERGROUND_FACILITY: return "unf";
        case PACKAGE_UNDERGROUND_BUILDING: return "ubld";
        case PACKAGE_AREA: return "area";
        case PACKAGE_OTHER_CONSTRUCTION: return "cons";
        case PACKAGE_GENERIC: return "gen";
        default: return "unknown";
    }
}

String PLATEAUCityObjectTypeHierarchy::get_package_display_name(int64_t package) {
    switch (package) {
        case PACKAGE_BUILDING: return String::utf8("建築物");
        case PACKAGE_ROAD: return String::utf8("道路");
        case PACKAGE_URBAN_PLANNING: return String::utf8("都市計画決定情報");
        case PACKAGE_LAND_USE: return String::utf8("土地利用");
        case PACKAGE_CITY_FURNITURE: return String::utf8("都市設備");
        case PACKAGE_VEGETATION: return String::utf8("植生");
        case PACKAGE_RELIEF: return String::utf8("起伏");
        case PACKAGE_FLOOD: return String::utf8("洪水浸水想定区域");
        case PACKAGE_TSUNAMI: return String::utf8("津波浸水想定");
        case PACKAGE_LANDSLIDE: return String::utf8("土砂災害警戒区域");
        case PACKAGE_STORM_SURGE: return String::utf8("高潮浸水想定区域");
        case PACKAGE_INLAND_FLOOD: return String::utf8("内水浸水想定区域");
        case PACKAGE_RAILWAY: return String::utf8("鉄道");
        case PACKAGE_WATERWAY: return String::utf8("航路");
        case PACKAGE_WATER_BODY: return String::utf8("水部");
        case PACKAGE_BRIDGE: return String::utf8("橋梁");
        case PACKAGE_TRACK: return String::utf8("徒歩道");
        case PACKAGE_SQUARE: return String::utf8("広場");
        case PACKAGE_TUNNEL: return String::utf8("トンネル");
        case PACKAGE_UNDERGROUND_FACILITY: return String::utf8("地下埋設物");
        case PACKAGE_UNDERGROUND_BUILDING: return String::utf8("地下街");
        case PACKAGE_AREA: return String::utf8("区域");
        case PACKAGE_OTHER_CONSTRUCTION: return String::utf8("その他の構造物");
        case PACKAGE_GENERIC: return String::utf8("汎用都市オブジェクト");
        default: return String::utf8("不明");
    }
}

bool PLATEAUCityObjectTypeHierarchy::type_belongs_to_package(int64_t type, int64_t package) {
    return type_to_package(type) == package;
}

int64_t PLATEAUCityObjectTypeHierarchy::get_types_for_package(int64_t package) {
    int64_t result = 0;

    switch (package) {
        case PACKAGE_BUILDING:
            result = COT_Building | COT_BuildingPart | COT_BuildingInstallation |
                     COT_BuildingFurniture | COT_Room | COT_Door | COT_Window |
                     COT_RoofSurface | COT_WallSurface | COT_GroundSurface |
                     COT_ClosureSurface | COT_FloorSurface | COT_InteriorWallSurface |
                     COT_CeilingSurface | COT_OuterCeilingSurface | COT_OuterFloorSurface |
                     COT_IntBuildingInstallation;
            break;
        case PACKAGE_ROAD:
            result = COT_Road | COT_TransportationObject;
            break;
        case PACKAGE_RAILWAY:
            result = COT_Railway;
            break;
        case PACKAGE_TRACK:
            result = COT_Track;
            break;
        case PACKAGE_SQUARE:
            result = COT_Square;
            break;
        case PACKAGE_RELIEF:
            result = COT_ReliefFeature | COT_ReliefComponent | COT_TINRelief |
                     COT_MassPointRelief | COT_BreaklineRelief | COT_RasterRelief;
            break;
        case PACKAGE_VEGETATION:
            result = COT_PlantCover | COT_SolitaryVegetationObject;
            break;
        case PACKAGE_WATER_BODY:
            result = COT_WaterBody | COT_WaterSurface;
            break;
        case PACKAGE_CITY_FURNITURE:
            result = COT_CityFurniture;
            break;
        case PACKAGE_LAND_USE:
            result = COT_LandUse;
            break;
        case PACKAGE_BRIDGE:
            result = COT_Bridge | COT_BridgePart | COT_BridgeConstructionElement | COT_BridgeInstallation;
            break;
        case PACKAGE_TUNNEL:
            result = COT_Tunnel;
            break;
        case PACKAGE_GENERIC:
            result = COT_GenericCityObject;
            break;
        case PACKAGE_AREA:
            result = COT_CityObjectGroup;
            break;
        default:
            result = 0;
            break;
    }

    return result;
}

Ref<PLATEAUCityObjectTypeNode> PLATEAUCityObjectTypeHierarchy::create_node(
    const String &name, const String &display_name, int64_t type, int64_t package) {
    Ref<PLATEAUCityObjectTypeNode> node;
    node.instantiate();
    node->set_name(name);
    node->set_display_name(display_name);
    node->set_type(type);
    node->set_package(package);
    return node;
}

void PLATEAUCityObjectTypeHierarchy::add_to_lookup(const Ref<PLATEAUCityObjectTypeNode> &node) {
    if (node->get_type() != 0) {
        type_to_node_[node->get_type()] = node;
    }
    if (node->get_package() != 0 && !package_to_node_.has(node->get_package())) {
        package_to_node_[node->get_package()] = node;
    }
}

void PLATEAUCityObjectTypeHierarchy::collect_nodes(
    const Ref<PLATEAUCityObjectTypeNode> &node,
    TypedArray<PLATEAUCityObjectTypeNode> &leaves,
    TypedArray<PLATEAUCityObjectTypeNode> &categories) const {

    if (node.is_null()) return;

    if (node->is_leaf()) {
        leaves.append(node);
    } else if (node != root_) {
        categories.append(node);
    }

    for (int i = 0; i < node->get_child_count(); i++) {
        collect_nodes(node->get_child(i), leaves, categories);
    }
}

void PLATEAUCityObjectTypeHierarchy::build_hierarchy() {
    // Create root
    root_ = create_node("Root", String::utf8("ルート"), 0, 0);

    // Building category
    auto building = create_node("Building", String::utf8("建築物"), 0, PACKAGE_BUILDING);
    building->add_child(create_node("Building", String::utf8("建築物"), COT_Building, PACKAGE_BUILDING));
    building->add_child(create_node("BuildingPart", String::utf8("建築物部分"), COT_BuildingPart, PACKAGE_BUILDING));
    building->add_child(create_node("BuildingInstallation", String::utf8("建築物付属設備"), COT_BuildingInstallation, PACKAGE_BUILDING));
    building->add_child(create_node("RoofSurface", String::utf8("屋根面"), COT_RoofSurface, PACKAGE_BUILDING));
    building->add_child(create_node("WallSurface", String::utf8("壁面"), COT_WallSurface, PACKAGE_BUILDING));
    building->add_child(create_node("GroundSurface", String::utf8("底面"), COT_GroundSurface, PACKAGE_BUILDING));
    building->add_child(create_node("ClosureSurface", String::utf8("閉鎖面"), COT_ClosureSurface, PACKAGE_BUILDING));
    building->add_child(create_node("FloorSurface", String::utf8("床面"), COT_FloorSurface, PACKAGE_BUILDING));
    building->add_child(create_node("InteriorWallSurface", String::utf8("内壁面"), COT_InteriorWallSurface, PACKAGE_BUILDING));
    building->add_child(create_node("CeilingSurface", String::utf8("天井面"), COT_CeilingSurface, PACKAGE_BUILDING));
    building->add_child(create_node("Door", String::utf8("ドア"), COT_Door, PACKAGE_BUILDING));
    building->add_child(create_node("Window", String::utf8("窓"), COT_Window, PACKAGE_BUILDING));
    building->add_child(create_node("Room", String::utf8("部屋"), COT_Room, PACKAGE_BUILDING));
    root_->add_child(building);

    // Add building nodes to lookup
    add_to_lookup(building);
    for (int i = 0; i < building->get_child_count(); i++) {
        add_to_lookup(building->get_child(i));
    }

    // Transportation category
    auto transportation = create_node("Transportation", String::utf8("交通"), 0, PACKAGE_ROAD);
    transportation->add_child(create_node("Road", String::utf8("道路"), COT_Road, PACKAGE_ROAD));
    transportation->add_child(create_node("Railway", String::utf8("鉄道"), COT_Railway, PACKAGE_RAILWAY));
    transportation->add_child(create_node("Track", String::utf8("徒歩道"), COT_Track, PACKAGE_TRACK));
    transportation->add_child(create_node("Square", String::utf8("広場"), COT_Square, PACKAGE_SQUARE));
    transportation->add_child(create_node("TransportationObject", String::utf8("交通オブジェクト"), COT_TransportationObject, PACKAGE_ROAD));
    root_->add_child(transportation);

    for (int i = 0; i < transportation->get_child_count(); i++) {
        add_to_lookup(transportation->get_child(i));
    }

    // Relief category
    auto relief = create_node("Relief", String::utf8("起伏"), 0, PACKAGE_RELIEF);
    relief->add_child(create_node("ReliefFeature", String::utf8("起伏"), COT_ReliefFeature, PACKAGE_RELIEF));
    relief->add_child(create_node("TINRelief", String::utf8("TIN起伏"), COT_TINRelief, PACKAGE_RELIEF));
    relief->add_child(create_node("MassPointRelief", String::utf8("点群起伏"), COT_MassPointRelief, PACKAGE_RELIEF));
    relief->add_child(create_node("BreaklineRelief", String::utf8("ブレークライン起伏"), COT_BreaklineRelief, PACKAGE_RELIEF));
    relief->add_child(create_node("RasterRelief", String::utf8("ラスタ起伏"), COT_RasterRelief, PACKAGE_RELIEF));
    root_->add_child(relief);

    add_to_lookup(relief);
    for (int i = 0; i < relief->get_child_count(); i++) {
        add_to_lookup(relief->get_child(i));
    }

    // Vegetation category
    auto vegetation = create_node("Vegetation", String::utf8("植生"), 0, PACKAGE_VEGETATION);
    vegetation->add_child(create_node("PlantCover", String::utf8("植被"), COT_PlantCover, PACKAGE_VEGETATION));
    vegetation->add_child(create_node("SolitaryVegetationObject", String::utf8("単独植生"), COT_SolitaryVegetationObject, PACKAGE_VEGETATION));
    root_->add_child(vegetation);

    add_to_lookup(vegetation);
    for (int i = 0; i < vegetation->get_child_count(); i++) {
        add_to_lookup(vegetation->get_child(i));
    }

    // Water body
    auto water = create_node("WaterBody", String::utf8("水部"), COT_WaterBody, PACKAGE_WATER_BODY);
    water->add_child(create_node("WaterSurface", String::utf8("水面"), COT_WaterSurface, PACKAGE_WATER_BODY));
    root_->add_child(water);
    add_to_lookup(water);

    // City furniture
    auto furniture = create_node("CityFurniture", String::utf8("都市設備"), COT_CityFurniture, PACKAGE_CITY_FURNITURE);
    root_->add_child(furniture);
    add_to_lookup(furniture);

    // Land use
    auto landuse = create_node("LandUse", String::utf8("土地利用"), COT_LandUse, PACKAGE_LAND_USE);
    root_->add_child(landuse);
    add_to_lookup(landuse);

    // Bridge
    auto bridge = create_node("Bridge", String::utf8("橋梁"), 0, PACKAGE_BRIDGE);
    bridge->add_child(create_node("Bridge", String::utf8("橋梁"), COT_Bridge, PACKAGE_BRIDGE));
    bridge->add_child(create_node("BridgePart", String::utf8("橋梁部分"), COT_BridgePart, PACKAGE_BRIDGE));
    bridge->add_child(create_node("BridgeConstructionElement", String::utf8("橋梁構造要素"), COT_BridgeConstructionElement, PACKAGE_BRIDGE));
    bridge->add_child(create_node("BridgeInstallation", String::utf8("橋梁付属物"), COT_BridgeInstallation, PACKAGE_BRIDGE));
    root_->add_child(bridge);

    add_to_lookup(bridge);
    for (int i = 0; i < bridge->get_child_count(); i++) {
        add_to_lookup(bridge->get_child(i));
    }

    // Tunnel
    auto tunnel = create_node("Tunnel", String::utf8("トンネル"), COT_Tunnel, PACKAGE_TUNNEL);
    root_->add_child(tunnel);
    add_to_lookup(tunnel);

    // Generic
    auto generic = create_node("GenericCityObject", String::utf8("汎用都市オブジェクト"), COT_GenericCityObject, PACKAGE_GENERIC);
    root_->add_child(generic);
    add_to_lookup(generic);

    // City object group
    auto group = create_node("CityObjectGroup", String::utf8("都市オブジェクトグループ"), COT_CityObjectGroup, PACKAGE_AREA);
    root_->add_child(group);
    add_to_lookup(group);
}

void PLATEAUCityObjectTypeHierarchy::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_root"), &PLATEAUCityObjectTypeHierarchy::get_root);
    ClassDB::bind_method(D_METHOD("get_node_by_type", "type"), &PLATEAUCityObjectTypeHierarchy::get_node_by_type);
    ClassDB::bind_method(D_METHOD("get_node_by_package", "package"), &PLATEAUCityObjectTypeHierarchy::get_node_by_package);
    ClassDB::bind_method(D_METHOD("get_all_types"), &PLATEAUCityObjectTypeHierarchy::get_all_types);
    ClassDB::bind_method(D_METHOD("get_all_categories"), &PLATEAUCityObjectTypeHierarchy::get_all_categories);

    ClassDB::bind_static_method("PLATEAUCityObjectTypeHierarchy", D_METHOD("type_to_package", "type"), &PLATEAUCityObjectTypeHierarchy::type_to_package);
    ClassDB::bind_static_method("PLATEAUCityObjectTypeHierarchy", D_METHOD("get_type_name", "type"), &PLATEAUCityObjectTypeHierarchy::get_type_name);
    ClassDB::bind_static_method("PLATEAUCityObjectTypeHierarchy", D_METHOD("get_type_display_name", "type"), &PLATEAUCityObjectTypeHierarchy::get_type_display_name);
    ClassDB::bind_static_method("PLATEAUCityObjectTypeHierarchy", D_METHOD("get_package_name", "package"), &PLATEAUCityObjectTypeHierarchy::get_package_name);
    ClassDB::bind_static_method("PLATEAUCityObjectTypeHierarchy", D_METHOD("get_package_display_name", "package"), &PLATEAUCityObjectTypeHierarchy::get_package_display_name);
    ClassDB::bind_static_method("PLATEAUCityObjectTypeHierarchy", D_METHOD("type_belongs_to_package", "type", "package"), &PLATEAUCityObjectTypeHierarchy::type_belongs_to_package);
    ClassDB::bind_static_method("PLATEAUCityObjectTypeHierarchy", D_METHOD("get_types_for_package", "package"), &PLATEAUCityObjectTypeHierarchy::get_types_for_package);
}

} // namespace godot
