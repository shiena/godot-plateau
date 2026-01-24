extends Node3D
## API Sample
##
## Demonstrates the main PLATEAU SDK APIs:
## - Import: Load CityGML files with various options (async to prevent UI freeze)
## - Export: Save meshes to glTF/GLB/OBJ formats
## - Granularity: Convert between mesh granularity levels
## - Dataset: Access local dataset files
##
## Usage:
## 1. Set gml_path to your CityGML file path
## 2. Run the scene
## 3. Use the UI to test various API functions

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9  ## Japan Plane Rectangular CS (1-19)

var city_model: PLATEAUCityModel
var importer: PLATEAUImporter
var city_model_root: PLATEAUInstancedCityModel
var current_mesh_data: Array = []
var pending_options: PLATEAUMeshExtractOptions
var pending_geo_reference: PLATEAUGeoReference

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var stats_label: Label = $UI/StatsLabel
@onready var loading_panel: Panel = $UI/LoadingPanel
@onready var loading_label: Label = $UI/LoadingPanel/LoadingLabel


func _ready() -> void:
	# Connect buttons
	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/ImportPanel/GranularityOption.item_selected.connect(_on_granularity_changed)

	$UI/ExportPanel/ExportGLTFButton.pressed.connect(func(): _export_meshes(PLATEAUMeshExporter.EXPORT_FORMAT_GLTF))
	$UI/ExportPanel/ExportGLBButton.pressed.connect(func(): _export_meshes(PLATEAUMeshExporter.EXPORT_FORMAT_GLB))
	$UI/ExportPanel/ExportOBJButton.pressed.connect(func(): _export_meshes(PLATEAUMeshExporter.EXPORT_FORMAT_OBJ))

	$UI/ConvertPanel/ConvertAtomicButton.pressed.connect(func(): _convert_granularity(PLATEAUGranularityConverter.CONVERT_GRANULARITY_ATOMIC))
	$UI/ConvertPanel/ConvertPrimaryButton.pressed.connect(func(): _convert_granularity(PLATEAUGranularityConverter.CONVERT_GRANULARITY_PRIMARY))
	$UI/ConvertPanel/ConvertAreaButton.pressed.connect(func(): _convert_granularity(PLATEAUGranularityConverter.CONVERT_GRANULARITY_AREA))

	# Setup granularity options
	var granularity_option = $UI/ImportPanel/GranularityOption as OptionButton
	granularity_option.add_item("Atomic (finest)", 0)
	granularity_option.add_item("Primary (per building)", 1)
	granularity_option.add_item("Area (merged)", 2)
	granularity_option.select(1)

	# Hide loading panel initially
	loading_panel.visible = false

	# Setup async signal handlers
	city_model = PLATEAUCityModel.new()
	city_model.load_completed.connect(_on_load_completed)
	city_model.extract_completed.connect(_on_extract_completed)

	# Setup importer
	importer = PLATEAUImporter.new()

	_log("PLATEAU SDK API Sample ready.")
	_log("Click 'Import GML' to load a CityGML file.")


func _on_import_pressed() -> void:
	if city_model.is_processing():
		_log("[color=yellow]Already processing, please wait...[/color]")
		return

	if gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _on_file_selected)
	else:
		_import_gml(gml_path)


func _on_file_selected(path: String) -> void:
	gml_path = path
	_import_gml(path)


func _on_granularity_changed(_index: int) -> void:
	if city_model == null or not city_model.is_loaded():
		return
	if city_model.is_processing():
		return
	_import_gml(gml_path)


