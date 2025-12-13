// Platform detection for mobile exclusions
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_OS_VISION) && TARGET_OS_VISION)
#define PLATEAU_MOBILE_PLATFORM 1
#endif

#ifndef PLATEAU_MOBILE_PLATFORM

#include "plateau_basemap.h"
#include "plateau_platform.h"
#include "plateau_parallel.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/os.hpp>
#include <mutex>

using namespace godot;

// ============================================================================
// PLATEAUTileCoordinate
// ============================================================================

PLATEAUTileCoordinate::PLATEAUTileCoordinate()
    : column_(-1), row_(-1), zoom_level_(-1) {
}

PLATEAUTileCoordinate::~PLATEAUTileCoordinate() {
}

void PLATEAUTileCoordinate::set_column(int column) {
    column_ = column;
}

int PLATEAUTileCoordinate::get_column() const {
    return column_;
}

void PLATEAUTileCoordinate::set_row(int row) {
    row_ = row;
}

int PLATEAUTileCoordinate::get_row() const {
    return row_;
}

void PLATEAUTileCoordinate::set_zoom_level(int level) {
    zoom_level_ = level;
}

int PLATEAUTileCoordinate::get_zoom_level() const {
    return zoom_level_;
}

Dictionary PLATEAUTileCoordinate::get_extent() const {
    Dictionary result;

    if (column_ < 0 || row_ < 0 || zoom_level_ < 0) {
        return result;
    }

    TileCoordinate coord(column_, row_, zoom_level_);
    auto extent = TileProjection::unproject(coord);

    result["min_lat"] = extent.min.latitude;
    result["min_lon"] = extent.min.longitude;
    result["max_lat"] = extent.max.latitude;
    result["max_lon"] = extent.max.longitude;

    return result;
}

void PLATEAUTileCoordinate::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_column", "column"), &PLATEAUTileCoordinate::set_column);
    ClassDB::bind_method(D_METHOD("get_column"), &PLATEAUTileCoordinate::get_column);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "column"), "set_column", "get_column");

    ClassDB::bind_method(D_METHOD("set_row", "row"), &PLATEAUTileCoordinate::set_row);
    ClassDB::bind_method(D_METHOD("get_row"), &PLATEAUTileCoordinate::get_row);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "row"), "set_row", "get_row");

    ClassDB::bind_method(D_METHOD("set_zoom_level", "level"), &PLATEAUTileCoordinate::set_zoom_level);
    ClassDB::bind_method(D_METHOD("get_zoom_level"), &PLATEAUTileCoordinate::get_zoom_level);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "zoom_level"), "set_zoom_level", "get_zoom_level");

    ClassDB::bind_method(D_METHOD("get_extent"), &PLATEAUTileCoordinate::get_extent);
}

// ============================================================================
// PLATEAUVectorTile
// ============================================================================

PLATEAUVectorTile::PLATEAUVectorTile()
    : success_(false) {
}

PLATEAUVectorTile::~PLATEAUVectorTile() {
}

void PLATEAUVectorTile::set_coordinate(const Ref<PLATEAUTileCoordinate> &coord) {
    coordinate_ = coord;
}

Ref<PLATEAUTileCoordinate> PLATEAUVectorTile::get_coordinate() const {
    return coordinate_;
}

void PLATEAUVectorTile::set_image_path(const String &path) {
    image_path_ = path;
}

String PLATEAUVectorTile::get_image_path() const {
    return image_path_;
}

void PLATEAUVectorTile::set_success(bool success) {
    success_ = success;
}

bool PLATEAUVectorTile::is_success() const {
    return success_;
}

Ref<Image> PLATEAUVectorTile::load_image() const {
    if (!success_ || image_path_.is_empty()) {
        return Ref<Image>();
    }

    // Return cached image if available
    if (cached_image_.is_valid()) {
        return cached_image_;
    }

    Ref<Image> image;
    image.instantiate();

    Error err = image->load(image_path_);
    if (err != OK) {
        UtilityFunctions::printerr("PLATEAUVectorTile: Failed to load image: ", image_path_);
        return Ref<Image>();
    }

    // Cache the loaded image for subsequent calls
    cached_image_ = image;
    return image;
}

void PLATEAUVectorTile::clear_image_cache() {
    cached_image_ = Ref<Image>();
}

