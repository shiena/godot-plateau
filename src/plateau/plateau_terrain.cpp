#include "plateau_terrain.h"
#include "plateau_platform.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>

#ifndef PLATEAU_MOBILE_PLATFORM
#include <plateau/height_map_generator/heightmap_generator.h>
#include <plateau/height_map_generator/heightmap_mesh_generator.h>
#include <plateau/height_map_generator/heightmap_types.h>
#include <plateau/polygon_mesh/mesh.h>
#include <plateau/geometry/geo_reference.h>
#endif

using namespace godot;

#ifndef PLATEAU_MOBILE_PLATFORM
// Type aliases
using PlateauMesh = plateau::polygonMesh::Mesh;
using HeightMapT = plateau::heightMapGenerator::HeightMapT;
using HeightMapElemT = plateau::heightMapGenerator::HeightMapElemT;
#endif

// ============================================================================
// PLATEAUHeightMapData implementation
// ============================================================================

PLATEAUHeightMapData::PLATEAUHeightMapData()
    : width_(0), height_(0)
#ifndef PLATEAU_MOBILE_PLATFORM
    , min_(0, 0, 0), max_(0, 0, 0), uv_min_(0, 0), uv_max_(1, 1)
#endif
{
}

PLATEAUHeightMapData::~PLATEAUHeightMapData() {
}

void PLATEAUHeightMapData::set_name(const String &name) {
    name_ = name;
}

String PLATEAUHeightMapData::get_name() const {
    return name_;
}

int PLATEAUHeightMapData::get_width() const {
    return width_;
}

int PLATEAUHeightMapData::get_height() const {
    return height_;
}

Vector3 PLATEAUHeightMapData::get_min_bounds() const {
#ifdef PLATEAU_MOBILE_PLATFORM
    return Vector3();
#else
    return Vector3(min_.x, min_.y, min_.z);
#endif
}

Vector3 PLATEAUHeightMapData::get_max_bounds() const {
#ifdef PLATEAU_MOBILE_PLATFORM
    return Vector3();
#else
    return Vector3(max_.x, max_.y, max_.z);
#endif
}

Vector2 PLATEAUHeightMapData::get_min_uv() const {
#ifdef PLATEAU_MOBILE_PLATFORM
    return Vector2();
#else
    return Vector2(uv_min_.x, uv_min_.y);
#endif
}

Vector2 PLATEAUHeightMapData::get_max_uv() const {
#ifdef PLATEAU_MOBILE_PLATFORM
    return Vector2();
#else
    return Vector2(uv_max_.x, uv_max_.y);
#endif
}

void PLATEAUHeightMapData::set_texture_path(const String &path) {
    texture_path_ = path;
}

String PLATEAUHeightMapData::get_texture_path() const {
    return texture_path_;
}

PackedByteArray PLATEAUHeightMapData::get_heightmap_raw() const {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(PackedByteArray());
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (heightmap_data_.empty()) {
        return PackedByteArray();
    }

    // Return cached data if valid
    if (cached_raw_valid_) {
        return cached_raw_;
    }

    // Convert uint16 to bytes (little-endian) and cache
    cached_raw_.resize(heightmap_data_.size() * 2);
    for (size_t i = 0; i < heightmap_data_.size(); i++) {
        uint16_t val = heightmap_data_[i];
        cached_raw_.set(i * 2, val & 0xFF);
        cached_raw_.set(i * 2 + 1, (val >> 8) & 0xFF);
    }
    cached_raw_valid_ = true;
    return cached_raw_;
#endif
}

PackedFloat32Array PLATEAUHeightMapData::get_heightmap_normalized() const {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(PackedFloat32Array());
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (heightmap_data_.empty()) {
        return PackedFloat32Array();
    }

    // Return cached data if valid
    if (cached_normalized_valid_) {
        return cached_normalized_;
    }

    // Normalize uint16 to [0.0, 1.0] and cache
    cached_normalized_.resize(heightmap_data_.size());
    for (size_t i = 0; i < heightmap_data_.size(); i++) {
        cached_normalized_.set(i, static_cast<float>(heightmap_data_[i]) / 65535.0f);
    }
    cached_normalized_valid_ = true;
    return cached_normalized_;
#endif
}

