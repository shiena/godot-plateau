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
@export var memory_optimized: bool = true  ## Clear mesh data after import to save memory (export will re-extract)

var dataset_source: PLATEAUDatasetSource
var dataset_path: String = ""
var city_model: PLATEAUCityModel
var current_mesh_data: Array = []
var imported_root: Node3D = null  # Container for all PLATEAUInstancedCityModel nodes
var pending_options: PLATEAUMeshExtractOptions
var pending_geo_reference: PLATEAUGeoReference
var pending_gml_files: Array = []
var current_gml_index: int = 0
var bounds_min: Vector3 = Vector3.INF
var bounds_max: Vector3 = -Vector3.INF
var all_gml_files: Array = []  # All GML files for selected packages
var mesh_code_to_files: Dictionary = {}  # mesh_code -> [PLATEAUGmlFileInfo]

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
		{"flag": PLATEAUDatasetSource.PACKAGE_DISASTER_RISK, "name": "DisasterRisk (fld)", "display": "災害リスク"},
		{"flag": PLATEAUDatasetSource.PACKAGE_URBAN_PLANNING, "name": "UrbanPlanning (urf)", "display": "都市計画"},
	]

	for info in package_info:
		if packages & info["flag"]:
			var checkbox = CheckBox.new()
			checkbox.text = info["display"] + " - " + info["name"]
			checkbox.set_meta("package_flag", info["flag"])
			checkbox.toggled.connect(func(_pressed): _on_package_selection_changed())
			# Default: select Building if available
			if info["flag"] == PLATEAUDatasetSource.PACKAGE_BUILDING:
				checkbox.button_pressed = true
			package_container.add_child(checkbox)

	# Update mesh codes for initial selection
	_on_package_selection_changed()


func _clear_package_list() -> void:
	var package_container = $UI/DatasetPanel/PackageScroll/PackageContainer
	for child in package_container.get_children():
		child.queue_free()
	_clear_mesh_code_list()
	_update_import_button_state()


func _on_package_selection_changed() -> void:
	# Get GML files for selected packages and update mesh code list
	var selected_packages = _get_selected_packages()
	if selected_packages == 0 or dataset_source == null or not dataset_source.is_valid():
		_clear_mesh_code_list()
		_update_import_button_state()
		return

	all_gml_files = Array(dataset_source.get_gml_files(selected_packages))
	mesh_code_to_files.clear()

	# Group files by mesh code
	for file_info in all_gml_files:
		var code = file_info.get_mesh_code()
		if code.is_empty():
			code = "(no code)"
		if not mesh_code_to_files.has(code):
			mesh_code_to_files[code] = []
		mesh_code_to_files[code].append(file_info)

	_update_mesh_code_checkboxes()
	_update_import_button_state()


func _update_mesh_code_checkboxes() -> void:
	var mesh_code_container = $UI/DatasetPanel/MeshCodeScroll/MeshCodeContainer
	# Clear existing checkboxes
	for child in mesh_code_container.get_children():
		child.queue_free()

	# Sort mesh codes
	var codes = mesh_code_to_files.keys()
	codes.sort()

	for code in codes:
		var file_count = mesh_code_to_files[code].size()
		var checkbox = CheckBox.new()
		checkbox.text = "%s (%d files)" % [code, file_count]
		checkbox.set_meta("mesh_code", code)
		checkbox.toggled.connect(func(_pressed): _update_import_button_state())
		mesh_code_container.add_child(checkbox)


func _clear_mesh_code_list() -> void:
	var mesh_code_container = $UI/DatasetPanel/MeshCodeScroll/MeshCodeContainer
	for child in mesh_code_container.get_children():
		child.queue_free()
	all_gml_files.clear()
	mesh_code_to_files.clear()