Ref<ImageTexture> PLATEAUVectorTile::load_texture() const {
    Ref<Image> image = load_image();
    if (image.is_null()) {
        return Ref<ImageTexture>();
    }

    Ref<ImageTexture> texture = ImageTexture::create_from_image(image);
    return texture;
}

void PLATEAUVectorTile::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_coordinate", "coord"), &PLATEAUVectorTile::set_coordinate);
    ClassDB::bind_method(D_METHOD("get_coordinate"), &PLATEAUVectorTile::get_coordinate);

    ClassDB::bind_method(D_METHOD("set_image_path", "path"), &PLATEAUVectorTile::set_image_path);
    ClassDB::bind_method(D_METHOD("get_image_path"), &PLATEAUVectorTile::get_image_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "image_path"), "set_image_path", "get_image_path");

    ClassDB::bind_method(D_METHOD("set_success", "success"), &PLATEAUVectorTile::set_success);
    ClassDB::bind_method(D_METHOD("is_success"), &PLATEAUVectorTile::is_success);

    ClassDB::bind_method(D_METHOD("load_image"), &PLATEAUVectorTile::load_image);
    ClassDB::bind_method(D_METHOD("load_texture"), &PLATEAUVectorTile::load_texture);
    ClassDB::bind_method(D_METHOD("clear_image_cache"), &PLATEAUVectorTile::clear_image_cache);
}

// ============================================================================
// PLATEAUVectorTileDownloader
// ============================================================================

PLATEAUVectorTileDownloader::PLATEAUVectorTileDownloader()
    : zoom_level_(15),
      tile_source_(TILE_SOURCE_GSI_PHOTO),
      extent_(plateau::geometry::GeoCoordinate(0, 0, 0),
              plateau::geometry::GeoCoordinate(0, 0, 0)) {
    url_template_ = String(VectorTileDownloader::getDefaultUrl().c_str());
}

PLATEAUVectorTileDownloader::~PLATEAUVectorTileDownloader() {
}

void PLATEAUVectorTileDownloader::set_destination(const String &path) {
    destination_ = path;
    invalidate_downloader();
}

String PLATEAUVectorTileDownloader::get_destination() const {
    return destination_;
}

void PLATEAUVectorTileDownloader::set_zoom_level(int level) {
    zoom_level_ = Math::clamp(level, 1, 18);
    invalidate_downloader();
}

int PLATEAUVectorTileDownloader::get_zoom_level() const {
    return zoom_level_;
}

void PLATEAUVectorTileDownloader::set_tile_source(int source) {
    tile_source_ = source;
    update_url_from_source();
    invalidate_downloader();
}

int PLATEAUVectorTileDownloader::get_tile_source() const {
    return tile_source_;
}

void PLATEAUVectorTileDownloader::update_url_from_source() {
    switch (tile_source_) {
        case TILE_SOURCE_GSI_PHOTO:
            url_template_ = "https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto/{z}/{x}/{y}.jpg";
            break;
        case TILE_SOURCE_GSI_STD:
            url_template_ = "https://cyberjapandata.gsi.go.jp/xyz/std/{z}/{x}/{y}.png";
            break;
        case TILE_SOURCE_GSI_PALE:
            url_template_ = "https://cyberjapandata.gsi.go.jp/xyz/pale/{z}/{x}/{y}.png";
            break;
        case TILE_SOURCE_OSM:
            url_template_ = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
            break;
        case TILE_SOURCE_CUSTOM:
            // Keep current url_template_
            break;
        default:
            break;
    }
}

void PLATEAUVectorTileDownloader::set_url_template(const String &url) {
    url_template_ = url;
    if (downloader_) {
        downloader_->setUrl(url.utf8().get_data());
    }
}

String PLATEAUVectorTileDownloader::get_url_template() const {
    return url_template_;
}

void PLATEAUVectorTileDownloader::set_extent(double min_lat, double min_lon, double max_lat, double max_lon) {
    extent_ = plateau::geometry::Extent(
        plateau::geometry::GeoCoordinate(min_lat, min_lon, -10000),
        plateau::geometry::GeoCoordinate(max_lat, max_lon, 10000)
    );
    invalidate_downloader();
}

void PLATEAUVectorTileDownloader::set_extent_from_center(double center_lat, double center_lon, double radius_deg) {
    set_extent(
        center_lat - radius_deg,
        center_lon - radius_deg,
        center_lat + radius_deg,
        center_lon + radius_deg
    );
}