#ifndef PLATEAU_MOBILE_PLATFORM
void PLATEAUHeightMapData::invalidate_cache() {
    cached_raw_valid_ = false;
    cached_normalized_valid_ = false;
    cached_raw_.resize(0);
    cached_normalized_.resize(0);
}
#endif

void PLATEAUHeightMapData::clear_cache() {
#ifndef PLATEAU_MOBILE_PLATFORM
    invalidate_cache();
#endif
}

bool PLATEAUHeightMapData::save_png(const String &path) const {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(false);
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (heightmap_data_.empty() || width_ <= 0 || height_ <= 0) {
        UtilityFunctions::printerr("Cannot save PNG: no heightmap data");
        return false;
    }

    std::string path_str = path.utf8().get_data();
    try {
        // Use libplateau's built-in PNG save
        plateau::heightMapGenerator::HeightmapGenerator::savePngFile(
            path_str, width_, height_, const_cast<HeightMapElemT*>(heightmap_data_.data()));
        UtilityFunctions::print("Saved heightmap PNG to: ", path);
        return true;
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Failed to save PNG: ", String(e.what()));
        return false;
    }
#endif
}

bool PLATEAUHeightMapData::save_raw(const String &path) const {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(false);
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (heightmap_data_.empty() || width_ <= 0 || height_ <= 0) {
        UtilityFunctions::printerr("Cannot save RAW: no heightmap data");
        return false;
    }

    std::string path_str = path.utf8().get_data();
    try {
        // Use libplateau's built-in RAW save
        plateau::heightMapGenerator::HeightmapGenerator::saveRawFile(
            path_str, width_, height_, const_cast<HeightMapElemT*>(heightmap_data_.data()));
        UtilityFunctions::print("Saved heightmap RAW to: ", path);
        return true;
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Failed to save RAW: ", String(e.what()));
        return false;
    }
#endif
}

Ref<ArrayMesh> PLATEAUHeightMapData::generate_mesh() const {
    Ref<ArrayMesh> array_mesh;
    array_mesh.instantiate();

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(array_mesh);
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (heightmap_data_.empty() || width_ <= 0 || height_ <= 0) {
        UtilityFunctions::printerr("Cannot generate mesh: no heightmap data");
        return array_mesh;
    }

    try {
        // Calculate height scale
        double terrain_height = std::abs(max_.y - min_.y);
        float height_scale = static_cast<float>(terrain_height);

        // Generate mesh using libplateau
        plateau::heightMapGenerator::HeightmapMeshGenerator generator;
        PlateauMesh native_mesh;

        generator.generateMeshFromHeightmap(
            native_mesh,
            width_,
            height_,
            height_scale,
            heightmap_data_.data(),
            plateau::geometry::CoordinateSystem::EUN, // Godot uses Y-up (EUN-like)
            min_,
            max_,
            uv_min_,
            uv_max_,
            true  // invert mesh for correct orientation
        );

        // Convert native mesh to Godot ArrayMesh
        const auto &vertices = native_mesh.getVertices();
        const auto &indices = native_mesh.getIndices();
        const auto &uvs = native_mesh.getUV1();

        if (vertices.empty() || indices.empty()) {
            UtilityFunctions::printerr("Generated mesh is empty");
            return array_mesh;
        }

        // Convert vertices
        PackedVector3Array godot_vertices;
        godot_vertices.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); i++) {
            const auto &v = vertices[i];
            godot_vertices.set(i, Vector3(v.x, v.y, v.z));
        }

        // Convert indices with winding order inversion
        // libplateau outputs CW winding, but Godot expects CCW for front faces
        PackedInt32Array godot_indices;
        godot_indices.resize(indices.size());
        for (size_t i = 0; i < indices.size(); i += 3) {
            // Swap first and third vertex of each triangle to invert winding
            godot_indices.set(i, indices[i + 2]);
            godot_indices.set(i + 1, indices[i + 1]);
            godot_indices.set(i + 2, indices[i]);
        }

        // Convert UVs
        PackedVector2Array godot_uvs;
        if (!uvs.empty()) {
            godot_uvs.resize(uvs.size());
            for (size_t i = 0; i < uvs.size(); i++) {
                const auto &uv = uvs[i];
                godot_uvs.set(i, Vector2(uv.x, 1.0f - uv.y)); // Flip Y
            }
        }

        // Compute normals
        PackedVector3Array godot_normals;
        godot_normals.resize(vertices.size());
        for (int i = 0; i < godot_normals.size(); i++) {
            godot_normals.set(i, Vector3(0, 0, 0));
        }

        // Calculate face normals and accumulate
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

        // Normalize
        for (int i = 0; i < godot_normals.size(); i++) {
            Vector3 n = godot_normals[i];
            if (n.length_squared() > 0.0001f) {
                godot_normals.set(i, n.normalized());
            } else {
                godot_normals.set(i, Vector3(0, 1, 0));
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

        // Add surface
        array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

        // Create default material
        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_cull_mode(StandardMaterial3D::CULL_BACK);
        material->set_albedo(Color(0.6f, 0.6f, 0.6f, 1.0f));
        material->set_metallic(0.0f);
        material->set_roughness(0.9f);
        array_mesh->surface_set_material(0, material);

        UtilityFunctions::print("Generated terrain mesh with ", godot_vertices.size(), " vertices");

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception generating mesh: ", String(e.what()));
    }
#endif

    return array_mesh;
}

