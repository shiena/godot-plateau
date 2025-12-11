#include "plateau_mesh_utils.h"
#include "plateau_parallel.h"
#include <godot_cpp/classes/mesh.hpp>

namespace godot {
namespace plateau_utils {

PackedVector3Array compute_smooth_normals(
    const PackedVector3Array &vertices,
    const PackedInt32Array &indices) {

    int vertex_count = vertices.size();
    PackedVector3Array normals;
    normals.resize(vertex_count);

    // Initialize all normals to zero (parallel)
    plateau_parallel::parallel_for(0, static_cast<size_t>(vertex_count), [&](size_t i) {
        normals.set(i, Vector3(0, 0, 0));
    });

    // Calculate face normals using thread-local buffers to avoid data races
    int face_count = indices.size() / 3;
    using NormalBuffer = std::vector<Vector3>;

    plateau_parallel::parallel_for_reduce<NormalBuffer>(
        0, static_cast<size_t>(face_count),
        [vertex_count]() {
            return NormalBuffer(vertex_count, Vector3(0, 0, 0));
        },
        [&](size_t face, NormalBuffer& local_normals) {
            int i0 = indices[face * 3];
            int i1 = indices[face * 3 + 1];
            int i2 = indices[face * 3 + 2];

            if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count) {
                return;
            }

            Vector3 v0 = vertices[i0];
            Vector3 v1 = vertices[i1];
            Vector3 v2 = vertices[i2];

            // Calculate face normal (cross product) - not normalized for area weighting
            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 face_normal = edge1.cross(edge2);

            // Accumulate to thread-local buffer
            local_normals[i0] += face_normal;
            local_normals[i1] += face_normal;
            local_normals[i2] += face_normal;
        },
        [&](NormalBuffer& local_normals) {
            for (int i = 0; i < vertex_count; i++) {
                normals.set(i, normals[i] + local_normals[i]);
            }
        },
        500
    );

    // Normalize all normals (parallel)
    plateau_parallel::parallel_for(0, static_cast<size_t>(vertex_count), [&](size_t i) {
        Vector3 n = normals[i];
        if (n.length_squared() > 0.0001f) {
            normals.set(i, n.normalized());
        } else {
            normals.set(i, Vector3(0, 1, 0)); // Default up normal
        }
    });