func _import_gml(path: String) -> void:
	if city_model.is_processing():
		_log("[color=yellow]Already processing, please wait...[/color]")
		return

	_log("--- Import Start ---")
	_log("File: " + path.get_file())

	# Use PLATEAUGmlFile to get metadata
	var gml_file = PLATEAUGmlFile.create(path)
	if gml_file.is_valid():
		_log("[color=cyan]GML File Info:[/color]")
		_log("  Grid Code: " + gml_file.get_grid_code())
		_log("  Feature Type: " + gml_file.get_feature_type())
		_log("  EPSG: " + str(gml_file.get_epsg()))
		_log("  Dataset Root: " + gml_file.get_dataset_root_path())

		# Search for textures referenced in GML (like Unity)
		var texture_paths = gml_file.search_image_paths()
		if texture_paths.size() > 0:
			var appearance_dir = gml_file.get_appearance_directory_path()
			_log("[color=cyan]Loading textures in " + appearance_dir + "[/color]")
			# Show texture filenames (up to 5)
			var texture_names: PackedStringArray = []
			for i in range(mini(5, texture_paths.size())):
				texture_names.append(texture_paths[i].get_file())
			var names_str = ", ".join(texture_names)
			if texture_paths.size() > 5:
				names_str += ", ..."
			_log("  Loaded " + str(texture_paths.size()) + " textures: " + names_str)
	else:
		_log("[color=yellow]Warning: Could not parse GML file metadata[/color]")

	# Clear previous
	_clear_meshes()

	# Prepare extraction options for later use
	var granularity_option = $UI/ImportPanel/GranularityOption as OptionButton
	var granularity = granularity_option.get_selected_id()

	pending_options = PLATEAUMeshExtractOptions.new()
	pending_options.coordinate_zone_id = zone_id
	pending_options.min_lod = $UI/ImportPanel/MinLODSpin.value
	pending_options.max_lod = $UI/ImportPanel/MaxLODSpin.value
	pending_options.mesh_granularity = granularity
	pending_options.export_appearance = $UI/ImportPanel/TextureCheck.button_pressed
	pending_options.highest_lod_only = true

	# Create GeoReference for import
	pending_geo_reference = PLATEAUGeoReference.new()
	pending_geo_reference.zone_id = zone_id

	_log("Options: LOD " + str(pending_options.min_lod) + "-" + str(pending_options.max_lod) +
		 ", Granularity: " + str(granularity) +
		 ", Textures: " + str(pending_options.export_appearance))

	# Start async load
	_show_loading("Loading CityGML...")
	city_model.load_async(path)


func _on_load_completed(success: bool) -> void:
	if not success:
		_hide_loading()
		_log("[color=red]ERROR: Failed to load CityGML[/color]")
		return

	_log("Load completed successfully")

	# Update geo reference with center point
	var center = city_model.get_center_point(zone_id)
	pending_geo_reference.reference_point = center
	pending_options.reference_point = center

	# Start async mesh extraction
	_update_loading("Extracting meshes...")
	city_model.extract_meshes_async(pending_options)


func _on_extract_completed(root_mesh_data: Array) -> void:
	_log("Extract completed")
	_log("Root nodes extracted: " + str(root_mesh_data.size()))

	if root_mesh_data.is_empty():
		_hide_loading()
		_log("[color=yellow]WARNING: No meshes extracted[/color]")
		return

	# Convert to TypedArray for import_to_scene
	var typed_mesh_data: Array[PLATEAUMeshData] = []
	for md in root_mesh_data:
		typed_mesh_data.append(md)

	# Use PLATEAUImporter to create PLATEAUInstancedCityModel hierarchy
	_update_loading("Creating scene hierarchy...")
	city_model_root = importer.import_to_scene(
		typed_mesh_data,
		gml_path.get_file().get_basename(),
		pending_geo_reference,
		pending_options,
		gml_path
	)

	if city_model_root == null:
		_hide_loading()
		_log("[color=red]ERROR: Failed to create scene[/color]")
		return

	add_child(city_model_root)

	# Flatten hierarchy for stats and export
	current_mesh_data = PLATEAUUtils.flatten_mesh_data(root_mesh_data)
	_log("Total meshes: " + str(current_mesh_data.size()))

	# Display PLATEAUInstancedCityModel info
	_log("[color=cyan]PLATEAUInstancedCityModel Info:[/color]")
	_log("  Zone ID: " + str(city_model_root.zone_id))
	_log("  Latitude: %.6f" % city_model_root.get_latitude())
	_log("  Longitude: %.6f" % city_model_root.get_longitude())
	_log("  Reference Point: " + str(city_model_root.reference_point))
	_log("  LOD Range: " + str(city_model_root.min_lod) + "-" + str(city_model_root.max_lod))

	# Fit camera to bounds
	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF
	for mesh_data in current_mesh_data:
		var mesh = mesh_data.get_mesh()
		if mesh != null and mesh.get_surface_count() > 0:
			var aabb = mesh.get_aabb()
			var mesh_transform = mesh_data.get_transform()
			var global_aabb = mesh_transform * aabb
			bounds_min = bounds_min.min(global_aabb.position)
			bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	_hide_loading()
	_update_stats()
	_log("[color=green]Import complete![/color]")


func _show_loading(message: String) -> void:
	loading_label.text = message
	loading_panel.visible = true


func _update_loading(message: String) -> void:
	loading_label.text = message


func _hide_loading() -> void:
	loading_panel.visible = false


func _clear_meshes() -> void:
	if city_model_root != null and is_instance_valid(city_model_root):
		city_model_root.queue_free()
		city_model_root = null
	current_mesh_data.clear()