Dictionary PLATEAUVectorTileDownloader::get_extent() const {
    Dictionary result;
    result["min_lat"] = extent_.min.latitude;
    result["min_lon"] = extent_.min.longitude;
    result["max_lat"] = extent_.max.latitude;
    result["max_lon"] = extent_.max.longitude;
    return result;
}

int PLATEAUVectorTileDownloader::get_tile_count() const {
    const_cast<PLATEAUVectorTileDownloader*>(this)->ensure_downloader();
    if (!downloader_) {
        return 0;
    }
    return downloader_->getTileCount();
}

Ref<PLATEAUTileCoordinate> PLATEAUVectorTileDownloader::get_tile_coordinate(int index) const {
    const_cast<PLATEAUVectorTileDownloader*>(this)->ensure_downloader();
    if (!downloader_ || index < 0 || index >= downloader_->getTileCount()) {
        return Ref<PLATEAUTileCoordinate>();
    }

    TileCoordinate coord = downloader_->getTile(index);

    Ref<PLATEAUTileCoordinate> result;
    result.instantiate();
    result->set_column(coord.column);
    result->set_row(coord.row);
    result->set_zoom_level(coord.zoom_level);

    return result;
}

Ref<PLATEAUVectorTile> PLATEAUVectorTileDownloader::download(int index) {
    Ref<PLATEAUVectorTile> result;
    result.instantiate();

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

    ensure_downloader();

    if (!downloader_ || index < 0 || index >= downloader_->getTileCount()) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader: Invalid index: ", index);
        return result;
    }

    try {
        VectorTile tile;
        bool success = downloader_->download(index, tile);

        Ref<PLATEAUTileCoordinate> coord;
        coord.instantiate();
        coord->set_column(tile.coordinate.column);
        coord->set_row(tile.coordinate.row);
        coord->set_zoom_level(tile.coordinate.zoom_level);

        result->set_coordinate(coord);
        result->set_image_path(String::utf8(tile.image_path.c_str()));
        result->set_success(success && tile.result == HttpResult::Success);

        if (result->is_success()) {
            UtilityFunctions::print("PLATEAUVectorTileDownloader: Downloaded tile ",
                                   coord->get_column(), "/", coord->get_row());
        }

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader exception: ", String(e.what()));
    }

    return result;
}

String PLATEAUVectorTileDownloader::get_tile_url(const Ref<PLATEAUTileCoordinate> &coord) const {
    if (coord.is_null()) {
        return "";
    }

    String url = url_template_;
    url = url.replace("{z}", String::num_int64(coord->get_zoom_level()));
    url = url.replace("{x}", String::num_int64(coord->get_column()));
    url = url.replace("{y}", String::num_int64(coord->get_row()));
    return url;
}

String PLATEAUVectorTileDownloader::get_tile_file_path(const Ref<PLATEAUTileCoordinate> &coord) const {
    if (coord.is_null() || destination_.is_empty()) {
        return "";
    }

    // Convert Godot path to absolute filesystem path
    String abs_dest = destination_;
    if (destination_.begins_with("user://") || destination_.begins_with("res://")) {
        abs_dest = ProjectSettings::get_singleton()->globalize_path(destination_);
    }

    // Get file extension from URL
    String ext = ".png";
    if (url_template_.ends_with(".jpg") || url_template_.ends_with(".jpeg")) {
        ext = ".jpg";
    }

    // Build path: destination/zoom/column/row.ext
    String path = abs_dest + "/" + String::num_int64(coord->get_zoom_level())
                  + "/" + String::num_int64(coord->get_column())
                  + "/" + String::num_int64(coord->get_row()) + ext;
    return path;
}

Ref<PLATEAUVectorTile> PLATEAUVectorTileDownloader::download_tile(const Ref<PLATEAUTileCoordinate> &coord) {
    Ref<PLATEAUVectorTile> result;
    result.instantiate();

    if (coord.is_null()) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader: Invalid coordinate");
        return result;
    }

    // NOTE: libplateau's HTTP client (cpp-httplib with OpenSSL) crashes on Windows.
    // Use get_tile_url() and get_tile_file_path() with Godot's HTTPRequest instead.
    // See basemap_sample.gd for GDScript-based download implementation.

    result->set_coordinate(coord);
    result->set_image_path("");
    result->set_success(false);

    UtilityFunctions::printerr("PLATEAUVectorTileDownloader::download_tile - C++ HTTP disabled. Use GDScript HTTPRequest instead.");
    UtilityFunctions::print("  URL: ", get_tile_url(coord));
    UtilityFunctions::print("  File: ", get_tile_file_path(coord));

    return result;
}

