#include "plateau_height_map_aligner.h"
#include "plateau_platform.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>

#ifndef PLATEAU_MOBILE_PLATFORM
#include <plateau/height_map_alighner/height_map_aligner.h>
#endif

#include "plateau_types.h"

using namespace godot;

#ifndef PLATEAU_MOBILE_PLATFORM
// Type aliases for file-local types
using HeightMapFrame = plateau::heightMapAligner::HeightMapFrame;
using HeightMapAligner = plateau::heightMapAligner::HeightMapAligner;
#endif

PLATEAUHeightMapAligner::PLATEAUHeightMapAligner()
    : height_offset_(0.0),
      max_edge_length_(10.0f),  // Increased from 4.0 to reduce mesh subdivision
      alpha_expand_width_(2),
      alpha_averaging_width_(2),
      invert_height_offset_(-0.15),
      skip_threshold_distance_(0.5f) {
}

PLATEAUHeightMapAligner::~PLATEAUHeightMapAligner() {
}

void PLATEAUHeightMapAligner::set_height_offset(double offset) {
    height_offset_ = offset;
}

double PLATEAUHeightMapAligner::get_height_offset() const {
    return height_offset_;
}

void PLATEAUHeightMapAligner::set_max_edge_length(float length) {
    max_edge_length_ = std::max(0.1f, length);
}

float PLATEAUHeightMapAligner::get_max_edge_length() const {
    return max_edge_length_;
}

void PLATEAUHeightMapAligner::set_alpha_expand_width(int width) {
    alpha_expand_width_ = std::max(0, width);
}

int PLATEAUHeightMapAligner::get_alpha_expand_width() const {
    return alpha_expand_width_;
}

void PLATEAUHeightMapAligner::set_alpha_averaging_width(int width) {
    alpha_averaging_width_ = std::max(0, width);
}

int PLATEAUHeightMapAligner::get_alpha_averaging_width() const {
    return alpha_averaging_width_;
}

void PLATEAUHeightMapAligner::set_invert_height_offset(double offset) {
    invert_height_offset_ = offset;
}

double PLATEAUHeightMapAligner::get_invert_height_offset() const {
    return invert_height_offset_;
}

void PLATEAUHeightMapAligner::set_skip_threshold_distance(float distance) {
    skip_threshold_distance_ = std::max(0.0f, distance);
}

float PLATEAUHeightMapAligner::get_skip_threshold_distance() const {
    return skip_threshold_distance_;
}

void PLATEAUHeightMapAligner::add_heightmap(const Ref<PLATEAUHeightMapData> &heightmap_data) {
    ERR_FAIL_COND_MSG(heightmap_data.is_null(), "PLATEAUHeightMapAligner: heightmap_data is null.");
    ERR_FAIL_COND_MSG(heightmap_data->get_width() <= 0 || heightmap_data->get_height() <= 0, "PLATEAUHeightMapAligner: heightmap_data has invalid dimensions.");

    heightmap_refs_.push_back(heightmap_data);
    UtilityFunctions::print("Added heightmap: ", heightmap_data->get_name(),
                           " (", heightmap_data->get_width(), "x", heightmap_data->get_height(), ")");
}

void PLATEAUHeightMapAligner::clear_heightmaps() {
    heightmap_refs_.clear();
}

int PLATEAUHeightMapAligner::get_heightmap_count() const {
    return static_cast<int>(heightmap_refs_.size());
}

