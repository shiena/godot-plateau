#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <plateau/dataset/gml_file.h>

namespace godot {

/**
 * PLATEAUGmlFile - GML file information and utilities
 *
 * Provides access to GML file metadata and related files.
 *
 * Usage:
 * ```gdscript
 * var gml = PLATEAUGmlFile.create("C:/path/to/udx/bldg/53394601_bldg_6697_op.gml")
 * if gml.is_valid():
 *     print("Grid code: ", gml.get_grid_code())        # 53394601
 *     print("Feature type: ", gml.get_feature_type())  # bldg
 *     print("EPSG: ", gml.get_epsg())                  # 6697
 *     print("Dataset root: ", gml.get_dataset_root_path())
 *     print("Max LOD: ", gml.get_max_lod())
 *
 *     # Get texture paths referenced in GML
 *     var textures = gml.search_image_paths()
 *     for tex_path in textures:
 *         print("Texture: ", tex_path)
 * ```
 */
class PLATEAUGmlFile : public RefCounted {
    GDCLASS(PLATEAUGmlFile, RefCounted)

public:
    PLATEAUGmlFile();
    ~PLATEAUGmlFile();

    // Static constructor from file path
    static Ref<PLATEAUGmlFile> create(const String &path);

    // Get the GML file path
    String get_path() const;

    // Get the grid code (mesh code) from filename
    // e.g., "53394601" from "53394601_bldg_6697_op.gml"
    String get_grid_code() const;

    // Get the EPSG code
    // e.g., 6697 from "53394601_bldg_6697_op.gml"
    int get_epsg() const;

    // Get the feature type
    // e.g., "bldg" from "53394601_bldg_6697_op.gml"
    String get_feature_type() const;

    // Get the package type (PLATEAUCityModelPackage enum value)
    int64_t get_package_type() const;

    // Get the appearance directory path (where textures are stored)
    // e.g., "C:/path/to/udx/bldg/53394601_bldg_6697_appearance"
    String get_appearance_directory_path() const;

    // Get the dataset root path (parent of /udx/)
    // e.g., "C:/path/to/13103_minato-ku_pref_2023_citygml_2_op"
    String get_dataset_root_path() const;

    // Get the max LOD level (searches GML content, may be slow)
    int get_max_lod();

    // Check if the GML file info is valid
    bool is_valid() const;

    // Search for image paths (textures) referenced in GML
    // Returns relative paths from the GML file directory
    PackedStringArray search_image_paths() const;

    // Search for codelist paths referenced in GML
    PackedStringArray search_codelist_paths() const;

    // Get the grid code's geographic extent (min/max lat/lon)
    // Returns Dictionary with keys: min_lat, max_lat, min_lon, max_lon
    Dictionary get_grid_extent() const;

protected:
    static void _bind_methods();

private:
    std::shared_ptr<plateau::dataset::GmlFile> gml_file_;
    String path_;
};

} // namespace godot