TypedArray<PLATEAUVectorTile> PLATEAUVectorTileDownloader::download_all() {
    TypedArray<PLATEAUVectorTile> result;

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

    ensure_downloader();
    if (!downloader_) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader: Downloader not initialized");
        return result;
    }

    try {
        VectorTiles tiles = downloader_->downloadAll();

        int success_count = 0;
        for (auto &tile : tiles.tiles()) {
            Ref<PLATEAUVectorTile> gd_tile;
            gd_tile.instantiate();

            Ref<PLATEAUTileCoordinate> coord;
            coord.instantiate();
            coord->set_column(tile.coordinate.column);
            coord->set_row(tile.coordinate.row);
            coord->set_zoom_level(tile.coordinate.zoom_level);

            gd_tile->set_coordinate(coord);
            gd_tile->set_image_path(String::utf8(tile.image_path.c_str()));
            gd_tile->set_success(tile.result == HttpResult::Success);

            if (gd_tile->is_success()) {
                success_count++;
            }

            result.push_back(gd_tile);
        }

        UtilityFunctions::print("PLATEAUVectorTileDownloader: Downloaded ", success_count,
                               "/", result.size(), " tiles");

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader exception: ", String(e.what()));
    }

    return result;
}

Ref<ImageTexture> PLATEAUVectorTileDownloader::create_combined_texture(const TypedArray<PLATEAUVectorTile> &tiles) {
    if (tiles.is_empty()) {
        return Ref<ImageTexture>();
    }

    int tile_count = tiles.size();

    // Find min/max column and row using parallel reduction
    struct MinMax {
        int min_col = INT_MAX, max_col = INT_MIN;
        int min_row = INT_MAX, max_row = INT_MIN;
    };

    MinMax bounds;
    std::mutex bounds_mutex;

    plateau_parallel::parallel_for(0, static_cast<size_t>(tile_count), [&](size_t i) {
        Ref<PLATEAUVectorTile> tile = tiles[i];
        if (tile.is_null() || tile->get_coordinate().is_null()) return;

        Ref<PLATEAUTileCoordinate> coord = tile->get_coordinate();
        int col = coord->get_column();
        int row = coord->get_row();

        std::lock_guard<std::mutex> lock(bounds_mutex);
        bounds.min_col = Math::min(bounds.min_col, col);
        bounds.max_col = Math::max(bounds.max_col, col);
        bounds.min_row = Math::min(bounds.min_row, row);
        bounds.max_row = Math::max(bounds.max_row, row);
    }, 10);  // Small min_chunk since this is quick per item

    if (bounds.min_col > bounds.max_col || bounds.min_row > bounds.max_row) {
        return Ref<ImageTexture>();
    }

    int min_col = bounds.min_col;
    int max_col = bounds.max_col;
    int min_row = bounds.min_row;
    int max_row = bounds.max_row;

    int cols = max_col - min_col + 1;
    int rows = max_row - min_row + 1;

    // Load first successful tile to get tile size
    int tile_width = 256;
    int tile_height = 256;
    Image::Format format = Image::FORMAT_RGBA8;

    for (int i = 0; i < tile_count; i++) {
        Ref<PLATEAUVectorTile> tile = tiles[i];
        if (tile.is_valid() && tile->is_success()) {
            Ref<Image> img = tile->load_image();
            if (img.is_valid()) {
                tile_width = img->get_width();
                tile_height = img->get_height();
                format = img->get_format();
                break;
            }
        }
    }

    // Create combined image
    int combined_width = cols * tile_width;
    int combined_height = rows * tile_height;

    Ref<Image> combined = Image::create_empty(combined_width, combined_height, false, format);
    if (combined.is_null()) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader: Failed to create combined image");
        return Ref<ImageTexture>();
    }
    combined->fill(Color(0.5, 0.5, 0.5, 1.0)); // Gray background for missing tiles

    // Pre-load and convert all tile images in parallel
    // Store results for serial blit (blit_rect may not be thread-safe)
    struct TileData {
        Ref<Image> image;
        int x = 0;
        int y = 0;
        bool valid = false;
    };
    std::vector<TileData> tile_data(tile_count);

    plateau_parallel::parallel_for(0, static_cast<size_t>(tile_count), [&](size_t i) {
        Ref<PLATEAUVectorTile> tile = tiles[i];
        if (tile.is_null() || !tile->is_success() || tile->get_coordinate().is_null()) return;

        Ref<Image> tile_img = tile->load_image();
        if (tile_img.is_null()) return;

        // Convert format if needed (this is the expensive operation)
        if (tile_img->get_format() != format) {
            // Create a copy to avoid modifying cached image
            tile_img = tile_img->duplicate();
            tile_img->convert(format);
        }

        Ref<PLATEAUTileCoordinate> coord = tile->get_coordinate();
        tile_data[i].image = tile_img;
        tile_data[i].x = (coord->get_column() - min_col) * tile_width;
        tile_data[i].y = (coord->get_row() - min_row) * tile_height;
        tile_data[i].valid = true;
    }, 1);  // Process each tile in parallel (disk I/O bound)

    // Serial blit to combined image (ensure thread safety)
    for (int i = 0; i < tile_count; i++) {
        if (tile_data[i].valid) {
            combined->blit_rect(tile_data[i].image,
                               Rect2i(0, 0, tile_width, tile_height),
                               Vector2i(tile_data[i].x, tile_data[i].y));
        }
    }

    return ImageTexture::create_from_image(combined);
}