func _export_meshes(format: int) -> void:
	if current_mesh_data.is_empty():
		_log("[color=red]ERROR: No meshes to export. Import first.[/color]")
		return

	var ext = PLATEAUMeshExporter.get_format_extension(format)
	var format_name = PLATEAUMeshExporter.get_supported_formats()[format]

	PLATEAUUtils.show_save_file_dialog(
		self,
		PackedStringArray(["*." + ext + " ; " + format_name + " Files"]),
		"export." + ext,
		func(path): _do_export(path, format)
	)


func _do_export(path: String, format: int) -> void:
	_log("--- Export Start ---")
	_log("Format: " + PLATEAUMeshExporter.get_supported_formats()[format])
	_log("Path: " + path)

	var exporter = PLATEAUMeshExporter.new()
	var start_time = Time.get_ticks_msec()

	# Convert Array to TypedArray
	var typed_array: Array[PLATEAUMeshData] = []
	for md in current_mesh_data:
		typed_array.append(md)

	var success = exporter.export_to_file(typed_array, path, format)
	var export_time = Time.get_ticks_msec() - start_time

	if success:
		_log("Export time: " + str(export_time) + " ms")
		_log("[color=green]Export complete: " + path + "[/color]")
	else:
		_log("[color=red]ERROR: Export failed[/color]")


func _convert_granularity(target_granularity: int) -> void:
	if current_mesh_data.is_empty():
		_log("[color=red]ERROR: No meshes to convert. Import first.[/color]")
		return

	_log("--- Granularity Conversion ---")

	var granularity_names = ["Atomic", "Primary", "Area", "MaterialInPrimary"]
	_log("Target: " + granularity_names[target_granularity])

	var converter = PLATEAUGranularityConverter.new()
	converter.grid_count = 1

	# Detect current granularity
	var typed_array: Array[PLATEAUMeshData] = []
	for md in current_mesh_data:
		typed_array.append(md)

	var current_granularity = PLATEAUGranularityConverter.detect_granularity(typed_array)
	_log("Current granularity (detected): " + granularity_names[current_granularity])

	var start_time = Time.get_ticks_msec()
	var converted = converter.convert(typed_array, target_granularity)
	var convert_time = Time.get_ticks_msec() - start_time

	_log("Convert time: " + str(convert_time) + " ms")
	_log("Before: " + str(current_mesh_data.size()) + " meshes")
	_log("After: " + str(converted.size()) + " meshes")

	# Update current mesh data
	_clear_meshes()
	current_mesh_data = Array(converted)

	# Update options with new granularity
	if pending_options != null:
		pending_options.mesh_granularity = target_granularity

	# Create new city model root using import_to_scene
	_show_loading("Creating scene hierarchy...")
	city_model_root = importer.import_to_scene(
		converted,
		gml_path.get_file().get_basename() + "_converted",
		pending_geo_reference,
		pending_options,
		gml_path
	)

	if city_model_root != null:
		add_child(city_model_root)

		# Fit camera to bounds
		var bounds_min := Vector3.INF
		var bounds_max := -Vector3.INF
		for mesh_data in current_mesh_data:
			var mesh = mesh_data.get_mesh()
			if mesh != null and mesh.get_surface_count() > 0:
				var aabb = mesh.get_aabb()
				var mesh_transform = mesh_data.get_transform()
				var global_aabb = mesh_transform * aabb
				bounds_min = bounds_min.min(global_aabb.position)
				bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	_hide_loading()
	_update_stats()
	_log("[color=green]Conversion complete![/color]")


func _update_stats() -> void:
	var total_vertices = 0
	var total_triangles = 0

	for mesh_data in current_mesh_data:
		var mesh = mesh_data.get_mesh()
		if mesh != null:
			# Sum vertices and triangles from all surfaces
			for surface_idx in range(mesh.get_surface_count()):
				var arrays = mesh.surface_get_arrays(surface_idx)
				if arrays.size() > Mesh.ARRAY_VERTEX:
					var vertices = arrays[Mesh.ARRAY_VERTEX] as PackedVector3Array
					total_vertices += vertices.size()
				if arrays.size() > Mesh.ARRAY_INDEX:
					var indices = arrays[Mesh.ARRAY_INDEX] as PackedInt32Array
					@warning_ignore("integer_division")
					total_triangles += indices.size() / 3

	stats_label.text = "Meshes: %d | Vertices: %d | Triangles: %d" % [
		current_mesh_data.size(), total_vertices, total_triangles
	]


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	# Auto-scroll to bottom
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
