#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "plateau_city_model.h"
#include "plateau_dataset_source.h"

namespace godot {

/**
 * PLATEAUCityObjectTypeNode - Node in the city object type hierarchy tree
 *
 * Represents a category or specific type in the hierarchy.
 * For example: Building category contains RoofSurface, WallSurface, etc.
 */
class PLATEAUCityObjectTypeNode : public RefCounted {
    GDCLASS(PLATEAUCityObjectTypeNode, RefCounted)

public:
    PLATEAUCityObjectTypeNode();
    ~PLATEAUCityObjectTypeNode();

    // Node name (category or type name)
    void set_name(const String &name);
    String get_name() const;

    // Display name (localized/human-readable)
    void set_display_name(const String &name);
    String get_display_name() const;

    // City object type (0 for category nodes, actual type for leaf nodes)
    void set_type(int64_t type);
    int64_t get_type() const;

    // Package type this node belongs to
    void set_package(int64_t package);
    int64_t get_package() const;

    // Parent node (null for root)
    void set_parent(const Ref<PLATEAUCityObjectTypeNode> &parent);
    Ref<PLATEAUCityObjectTypeNode> get_parent() const;

    // Child nodes
    void add_child(const Ref<PLATEAUCityObjectTypeNode> &child);
    TypedArray<PLATEAUCityObjectTypeNode> get_children() const;
    int get_child_count() const;
    Ref<PLATEAUCityObjectTypeNode> get_child(int index) const;

    // Check if this is a leaf node (actual type, not category)
    bool is_leaf() const;

    // Check if this node or any descendant matches the given type
    bool contains_type(int64_t type) const;

    // Get all types in this subtree (including this node if it's a type)
    PackedInt64Array get_all_types() const;

protected:
    static void _bind_methods();

private:
    String name_;
    String display_name_;
    int64_t type_;
    int64_t package_;
    Ref<PLATEAUCityObjectTypeNode> parent_;
    TypedArray<PLATEAUCityObjectTypeNode> children_;

    void collect_types(PackedInt64Array &result) const;
};

/**
 * PLATEAUCityObjectTypeHierarchy - Static hierarchy of city object types
 *
 * Provides a tree structure for organizing city object types.
 * Based on CityGML specification and PLATEAU extensions.
 *
 * Hierarchy:
 * - Building
 *   - BuildingPart
 *   - RoofSurface
 *   - WallSurface
 *   - GroundSurface
 *   - Door
 *   - Window
 *   - BuildingInstallation
 *   - ...
 * - Transportation
 *   - Road
 *   - Railway
 *   - Track
 *   - Square
 * - Relief
 *   - TINRelief
 *   - MassPointRelief
 *   - ...
 * - Vegetation
 *   - PlantCover
 *   - SolitaryVegetationObject
 * - WaterBody
 * - CityFurniture
 * - LandUse
 * - Bridge
 * - Tunnel
 * - Generic
 *
 * Usage:
 * ```gdscript
 * var hierarchy = PLATEAUCityObjectTypeHierarchy.new()
 * var root = hierarchy.get_root()
 *
 * # Get building category
 * var building_node = hierarchy.get_node_by_type(COT_Building)
 *
 * # Get node by package
 * var relief_node = hierarchy.get_node_by_package(PACKAGE_RELIEF)
 *
 * # Convert type to package
 * var package = PLATEAUCityObjectTypeHierarchy.type_to_package(COT_RoofSurface)
 * # Returns PACKAGE_BUILDING
 * ```
 */
class PLATEAUCityObjectTypeHierarchy : public RefCounted {
    GDCLASS(PLATEAUCityObjectTypeHierarchy, RefCounted)

public:
    PLATEAUCityObjectTypeHierarchy();
    ~PLATEAUCityObjectTypeHierarchy();

    // Get root node of the hierarchy
    Ref<PLATEAUCityObjectTypeNode> get_root() const;

    // Get node by city object type
    Ref<PLATEAUCityObjectTypeNode> get_node_by_type(int64_t type) const;

    // Get node by package type
    Ref<PLATEAUCityObjectTypeNode> get_node_by_package(int64_t package) const;

    // Get all leaf nodes (actual types)
    TypedArray<PLATEAUCityObjectTypeNode> get_all_types() const;

    // Get all category nodes
    TypedArray<PLATEAUCityObjectTypeNode> get_all_categories() const;

    // Static: Convert city object type to package
    static int64_t type_to_package(int64_t type);

    // Static: Get type name as string
    static String get_type_name(int64_t type);

    // Static: Get type display name (Japanese)
    static String get_type_display_name(int64_t type);

    // Static: Get package name as string
    static String get_package_name(int64_t package);

    // Static: Get package display name (Japanese)
    static String get_package_display_name(int64_t package);

    // Static: Check if type belongs to package
    static bool type_belongs_to_package(int64_t type, int64_t package);

    // Static: Get all types for a package (bitmask)
    static int64_t get_types_for_package(int64_t package);

protected:
    static void _bind_methods();

private:
    Ref<PLATEAUCityObjectTypeNode> root_;
    Dictionary type_to_node_;
    Dictionary package_to_node_;

    void build_hierarchy();
    Ref<PLATEAUCityObjectTypeNode> create_node(const String &name, const String &display_name, int64_t type, int64_t package);
    void add_to_lookup(const Ref<PLATEAUCityObjectTypeNode> &node);
    void collect_nodes(const Ref<PLATEAUCityObjectTypeNode> &node, TypedArray<PLATEAUCityObjectTypeNode> &leaves, TypedArray<PLATEAUCityObjectTypeNode> &categories) const;
};

} // namespace godot