String PLATEAUVectorTileDownloader::get_tile_path(int index) const {
    const_cast<PLATEAUVectorTileDownloader*>(this)->ensure_downloader();
    if (!downloader_ || index < 0 || index >= downloader_->getTileCount()) {
        return String();
    }

    auto path = downloader_->calcDestinationPath(index);
    return String(path.u8string().c_str());
}

String PLATEAUVectorTileDownloader::get_default_url() {
    return String(VectorTileDownloader::getDefaultUrl().c_str());
}

Ref<PLATEAUTileCoordinate> PLATEAUVectorTileDownloader::project(double latitude, double longitude, int zoom_level) {
    plateau::geometry::GeoCoordinate geo(latitude, longitude, 0);
    TileCoordinate coord = TileProjection::project(geo, zoom_level);

    Ref<PLATEAUTileCoordinate> result;
    result.instantiate();
    result->set_column(coord.column);
    result->set_row(coord.row);
    result->set_zoom_level(coord.zoom_level);

    return result;
}

Dictionary PLATEAUVectorTileDownloader::unproject(const Ref<PLATEAUTileCoordinate> &coord) {
    Dictionary result;

    if (coord.is_null()) {
        return result;
    }

    TileCoordinate native_coord(coord->get_column(), coord->get_row(), coord->get_zoom_level());
    auto extent = TileProjection::unproject(native_coord);

    result["min_lat"] = extent.min.latitude;
    result["min_lon"] = extent.min.longitude;
    result["max_lat"] = extent.max.latitude;
    result["max_lon"] = extent.max.longitude;

    return result;
}

void PLATEAUVectorTileDownloader::ensure_downloader() {
    if (downloader_) {
        return;
    }

    if (destination_.is_empty()) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader: destination is empty");
        return;
    }

    try {
        // Convert Godot path to absolute filesystem path
        String abs_dest = destination_;
        if (destination_.begins_with("user://") || destination_.begins_with("res://")) {
            abs_dest = ProjectSettings::get_singleton()->globalize_path(destination_);
        }

        // Ensure directory exists
        Ref<DirAccess> dir = DirAccess::open("user://");
        if (dir.is_valid()) {
            if (destination_.begins_with("user://")) {
                String relative_path = destination_.substr(7); // Remove "user://"
                dir->make_dir_recursive(relative_path);
            }
        }

        std::string dest = abs_dest.utf8().get_data();
        UtilityFunctions::print("PLATEAUVectorTileDownloader: Using path: ", abs_dest);

        downloader_ = std::make_unique<VectorTileDownloader>(dest, extent_, zoom_level_);

        if (!url_template_.is_empty()) {
            downloader_->setUrl(url_template_.utf8().get_data());
        }

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUVectorTileDownloader: Failed to create downloader: ", String(e.what()));
    }
}

