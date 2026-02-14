#include "plateau_granularity_converter.h"
#include "plateau_platform.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/array_mesh.hpp>

using namespace godot;

#include "plateau_types.h"

// Type aliases for file-local types
using GranularityOption = plateau::granularityConvert::GranularityConvertOption;
using ConvertGranularity = plateau::granularityConvert::ConvertGranularity;

PLATEAUGranularityConverter::PLATEAUGranularityConverter()
    : grid_count_(1) {
}

PLATEAUGranularityConverter::~PLATEAUGranularityConverter() {
}

void PLATEAUGranularityConverter::set_grid_count(int count) {
    grid_count_ = std::max(1, count);
}

int PLATEAUGranularityConverter::get_grid_count() const {
    return grid_count_;
}

TypedArray<PLATEAUMeshData> PLATEAUGranularityConverter::convert(
    const TypedArray<PLATEAUMeshData> &mesh_data_array,
    int target_granularity) {

    TypedArray<PLATEAUMeshData> result;

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

    if (mesh_data_array.is_empty()) {
        UtilityFunctions::printerr("PLATEAUGranularityConverter: mesh_data_array is empty");
        return result;
    }

    try {
        // Convert to native model
        auto model = create_model_from_mesh_data(mesh_data_array);
        if (!model) {
            UtilityFunctions::printerr("PLATEAUGranularityConverter: Failed to create model");
            return result;
        }

        // Create converter
        plateau::granularityConvert::GranularityConverter converter;

        // Create option
        ConvertGranularity granularity;
        switch (target_granularity) {
            case CONVERT_GRANULARITY_ATOMIC:
                granularity = ConvertGranularity::PerAtomicFeatureObject;
                break;
            case CONVERT_GRANULARITY_PRIMARY:
                granularity = ConvertGranularity::PerPrimaryFeatureObject;
                break;
            case CONVERT_GRANULARITY_AREA:
                granularity = ConvertGranularity::PerCityModelArea;
                break;
            case CONVERT_GRANULARITY_MATERIAL_IN_PRIMARY:
                granularity = ConvertGranularity::MaterialInPrimary;
                break;
            default:
                UtilityFunctions::printerr("PLATEAUGranularityConverter: Invalid granularity: ", target_granularity);
                return result;
        }

        GranularityOption option(granularity, grid_count_);

        // Perform conversion
        auto converted_model = converter.convert(*model, option);

        // Convert back to Godot mesh data
        result = create_mesh_data_from_model(converted_model);

        UtilityFunctions::print("PLATEAUGranularityConverter: Converted ", mesh_data_array.size(),
                               " meshes to ", result.size(), " meshes (granularity: ", target_granularity, ")");

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGranularityConverter exception: ", String(e.what()));
    }

    return result;
}

int PLATEAUGranularityConverter::detect_granularity(const TypedArray<PLATEAUMeshData> &mesh_data_array) {
    if (mesh_data_array.is_empty()) {
        return CONVERT_GRANULARITY_AREA;
    }

    // Heuristic: check child count and naming patterns
    int total_meshes = 0;
    int meshes_with_children = 0;

    for (int i = 0; i < mesh_data_array.size(); i++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];
        if (mesh_data.is_null()) continue;

        total_meshes++;
        if (mesh_data->get_child_count() > 0) {
            meshes_with_children++;
        }
    }

    // If most meshes have children, likely primary or area
    // If no children, likely atomic
    if (meshes_with_children == 0 && total_meshes > 10) {
        return CONVERT_GRANULARITY_ATOMIC;
    } else if (meshes_with_children > total_meshes / 2) {
        return CONVERT_GRANULARITY_PRIMARY;
    } else if (total_meshes <= 1) {
        return CONVERT_GRANULARITY_AREA;
    }

    return CONVERT_GRANULARITY_PRIMARY; // Default guess
}

std::shared_ptr<PlateauModel> PLATEAUGranularityConverter::create_model_from_mesh_data(
    const TypedArray<PLATEAUMeshData> &mesh_data_array) {

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

        // Add mesh
        add_mesh_data_to_node(node, mesh_data);

        model->addNode(std::move(node));
    }

    return model;
}

