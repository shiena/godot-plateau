#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <plateau/mesh_writer/gltf_writer.h>
#include <plateau/mesh_writer/obj_writer.h>
#include <plateau/polygon_mesh/model.h>

#include "plateau_city_model.h"

namespace godot {

// Export file format
enum PLATEAUExportFormat : int {
    EXPORT_FORMAT_GLTF = 0,
    EXPORT_FORMAT_GLB = 1,
    EXPORT_FORMAT_OBJ = 2,
};

/**
 * PLATEAUMeshExporter - Export mesh data to various file formats
 *
 * Supports exporting PLATEAUMeshData to:
 * - glTF (.gltf) - with separate binary and texture files
 * - GLB (.glb) - binary glTF with embedded data
 * - OBJ (.obj) - Wavefront OBJ with MTL material file
 *
 * Usage:
 * ```gdscript
 * var exporter = PLATEAUMeshExporter.new()
 *
 * # Export to GLB (binary glTF)
 * exporter.export_to_file(
 *     mesh_data_array,
 *     "C:/output/model.glb",
 *     PLATEAUExportFormat.EXPORT_FORMAT_GLB
 * )
 *
 * # Export to glTF with custom texture directory
 * exporter.texture_directory = "textures"
 * exporter.export_to_file(
 *     mesh_data_array,
 *     "C:/output/model.gltf",
 *     PLATEAUExportFormat.EXPORT_FORMAT_GLTF
 * )
 *
 * # Export to OBJ
 * exporter.export_to_file(
 *     mesh_data_array,
 *     "C:/output/model.obj",
 *     PLATEAUExportFormat.EXPORT_FORMAT_OBJ
 * )
 * ```
 */
class PLATEAUMeshExporter : public RefCounted {
    GDCLASS(PLATEAUMeshExporter, RefCounted)

public:
    PLATEAUMeshExporter();
    ~PLATEAUMeshExporter();

    // Texture directory path (relative to output file, for glTF only)
    void set_texture_directory(const String &dir);
    String get_texture_directory() const;

    // Export mesh data to file
    // Returns true on success, false on failure
    bool export_to_file(
        const TypedArray<PLATEAUMeshData> &mesh_data_array,
        const String &file_path,
        int format);

    // Get supported export formats as array of strings
    static PackedStringArray get_supported_formats();

    // Get file extension for format
    static String get_format_extension(int format);

protected:
    static void _bind_methods();

private:
    String texture_directory_;

    // Internal helper: Convert mesh data to plateau model
    std::shared_ptr<plateau::polygonMesh::Model> create_model_from_mesh_data(
        const TypedArray<PLATEAUMeshData> &mesh_data_array);

    // Internal helper: Add mesh data to node recursively
    void add_mesh_data_to_node(
        plateau::polygonMesh::Node &parent_node,
        const Ref<PLATEAUMeshData> &mesh_data);

    // Internal export methods
    bool export_gltf(
        const plateau::polygonMesh::Model &model,
        const String &file_path,
        bool binary);

    bool export_obj(
        const plateau::polygonMesh::Model &model,
        const String &file_path);
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PLATEAUExportFormat);