#ifndef PLATEAU_MOBILE_PLATFORM
void PLATEAUHeightMapData::set_data(const HeightMapT &heightmap,
                                     int width, int height,
                                     const TVec3d &min, const TVec3d &max,
                                     const TVec2f &uv_min, const TVec2f &uv_max) {
    heightmap_data_ = heightmap;
    width_ = width;
    height_ = height;
    min_ = min;
    max_ = max;
    uv_min_ = uv_min;
    uv_max_ = uv_max;
    // Invalidate cache when data changes
    invalidate_cache();
}

const HeightMapT& PLATEAUHeightMapData::get_heightmap_internal() const {
    return heightmap_data_;
}

TVec3d PLATEAUHeightMapData::get_min_internal() const {
    return min_;
}

TVec3d PLATEAUHeightMapData::get_max_internal() const {
    return max_;
}
#endif

void PLATEAUHeightMapData::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_name", "name"), &PLATEAUHeightMapData::set_name);
    ClassDB::bind_method(D_METHOD("get_name"), &PLATEAUHeightMapData::get_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");

    ClassDB::bind_method(D_METHOD("get_width"), &PLATEAUHeightMapData::get_width);
    ClassDB::bind_method(D_METHOD("get_height"), &PLATEAUHeightMapData::get_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "", "get_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "", "get_height");

    ClassDB::bind_method(D_METHOD("get_min_bounds"), &PLATEAUHeightMapData::get_min_bounds);
    ClassDB::bind_method(D_METHOD("get_max_bounds"), &PLATEAUHeightMapData::get_max_bounds);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "min_bounds"), "", "get_min_bounds");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "max_bounds"), "", "get_max_bounds");

    ClassDB::bind_method(D_METHOD("get_min_uv"), &PLATEAUHeightMapData::get_min_uv);
    ClassDB::bind_method(D_METHOD("get_max_uv"), &PLATEAUHeightMapData::get_max_uv);

    ClassDB::bind_method(D_METHOD("set_texture_path", "path"), &PLATEAUHeightMapData::set_texture_path);
    ClassDB::bind_method(D_METHOD("get_texture_path"), &PLATEAUHeightMapData::get_texture_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "texture_path"), "set_texture_path", "get_texture_path");

    ClassDB::bind_method(D_METHOD("get_heightmap_raw"), &PLATEAUHeightMapData::get_heightmap_raw);
    ClassDB::bind_method(D_METHOD("get_heightmap_normalized"), &PLATEAUHeightMapData::get_heightmap_normalized);

    ClassDB::bind_method(D_METHOD("save_png", "path"), &PLATEAUHeightMapData::save_png);
    ClassDB::bind_method(D_METHOD("save_raw", "path"), &PLATEAUHeightMapData::save_raw);

    ClassDB::bind_method(D_METHOD("generate_mesh"), &PLATEAUHeightMapData::generate_mesh);

    ClassDB::bind_method(D_METHOD("clear_cache"), &PLATEAUHeightMapData::clear_cache);
}

// ============================================================================
// PLATEAUTerrain implementation
// ============================================================================

PLATEAUTerrain::PLATEAUTerrain()
    : texture_width_(257), texture_height_(257), offset_(0, 0),
      fill_edges_(true), apply_blur_filter_(true) {
}

PLATEAUTerrain::~PLATEAUTerrain() {
}