func _get_selected_mesh_codes() -> PackedStringArray:
	var selected: PackedStringArray = []
	var mesh_code_container = $UI/DatasetPanel/MeshCodeScroll/MeshCodeContainer
	for child in mesh_code_container.get_children():
		if child is CheckBox and child.button_pressed:
			selected.append(child.get_meta("mesh_code", ""))
	return selected


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
	var has_packages = _get_selected_packages() > 0
	var has_mesh_codes = _get_selected_mesh_codes().size() > 0
	import_button.disabled = not (has_dataset and has_packages and has_mesh_codes)


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

	var selected_mesh_codes = _get_selected_mesh_codes()
	if selected_mesh_codes.is_empty():
		_log("[color=yellow]No mesh codes selected.[/color]")
		return

	_log("--- Import Start ---")
	_log("Selected packages: " + _format_packages(selected_packages))
	_log("Selected mesh codes: " + ", ".join(selected_mesh_codes))

	# Filter GML files by selected mesh codes
	var gml_files: Array = []
	for code in selected_mesh_codes:
		if mesh_code_to_files.has(code):
			gml_files.append_array(mesh_code_to_files[code])

	_log("GML files to import: " + str(gml_files.size()))

	if gml_files.is_empty():
		_log("[color=yellow]No GML files found for selected mesh codes.[/color]")
		return

	# Clear previous
	_clear_meshes()

	# Setup extraction options
	var granularity_option = $UI/DatasetPanel/GranularityOption as OptionButton
	var granularity = granularity_option.get_selected_id()
	var current_zone_id = int($UI/DatasetPanel/ZoneSpin.value)

	pending_options = PLATEAUMeshExtractOptions.new()
	pending_options.coordinate_zone_id = current_zone_id
	pending_options.min_lod = $UI/DatasetPanel/MinLODSpin.value
	pending_options.max_lod = $UI/DatasetPanel/MaxLODSpin.value
	pending_options.mesh_granularity = granularity
	pending_options.export_appearance = $UI/DatasetPanel/TextureCheck.button_pressed
	pending_options.highest_lod_only = true

	# Create GeoReference for import
	pending_geo_reference = PLATEAUGeoReference.new()
	pending_geo_reference.zone_id = current_zone_id

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

	# Show GML file metadata using PLATEAUGmlFile (like Unity)
	var gml_file = PLATEAUGmlFile.create(path)
	if gml_file.is_valid():
		_log("  Grid Code: " + gml_file.get_grid_code() + ", Type: " + gml_file.get_feature_type())
		var textures = gml_file.search_image_paths()
		if textures.size() > 0:
			var appearance_dir = gml_file.get_appearance_directory_path()
			_log("  Loading textures in " + appearance_dir)
			# Show texture filenames (up to 3 for compact display)
			var texture_names: PackedStringArray = []
			for i in range(mini(3, textures.size())):
				texture_names.append(textures[i].get_file())
			var names_str = ", ".join(texture_names)
			if textures.size() > 3:
				names_str += ", ..."
			_log("  Loaded " + str(textures.size()) + " textures: " + names_str)

	city_model.load_async(path)


func _on_load_completed(success: bool) -> void:
	if not success:
		_log("[color=red]ERROR: Failed to load GML[/color]")
		# Continue to next file
		current_gml_index += 1
		_import_next_gml()
		return

	# Update geo reference with center point
	var center = city_model.get_center_point(pending_geo_reference.zone_id)
	pending_geo_reference.reference_point = center
	pending_options.reference_point = center

	var progress_text = "(%d/%d)" % [current_gml_index + 1, pending_gml_files.size()]
	_update_loading("Extracting meshes " + progress_text + "...")
	city_model.extract_meshes_async(pending_options)