void PLATEAUGranularityConverter::add_mesh_data_to_node(
    PlateauNode &parent_node,
    const Ref<PLATEAUMeshData> &mesh_data) {

    if (mesh_data.is_null()) {
        return;
    }

    Ref<ArrayMesh> godot_mesh = mesh_data->get_mesh();
    if (godot_mesh.is_valid() && godot_mesh->get_surface_count() > 0) {
        PlateauMesh native_mesh;

        Array arrays = godot_mesh->surface_get_arrays(0);
        if (arrays.size() > Mesh::ARRAY_VERTEX) {
            PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
            PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX];

            // Convert vertices
            std::vector<TVec3d> native_vertices;
            native_vertices.reserve(vertices.size());
            for (int j = 0; j < vertices.size(); j++) {
                Vector3 v = vertices[j];
                native_vertices.push_back(TVec3d(v.x, v.y, v.z));
            }

            // Convert indices
            std::vector<unsigned int> native_indices;
            native_indices.reserve(indices.size());
            for (int j = 0; j < indices.size(); j++) {
                native_indices.push_back(static_cast<unsigned int>(indices[j]));
            }

            native_mesh.addVerticesList(native_vertices);
            native_mesh.addIndicesList(native_indices, 0, false);

            // Convert UVs
            if (arrays.size() > Mesh::ARRAY_TEX_UV &&
                arrays[Mesh::ARRAY_TEX_UV].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
                PackedVector2Array uvs = arrays[Mesh::ARRAY_TEX_UV];
                std::vector<TVec2f> native_uvs;
                native_uvs.reserve(uvs.size());
                for (int j = 0; j < uvs.size(); j++) {
                    Vector2 uv = uvs[j];
                    native_uvs.push_back(TVec2f(uv.x, 1.0f - uv.y));
                }
                native_mesh.setUV1(std::move(native_uvs));
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

TypedArray<PLATEAUMeshData> PLATEAUGranularityConverter::create_mesh_data_from_model(
    const PlateauModel &model) {

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

Ref<PLATEAUMeshData> PLATEAUGranularityConverter::create_mesh_data_from_node(
    const PlateauNode &node) {

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

        // Convert indices with winding order inversion
        // libplateau outputs CW winding, but Godot expects CCW for front faces
        PackedInt32Array godot_indices;
        godot_indices.resize(indices.size());
        for (size_t j = 0; j < indices.size(); j += 3) {
            // Swap first and third vertex of each triangle to invert winding
            godot_indices.set(j, indices[j + 2]);
            godot_indices.set(j + 1, indices[j + 1]);
            godot_indices.set(j + 2, indices[j]);
        }

        // Calculate normals
        PackedVector3Array godot_normals;
        godot_normals.resize(vertices.size());
        for (int j = 0; j < godot_normals.size(); j++) {
            godot_normals.set(j, Vector3(0, 0, 0));
        }

        int face_count = godot_indices.size() / 3;
        for (int face = 0; face < face_count; face++) {
            int i0 = godot_indices[face * 3];
            int i1 = godot_indices[face * 3 + 1];
            int i2 = godot_indices[face * 3 + 2];

            if (i0 >= godot_vertices.size() || i1 >= godot_vertices.size() || i2 >= godot_vertices.size()) {
                continue;
            }

            Vector3 v0 = godot_vertices[i0];
            Vector3 v1 = godot_vertices[i1];
            Vector3 v2 = godot_vertices[i2];

            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 face_normal = edge1.cross(edge2);

            godot_normals.set(i0, godot_normals[i0] + face_normal);
            godot_normals.set(i1, godot_normals[i1] + face_normal);
            godot_normals.set(i2, godot_normals[i2] + face_normal);
        }

        for (int j = 0; j < godot_normals.size(); j++) {
            Vector3 n = godot_normals[j];
            if (n.length_squared() > 0.0001f) {
                godot_normals.set(j, n.normalized());
            } else {
                godot_normals.set(j, Vector3(0, 1, 0));
            }
        }

        // Convert UVs
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

void PLATEAUGranularityConverter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_grid_count", "count"), &PLATEAUGranularityConverter::set_grid_count);
    ClassDB::bind_method(D_METHOD("get_grid_count"), &PLATEAUGranularityConverter::get_grid_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_count"), "set_grid_count", "get_grid_count");

    ClassDB::bind_method(D_METHOD("convert", "mesh_data_array", "target_granularity"),
                         &PLATEAUGranularityConverter::convert);
    ClassDB::bind_static_method("PLATEAUGranularityConverter",
                                D_METHOD("detect_granularity", "mesh_data_array"),
                                &PLATEAUGranularityConverter::detect_granularity);

    // Granularity enum
    BIND_ENUM_CONSTANT(CONVERT_GRANULARITY_ATOMIC);
    BIND_ENUM_CONSTANT(CONVERT_GRANULARITY_PRIMARY);
    BIND_ENUM_CONSTANT(CONVERT_GRANULARITY_AREA);
    BIND_ENUM_CONSTANT(CONVERT_GRANULARITY_MATERIAL_IN_PRIMARY);
}