void PLATEAUTerrain::set_texture_width(int width) {
    texture_width_ = std::max(1, width);
}

int PLATEAUTerrain::get_texture_width() const {
    return texture_width_;
}

void PLATEAUTerrain::set_texture_height(int height) {
    texture_height_ = std::max(1, height);
}

int PLATEAUTerrain::get_texture_height() const {
    return texture_height_;
}

void PLATEAUTerrain::set_offset(const Vector2 &offset) {
    offset_ = offset;
}

Vector2 PLATEAUTerrain::get_offset() const {
    return offset_;
}

void PLATEAUTerrain::set_fill_edges(bool fill) {
    fill_edges_ = fill;
}

bool PLATEAUTerrain::get_fill_edges() const {
    return fill_edges_;
}

void PLATEAUTerrain::set_apply_blur_filter(bool apply) {
    apply_blur_filter_ = apply;
}

bool PLATEAUTerrain::get_apply_blur_filter() const {
    return apply_blur_filter_;
}

Ref<PLATEAUHeightMapData> PLATEAUTerrain::generate_from_mesh(const Ref<PLATEAUMeshData> &mesh_data) {
    Ref<PLATEAUHeightMapData> result;

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (mesh_data.is_null()) {
        UtilityFunctions::printerr("PLATEAUTerrain: mesh_data is null");
        return result;
    }

    Ref<ArrayMesh> godot_mesh = mesh_data->get_mesh();
    if (godot_mesh.is_null() || godot_mesh->get_surface_count() == 0) {
        UtilityFunctions::printerr("PLATEAUTerrain: mesh_data has no mesh");
        return result;
    }

    // Convert Godot ArrayMesh to plateau::polygonMesh::Mesh
    PlateauMesh native_mesh;

    // Get surface data
    Array arrays = godot_mesh->surface_get_arrays(0);
    if (arrays.size() < Mesh::ARRAY_VERTEX + 1) {
        UtilityFunctions::printerr("PLATEAUTerrain: invalid mesh arrays");
        return result;
    }

    PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
    PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX];
    PackedVector2Array uvs;
    if (arrays.size() > Mesh::ARRAY_TEX_UV && arrays[Mesh::ARRAY_TEX_UV].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
        uvs = arrays[Mesh::ARRAY_TEX_UV];
    }

    if (vertices.is_empty() || indices.is_empty()) {
        UtilityFunctions::printerr("PLATEAUTerrain: mesh has no vertices or indices");
        return result;
    }

    // Convert to native mesh
    std::vector<TVec3d> native_vertices;
    native_vertices.reserve(vertices.size());
    for (int i = 0; i < vertices.size(); i++) {
        Vector3 v = vertices[i];
        native_vertices.push_back(TVec3d(v.x, v.y, v.z));
    }

    std::vector<unsigned int> native_indices;
    native_indices.reserve(indices.size());
    for (int i = 0; i < indices.size(); i++) {
        native_indices.push_back(static_cast<unsigned int>(indices[i]));
    }

    std::vector<TVec2f> native_uvs;
    if (!uvs.is_empty()) {
        native_uvs.reserve(uvs.size());
        for (int i = 0; i < uvs.size(); i++) {
            Vector2 uv = uvs[i];
            native_uvs.push_back(TVec2f(uv.x, 1.0f - uv.y)); // Flip Y back
        }
    }

    // Set mesh data
    native_mesh.addVerticesList(native_vertices);
    native_mesh.addIndicesList(native_indices, 0, false);
    if (!native_uvs.empty()) {
        native_mesh.setUV1(std::move(native_uvs));
    }

    return generate_from_plateau_mesh(native_mesh, mesh_data->get_name());
#endif
}

