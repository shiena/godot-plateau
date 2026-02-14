#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <plateau/dataset/dataset_source.h>
#include <plateau/dataset/i_dataset_accessor.h>

#include "plateau_grid_code.h"

namespace godot {

// Predefined city model package types (bitmask)
// Bit positions match libplateau's PredefinedCityModelPackage exactly.
enum PLATEAUCityModelPackage : int64_t {
    PACKAGE_NONE = 0,
    PACKAGE_BUILDING = 1 << 0,              // bldg - Buildings
    PACKAGE_ROAD = 1 << 1,                  // tran - Roads
    PACKAGE_URBAN_PLANNING = 1 << 2,        // urf - Urban planning
    PACKAGE_LAND_USE = 1 << 3,              // luse - Land use
    PACKAGE_CITY_FURNITURE = 1 << 4,        // frn - City furniture
    PACKAGE_VEGETATION = 1 << 5,            // veg - Vegetation
    PACKAGE_RELIEF = 1 << 6,                // dem - Relief/DEM
    PACKAGE_DISASTER_RISK = 1 << 7,         // fld/tnm/lsld/htd/ifld - Disaster risk
    PACKAGE_RAILWAY = 1 << 8,               // rwy - Railway
    PACKAGE_WATERWAY = 1 << 9,              // wwy - Waterway
    PACKAGE_WATER_BODY = 1 << 10,           // wtr - Water body
    PACKAGE_BRIDGE = 1 << 11,               // brid - Bridge
    PACKAGE_TRACK = 1 << 12,                // trk - Track
    PACKAGE_SQUARE = 1 << 13,               // squr - Square
    PACKAGE_TUNNEL = 1 << 14,               // tun - Tunnel
    PACKAGE_UNDERGROUND_FACILITY = 1 << 15, // unf - Underground facility
    PACKAGE_UNDERGROUND_BUILDING = 1 << 16, // ubld - Underground building
    PACKAGE_AREA = 1 << 17,                 // area - Area
    PACKAGE_OTHER_CONSTRUCTION = 1 << 18,   // cons - Other construction
    PACKAGE_GENERIC = 1 << 19,              // gen - Generic
    PACKAGE_UNKNOWN = 1 << 31,
    PACKAGE_ALL = 0x000FFFFF | (1 << 31),
};

/**
 * PLATEAUDatasetMetadata - Metadata for a dataset
 */
class PLATEAUDatasetMetadata : public RefCounted {
    GDCLASS(PLATEAUDatasetMetadata, RefCounted)

public:
    PLATEAUDatasetMetadata();
    ~PLATEAUDatasetMetadata();

    void set_id(const String &id);
    String get_id() const;

    void set_title(const String &title);
    String get_title() const;

    void set_description(const String &description);
    String get_description() const;

    void set_feature_types(const PackedStringArray &types);
    PackedStringArray get_feature_types() const;

protected:
    static void _bind_methods();

private:
    String id_;
    String title_;
    String description_;
    PackedStringArray feature_types_;
};

/**
 * PLATEAUDatasetGroup - Group of datasets (typically represents a prefecture)
 */
class PLATEAUDatasetGroup : public RefCounted {
    GDCLASS(PLATEAUDatasetGroup, RefCounted)

public:
    PLATEAUDatasetGroup();
    ~PLATEAUDatasetGroup();

    void set_id(const String &id);
    String get_id() const;

    void set_title(const String &title);
    String get_title() const;

    void set_datasets(const TypedArray<PLATEAUDatasetMetadata> &datasets);
    TypedArray<PLATEAUDatasetMetadata> get_datasets() const;

    int get_dataset_count() const;
    Ref<PLATEAUDatasetMetadata> get_dataset(int index) const;

protected:
    static void _bind_methods();

private:
    String id_;
    String title_;
    TypedArray<PLATEAUDatasetMetadata> datasets_;
};

/**
 * PLATEAUGmlFileInfo - Information about a GML file in a dataset
 */
class PLATEAUGmlFileInfo : public RefCounted {
    GDCLASS(PLATEAUGmlFileInfo, RefCounted)

public:
    PLATEAUGmlFileInfo();
    ~PLATEAUGmlFileInfo();

    void set_path(const String &path);
    String get_path() const;

    void set_mesh_code(const String &code);
    String get_mesh_code() const;

    void set_max_lod(int lod);
    int get_max_lod() const;

    void set_epsg(int epsg);
    int get_epsg() const;

    void set_package_type(int64_t type);
    int64_t get_package_type() const;

protected:
    static void _bind_methods();

private:
    String path_;
    String mesh_code_;
    int max_lod_;
    int epsg_;
    int64_t package_type_;
};

/**
 * PLATEAUDatasetSource - Access to PLATEAU dataset (local)
 *
 * Usage:
 * ```gdscript
 * var source = PLATEAUDatasetSource.create_local("C:/path/to/plateau/data")
 * var gml_files = source.get_gml_files(PLATEAUCityModelPackage.PACKAGE_BUILDING)
 * for file_info in gml_files:
 *     print(file_info.get_path())
 * ```
 *
 * For server access, use HTTPRequest in GDScript directly.
 */
class PLATEAUDatasetSource : public RefCounted {
    GDCLASS(PLATEAUDatasetSource, RefCounted)

public:
    PLATEAUDatasetSource();
    ~PLATEAUDatasetSource();

    // Create from local path
    static Ref<PLATEAUDatasetSource> create_local(const String &local_path);

    // Get default server URL
    static String get_default_server_url();

    // Get mock server URL
    static String get_mock_server_url();

    // Build authentication headers for PLATEAU API requests
    // - custom_token: User-provided token (used if not empty)
    // - use_default_token: If true and custom_token is empty, uses built-in default token
    // Returns: ["Content-Type: application/json", "Authorization: Bearer xxx"] or just content-type if no auth
    static PackedStringArray build_auth_headers(const String &custom_token, bool use_default_token);

    // Check if source is valid
    bool is_valid() const;

    // Get available packages (bitmask of PLATEAUCityModelPackage)
    int64_t get_available_packages() const;

    // Get GML files for specified package type(s)
    TypedArray<PLATEAUGmlFileInfo> get_gml_files(int64_t package_flags);

    // Get mesh codes (region IDs) in this dataset as strings
    PackedStringArray get_mesh_codes();

    // Get grid codes as PLATEAUGridCode objects (richer API than get_mesh_codes)
    TypedArray<PLATEAUGridCode> get_grid_codes();

    // Filter by grid code strings
    Ref<PLATEAUDatasetSource> filter_by_mesh_codes(const PackedStringArray &codes);

protected:
    static void _bind_methods();

private:
    std::shared_ptr<plateau::dataset::IDatasetAccessor> accessor_;
    bool is_valid_;
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PLATEAUCityModelPackage);