TypedArray<PLATEAUMeshData> PLATEAUHeightMapAligner::align(const TypedArray<PLATEAUMeshData> &mesh_data_array) {
    TypedArray<PLATEAUMeshData> result;

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    ERR_FAIL_COND_V_MSG(heightmap_refs_.empty(), result, "PLATEAUHeightMapAligner: no heightmaps registered.");
    ERR_FAIL_COND_V_MSG(mesh_data_array.is_empty(), result, "PLATEAUHeightMapAligner: mesh_data_array is empty.");

    try {
        // Create HeightMapAligner with ENU coordinate system (Godot's coordinate system)
        HeightMapAligner aligner(height_offset_, plateau::geometry::CoordinateSystem::ENU);

        // Add all heightmap frames
        for (size_t hm_idx = 0; hm_idx < heightmap_refs_.size(); hm_idx++) {
            const auto &heightmap_ref = heightmap_refs_[hm_idx];
            TVec3d min = heightmap_ref->get_min_internal();
            TVec3d max = heightmap_ref->get_max_internal();
            const auto &heightmap_data = heightmap_ref->get_heightmap_internal();

            // Use ENU (East-North-Up) since data is already in this coordinate system
            // HeightMapFrame constructor will convert from specified axis to ENU internally
            HeightMapFrame frame(
                heightmap_data,
                heightmap_ref->get_width(),
                heightmap_ref->get_height(),
                static_cast<float>(min.x), static_cast<float>(max.x),
                static_cast<float>(min.y), static_cast<float>(max.y),
                static_cast<float>(min.z), static_cast<float>(max.z),
                plateau::geometry::CoordinateSystem::ENU  // Changed from EUN to ENU (no conversion needed)
            );

            aligner.addHeightmapFrame(frame);
        }

        // Create model from mesh data
        auto model = create_model_from_mesh_data(mesh_data_array);
        if (!model) {
            UtilityFunctions::printerr("PLATEAUHeightMapAligner: failed to create model from mesh data");
            return result;
        }

        // Perform alignment
        aligner.align(*model, max_edge_length_);

        // Copy input to result and update with aligned data
        for (int i = 0; i < mesh_data_array.size(); i++) {
            result.push_back(mesh_data_array[i]);
        }
        update_mesh_data_from_model(result, *model);

        UtilityFunctions::print("Aligned ", result.size(), " mesh(es) to terrain");

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception during alignment: ", String(e.what()));
    }
#endif

    return result;
}

TypedArray<PLATEAUHeightMapData> PLATEAUHeightMapAligner::align_invert(const TypedArray<PLATEAUMeshData> &mesh_data_array) {
    TypedArray<PLATEAUHeightMapData> result;

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    ERR_FAIL_COND_V_MSG(heightmap_refs_.empty(), result, "PLATEAUHeightMapAligner: no heightmaps registered.");
    ERR_FAIL_COND_V_MSG(mesh_data_array.is_empty(), result, "PLATEAUHeightMapAligner: mesh_data_array is empty.");

    try {
        // Create HeightMapAligner
        HeightMapAligner aligner(height_offset_, plateau::geometry::CoordinateSystem::EUN);

        // Add all heightmap frames
        for (const auto &heightmap_ref : heightmap_refs_) {
            TVec3d min = heightmap_ref->get_min_internal();
            TVec3d max = heightmap_ref->get_max_internal();
            const auto &heightmap_data = heightmap_ref->get_heightmap_internal();

            HeightMapFrame frame(
                heightmap_data,
                heightmap_ref->get_width(),
                heightmap_ref->get_height(),
                static_cast<float>(min.x), static_cast<float>(max.x),
                static_cast<float>(min.y), static_cast<float>(max.y),
                static_cast<float>(min.z), static_cast<float>(max.z),
                plateau::geometry::CoordinateSystem::EUN
            );

            aligner.addHeightmapFrame(frame);
        }

        // Create model from mesh data
        auto model = create_model_from_mesh_data(mesh_data_array);
        if (!model) {
            UtilityFunctions::printerr("PLATEAUHeightMapAligner: failed to create model from mesh data");
            return result;
        }

        // Perform inverse alignment
        aligner.alignInvert(*model,
                           alpha_expand_width_,
                           alpha_averaging_width_,
                           invert_height_offset_,
                           skip_threshold_distance_);

        // Extract updated heightmaps
        for (int i = 0; i < aligner.heightmapCount(); i++) {
            auto &updated_frame = aligner.getHeightMapFrameAt(i);

            Ref<PLATEAUHeightMapData> new_data;
            new_data.instantiate();

            if (i < static_cast<int>(heightmap_refs_.size())) {
                new_data->set_name(heightmap_refs_[i]->get_name());
                new_data->set_texture_path(heightmap_refs_[i]->get_texture_path());
            }

            new_data->set_data(
                updated_frame.heightmap,
                updated_frame.map_width,
                updated_frame.map_height,
                TVec3d(updated_frame.min_x, updated_frame.min_y, updated_frame.min_height),
                TVec3d(updated_frame.max_x, updated_frame.max_y, updated_frame.max_height),
                TVec2f(0, 0),  // UV will need to be recalculated if needed
                TVec2f(1, 1)
            );

            result.push_back(new_data);
        }

        UtilityFunctions::print("Inverse aligned ", result.size(), " heightmap(s) to model");

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception during inverse alignment: ", String(e.what()));
    }
#endif

    return result;
}