func _on_extract_completed(root_mesh_data: Array) -> void:
	var gml_path = pending_gml_files[current_gml_index].get_path()
	var gml_name = gml_path.get_file()

	# Create mesh instances incrementally (with batching to prevent blocking)
	var progress_text = "(%d/%d)" % [current_gml_index + 1, pending_gml_files.size()]
	_update_loading("Creating scene " + progress_text + "...")

	# Use PLATEAUUtils to import to city model
	var result = PLATEAUUtils.import_to_city_model(
		root_mesh_data,
		imported_root,
		gml_name.get_basename(),
		pending_geo_reference,
		pending_options,
		gml_path
	)

	var city_model_node: PLATEAUInstancedCityModel = result["city_model_root"]
	var flat_meshes: Array = result["flat_mesh_data"]

	_log("Extracted " + str(flat_meshes.size()) + " meshes from " + gml_name)

	# Add to current mesh data for export/conversion
	current_mesh_data.append_array(flat_meshes)

	# Update bounds
	if result["bounds_min"] != Vector3.INF:
		bounds_min = bounds_min.min(result["bounds_min"])
		bounds_max = bounds_max.max(result["bounds_max"])

	# Log PLATEAUInstancedCityModel info for first file
	if city_model_node != null and current_gml_index == 0:
		_log("[color=cyan]PLATEAUInstancedCityModel Info:[/color]")
		_log("  Zone ID: " + str(city_model_node.zone_id))
		_log("  Latitude: %.6f" % city_model_node.get_latitude())
		_log("  Longitude: %.6f" % city_model_node.get_longitude())

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

	# Memory optimization: clear mesh data after import
	# Export will re-extract from GML files, so we don't need to keep this data
	if memory_optimized and pending_gml_files.size() > 1:
		var mesh_count = current_mesh_data.size()
		current_mesh_data.clear()
		_log("[color=cyan]Memory optimized: Cleared %d mesh data references[/color]" % mesh_count)
		_log("[color=cyan]Note: Granularity conversion disabled, export will re-extract[/color]")

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
	if packages & PLATEAUDatasetSource.PACKAGE_DISASTER_RISK: names.append("DisasterRisk")
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
	# Check if we have data to export (either mesh data or GML files for re-extraction)
	if current_mesh_data.is_empty() and pending_gml_files.is_empty():
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

	var start_time = Time.get_ticks_msec()

	# Memory optimization: If mesh data was cleared or multiple GML files, export per-file
	if current_mesh_data.is_empty() or pending_gml_files.size() > 1:
		if pending_gml_files.is_empty():
			_log("[color=red]ERROR: No GML files to export from[/color]")
			return
		_log("Exporting %d files with memory optimization..." % pending_gml_files.size())
		_do_export_per_file(path, format)
		return

	# Single file with mesh data available: use current_mesh_data
	var exporter = PLATEAUMeshExporter.new()

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


## Export each GML file to a separate output file (memory optimized)
func _do_export_per_file(base_path: String, format: int) -> void:
	var start_time = Time.get_ticks_msec()
	var output_dir = base_path.get_base_dir()
	var output_ext = base_path.get_extension()
	var total_files = pending_gml_files.size()
	var export_count = 0
	var total_meshes = 0

	_show_loading("Exporting...")

	for i in range(total_files):
		var file_info = pending_gml_files[i]
		var gml_path = file_info.get_path()
		var file_name = gml_path.get_file()

		_update_loading("Exporting: %s (%d/%d)" % [file_name, i + 1, total_files])

		# Create new city model for this file
		var temp_city_model = PLATEAUCityModel.new()
		if not temp_city_model.load(gml_path):
			_log("[color=yellow]Skip: Failed to load " + file_name + "[/color]")
			continue

		# Extract meshes
		var mesh_data = temp_city_model.extract_meshes(pending_options)
		if mesh_data.is_empty():
			_log("[color=yellow]Skip: No meshes in " + file_name + "[/color]")
			temp_city_model = null
			continue

		# Flatten mesh data
		var flat_meshes = PLATEAUUtils.flatten_mesh_data(Array(mesh_data))
		total_meshes += flat_meshes.size()

		# Create output path for this file
		var file_output_name = file_name.get_basename() + "." + output_ext
		var file_output_path = output_dir.path_join(file_output_name)

		# Export
		var exporter = PLATEAUMeshExporter.new()
		var gml_dir = gml_path.get_base_dir()
		exporter.texture_directory = gml_dir.get_base_dir()

		var typed_array: Array[PLATEAUMeshData] = []
		for md in flat_meshes:
			typed_array.append(md)

		var success = exporter.export_to_file(typed_array, file_output_path, format)

		if success:
			export_count += 1
			_log("Exported: %s (%d meshes)" % [file_output_path.get_file(), flat_meshes.size()])
		else:
			_log("[color=red]Failed: " + file_output_path.get_file() + "[/color]")

		# Clear references to release memory
		flat_meshes.clear()
		mesh_data.clear()
		temp_city_model = null
		exporter = null

	_hide_loading()

	var export_time = Time.get_ticks_msec() - start_time

	if export_count > 0:
		_log("Export time: " + str(export_time) + " ms")
		_log("[color=green]Export complete: %d files, %d meshes[/color]" % [export_count, total_meshes])
		_log("Output directory: " + output_dir)
	else:
		_log("[color=red]ERROR: No files exported[/color]")