void PLATEAUVectorTileDownloader::invalidate_downloader() {
    downloader_.reset();
}

void PLATEAUVectorTileDownloader::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_destination", "path"), &PLATEAUVectorTileDownloader::set_destination);
    ClassDB::bind_method(D_METHOD("get_destination"), &PLATEAUVectorTileDownloader::get_destination);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "destination"), "set_destination", "get_destination");

    ClassDB::bind_method(D_METHOD("set_zoom_level", "level"), &PLATEAUVectorTileDownloader::set_zoom_level);
    ClassDB::bind_method(D_METHOD("get_zoom_level"), &PLATEAUVectorTileDownloader::get_zoom_level);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "zoom_level"), "set_zoom_level", "get_zoom_level");

    ClassDB::bind_method(D_METHOD("set_tile_source", "source"), &PLATEAUVectorTileDownloader::set_tile_source);
    ClassDB::bind_method(D_METHOD("get_tile_source"), &PLATEAUVectorTileDownloader::get_tile_source);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_source"), "set_tile_source", "get_tile_source");

    ClassDB::bind_method(D_METHOD("set_url_template", "url"), &PLATEAUVectorTileDownloader::set_url_template);
    ClassDB::bind_method(D_METHOD("get_url_template"), &PLATEAUVectorTileDownloader::get_url_template);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "url_template"), "set_url_template", "get_url_template");

    ClassDB::bind_method(D_METHOD("set_extent", "min_lat", "min_lon", "max_lat", "max_lon"),
                         &PLATEAUVectorTileDownloader::set_extent);
    ClassDB::bind_method(D_METHOD("set_extent_from_center", "center_lat", "center_lon", "radius_deg"),
                         &PLATEAUVectorTileDownloader::set_extent_from_center);
    ClassDB::bind_method(D_METHOD("get_extent"), &PLATEAUVectorTileDownloader::get_extent);

    ClassDB::bind_method(D_METHOD("get_tile_count"), &PLATEAUVectorTileDownloader::get_tile_count);
    ClassDB::bind_method(D_METHOD("get_tile_coordinate", "index"), &PLATEAUVectorTileDownloader::get_tile_coordinate);
    ClassDB::bind_method(D_METHOD("download", "index"), &PLATEAUVectorTileDownloader::download);
    ClassDB::bind_method(D_METHOD("download_tile", "coord"), &PLATEAUVectorTileDownloader::download_tile);
    ClassDB::bind_method(D_METHOD("get_tile_url", "coord"), &PLATEAUVectorTileDownloader::get_tile_url);
    ClassDB::bind_method(D_METHOD("get_tile_file_path", "coord"), &PLATEAUVectorTileDownloader::get_tile_file_path);
    ClassDB::bind_method(D_METHOD("download_all"), &PLATEAUVectorTileDownloader::download_all);
    ClassDB::bind_method(D_METHOD("create_combined_texture", "tiles"),
                         &PLATEAUVectorTileDownloader::create_combined_texture);
    ClassDB::bind_method(D_METHOD("get_tile_path", "index"), &PLATEAUVectorTileDownloader::get_tile_path);

    ClassDB::bind_static_method("PLATEAUVectorTileDownloader", D_METHOD("get_default_url"),
                                &PLATEAUVectorTileDownloader::get_default_url);
    ClassDB::bind_static_method("PLATEAUVectorTileDownloader",
                                D_METHOD("project", "latitude", "longitude", "zoom_level"),
                                &PLATEAUVectorTileDownloader::project);
    ClassDB::bind_static_method("PLATEAUVectorTileDownloader", D_METHOD("unproject", "coord"),
                                &PLATEAUVectorTileDownloader::unproject);

    // Tile source enum constants
    BIND_ENUM_CONSTANT(TILE_SOURCE_GSI_PHOTO);
    BIND_ENUM_CONSTANT(TILE_SOURCE_GSI_STD);
    BIND_ENUM_CONSTANT(TILE_SOURCE_GSI_PALE);
    BIND_ENUM_CONSTANT(TILE_SOURCE_OSM);
    BIND_ENUM_CONSTANT(TILE_SOURCE_CUSTOM);
}

#endif // !PLATEAU_MOBILE_PLATFORM
