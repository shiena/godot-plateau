extends Node3D
## API Sample
##
## Demonstrates the main PLATEAU SDK APIs:
## - Import: Load CityGML files with various options
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
var current_mesh_data: Array = []
var mesh_instances: Array[MeshInstance3D] = []

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var stats_label: Label = $UI/StatsLabel


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

	_log("PLATEAU SDK API Sample ready.")
	_log("Click 'Import GML' to load a CityGML file.")


func _on_import_pressed() -> void:
	if gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _on_file_selected)
	else:
		_import_gml(gml_path)


func _on_file_selected(path: String) -> void:
	gml_path = path
	_import_gml(path)


func _on_granularity_changed(_index: int) -> void:
	if city_model == null:
		return
	_import_gml(gml_path)


func _import_gml(path: String) -> void:
	_log("--- Import Start ---")
	_log("File: " + path.get_file())

	# Clear previous
	_clear_meshes()

	# Load CityGML
	city_model = PLATEAUCityModel.new()
	var start_time = Time.get_ticks_msec()

	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load CityGML[/color]")
		return

	var load_time = Time.get_ticks_msec() - start_time
	_log("Load time: " + str(load_time) + " ms")

	# Get granularity setting
	var granularity_option = $UI/ImportPanel/GranularityOption as OptionButton
	var granularity = granularity_option.get_selected_id()

	# Setup extraction options
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.min_lod = $UI/ImportPanel/MinLODSpin.value
	options.max_lod = $UI/ImportPanel/MaxLODSpin.value
	options.mesh_granularity = granularity
	options.export_appearance = $UI/ImportPanel/TextureCheck.button_pressed

	_log("Options: LOD " + str(options.min_lod) + "-" + str(options.max_lod) +
		 ", Granularity: " + str(granularity) +
		 ", Textures: " + str(options.export_appearance))

	# Extract meshes
	start_time = Time.get_ticks_msec()
	var root_mesh_data = city_model.extract_meshes(options)
	var extract_time = Time.get_ticks_msec() - start_time

	_log("Extract time: " + str(extract_time) + " ms")
	_log("Root nodes extracted: " + str(root_mesh_data.size()))

	# Flatten hierarchy to get actual meshes (children of LOD nodes)
	current_mesh_data = PLATEAUUtils.flatten_mesh_data(Array(root_mesh_data))
	_log("Meshes after flatten: " + str(current_mesh_data.size()))

	if current_mesh_data.is_empty():
		_log("[color=yellow]WARNING: No meshes extracted[/color]")
		return

	# Create visual representation
	_create_mesh_instances()
	_update_stats()
	_log("[color=green]Import complete![/color]")


func _clear_meshes() -> void:
	PLATEAUUtils.clear_mesh_instances(mesh_instances)
	current_mesh_data.clear()


func _create_mesh_instances() -> void:
	var result = PLATEAUUtils.create_mesh_instances(current_mesh_data, self)
	mesh_instances = result["instances"]
	PLATEAUUtils.fit_camera_to_bounds(camera, result["bounds_min"], result["bounds_max"])


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

	var current = PLATEAUGranularityConverter.detect_granularity(typed_array)
	_log("Current granularity (detected): " + granularity_names[current])

	var start_time = Time.get_ticks_msec()
	var converted = converter.convert(typed_array, target_granularity)
	var convert_time = Time.get_ticks_msec() - start_time

	_log("Convert time: " + str(convert_time) + " ms")
	_log("Before: " + str(current_mesh_data.size()) + " meshes")
	_log("After: " + str(converted.size()) + " meshes")

	# Update current mesh data
	_clear_meshes()
	current_mesh_data = Array(converted)
	_create_mesh_instances()
	_update_stats()

	_log("[color=green]Conversion complete![/color]")


func _update_stats() -> void:
	var total_vertices = 0
	var total_triangles = 0

	for mesh_data in current_mesh_data:
		var mesh = mesh_data.get_mesh()
		if mesh != null and mesh.get_surface_count() > 0:
			var arrays = mesh.surface_get_arrays(0)
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
