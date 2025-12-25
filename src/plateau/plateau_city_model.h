#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <atomic>

#include <citygml/citygml.h>
#include <citygml/citymodel.h>
#include <citygml/attributesmap.h>
#include <plateau/polygon_mesh/model.h>
#include <plateau/polygon_mesh/mesh_extractor.h>
#include <plateau/polygon_mesh/city_object_list.h>

#include "plateau_mesh_extract_options.h"

namespace godot {

// CityObject types from CityGML specification
// Matches citygml::CityObject::CityObjectsType
enum PLATEAUCityObjectType : int64_t {
    COT_GenericCityObject           = 1ll,
    COT_Building                    = 1ll << 1,
    COT_Room                        = 1ll << 2,
    COT_BuildingInstallation        = 1ll << 3,
    COT_BuildingFurniture           = 1ll << 4,
    COT_Door                        = 1ll << 5,
    COT_Window                      = 1ll << 6,
    COT_CityFurniture               = 1ll << 7,
    COT_Track                       = 1ll << 8,
    COT_Road                        = 1ll << 9,
    COT_Railway                     = 1ll << 10,
    COT_Square                      = 1ll << 11,
    COT_PlantCover                  = 1ll << 12,
    COT_SolitaryVegetationObject    = 1ll << 13,
    COT_WaterBody                   = 1ll << 14,
    COT_ReliefFeature               = 1ll << 15,
    COT_LandUse                     = 1ll << 16,
    COT_Tunnel                      = 1ll << 17,
    COT_Bridge                      = 1ll << 18,
    COT_BridgeConstructionElement   = 1ll << 19,
    COT_BridgeInstallation          = 1ll << 20,
    COT_BridgePart                  = 1ll << 21,
    COT_BuildingPart                = 1ll << 22,
    COT_WallSurface                 = 1ll << 23,
    COT_RoofSurface                 = 1ll << 24,
    COT_GroundSurface               = 1ll << 25,
    COT_ClosureSurface              = 1ll << 26,
    COT_FloorSurface                = 1ll << 27,
    COT_InteriorWallSurface         = 1ll << 28,
    COT_CeilingSurface              = 1ll << 29,
    COT_CityObjectGroup             = 1ll << 30,
    COT_OuterCeilingSurface         = 1ll << 31,
    COT_OuterFloorSurface           = 1ll << 32,
    COT_TransportationObject        = 1ll << 33,
    COT_IntBuildingInstallation     = 1ll << 34,
    COT_WaterSurface                = 1ll << 35,
    COT_ReliefComponent             = 1ll << 36,
    COT_TINRelief                   = 1ll << 37,
    COT_MassPointRelief             = 1ll << 38,
    COT_BreaklineRelief             = 1ll << 39,
    COT_RasterRelief                = 1ll << 40,
    COT_Unknown                     = 1ll << 41,
};

// Log levels for CityGML parser
// Controls verbosity of log messages during GML loading
enum PLATEAULogLevel : int {
    LOG_LEVEL_NONE = 0,     // No log messages
    LOG_LEVEL_ERROR = 1,    // Errors only
    LOG_LEVEL_WARNING = 2,  // Warnings and errors
    LOG_LEVEL_INFO = 3,     // Info, warnings, and errors
    LOG_LEVEL_DEBUG = 4,    // All messages including debug
};

// Represents extracted mesh data for a single node
class PLATEAUMeshData : public RefCounted {
    GDCLASS(PLATEAUMeshData, RefCounted)

public:
    PLATEAUMeshData();
    ~PLATEAUMeshData();

    void set_name(const String &name);
    String get_name() const;

    void set_mesh(const Ref<ArrayMesh> &mesh);
    Ref<ArrayMesh> get_mesh() const;

    void set_transform(const Transform3D &transform);
    Transform3D get_transform() const;

    void add_child(const Ref<PLATEAUMeshData> &child);
    TypedArray<PLATEAUMeshData> get_children() const;
    int get_child_count() const;
    Ref<PLATEAUMeshData> get_child(int index) const;

    // Phase 1: Attribute access
    void set_gml_id(const String &gml_id);
    String get_gml_id() const;

    void set_city_object_type(int64_t type);
    int64_t get_city_object_type() const;

    void set_attributes(const Dictionary &attributes);
    Dictionary get_attributes() const;

