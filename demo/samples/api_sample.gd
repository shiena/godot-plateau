extends Node3D
## API Sample
##
## Demonstrates the main PLATEAU SDK APIs using DatasetSource:
## - Import: Select dataset folder → choose packages → import GML files
## - Export: Save meshes to glTF/GLB/OBJ formats
## - Granularity: Convert between mesh granularity levels
##
## Based on Unity SDK's RuntimeAPISample pattern.

@export var zone_id: int = 9  ## Japan Plane Rectangular CS (1-19)

var dataset_source: PLATEAUDatasetSource
var dataset_path: String = ""
var city_model: PLATEAUCityModel
var current_mesh_data: Array = []
var imported_root: Node3D = null
var pending_options: PLATEAUMeshExtractOptions
var pending_gml_files: Array = []
var current_gml_index: int = 0
var bounds_min: Vector3 = Vector3.INF
var bounds_max: Vector3 = -Vector3.INF

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var stats_label: Label = $UI/StatsLabel
@onready var loading_panel: Panel = $UI/LoadingPanel
@onready var loading_label: Label = $UI/LoadingPanel/LoadingLabel


func _ready() -> void:
	# Connect dataset selection buttons
	$UI/DatasetPanel/SelectFolderButton.pressed.connect(_on_select_folder_pressed)
	$UI/DatasetPanel/ImportButton.pressed.connect(_on_import_pressed)

	# Connect export buttons
	$UI/ExportPanel/ExportGLTFButton.pressed.connect(func(): _export_meshes(PLATEAUMeshExporter.EXPORT_FORMAT_GLTF))
	$UI/ExportPanel/ExportGLBButton.pressed.connect(func(): _export_meshes(PLATEAUMeshExporter.EXPORT_FORMAT_GLB))
	$UI/ExportPanel/ExportOBJButton.pressed.connect(func(): _export_meshes(PLATEAUMeshExporter.EXPORT_FORMAT_OBJ))

	# Connect convert buttons
	$UI/ConvertPanel/ConvertAtomicButton.pressed.connect(func(): _convert_granularity(PLATEAUGranularityConverter.CONVERT_GRANULARITY_ATOMIC))
	$UI/ConvertPanel/ConvertPrimaryButton.pressed.connect(func(): _convert_granularity(PLATEAUGranularityConverter.CONVERT_GRANULARITY_PRIMARY))
	$UI/ConvertPanel/ConvertAreaButton.pressed.connect(func(): _convert_granularity(PLATEAUGranularityConverter.CONVERT_GRANULARITY_AREA))

	# Setup granularity options
	var granularity_option = $UI/DatasetPanel/GranularityOption as OptionButton
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

	# Initial UI state
	_update_import_button_state()

	_log("PLATEAU SDK API Sample ready.")
	_log("Click 'Select Dataset Folder...' to choose a PLATEAU dataset.")


func _on_select_folder_pressed() -> void:
	PLATEAUUtils.show_directory_dialog(self, _on_folder_selected)


func _on_folder_selected(path: String) -> void:
	_log("--- Dataset Selection ---")
	_log("Folder: " + path)

	# Store path and create dataset source
	dataset_path = path
	dataset_source = PLATEAUDatasetSource.create_local(path)

	if not dataset_source.is_valid():
		_log("[color=red]ERROR: Invalid dataset folder. Not a valid PLATEAU dataset.[/color]")
		_clear_package_list()
		return

	# Show available packages
	var packages = dataset_source.get_available_packages()
	_log("Available packages: " + _format_packages(packages))

	_update_package_checkboxes(packages)
	_update_import_button_state()


