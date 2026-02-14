#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <plateau/dataset/grid_code.h>

namespace godot {

/**
 * PLATEAUGridCode - Grid code for map tile identification
 *
 * Wraps libplateau's GridCode API. Supports both standard mesh codes
 * (地域メッシュコード) and national base map grid codes (国土基本図郭).
 *
 * Usage:
 * ```gdscript
 * var code = PLATEAUGridCode.parse("53394601")
 * if code.is_valid():
 *     print("Code: ", code.get_code())         # "53394601"
 *     print("Level: ", code.get_level())        # 3
 *     print("Extent: ", code.get_extent())      # {min_lat, max_lat, min_lon, max_lon}
 *
 *     var parent = code.upper()
 *     print("Parent: ", parent.get_code())      # "533946"
 *
 *     print("Is normal GML level: ", code.is_normal_gml_level())  # true
 * ```
 */
class PLATEAUGridCode : public RefCounted {
    GDCLASS(PLATEAUGridCode, RefCounted)

public:
    PLATEAUGridCode();
    ~PLATEAUGridCode();

    /// Parse a grid code string into a PLATEAUGridCode object.
    /// Automatically detects MeshCode vs StandardMapGrid format.
    static Ref<PLATEAUGridCode> parse(const String &code);

    /// Get the code as a string (e.g., "53394601")
    String get_code() const;

    /// Get the geographic extent as Dictionary {min_lat, max_lat, min_lon, max_lon}
    Dictionary get_extent() const;

    /// Check if the code is valid
    bool is_valid() const;

    /// Get one level up (less detailed) grid code
    Ref<PLATEAUGridCode> upper() const;

    /// Get the detail level (higher = more detailed)
    int get_level() const;

    /// True if this is the broadest possible level
    bool is_largest_level() const;

    /// True if more detailed than typical GML file level
    bool is_smaller_than_normal_gml() const;

    /// True if at the typical GML file level (3rd mesh level)
    bool is_normal_gml_level() const;

    // Internal: set from native grid code
    void set_native(const std::shared_ptr<plateau::dataset::GridCode> &grid_code);

protected:
    static void _bind_methods();

private:
    std::shared_ptr<plateau::dataset::GridCode> grid_code_;
};

} // namespace godot