    // Get attribute value by key (supports nested keys with "/" separator)
    Variant get_attribute(const String &key) const;

    // Check if this node has city object info
    bool has_city_object_info() const;

    // Get city object type as human-readable string
    String get_city_object_type_name() const;

    // Internal: store CityObjectList for UV4-based lookup
    void set_city_object_list(const plateau::polygonMesh::CityObjectList &list);
    const plateau::polygonMesh::CityObjectList& get_city_object_list_internal() const;

    // Get GML ID from UV4 coordinates (for raycast hit lookup)
    String get_gml_id_from_uv(const Vector2 &uv) const;

    // Texture paths for each surface (for export)
    void set_texture_paths(const PackedStringArray &paths);
    PackedStringArray get_texture_paths() const;
    void add_texture_path(const String &path);
    String get_texture_path(int surface_index) const;
    int get_texture_path_count() const;

protected:
    static void _bind_methods();

private:
    String name_;
    Ref<ArrayMesh> mesh_;
    Transform3D transform_;
    TypedArray<PLATEAUMeshData> children_;

    // Phase 1: Attribute data
    String gml_id_;
    int64_t city_object_type_;
    Dictionary attributes_;
    plateau::polygonMesh::CityObjectList city_object_list_;

    // Texture paths for each surface (for export)
    PackedStringArray texture_paths_;
};

// Main class for loading CityGML and extracting meshes
class PLATEAUCityModel : public RefCounted {
    GDCLASS(PLATEAUCityModel, RefCounted)

public:
    PLATEAUCityModel();
    ~PLATEAUCityModel();

    // Load a CityGML file
    bool load(const String &gml_path);

    // Check if model is loaded
    bool is_loaded() const;

    // Get the GML file path
    String get_gml_path() const;

    // Extract meshes with given options
    TypedArray<PLATEAUMeshData> extract_meshes(const Ref<PLATEAUMeshExtractOptions> &options);

    // Get center point of the city model
    Vector3 get_center_point(int coordinate_zone_id) const;

    // Phase 1: Get CityObject by GML ID
    Dictionary get_city_object_attributes(const String &gml_id) const;

    // Get city object type by GML ID
    int64_t get_city_object_type(const String &gml_id) const;

    // Async API - load/extract in background thread
    void load_async(const String &gml_path);
    void extract_meshes_async(const Ref<PLATEAUMeshExtractOptions> &options);
    bool is_processing() const;

    // Log level control
    void set_log_level(int level);
    int get_log_level() const;

protected:
    static void _bind_methods();

private:
    std::shared_ptr<const citygml::CityModel> city_model_;
    String gml_path_;
    bool is_loaded_;
    int log_level_ = LOG_LEVEL_WARNING;  // Default: show warnings and errors

    // Async processing state
    std::atomic<bool> is_processing_{false};
    String pending_gml_path_;
    Ref<PLATEAUMeshExtractOptions> pending_options_;
    std::shared_ptr<plateau::polygonMesh::Model> pending_model_;

    // Async worker functions (called from WorkerThreadPool)
    void _load_thread_func();
    void _extract_model_thread_func();  // Stage 1: Extract libplateau Model (worker thread)
    void _finalize_meshes_on_main_thread();  // Stage 2: Create Godot resources (main thread)

    // Texture cache to avoid reloading same textures
    HashMap<String, Ref<ImageTexture>> texture_cache_;

    // Helper methods for mesh conversion
    Ref<PLATEAUMeshData> convert_node(const plateau::polygonMesh::Node &node);
    Ref<ArrayMesh> convert_mesh(const plateau::polygonMesh::Mesh &mesh, plateau::polygonMesh::CityObjectList &out_city_object_list, PackedStringArray &out_texture_paths);
    Ref<StandardMaterial3D> create_material(const plateau::polygonMesh::SubMesh &sub_mesh);

    // Compute flat normals for mesh
    static PackedVector3Array compute_normals(const PackedVector3Array &vertices, const PackedInt32Array &indices);

    // Load texture with caching
    Ref<ImageTexture> load_texture_cached(const String &texture_path);

    // Phase 1: Helper to convert citygml attributes to Godot Dictionary
    static Dictionary convert_attributes(const citygml::AttributesMap &attrs);
    static Variant convert_attribute_value(const citygml::AttributeValue &value);
};

} // namespace godot
