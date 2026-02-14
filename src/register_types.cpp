#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

// PLATEAU classes
#include "plateau/plateau_geo_reference.h"
#include "plateau/plateau_mesh_extract_options.h"
#include "plateau/plateau_city_model.h"
#include "plateau/plateau_importer.h"
#include "plateau/plateau_instanced_city_model.h"
#include "plateau/plateau_dataset_source.h"
#include "plateau/plateau_grid_code.h"
#include "plateau/plateau_gml_file.h"
#include "plateau/plateau_granularity_converter.h"
#include "plateau/plateau_mesh_exporter.h"

// New API classes
#include "plateau/plateau_city_model_scene.h"
#include "plateau/plateau_city_object_type.h"
#include "plateau/plateau_dynamic_tile.h"
#include "plateau/plateau_road_network.h"

// Terrain/HeightMap/Basemap classes (stub on mobile)
#include "plateau/plateau_terrain.h"
#include "plateau/plateau_height_map_aligner.h"
#include "plateau/plateau_basemap.h"

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Register PLATEAU classes
	GDREGISTER_CLASS(PLATEAUGeoReference);
	GDREGISTER_CLASS(PLATEAUMeshExtractOptions);
	GDREGISTER_CLASS(PLATEAUMeshData);
	GDREGISTER_CLASS(PLATEAUCityModel);
	GDREGISTER_CLASS(PLATEAUImporter);
	GDREGISTER_CLASS(PLATEAUInstancedCityModel);

	// Phase 5: Extended features
	GDREGISTER_CLASS(PLATEAUDatasetMetadata);
	GDREGISTER_CLASS(PLATEAUDatasetGroup);
	GDREGISTER_CLASS(PLATEAUGmlFileInfo);
	GDREGISTER_CLASS(PLATEAUDatasetSource);
	GDREGISTER_CLASS(PLATEAUGridCode);
	GDREGISTER_CLASS(PLATEAUGmlFile);
	GDREGISTER_CLASS(PLATEAUGranularityConverter);
	GDREGISTER_CLASS(PLATEAUMeshExporter);

	// Terrain/HeightMap/Basemap classes (stub on mobile platforms)
	GDREGISTER_CLASS(PLATEAUHeightMapData);
	GDREGISTER_CLASS(PLATEAUTerrain);
	GDREGISTER_CLASS(PLATEAUHeightMapAligner);
	GDREGISTER_CLASS(PLATEAUTileCoordinate);
	GDREGISTER_CLASS(PLATEAUVectorTile);
	GDREGISTER_CLASS(PLATEAUVectorTileDownloader);

	// New API: City Model Scene and Filter
	GDREGISTER_CLASS(PLATEAUFilterCondition);
	GDREGISTER_CLASS(PLATEAUCityModelScene);

	// New API: City Object Type Hierarchy
	GDREGISTER_CLASS(PLATEAUCityObjectTypeNode);
	GDREGISTER_CLASS(PLATEAUCityObjectTypeHierarchy);

	// New API: Dynamic Tile
	GDREGISTER_CLASS(PLATEAUDynamicTile);
	GDREGISTER_CLASS(PLATEAUDynamicTileMetaInfo);
	GDREGISTER_CLASS(PLATEAUDynamicTileMetaStore);
	GDREGISTER_CLASS(PLATEAUDynamicTileManager);

	// New API: Road Network
	GDREGISTER_CLASS(PLATEAURnPoint);
	GDREGISTER_CLASS(PLATEAURnLineString);
	GDREGISTER_CLASS(PLATEAURnWay);
	GDREGISTER_CLASS(PLATEAURnTrack);
	GDREGISTER_CLASS(PLATEAURnIntersectionEdge);
	GDREGISTER_CLASS(PLATEAURnLane);
	GDREGISTER_CLASS(PLATEAURnSideWalk);
	GDREGISTER_CLASS(PLATEAURnRoadBase);
	GDREGISTER_CLASS(PLATEAURnRoad);
	GDREGISTER_CLASS(PLATEAURnIntersection);
	GDREGISTER_CLASS(PLATEAURnModel);
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT godot_plateau_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