Ref<PLATEAUHeightMapData> PLATEAUTerrain::generate_from_meshes(const TypedArray<PLATEAUMeshData> &mesh_data_array) {
    Ref<PLATEAUHeightMapData> result;

#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED_V(result);
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (mesh_data_array.is_empty()) {
        UtilityFunctions::printerr("PLATEAUTerrain: mesh_data_array is empty");
        return result;
    }

    // Merge all meshes into one
    PlateauMesh merged_mesh;
    String combined_name;

    for (int mesh_idx = 0; mesh_idx < mesh_data_array.size(); mesh_idx++) {
        Ref<PLATEAUMeshData> mesh_data = mesh_data_array[mesh_idx];
        if (mesh_data.is_null()) {
            continue;
        }

        Ref<ArrayMesh> godot_mesh = mesh_data->get_mesh();
        if (godot_mesh.is_null() || godot_mesh->get_surface_count() == 0) {
            continue;
        }

        // Build combined name
        if (!combined_name.is_empty()) {
            combined_name += "_";
        }
        combined_name += mesh_data->get_name();

        // Process all surfaces
        for (int surface_idx = 0; surface_idx < godot_mesh->get_surface_count(); surface_idx++) {
            Array arrays = godot_mesh->surface_get_arrays(surface_idx);
            if (arrays.size() < Mesh::ARRAY_VERTEX + 1) {
                continue;
            }

            PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
            PackedInt32Array indices;
            if (arrays.size() > Mesh::ARRAY_INDEX && arrays[Mesh::ARRAY_INDEX].get_type() == Variant::PACKED_INT32_ARRAY) {
                indices = arrays[Mesh::ARRAY_INDEX];
            }

            if (vertices.is_empty() || indices.is_empty()) {
                continue;
            }

            // Get current vertex count for index offset
            size_t vertex_offset = merged_mesh.getVertices().size();

            // Add vertices
            std::vector<TVec3d> native_vertices;
            native_vertices.reserve(vertices.size());
            for (int i = 0; i < vertices.size(); i++) {
                Vector3 v = vertices[i];
                native_vertices.push_back(TVec3d(v.x, v.y, v.z));
            }
            merged_mesh.addVerticesList(native_vertices);

            // Add indices with offset
            std::vector<unsigned int> native_indices;
            native_indices.reserve(indices.size());
            for (int i = 0; i < indices.size(); i++) {
                native_indices.push_back(static_cast<unsigned int>(indices[i] + vertex_offset));
            }
            merged_mesh.addIndicesList(native_indices, 0, false);

            // Add UVs if present
            if (arrays.size() > Mesh::ARRAY_TEX_UV && arrays[Mesh::ARRAY_TEX_UV].get_type() == Variant::PACKED_VECTOR2_ARRAY) {
                PackedVector2Array uvs = arrays[Mesh::ARRAY_TEX_UV];
                if (!uvs.is_empty()) {
                    std::vector<TVec2f> native_uvs;
                    native_uvs.reserve(uvs.size());
                    for (int i = 0; i < uvs.size(); i++) {
                        Vector2 uv = uvs[i];
                        native_uvs.push_back(TVec2f(uv.x, 1.0f - uv.y));
                    }
                    // Append to existing UVs
                    auto& existing_uvs = const_cast<std::vector<TVec2f>&>(merged_mesh.getUV1());
                    existing_uvs.insert(existing_uvs.end(), native_uvs.begin(), native_uvs.end());
                }
            }
        }
    }

    if (merged_mesh.getVertices().empty() || merged_mesh.getIndices().empty()) {
        UtilityFunctions::printerr("PLATEAUTerrain: no valid mesh data found in array");
        return result;
    }

    UtilityFunctions::print("PLATEAUTerrain: merged ", static_cast<int64_t>(mesh_data_array.size()), " meshes, ",
                           static_cast<int64_t>(merged_mesh.getVertices().size()), " vertices");

    return generate_from_plateau_mesh(merged_mesh, combined_name);
#endif
}

#ifndef PLATEAU_MOBILE_PLATFORM
Ref<PLATEAUHeightMapData> PLATEAUTerrain::generate_from_plateau_mesh(const plateau::polygonMesh::Mesh &mesh, const String &name) {
    Ref<PLATEAUHeightMapData> result;

    result.instantiate();
    result->set_name(name);

    try {
        plateau::heightMapGenerator::HeightmapGenerator generator;

        TVec3d out_min, out_max;
        TVec2f out_uv_min, out_uv_max;
        TVec2d margin(offset_.x, offset_.y);

        HeightMapT heightmap_data = generator.generateFromMesh(
            mesh,
            texture_width_,
            texture_height_,
            margin,
            plateau::geometry::CoordinateSystem::EUN, // Godot uses Y-up
            fill_edges_,
            apply_blur_filter_,
            out_min,
            out_max,
            out_uv_min,
            out_uv_max
        );

        if (heightmap_data.empty()) {
            UtilityFunctions::printerr("PLATEAUTerrain: failed to generate heightmap");
            return Ref<PLATEAUHeightMapData>();
        }

        result->set_data(heightmap_data, texture_width_, texture_height_,
                        out_min, out_max, out_uv_min, out_uv_max);

        UtilityFunctions::print("Generated heightmap: ", texture_width_, "x", texture_height_,
                               " bounds: (", out_min.x, ",", out_min.y, ",", out_min.z, ") - (",
                               out_max.x, ",", out_max.y, ",", out_max.z, ")");

    } catch (const std::exception &e) {
        UtilityFunctions::printerr("Exception generating heightmap: ", String(e.what()));
        return Ref<PLATEAUHeightMapData>();
    }

    return result;
}
#endif