double PLATEAUHeightMapAligner::get_height_at(const Vector2 &xz_position) const {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(std::nan(""));
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (heightmap_refs_.empty()) {
        return std::nan("");
    }

    // Check each heightmap to find one that contains this position
    for (const auto &heightmap_ref : heightmap_refs_) {
        TVec3d min = heightmap_ref->get_min_internal();
        TVec3d max = heightmap_ref->get_max_internal();

        // Check if position is within this heightmap's bounds
        if (xz_position.x >= min.x && xz_position.x <= max.x &&
            xz_position.y >= min.z && xz_position.y <= max.z) {

            // Create a temporary HeightMapFrame to use posToHeight
            const auto &heightmap_data = heightmap_ref->get_heightmap_internal();
            HeightMapFrame frame(
                heightmap_data,
                heightmap_ref->get_width(),
                heightmap_ref->get_height(),
                static_cast<float>(min.x), static_cast<float>(max.x),
                static_cast<float>(min.y), static_cast<float>(max.y),
                static_cast<float>(min.z), static_cast<float>(max.z),
                plateau::geometry::CoordinateSystem::EUN
            );

            return frame.posToHeight(TVec2d(xz_position.x, xz_position.y), height_offset_);
        }
    }

    return std::nan("");
#endif
}

#ifndef PLATEAU_MOBILE_PLATFORM
std::shared_ptr<PlateauModel> PLATEAUHeightMapAligner::create_model_from_mesh_data(
    const TypedArray<PLATEAUMeshData> &mesh_data_array) {

    auto model = std::make_shared<PlateauModel>();

    for (int i = 0; i < mesh_data_array.size(); i++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];
        if (mesh_data.is_null()) {
            continue;
        }

        // Create a root node for each mesh data
        auto node = PlateauNode(mesh_data->get_name().utf8().get_data());

        // Set transform
        Transform3D t = mesh_data->get_transform();
        node.setLocalPosition(TVec3d(t.origin.x, t.origin.y, t.origin.z));

        // Add mesh if present
        add_mesh_data_to_node(node, mesh_data);

        model->addNode(std::move(node));
    }

    // Assign node hierarchy (required for HeightMapAligner)
    model->assignNodeHierarchy();

    return model;
}