func _update_package_checkboxes(packages: int) -> void:
	var package_container = $UI/DatasetPanel/PackageScroll/PackageContainer
	# Clear existing checkboxes
	for child in package_container.get_children():
		child.queue_free()

	# Package definitions
	var package_info = [
		{"flag": PLATEAUDatasetSource.PACKAGE_BUILDING, "name": "Building (bldg)", "display": "建築物"},
		{"flag": PLATEAUDatasetSource.PACKAGE_ROAD, "name": "Road (tran)", "display": "道路"},
		{"flag": PLATEAUDatasetSource.PACKAGE_RELIEF, "name": "Relief (dem)", "display": "地形"},
		{"flag": PLATEAUDatasetSource.PACKAGE_LAND_USE, "name": "LandUse (luse)", "display": "土地利用"},
		{"flag": PLATEAUDatasetSource.PACKAGE_CITY_FURNITURE, "name": "CityFurniture (frn)", "display": "都市設備"},
		{"flag": PLATEAUDatasetSource.PACKAGE_VEGETATION, "name": "Vegetation (veg)", "display": "植生"},
		{"flag": PLATEAUDatasetSource.PACKAGE_WATER_BODY, "name": "WaterBody (wtr)", "display": "水部"},
		{"flag": PLATEAUDatasetSource.PACKAGE_BRIDGE, "name": "Bridge (brid)", "display": "橋梁"},
		{"flag": PLATEAUDatasetSource.PACKAGE_TUNNEL, "name": "Tunnel (tun)", "display": "トンネル"},
		{"flag": PLATEAUDatasetSource.PACKAGE_FLOOD, "name": "Flood (fld)", "display": "浸水想定区域"},
		{"flag": PLATEAUDatasetSource.PACKAGE_URBAN_PLANNING, "name": "UrbanPlanning (urf)", "display": "都市計画"},
	]

	for info in package_info:
		if packages & info["flag"]:
			var checkbox = CheckBox.new()
			checkbox.text = info["display"] + " - " + info["name"]
			checkbox.set_meta("package_flag", info["flag"])
			checkbox.toggled.connect(func(_pressed): _update_import_button_state())
			# Default: select Building if available
			if info["flag"] == PLATEAUDatasetSource.PACKAGE_BUILDING:
				checkbox.button_pressed = true
			package_container.add_child(checkbox)


func _clear_package_list() -> void:
	var package_container = $UI/DatasetPanel/PackageScroll/PackageContainer
	for child in package_container.get_children():
		child.queue_free()
	_update_import_button_state()


func _get_selected_packages() -> int:
	var selected: int = 0
	var package_container = $UI/DatasetPanel/PackageScroll/PackageContainer
	for child in package_container.get_children():
		if child is CheckBox and child.button_pressed:
			selected |= child.get_meta("package_flag", 0)
	return selected


func _update_import_button_state() -> void:
	var import_button = $UI/DatasetPanel/ImportButton as Button
	var has_dataset = dataset_source != null and dataset_source.is_valid()
	var has_selection = _get_selected_packages() > 0
	import_button.disabled = not (has_dataset and has_selection)


func _on_import_pressed() -> void:
	if dataset_source == null or not dataset_source.is_valid():
		_log("[color=red]ERROR: No valid dataset selected.[/color]")
		return

	if city_model.is_processing():
		_log("[color=yellow]Already processing, please wait...[/color]")
		return

	var selected_packages = _get_selected_packages()
	if selected_packages == 0:
		_log("[color=yellow]No packages selected.[/color]")
		return

	_log("--- Import Start ---")
	_log("Selected packages: " + _format_packages(selected_packages))

	# Get GML files for selected packages
	var gml_files = dataset_source.get_gml_files(selected_packages)
	_log("GML files found: " + str(gml_files.size()))

	if gml_files.is_empty():
		_log("[color=yellow]No GML files found for selected packages.[/color]")
		return

	# List mesh codes
	var mesh_codes: Dictionary = {}
	for file_info in gml_files:
		var code = file_info.get_mesh_code()
		if not code.is_empty():
			mesh_codes[code] = true
	_log("Mesh codes: " + str(mesh_codes.keys()))

	# Clear previous
	_clear_meshes()

	# Setup extraction options
	var granularity_option = $UI/DatasetPanel/GranularityOption as OptionButton
	var granularity = granularity_option.get_selected_id()

	pending_options = PLATEAUMeshExtractOptions.new()
	pending_options.coordinate_zone_id = int($UI/DatasetPanel/ZoneSpin.value)
	pending_options.min_lod = $UI/DatasetPanel/MinLODSpin.value
	pending_options.max_lod = $UI/DatasetPanel/MaxLODSpin.value
	pending_options.mesh_granularity = granularity
	pending_options.export_appearance = $UI/DatasetPanel/TextureCheck.button_pressed

	_log("Options: LOD " + str(pending_options.min_lod) + "-" + str(pending_options.max_lod) +
		 ", Granularity: " + str(granularity) +
		 ", Textures: " + str(pending_options.export_appearance))

	# Store GML files for sequential import
	pending_gml_files = Array(gml_files)
	current_gml_index = 0

	# Reset bounds
	bounds_min = Vector3.INF
	bounds_max = -Vector3.INF

	# Create root node for imported data
	var root_name = dataset_path.get_file()
	if root_name.is_empty():
		root_name = "PLATEAU_Import"
	imported_root = Node3D.new()
	imported_root.name = root_name
	add_child(imported_root)

	# Start importing first GML file
	_import_next_gml()


