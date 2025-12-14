#pragma once

// Platform detection for mobile exclusions
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define PLATEAU_MOBILE_PLATFORM 1
#endif

#ifndef PLATEAU_MOBILE_PLATFORM

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <plateau/basemap/vector_tile_downloader.h>
#include <plateau/basemap/tile_projection.h>

namespace godot {

// Tile source presets
enum PLATEAUTileSource : int {
    TILE_SOURCE_GSI_PHOTO = 0,    // GSI seamless aerial photo
    TILE_SOURCE_GSI_STD = 1,      // GSI standard map
    TILE_SOURCE_GSI_PALE = 2,     // GSI pale map
    TILE_SOURCE_OSM = 3,          // OpenStreetMap
    TILE_SOURCE_CUSTOM = 4,       // Custom URL template
};

/**
 * PLATEAUTileCoordinate - Map tile coordinate (column, row, zoom level)
 *
 * Represents a tile coordinate in the GSI (Geospatial Information Authority of Japan) tile system.
 * Reference: https://maps.gsi.go.jp/development/tileCoordCheck.html
 */
class PLATEAUTileCoordinate : public RefCounted {
    GDCLASS(PLATEAUTileCoordinate, RefCounted)

public:
    PLATEAUTileCoordinate();
    ~PLATEAUTileCoordinate();

    void set_column(int column);
    int get_column() const;

    void set_row(int row);
    int get_row() const;

    void set_zoom_level(int level);
    int get_zoom_level() const;

    // Get extent (lat/lon bounds) for this tile
    Dictionary get_extent() const;

protected:
    static void _bind_methods();

private:
    int column_;
    int row_;
    int zoom_level_;
};

/**
 * PLATEAUVectorTile - Downloaded map tile information
 *
 * Contains tile coordinate, local image path, and download status.
 */
class PLATEAUVectorTile : public RefCounted {
    GDCLASS(PLATEAUVectorTile, RefCounted)

public:
    PLATEAUVectorTile();
    ~PLATEAUVectorTile();

    void set_coordinate(const Ref<PLATEAUTileCoordinate> &coord);
    Ref<PLATEAUTileCoordinate> get_coordinate() const;

    void set_image_path(const String &path);
    String get_image_path() const;

    void set_success(bool success);
    bool is_success() const;

    // Load the tile image as Godot Image (cached after first load)
    Ref<Image> load_image() const;

    // Load the tile image as ImageTexture
    Ref<ImageTexture> load_texture() const;

    // Clear the cached image to free memory
    void clear_image_cache();

protected:
    static void _bind_methods();

private:
    Ref<PLATEAUTileCoordinate> coordinate_;
    String image_path_;
    bool success_;

    // Cached image to avoid redundant disk reads
    mutable Ref<Image> cached_image_;
};

/**
 * PLATEAUVectorTileDownloader - Download map tiles from GSI or custom tile servers
 *
 * Downloads map tiles (aerial photos, standard maps, etc.) for a given geographic extent.
 * Tiles are saved as PNG files to the specified destination directory.
 *
 * Usage:
 * ```gdscript
 * var downloader = PLATEAUVectorTileDownloader.new()
 * downloader.destination = "user://map_tiles"
 * downloader.zoom_level = 15
 *
 * # Set extent from GeoReference
 * var geo_ref = PLATEAUGeoReference.new()
 * geo_ref.zone_id = 9  # Tokyo
 * var center = geo_ref.project_to_geo(Vector3(0, 0, 0))
 * downloader.set_extent_from_center(center.x, center.y, 0.01)  # lat, lon, radius in degrees
 *
 * # Or set extent directly
 * downloader.set_extent(35.6, 139.7, 35.7, 139.8)  # min_lat, min_lon, max_lat, max_lon
 *
 * # Download all tiles
 * var tiles = downloader.download_all()
 * for tile in tiles:
 *     if tile.is_success():
 *         var texture = tile.load_texture()
 *         # Use texture...
 *
 * # Create combined texture
 * var combined = downloader.create_combined_texture(tiles)
 * ```
 *
 * Default URL uses GSI seamless photo tiles. Other options:
 * - "https://cyberjapandata.gsi.go.jp/xyz/std/{z}/{x}/{y}.png" (standard map)
 * - "https://cyberjapandata.gsi.go.jp/xyz/pale/{z}/{x}/{y}.png" (pale map)
 * - "https://tile.openstreetmap.org/{z}/{x}/{y}.png" (OpenStreetMap)
 */
class PLATEAUVectorTileDownloader : public RefCounted {
    GDCLASS(PLATEAUVectorTileDownloader, RefCounted)

public:
    PLATEAUVectorTileDownloader();
    ~PLATEAUVectorTileDownloader();

    // Destination directory for downloaded tiles
    void set_destination(const String &path);
    String get_destination() const;

    // Zoom level (1-18, default 15)
    void set_zoom_level(int level);
    int get_zoom_level() const;

    // Tile source preset
    void set_tile_source(int source);
    int get_tile_source() const;

    // URL template with {x}, {y}, {z} placeholders
    void set_url_template(const String &url);
    String get_url_template() const;

    // Set extent using lat/lon bounds
    void set_extent(double min_lat, double min_lon, double max_lat, double max_lon);

    // Set extent from center point and radius (in degrees)
    void set_extent_from_center(double center_lat, double center_lon, double radius_deg);

    // Get current extent as Dictionary {min_lat, min_lon, max_lat, max_lon}
    Dictionary get_extent() const;

    // Get number of tiles to download
    int get_tile_count() const;

    // Get tile coordinate at index
    Ref<PLATEAUTileCoordinate> get_tile_coordinate(int index) const;

    // Download single tile at index
    Ref<PLATEAUVectorTile> download(int index);

    // Download single tile by coordinate
    // NOTE: C++ HTTP is disabled due to OpenSSL crash. Use GDScript HTTPRequest instead.
    Ref<PLATEAUVectorTile> download_tile(const Ref<PLATEAUTileCoordinate> &coord);

    // Get the download URL for a tile coordinate
    String get_tile_url(const Ref<PLATEAUTileCoordinate> &coord) const;

    // Get the local file path where a tile should be saved
    String get_tile_file_path(const Ref<PLATEAUTileCoordinate> &coord) const;

    // Download all tiles in extent
    TypedArray<PLATEAUVectorTile> download_all();

    // Create a combined texture from downloaded tiles
    // Tiles are arranged in a grid matching their geographic positions
    Ref<ImageTexture> create_combined_texture(const TypedArray<PLATEAUVectorTile> &tiles);

    // Get the expected file path for a tile at index
    String get_tile_path(int index) const;

    // Static: Get default GSI aerial photo URL
    static String get_default_url();

    // Static: Convert lat/lon to tile coordinate
    static Ref<PLATEAUTileCoordinate> project(double latitude, double longitude, int zoom_level);

    // Static: Convert tile coordinate to extent
    static Dictionary unproject(const Ref<PLATEAUTileCoordinate> &coord);

protected:
    static void _bind_methods();

private:
    String destination_;
    int zoom_level_;
    int tile_source_;
    String url_template_;
    plateau::geometry::Extent extent_;
    std::unique_ptr<VectorTileDownloader> downloader_;

    void ensure_downloader();
    void invalidate_downloader();
    void update_url_from_source();
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PLATEAUTileSource);

#endif // !PLATEAU_MOBILE_PLATFORM