void PLATEAUTerrain::generate_from_meshes_async(const TypedArray<PLATEAUMeshData> &mesh_data_array) {
#ifdef PLATEAU_MOBILE_PLATFORM
    PLATEAU_MOBILE_UNSUPPORTED();
#endif

#ifndef PLATEAU_MOBILE_PLATFORM
    if (is_processing_.load()) {
        UtilityFunctions::printerr("PLATEAUTerrain: Already processing");
        return;
    }

    if (mesh_data_array.is_empty()) {
        UtilityFunctions::printerr("PLATEAUTerrain: mesh_data_array is empty");
        call_deferred("emit_signal", "generate_completed", Ref<PLATEAUHeightMapData>());
        return;
    }

    is_processing_.store(true);
    pending_mesh_data_ = mesh_data_array;

    WorkerThreadPool::get_singleton()->add_task(
        callable_mp(this, &PLATEAUTerrain::_generate_thread_func)
    );
#endif
}

void PLATEAUTerrain::_generate_thread_func() {
#ifndef PLATEAU_MOBILE_PLATFORM
    Ref<PLATEAUHeightMapData> result = generate_from_meshes(pending_mesh_data_);

    is_processing_.store(false);
    pending_mesh_data_.clear();

    call_deferred("emit_signal", "generate_completed", result);
#endif
}

bool PLATEAUTerrain::is_processing() const {
    return is_processing_.load();
}

void PLATEAUTerrain::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_texture_width", "width"), &PLATEAUTerrain::set_texture_width);
    ClassDB::bind_method(D_METHOD("get_texture_width"), &PLATEAUTerrain::get_texture_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "texture_width"), "set_texture_width", "get_texture_width");

    ClassDB::bind_method(D_METHOD("set_texture_height", "height"), &PLATEAUTerrain::set_texture_height);
    ClassDB::bind_method(D_METHOD("get_texture_height"), &PLATEAUTerrain::get_texture_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "texture_height"), "set_texture_height", "get_texture_height");

    ClassDB::bind_method(D_METHOD("set_offset", "offset"), &PLATEAUTerrain::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset"), &PLATEAUTerrain::get_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_offset", "get_offset");

    ClassDB::bind_method(D_METHOD("set_fill_edges", "fill"), &PLATEAUTerrain::set_fill_edges);
    ClassDB::bind_method(D_METHOD("get_fill_edges"), &PLATEAUTerrain::get_fill_edges);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fill_edges"), "set_fill_edges", "get_fill_edges");

    ClassDB::bind_method(D_METHOD("set_apply_blur_filter", "apply"), &PLATEAUTerrain::set_apply_blur_filter);
    ClassDB::bind_method(D_METHOD("get_apply_blur_filter"), &PLATEAUTerrain::get_apply_blur_filter);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "apply_blur_filter"), "set_apply_blur_filter", "get_apply_blur_filter");

    ClassDB::bind_method(D_METHOD("generate_from_mesh", "mesh_data"), &PLATEAUTerrain::generate_from_mesh);
    ClassDB::bind_method(D_METHOD("generate_from_meshes", "mesh_data_array"), &PLATEAUTerrain::generate_from_meshes);
    ClassDB::bind_method(D_METHOD("generate_from_meshes_async", "mesh_data_array"), &PLATEAUTerrain::generate_from_meshes_async);
    ClassDB::bind_method(D_METHOD("is_processing"), &PLATEAUTerrain::is_processing);

    // Signal for async completion
    ADD_SIGNAL(MethodInfo("generate_completed", PropertyInfo(Variant::OBJECT, "heightmap_data", PROPERTY_HINT_RESOURCE_TYPE, "PLATEAUHeightMapData")));
}