func _convert_granularity(target_granularity: int) -> void:
	if current_mesh_data.is_empty():
		if memory_optimized and pending_gml_files.size() > 1:
			_log("[color=yellow]Granularity conversion disabled in memory optimized mode.[/color]")
			_log("[color=yellow]Set 'memory_optimized = false' to enable conversion.[/color]")
		else:
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

	var current_granularity = PLATEAUGranularityConverter.detect_granularity(typed_array)
	_log("Current granularity (detected): " + granularity_names[current_granularity])

	var start_time = Time.get_ticks_msec()
	var converted = converter.convert(typed_array, target_granularity)
	var convert_time = Time.get_ticks_msec() - start_time

	_log("Convert time: " + str(convert_time) + " ms")
	_log("Before: " + str(current_mesh_data.size()) + " meshes")
	_log("After: " + str(converted.size()) + " meshes")

	# Clear previous meshes
	_clear_meshes()
	current_mesh_data = Array(converted)

	# Update options with new granularity
	if pending_options != null:
		pending_options.mesh_granularity = target_granularity

	# Create root container
	var root_name = "Converted_" + granularity_names[target_granularity]
	imported_root = Node3D.new()
	imported_root.name = root_name
	add_child(imported_root)

	# Use PLATEAUUtils to import to city model
	_update_loading("Creating scene hierarchy...")
	var result = PLATEAUUtils.import_to_city_model(
		Array(converted),
		imported_root,
		root_name,
		pending_geo_reference,
		pending_options,
		""
	)

	bounds_min = result["bounds_min"]
	bounds_max = result["bounds_max"]

	# Fit camera
	if bounds_min != Vector3.INF and bounds_max != -Vector3.INF:
		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	_hide_loading()
	_update_stats()
	_log("[color=green]Conversion complete![/color]")


func _update_stats() -> void:
	var total_vertices = 0
	var total_triangles = 0
	var mesh_count = 0

	# If current_mesh_data is available, use it
	if not current_mesh_data.is_empty():
		for mesh_data in current_mesh_data:
			var mesh = mesh_data.get_mesh()
			if mesh != null:
				mesh_count += 1
				_count_mesh_stats(mesh, total_vertices, total_triangles)
	else:
		# Fallback: count from scene MeshInstance3D nodes
		if imported_root != null:
			var stats = _count_scene_mesh_stats(imported_root)
			mesh_count = stats["meshes"]
			total_vertices = stats["vertices"]
			total_triangles = stats["triangles"]

	stats_label.text = "Meshes: %d | Vertices: %d | Triangles: %d" % [
		mesh_count, total_vertices, total_triangles
	]


func _count_mesh_stats(mesh: Mesh, vertices: int, triangles: int) -> void:
	for surface_idx in range(mesh.get_surface_count()):
		var arrays = mesh.surface_get_arrays(surface_idx)
		if arrays.size() > Mesh.ARRAY_VERTEX:
			var verts = arrays[Mesh.ARRAY_VERTEX] as PackedVector3Array
			vertices += verts.size()
		if arrays.size() > Mesh.ARRAY_INDEX:
			var indices = arrays[Mesh.ARRAY_INDEX] as PackedInt32Array
			@warning_ignore("integer_division")
			triangles += indices.size() / 3


func _count_scene_mesh_stats(node: Node) -> Dictionary:
	var result = {"meshes": 0, "vertices": 0, "triangles": 0}

	if node is MeshInstance3D:
		var mi = node as MeshInstance3D
		if mi.mesh != null:
			result["meshes"] += 1
			for surface_idx in range(mi.mesh.get_surface_count()):
				var arrays = mi.mesh.surface_get_arrays(surface_idx)
				if arrays.size() > Mesh.ARRAY_VERTEX:
					var verts = arrays[Mesh.ARRAY_VERTEX] as PackedVector3Array
					result["vertices"] += verts.size()
				if arrays.size() > Mesh.ARRAY_INDEX:
					var indices = arrays[Mesh.ARRAY_INDEX] as PackedInt32Array
					@warning_ignore("integer_division")
					result["triangles"] += indices.size() / 3

	for child in node.get_children():
		var child_stats = _count_scene_mesh_stats(child)
		result["meshes"] += child_stats["meshes"]
		result["vertices"] += child_stats["vertices"]
		result["triangles"] += child_stats["triangles"]

	return result


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	# Auto-scroll to bottom
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