func _import_next_gml() -> void:
	if current_gml_index >= pending_gml_files.size():
		# All done
		_finalize_import()
		return

	var file_info = pending_gml_files[current_gml_index]
	var path = file_info.get_path()
	var progress_text = "(%d/%d)" % [current_gml_index + 1, pending_gml_files.size()]

	_show_loading("Loading GML " + progress_text + "...")
	_log("Loading: " + path.get_file() + " " + progress_text)

	city_model.load_async(path)


func _on_load_completed(success: bool) -> void:
	if not success:
		_log("[color=red]ERROR: Failed to load GML[/color]")
		# Continue to next file
		current_gml_index += 1
		_import_next_gml()
		return

	var progress_text = "(%d/%d)" % [current_gml_index + 1, pending_gml_files.size()]
	_update_loading("Extracting meshes " + progress_text + "...")
	city_model.extract_meshes_async(pending_options)


func _on_extract_completed(root_mesh_data: Array) -> void:
	# Flatten hierarchy to get actual meshes
	var flat_meshes = PLATEAUUtils.flatten_mesh_data(root_mesh_data)
	var gml_name = pending_gml_files[current_gml_index].get_path().get_file()
	_log("Extracted " + str(flat_meshes.size()) + " meshes from " + gml_name)

	# Add to current mesh data for export/conversion
	current_mesh_data.append_array(flat_meshes)

	# Create mesh instances incrementally (with batching to prevent blocking)
	var progress_text = "(%d/%d)" % [current_gml_index + 1, pending_gml_files.size()]
	_update_loading("Creating meshes " + progress_text + "...")

	# Create a container node for this GML file
	var gml_container = Node3D.new()
	gml_container.name = gml_name.get_basename()
	imported_root.add_child(gml_container)

	# Batch processing to prevent UI freeze
	var batch_size = 50
	var total = flat_meshes.size()
	for i in range(0, total, batch_size):
		var batch_end = mini(i + batch_size, total)
		for j in range(i, batch_end):
			var mesh_data = flat_meshes[j]
			var mesh = mesh_data.get_mesh()
			if mesh == null or mesh.get_surface_count() == 0:
				continue

			var mesh_instance = MeshInstance3D.new()
			mesh_instance.mesh = mesh
			mesh_instance.transform = mesh_data.get_transform()
			mesh_instance.name = mesh_data.get_name()
			gml_container.add_child(mesh_instance)

			# Update bounds
			var aabb = mesh.get_aabb()
			var tx = mesh_data.get_transform()
			bounds_min = bounds_min.min(tx * aabb.position)
			bounds_max = bounds_max.max(tx * aabb.end)

		# Yield to allow UI updates
		if total > batch_size:
			var percent = int((batch_end * 100.0) / total) if total > 0 else 100
			_update_loading("Creating meshes " + progress_text + ": %d%%" % percent)
			await get_tree().process_frame

	# Move to next GML file
	current_gml_index += 1
	_import_next_gml()