    return normals;
}

void extract_mesh_arrays(
    const Ref<ArrayMesh> &godot_mesh,
    std::vector<TVec3d> &out_vertices,
    std::vector<unsigned int> &out_indices,
    std::vector<TVec2f> &out_uvs) {

    out_vertices.clear();
    out_indices.clear();
    out_uvs.clear();

    if (godot_mesh.is_null() || godot_mesh->get_surface_count() == 0) {
        return;
    }

    unsigned int vertex_offset = 0;

    for (int surf_idx = 0; surf_idx < godot_mesh->get_surface_count(); surf_idx++) {
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

        // Add vertices
        for (int j = 0; j < vertices.size(); j++) {
            Vector3 v = vertices[j];
            out_vertices.push_back(TVec3d(v.x, v.y, v.z));
        }

        // Add indices with offset
        for (int j = 0; j < indices.size(); j++) {
            out_indices.push_back(static_cast<unsigned int>(indices[j]) + vertex_offset);
        }

        // Add UVs (with Y-flip for libplateau)
        if (arrays.size() > Mesh::ARRAY_TEX_UV &&
            arrays[Mesh::ARRAY_TEX_UV].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
            PackedVector2Array uvs = arrays[Mesh::ARRAY_TEX_UV];
            for (int j = 0; j < uvs.size(); j++) {
                Vector2 uv = uvs[j];
                out_uvs.push_back(TVec2f(uv.x, 1.0f - uv.y));
            }
        } else {
            // Add placeholder UVs
            for (int j = 0; j < vertices.size(); j++) {
                out_uvs.push_back(TVec2f(0.0f, 0.0f));
            }
        }

        vertex_offset += static_cast<unsigned int>(vertices.size());
    }
}

void add_mesh_data_to_node(
    PlateauNode &parent_node,
    const Ref<PLATEAUMeshData> &mesh_data,
    bool merge_surfaces,
    bool include_uv4) {

    if (mesh_data.is_null()) {
        return;
    }

    Ref<ArrayMesh> godot_mesh = mesh_data->get_mesh();
    if (godot_mesh.is_valid() && godot_mesh->get_surface_count() > 0) {
        PlateauMesh native_mesh;

        std::vector<TVec3d> all_vertices;
        std::vector<unsigned int> all_indices;
        std::vector<TVec2f> all_uvs;

        extract_mesh_arrays(godot_mesh, all_vertices, all_indices, all_uvs);

        // Add to native mesh if valid data collected
        if (!all_vertices.empty() && !all_indices.empty() && all_indices.size() % 3 == 0) {
            native_mesh.addVerticesList(all_vertices);
            native_mesh.addIndicesList(all_indices, 0, false);
            native_mesh.addSubMesh("", nullptr, 0, all_indices.size() - 1, -1);

            if (!all_uvs.empty()) {
                if (include_uv4) {
                    // HeightMapAligner requires both UV1 and UV4
                    std::vector<TVec2f> all_uvs_copy = all_uvs;
                    native_mesh.setUV1(std::move(all_uvs));
                    native_mesh.setUV4(std::move(all_uvs_copy));
                } else {
                    native_mesh.setUV1(std::move(all_uvs));
                }
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

            add_mesh_data_to_node(child_node, child, merge_surfaces, include_uv4);
            parent_node.addChildNode(std::move(child_node));
        }
    }
}

std::shared_ptr<PlateauModel> create_model_from_mesh_data(
    const TypedArray<PLATEAUMeshData> &mesh_data_array,
    bool assign_hierarchy) {

    auto model = std::make_shared<PlateauModel>();

    for (int i = 0; i < mesh_data_array.size(); i++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];
        if (mesh_data.is_null()) {
            continue;
        }

        auto node = PlateauNode(mesh_data->get_name().utf8().get_data());

        // Set transform
        Transform3D t = mesh_data->get_transform();
        node.setLocalPosition(TVec3d(t.origin.x, t.origin.y, t.origin.z));

        // Add mesh (include UV4 for HeightMapAligner compatibility)
        add_mesh_data_to_node(node, mesh_data, true, assign_hierarchy);

        model->addNode(std::move(node));
    }

    if (assign_hierarchy) {
        model->assignNodeHierarchy();
    }

    return model;
}

Ref<PLATEAUMeshData> create_mesh_data_from_node(const PlateauNode &node) {
    Ref<PLATEAUMeshData> mesh_data;
    mesh_data.instantiate();

    mesh_data->set_name(String(node.getName().c_str()));

    // Set transform
    auto pos = node.getLocalPosition();
    Transform3D transform;
    transform.origin = Vector3(pos.x, pos.y, pos.z);
    mesh_data->set_transform(transform);

    // Convert mesh
    const PlateauMesh *native_mesh = node.getMesh();
    if (native_mesh != nullptr && native_mesh->hasVertices()) {
        const auto &vertices = native_mesh->getVertices();
        const auto &indices = native_mesh->getIndices();
        const auto &uv1 = native_mesh->getUV1();

        // Convert vertices
        PackedVector3Array godot_vertices;
        godot_vertices.resize(vertices.size());
        for (size_t j = 0; j < vertices.size(); j++) {
            const auto &v = vertices[j];
            godot_vertices.set(j, Vector3(v.x, v.y, v.z));
        }

        // Convert indices
        PackedInt32Array godot_indices;
        godot_indices.resize(indices.size());
        for (size_t j = 0; j < indices.size(); j++) {
            godot_indices.set(j, indices[j]);
        }

        // Calculate normals using shared utility
        PackedVector3Array godot_normals = compute_smooth_normals(godot_vertices, godot_indices);

        // Convert UVs (with Y-flip back to Godot)
        PackedVector2Array godot_uvs;
        if (!uv1.empty()) {
            godot_uvs.resize(uv1.size());
            for (size_t j = 0; j < uv1.size(); j++) {
                const auto &uv = uv1[j];
                godot_uvs.set(j, Vector2(uv.x, 1.0f - uv.y));
            }
        }

        // Create mesh
        Ref<ArrayMesh> godot_mesh;
        godot_mesh.instantiate();

        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = godot_vertices;
        arrays[Mesh::ARRAY_INDEX] = godot_indices;
        arrays[Mesh::ARRAY_NORMAL] = godot_normals;
        if (!godot_uvs.is_empty()) {
            arrays[Mesh::ARRAY_TEX_UV] = godot_uvs;
        }

        godot_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
        mesh_data->set_mesh(godot_mesh);

        // Set city object list
        const auto &city_object_list = native_mesh->getCityObjectList();
        mesh_data->set_city_object_list(city_object_list);
    }

    // Process children
    for (size_t i = 0; i < node.getChildCount(); i++) {
        const auto &child_node = node.getChildAt(i);
        auto child_data = create_mesh_data_from_node(child_node);
        if (child_data.is_valid()) {
            mesh_data->add_child(child_data);
        }
    }

    return mesh_data;
}

TypedArray<PLATEAUMeshData> create_mesh_data_from_model(const PlateauModel &model) {
    TypedArray<PLATEAUMeshData> result;

    for (size_t i = 0; i < model.getRootNodeCount(); i++) {
        const auto &node = model.getRootNodeAt(i);
        auto mesh_data = create_mesh_data_from_node(node);
        if (mesh_data.is_valid()) {
            result.push_back(mesh_data);
        }
    }

    return result;
}

Ref<ArrayMesh> create_array_mesh(
    const std::vector<TVec3d> &vertices,
    const std::vector<unsigned int> &indices,
    const std::vector<TVec2f> &uvs) {

    Ref<ArrayMesh> array_mesh;
    array_mesh.instantiate();

    if (vertices.empty() || indices.empty()) {
        return array_mesh;
    }

    // Convert vertices
    PackedVector3Array godot_vertices;
    godot_vertices.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        const auto &v = vertices[i];
        godot_vertices.set(i, Vector3(v.x, v.y, v.z));
    }

    // Convert indices
    PackedInt32Array godot_indices;
    godot_indices.resize(indices.size());
    for (size_t i = 0; i < indices.size(); i++) {
        godot_indices.set(i, indices[i]);
    }

    // Calculate normals
    PackedVector3Array godot_normals = compute_smooth_normals(godot_vertices, godot_indices);

    // Convert UVs (with Y-flip)
    PackedVector2Array godot_uvs;
    if (!uvs.empty()) {
        godot_uvs.resize(uvs.size());
        for (size_t i = 0; i < uvs.size(); i++) {
            const auto &uv = uvs[i];
            godot_uvs.set(i, Vector2(uv.x, 1.0f - uv.y));
        }
    }

    // Create surface arrays
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = godot_vertices;
    arrays[Mesh::ARRAY_INDEX] = godot_indices;
    arrays[Mesh::ARRAY_NORMAL] = godot_normals;
    if (!godot_uvs.is_empty()) {
        arrays[Mesh::ARRAY_TEX_UV] = godot_uvs;
    }

    array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    return array_mesh;
}

} // namespace plateau_utils
} // namespace godot
