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
#include "plateau/plateau_terrain.h"
#include "plateau/plateau_height_map_aligner.h"
#include "plateau/plateau_dataset_source.h"
#include "plateau/plateau_granularity_converter.h"
#include "plateau/plateau_mesh_exporter.h"
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

	// Phase 2: Terrain classes
	GDREGISTER_CLASS(PLATEAUHeightMapData);
	GDREGISTER_CLASS(PLATEAUTerrain);
	GDREGISTER_CLASS(PLATEAUHeightMapAligner);

	// Phase 5: Extended features
	GDREGISTER_CLASS(PLATEAUDatasetMetadata);
	GDREGISTER_CLASS(PLATEAUDatasetGroup);
	GDREGISTER_CLASS(PLATEAUGmlFileInfo);
	GDREGISTER_CLASS(PLATEAUDatasetSource);
	GDREGISTER_CLASS(PLATEAUGranularityConverter);
	GDREGISTER_CLASS(PLATEAUMeshExporter);

	// Phase 5.4: Basemap
	GDREGISTER_CLASS(PLATEAUTileCoordinate);
	GDREGISTER_CLASS(PLATEAUVectorTile);
	GDREGISTER_CLASS(PLATEAUVectorTileDownloader);
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