void PLATEAUHeightMapAligner::add_mesh_data_to_node(
    PlateauNode &parent_node,
    const Ref<PLATEAUMeshData> &mesh_data) {

    if (mesh_data.is_null()) {
        return;
    }

    Ref<ArrayMesh> godot_mesh = mesh_data->get_mesh();
    if (godot_mesh.is_valid() && godot_mesh->get_surface_count() > 0) {
        // Convert Godot mesh to plateau mesh
        PlateauMesh native_mesh;

        // Collect all vertices and indices from all surfaces
        std::vector<TVec3d> all_vertices;
        std::vector<unsigned int> all_indices;
        std::vector<TVec2f> all_uvs;
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

            vertex_offset += static_cast<unsigned int>(vertices.size());
        }

        // Add to native mesh if valid data collected
        if (!all_vertices.empty() && !all_indices.empty() && all_indices.size() % 3 == 0) {
            native_mesh.addVerticesList(all_vertices);
            native_mesh.addIndicesList(all_indices, 0, false);
            native_mesh.addSubMesh("", nullptr, 0, all_indices.size() - 1, -1);

            // Set UVs if collected (both UV1 and UV4 are required by OpenMeshConverter)
            if (!all_uvs.empty()) {
                // Create a copy for UV4 (HeightMapAligner requires both UV1 and UV4)
                std::vector<TVec2f> all_uvs_copy = all_uvs;
                native_mesh.setUV1(std::move(all_uvs));
                native_mesh.setUV4(std::move(all_uvs_copy));
            }

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

// Helper function to update a single mesh from node
void PLATEAUHeightMapAligner::update_single_mesh_from_node(
    Ref<PLATEAUMeshData> &mesh_data,
    const PlateauNode &node) {

    const PlateauMesh *native_mesh = node.getMesh();
    if (native_mesh == nullptr || !native_mesh->hasVertices()) {
        return;
    }

    Ref<ArrayMesh> godot_mesh = mesh_data->get_mesh();
    if (godot_mesh.is_null() || godot_mesh->get_surface_count() == 0) {
        return;
    }

    // Get updated vertices from native mesh
    const auto &native_vertices = native_mesh->getVertices();
    const auto &native_indices = native_mesh->getIndices();
    const auto &native_uvs = native_mesh->getUV1();

    // Convert to Godot arrays
    PackedVector3Array godot_vertices;
    godot_vertices.resize(native_vertices.size());
    for (size_t j = 0; j < native_vertices.size(); j++) {
        const auto &v = native_vertices[j];
        godot_vertices.set(j, Vector3(v.x, v.y, v.z));
    }

    // Convert indices with winding order inversion
    // libplateau outputs CW winding, but Godot expects CCW for front faces
    PackedInt32Array godot_indices;
    godot_indices.resize(native_indices.size());
    for (size_t j = 0; j < native_indices.size(); j += 3) {
        // Swap first and third vertex of each triangle to invert winding
        godot_indices.set(j, native_indices[j + 2]);
        godot_indices.set(j + 1, native_indices[j + 1]);
        godot_indices.set(j + 2, native_indices[j]);
    }

    // Recompute normals
    PackedVector3Array godot_normals;
    godot_normals.resize(native_vertices.size());
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
    if (!native_uvs.empty()) {
        godot_uvs.resize(native_uvs.size());
        for (size_t j = 0; j < native_uvs.size(); j++) {
            const auto &uv = native_uvs[j];
            godot_uvs.set(j, Vector2(uv.x, 1.0f - uv.y));
        }
    }

    // Create new mesh
    Ref<ArrayMesh> new_mesh;
    new_mesh.instantiate();

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = godot_vertices;
    arrays[Mesh::ARRAY_INDEX] = godot_indices;
    arrays[Mesh::ARRAY_NORMAL] = godot_normals;
    if (!godot_uvs.is_empty()) {
        arrays[Mesh::ARRAY_TEX_UV] = godot_uvs;
    }

    new_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    // Copy material from original mesh
    if (godot_mesh->get_surface_count() > 0) {
        Ref<Material> material = godot_mesh->surface_get_material(0);
        if (material.is_valid()) {
            new_mesh->surface_set_material(0, material);
        }
    }

    mesh_data->set_mesh(new_mesh);
}

// Recursive helper to update mesh data from node hierarchy
void PLATEAUHeightMapAligner::update_node_recursive(
    Ref<PLATEAUMeshData> &mesh_data,
    const PlateauNode &node) {

    // Update this node's mesh
    update_single_mesh_from_node(mesh_data, node);

    // Process children
    TypedArray<PLATEAUMeshData> children = mesh_data->get_children();
    for (size_t i = 0; i < node.getChildCount() && i < static_cast<size_t>(children.size()); i++) {
        const auto &child_node = node.getChildAt(i);
        Ref<PLATEAUMeshData> child_mesh_data = children[i];
        if (child_mesh_data.is_valid()) {
            update_node_recursive(child_mesh_data, child_node);
        }
    }
}

void PLATEAUHeightMapAligner::update_mesh_data_from_model(
    TypedArray<PLATEAUMeshData> &mesh_data_array,
    const PlateauModel &model) {

    for (size_t i = 0; i < model.getRootNodeCount() && i < static_cast<size_t>(mesh_data_array.size()); i++) {
        const auto &node = model.getRootNodeAt(i);
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[i];

        if (mesh_data.is_null()) {
            continue;
        }

        // Recursively update this node and all children
        update_node_recursive(mesh_data, node);
    }
}
#endif

void PLATEAUHeightMapAligner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_height_offset", "offset"), &PLATEAUHeightMapAligner::set_height_offset);
    ClassDB::bind_method(D_METHOD("get_height_offset"), &PLATEAUHeightMapAligner::get_height_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_offset"), "set_height_offset", "get_height_offset");

    ClassDB::bind_method(D_METHOD("set_max_edge_length", "length"), &PLATEAUHeightMapAligner::set_max_edge_length);
    ClassDB::bind_method(D_METHOD("get_max_edge_length"), &PLATEAUHeightMapAligner::get_max_edge_length);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_edge_length"), "set_max_edge_length", "get_max_edge_length");

    ClassDB::bind_method(D_METHOD("set_alpha_expand_width", "width"), &PLATEAUHeightMapAligner::set_alpha_expand_width);
    ClassDB::bind_method(D_METHOD("get_alpha_expand_width"), &PLATEAUHeightMapAligner::get_alpha_expand_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alpha_expand_width"), "set_alpha_expand_width", "get_alpha_expand_width");

    ClassDB::bind_method(D_METHOD("set_alpha_averaging_width", "width"), &PLATEAUHeightMapAligner::set_alpha_averaging_width);
    ClassDB::bind_method(D_METHOD("get_alpha_averaging_width"), &PLATEAUHeightMapAligner::get_alpha_averaging_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alpha_averaging_width"), "set_alpha_averaging_width", "get_alpha_averaging_width");

    ClassDB::bind_method(D_METHOD("set_invert_height_offset", "offset"), &PLATEAUHeightMapAligner::set_invert_height_offset);
    ClassDB::bind_method(D_METHOD("get_invert_height_offset"), &PLATEAUHeightMapAligner::get_invert_height_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "invert_height_offset"), "set_invert_height_offset", "get_invert_height_offset");

    ClassDB::bind_method(D_METHOD("set_skip_threshold_distance", "distance"), &PLATEAUHeightMapAligner::set_skip_threshold_distance);
    ClassDB::bind_method(D_METHOD("get_skip_threshold_distance"), &PLATEAUHeightMapAligner::get_skip_threshold_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "skip_threshold_distance"), "set_skip_threshold_distance", "get_skip_threshold_distance");

    ClassDB::bind_method(D_METHOD("add_heightmap", "heightmap_data"), &PLATEAUHeightMapAligner::add_heightmap);
    ClassDB::bind_method(D_METHOD("clear_heightmaps"), &PLATEAUHeightMapAligner::clear_heightmaps);
    ClassDB::bind_method(D_METHOD("get_heightmap_count"), &PLATEAUHeightMapAligner::get_heightmap_count);

    ClassDB::bind_method(D_METHOD("align", "mesh_data_array"), &PLATEAUHeightMapAligner::align);
    ClassDB::bind_method(D_METHOD("align_invert", "mesh_data_array"), &PLATEAUHeightMapAligner::align_invert);
    ClassDB::bind_method(D_METHOD("get_height_at", "xz_position"), &PLATEAUHeightMapAligner::get_height_at);
}