func _finalize_import() -> void:
	_log("Total meshes: " + str(current_mesh_data.size()))

	if current_mesh_data.is_empty():
		_hide_loading()
		_log("[color=yellow]WARNING: No meshes extracted[/color]")
		# Remove empty root
		if imported_root:
			imported_root.queue_free()
			imported_root = null
		return

	# Fit camera to accumulated bounds
	if bounds_min != Vector3.INF and bounds_max != -Vector3.INF:
		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	_hide_loading()
	_update_stats()
	_log("[color=green]Import complete![/color]")


func _format_packages(packages: int) -> String:
	var names: PackedStringArray = []
	if packages & PLATEAUDatasetSource.PACKAGE_BUILDING: names.append("Building")
	if packages & PLATEAUDatasetSource.PACKAGE_ROAD: names.append("Road")
	if packages & PLATEAUDatasetSource.PACKAGE_RELIEF: names.append("Relief")
	if packages & PLATEAUDatasetSource.PACKAGE_LAND_USE: names.append("LandUse")
	if packages & PLATEAUDatasetSource.PACKAGE_CITY_FURNITURE: names.append("CityFurniture")
	if packages & PLATEAUDatasetSource.PACKAGE_VEGETATION: names.append("Vegetation")
	if packages & PLATEAUDatasetSource.PACKAGE_WATER_BODY: names.append("WaterBody")
	if packages & PLATEAUDatasetSource.PACKAGE_BRIDGE: names.append("Bridge")
	if packages & PLATEAUDatasetSource.PACKAGE_TUNNEL: names.append("Tunnel")
	if packages & PLATEAUDatasetSource.PACKAGE_FLOOD: names.append("Flood")
	if packages & PLATEAUDatasetSource.PACKAGE_URBAN_PLANNING: names.append("UrbanPlanning")
	return ", ".join(names) if not names.is_empty() else "(none)"


func _show_loading(message: String) -> void:
	loading_label.text = message
	loading_panel.visible = true


func _update_loading(message: String) -> void:
	loading_label.text = message


func _hide_loading() -> void:
	loading_panel.visible = false


func _clear_meshes() -> void:
	if imported_root:
		imported_root.queue_free()
		imported_root = null
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

	_show_loading("Converting granularity...")

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

	# Clear previous meshes
	_clear_meshes()
	current_mesh_data = Array(converted)

	# Create root node
	var root_name = "Converted_" + granularity_names[target_granularity]
	imported_root = Node3D.new()
	imported_root.name = root_name
	add_child(imported_root)

	# Reset bounds
	bounds_min = Vector3.INF
	bounds_max = -Vector3.INF

	# Batch processing to prevent UI freeze
	_update_loading("Creating mesh instances...")
	var batch_size = 50
	var total = converted.size()
	for i in range(0, total, batch_size):
		var batch_end = mini(i + batch_size, total)
		for j in range(i, batch_end):
			var mesh_data = converted[j]
			var mesh = mesh_data.get_mesh()
			if mesh == null or mesh.get_surface_count() == 0:
				continue

			var mesh_instance = MeshInstance3D.new()
			mesh_instance.mesh = mesh
			mesh_instance.transform = mesh_data.get_transform()
			mesh_instance.name = mesh_data.get_name()
			imported_root.add_child(mesh_instance)

			# Update bounds
			var aabb = mesh.get_aabb()
			var tx = mesh_data.get_transform()
			bounds_min = bounds_min.min(tx * aabb.position)
			bounds_max = bounds_max.max(tx * aabb.end)

		# Yield to allow UI updates
		if total > batch_size:
			var percent = int((batch_end * 100.0) / total) if total > 0 else 100
			_update_loading("Creating instances: %d%%" % percent)
			await get_tree().process_frame

	# Fit camera
	if bounds_min != Vector3.INF and bounds_max != -Vector3.INF:
		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	_hide_loading()
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
