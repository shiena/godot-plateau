#include "plateau_mesh_exporter.h"
#include "plateau_platform.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/array_mesh.hpp>

using namespace godot;

#include "plateau_types.h"

// Type aliases for file-local types
using GltfWriter = plateau::meshWriter::GltfWriter;
using GltfWriteOptions = plateau::meshWriter::GltfWriteOptions;
using GltfFileFormat = plateau::meshWriter::GltfFileFormat;
using ObjWriter = plateau::meshWriter::ObjWriter;

PLATEAUMeshExporter::PLATEAUMeshExporter()
    : texture_directory_("") {
}

PLATEAUMeshExporter::~PLATEAUMeshExporter() {
}

void PLATEAUMeshExporter::set_texture_directory(const String &dir) {
    texture_directory_ = dir;
}

String PLATEAUMeshExporter::get_texture_directory() const {
    return texture_directory_;
}

bool PLATEAUMeshExporter::export_to_file(
    const TypedArray<PLATEAUMeshData> &mesh_data_array,
    const String &file_path,
    int format) {

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(false);
#endif

    ERR_FAIL_COND_V_MSG(mesh_data_array.is_empty(), false, "PLATEAUMeshExporter: mesh_data_array is empty.");
    ERR_FAIL_COND_V_MSG(file_path.is_empty(), false, "PLATEAUMeshExporter: file_path is empty.");

    try {
        // Convert to native model
        auto model = create_model_from_mesh_data(mesh_data_array);
        ERR_FAIL_COND_V_MSG(!model, false, "PLATEAUMeshExporter: Failed to create model.");

        bool success = false;
        switch (format) {
            case EXPORT_FORMAT_GLTF:
                success = export_gltf(*model, file_path, false);
                break;
            case EXPORT_FORMAT_GLB:
                success = export_gltf(*model, file_path, true);
                break;
            case EXPORT_FORMAT_OBJ:
                success = export_obj(*model, file_path);
                break;
            default:
                ERR_FAIL_V_MSG(false, "PLATEAUMeshExporter: Invalid format: " + String::num_int64(format));
        }

        if (success) {
            UtilityFunctions::print("PLATEAUMeshExporter: Exported ", mesh_data_array.size(),
                                   " meshes to ", file_path);
        }

        return success;

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUMeshExporter exception: ", String(e.what()));
        return false;
    }
}

PackedStringArray PLATEAUMeshExporter::get_supported_formats() {
    PackedStringArray formats;
    formats.push_back("glTF");
    formats.push_back("GLB");
    formats.push_back("OBJ");
    return formats;
}

String PLATEAUMeshExporter::get_format_extension(int format) {
    switch (format) {
        case EXPORT_FORMAT_GLTF:
            return "gltf";
        case EXPORT_FORMAT_GLB:
            return "glb";
        case EXPORT_FORMAT_OBJ:
            return "obj";
        default:
            return "";
    }
}

std::shared_ptr<PlateauModel> PLATEAUMeshExporter::create_model_from_mesh_data(
    const TypedArray<PLATEAUMeshData> &mesh_data_array) {

    auto model = std::make_shared<PlateauModel>();

    for (int i = 0; i < mesh_data_array.size(); i++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];
        if (mesh_data.is_null()) {
            continue;
        }

        auto node = PlateauNode(mesh_data->get_name().utf8().get_data());

        // Set transform (position, rotation, scale)
        Transform3D t = mesh_data->get_transform();
        node.setLocalPosition(TVec3d(t.origin.x, t.origin.y, t.origin.z));

        // Extract rotation and scale from basis
        godot::Quaternion godot_quat = t.basis.get_rotation_quaternion();
        Vector3 godot_scale = t.basis.get_scale();

        node.setLocalRotation(plateau::polygonMesh::Quaternion(
            godot_quat.x, godot_quat.y, godot_quat.z, godot_quat.w));
        node.setLocalScale(TVec3d(godot_scale.x, godot_scale.y, godot_scale.z));

        // Add mesh
        add_mesh_data_to_node(node, mesh_data);

        model->addNode(std::move(node));
    }

    return model;
}

void PLATEAUMeshExporter::add_mesh_data_to_node(
    PlateauNode &parent_node,
    const Ref<PLATEAUMeshData> &mesh_data) {

    if (mesh_data.is_null()) {
        return;
    }

    Ref<ArrayMesh> godot_mesh = mesh_data->get_mesh();
    PackedStringArray texture_paths = mesh_data->get_texture_paths();

    if (godot_mesh.is_valid() && godot_mesh->get_surface_count() > 0) {
        PlateauMesh native_mesh;
        int surface_count = godot_mesh->get_surface_count();

        // Collect all vertices, indices, and UVs from all surfaces
        // Track index ranges for each surface to create separate submeshes
        std::vector<TVec3d> all_vertices;
        std::vector<unsigned int> all_indices;
        std::vector<TVec2f> all_uvs;
        unsigned int vertex_offset = 0;

        // Store submesh info: (start_index, end_index, texture_path)
        struct SubMeshInfo {
            size_t start_index;
            size_t end_index;
            std::string texture_path;
        };
        std::vector<SubMeshInfo> submesh_infos;

        for (int surf_idx = 0; surf_idx < surface_count; surf_idx++) {
            Array arrays = godot_mesh->surface_get_arrays(surf_idx);
            if (arrays.size() <= Mesh::ARRAY_VERTEX) {
                continue;
            }

            Variant vertex_var = arrays[Mesh::ARRAY_VERTEX];
            if (vertex_var.get_type() != Variant::PACKED_VECTOR3_ARRAY) {
                continue;
            }

            PackedVector3Array vertices = vertex_var;
            if (vertices.is_empty()) {
                continue;
            }

            // Get indices
            PackedInt32Array indices;
            if (arrays.size() > Mesh::ARRAY_INDEX &&
                arrays[Mesh::ARRAY_INDEX].get_type() == Variant::PACKED_INT32_ARRAY) {
                indices = arrays[Mesh::ARRAY_INDEX];
            }
            if (indices.is_empty()) {
                continue;
            }

            // Record submesh start index
            size_t submesh_start = all_indices.size();

            // Add vertices
            for (int j = 0; j < vertices.size(); j++) {
                Vector3 v = vertices[j];
                all_vertices.push_back(TVec3d(v.x, v.y, v.z));
            }

            // Add indices with offset
            for (int j = 0; j < indices.size(); j++) {
                all_indices.push_back(static_cast<unsigned int>(indices[j]) + vertex_offset);
            }

            // Add UVs
            if (arrays.size() > Mesh::ARRAY_TEX_UV &&
                arrays[Mesh::ARRAY_TEX_UV].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
                PackedVector2Array uvs = arrays[Mesh::ARRAY_TEX_UV];
                for (int j = 0; j < uvs.size(); j++) {
                    Vector2 uv = uvs[j];
                    all_uvs.push_back(TVec2f(uv.x, 1.0f - uv.y));
                }
            } else {
                // Add placeholder UVs
                for (int j = 0; j < vertices.size(); j++) {
                    all_uvs.push_back(TVec2f(0.0f, 0.0f));
                }
            }

            // Record submesh end index and texture path
            size_t submesh_end = all_indices.size() - 1;
            std::string tex_path = "";
            if (surf_idx < texture_paths.size()) {
                tex_path = texture_paths[surf_idx].utf8().get_data();
            }
            submesh_infos.push_back({submesh_start, submesh_end, tex_path});

            vertex_offset += static_cast<unsigned int>(vertices.size());
        }

        // Add to native mesh if valid data collected
        if (!all_vertices.empty() && !all_indices.empty() && all_indices.size() % 3 == 0) {
            native_mesh.addVerticesList(all_vertices);
            native_mesh.addIndicesList(all_indices, 0, false);

            // Add each submesh with its texture path
            for (const auto &info : submesh_infos) {
                native_mesh.addSubMesh(info.texture_path, nullptr, info.start_index, info.end_index, -1);
            }

            if (!all_uvs.empty() && all_uvs.size() == all_vertices.size()) {
                native_mesh.setUV1(std::move(all_uvs));
            }

            // Set city object list if available
            const auto &city_object_list = mesh_data->get_city_object_list_internal();
            native_mesh.setCityObjectList(city_object_list);

            parent_node.setMesh(std::make_unique<PlateauMesh>(std::move(native_mesh)));
        }
    }

    // Process children recursively
    TypedArray<PLATEAUMeshData> children = mesh_data->get_children();
    for (int i = 0; i < children.size(); i++) {
        Ref<PLATEAUMeshData> child = children[i];
        if (child.is_valid()) {
            auto child_node = PlateauNode(child->get_name().utf8().get_data());
            Transform3D t = child->get_transform();
            child_node.setLocalPosition(TVec3d(t.origin.x, t.origin.y, t.origin.z));

            add_mesh_data_to_node(child_node, child);
            parent_node.addChildNode(std::move(child_node));
        }
    }
}

bool PLATEAUMeshExporter::export_gltf(
    const PlateauModel &model,
    const String &file_path,
    bool binary) {

    GltfWriter writer;
    GltfWriteOptions options;

    options.mesh_file_format = binary ? GltfFileFormat::GLB : GltfFileFormat::GLTF;
    options.texture_directory_path = texture_directory_.utf8().get_data();

    std::string path = file_path.utf8().get_data();
    return writer.write(path, model, options);
}

bool PLATEAUMeshExporter::export_obj(
    const PlateauModel &model,
    const String &file_path) {

    ObjWriter writer;
    std::string path = file_path.utf8().get_data();
    return writer.write(path, model);
}

void PLATEAUMeshExporter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_texture_directory", "dir"), &PLATEAUMeshExporter::set_texture_directory);
    ClassDB::bind_method(D_METHOD("get_texture_directory"), &PLATEAUMeshExporter::get_texture_directory);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "texture_directory"), "set_texture_directory", "get_texture_directory");

    ClassDB::bind_method(D_METHOD("export_to_file", "mesh_data_array", "file_path", "format"),
                         &PLATEAUMeshExporter::export_to_file);

    ClassDB::bind_static_method("PLATEAUMeshExporter",
                                D_METHOD("get_supported_formats"),
                                &PLATEAUMeshExporter::get_supported_formats);
    ClassDB::bind_static_method("PLATEAUMeshExporter",
                                D_METHOD("get_format_extension", "format"),
                                &PLATEAUMeshExporter::get_format_extension);

    // Export format enum
    BIND_ENUM_CONSTANT(EXPORT_FORMAT_GLTF);
    BIND_ENUM_CONSTANT(EXPORT_FORMAT_GLB);
    BIND_ENUM_CONSTANT(EXPORT_FORMAT_OBJ);
}
