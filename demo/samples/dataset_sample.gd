extends Node3D
## Dataset Sample
##
## Demonstrates PLATEAU dataset access APIs:
## - HTTPRequest: Browse datasets from PLATEAU server
## - PLATEAUDatasetSource: Access local datasets
## - PLATEAUGmlFileInfo: Get GML file information
##
## Usage:
## 1. Configure server settings (URL, token, or use mock)
## 2. Click "Fetch Server Datasets" to browse available datasets
## 3. Or click "Open Local Dataset" to select a local PLATEAU folder
## 4. Select a package type to view GML files

# Server URLs are retrieved from PLATEAUDatasetSource static methods
# (same values as libplateau's src/network/client.cpp)
# Note: API token is NOT exposed to GDScript for security reasons.
# Use build_auth_headers() which handles token internally.
var PRODUCTION_SERVER: String
var MOCK_SERVER: String

@export_dir var local_dataset_path: String = ""

var current_source: PLATEAUDatasetSource
var selected_dataset_id: String = ""

@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var group_list: ItemList = $UI/DatasetPanel/GroupList
@onready var dataset_list: ItemList = $UI/DatasetPanel/DatasetList
@onready var package_option: OptionButton = $UI/FilePanel/PackageOption
@onready var file_list: ItemList = $UI/FilePanel/FileList
@onready var mesh_code_list: ItemList = $UI/FilePanel/MeshCodeList

@onready var server_url_edit: LineEdit = $UI/ServerPanel/ServerUrlEdit
@onready var token_edit: LineEdit = $UI/ServerPanel/TokenEdit
@onready var mock_check: CheckBox = $UI/ServerPanel/MockCheck
@onready var status_label: Label = $UI/ServerPanel/StatusLabel

@onready var http_request: HTTPRequest = $HTTPRequest
@onready var download_request: HTTPRequest = $DownloadRequest
@onready var camera: Camera3D = $Camera3D
@onready var mesh_container: Node3D = $MeshContainer

var dataset_groups: Array = []
var mesh_instances: Array[MeshInstance3D] = []
var pending_request_type: String = ""
var download_url: String = ""
var download_dest: String = ""


func _ready() -> void:
	# Initialize server URLs from C++ (same as libplateau)
	PRODUCTION_SERVER = PLATEAUDatasetSource.get_default_server_url()
	MOCK_SERVER = PLATEAUDatasetSource.get_mock_server_url()

	# Set default server URL in UI
	server_url_edit.text = PRODUCTION_SERVER

	# Connect buttons
	$UI/ServerPanel/FetchButton.pressed.connect(_on_fetch_server_pressed)
	$UI/ServerPanel/OpenLocalButton.pressed.connect(_on_open_local_pressed)
	mock_check.toggled.connect(_on_mock_toggled)

	group_list.item_selected.connect(_on_group_selected)
	dataset_list.item_selected.connect(_on_dataset_selected)
	package_option.item_selected.connect(_on_package_selected)

	$UI/FilePanel/LoadGMLButton.pressed.connect(_on_load_gml_pressed)
	$UI/FilePanel/DownloadButton.pressed.connect(_on_download_pressed)

	# Connect HTTP signals
	http_request.request_completed.connect(_on_http_request_completed)
	download_request.request_completed.connect(_on_download_completed)

	# Setup package options
	_setup_package_options()

	_log("Dataset Sample ready.")
	_log("Configure server settings and click 'Fetch Server Datasets'.")
	_log("Or click 'Open Local Dataset' to select a local folder.")
	_log("")
	_log("Note: Production server requires Bearer token.")
	_log("Check 'Use Mock Server' for testing without auth.")


func _setup_package_options() -> void:
	package_option.clear()
	package_option.add_item("Building (bldg)", PLATEAUDatasetSource.PACKAGE_BUILDING)
	package_option.add_item("Road (tran)", PLATEAUDatasetSource.PACKAGE_ROAD)
	package_option.add_item("Relief/DEM (dem)", PLATEAUDatasetSource.PACKAGE_RELIEF)
	package_option.add_item("Land Use (luse)", PLATEAUDatasetSource.PACKAGE_LAND_USE)
	package_option.add_item("Vegetation (veg)", PLATEAUDatasetSource.PACKAGE_VEGETATION)
	package_option.add_item("City Furniture (frn)", PLATEAUDatasetSource.PACKAGE_CITY_FURNITURE)
	package_option.add_item("Bridge (brid)", PLATEAUDatasetSource.PACKAGE_BRIDGE)
	package_option.add_item("Flood (fld)", PLATEAUDatasetSource.PACKAGE_FLOOD)
	package_option.add_item("All Packages", PLATEAUDatasetSource.PACKAGE_ALL)


func _on_mock_toggled(enabled: bool) -> void:
	if enabled:
		server_url_edit.text = MOCK_SERVER
		token_edit.editable = false
		token_edit.placeholder_text = "Not required for mock server"
		_log("Switched to mock server (no auth required)")
	else:
		server_url_edit.text = PRODUCTION_SERVER
		token_edit.editable = true
		token_edit.placeholder_text = "Bearer token (required for production server)"
		_log("Switched to production server (auth required)")


func _get_server_url() -> String:
	return server_url_edit.text.strip_edges()


func _get_auth_headers() -> PackedStringArray:
	# Build auth headers using C++ helper (token is handled internally for security)
	# - custom_token: User-entered token (empty = use default internally)
	# - use_default_token: true for production server, false for mock
	return PLATEAUDatasetSource.build_auth_headers(
		token_edit.text.strip_edges(),
		not mock_check.button_pressed
	)


func _on_fetch_server_pressed() -> void:
	_log("--- Fetching Server Datasets ---")

	var url = _get_server_url()
	if url.is_empty():
		_log("[color=red]ERROR: Server URL is empty[/color]")
		return

	_log("Connecting to: " + url)
	status_label.text = "Connecting..."

	group_list.clear()
	dataset_list.clear()
	file_list.clear()
	dataset_groups.clear()

	# Make HTTP request to /sdk/datasets
	var full_url = url + "/sdk/datasets"
	var headers = _get_auth_headers()

	pending_request_type = "datasets"
	var error = http_request.request(full_url, headers, HTTPClient.METHOD_GET)

	if error != OK:
		_log("[color=red]ERROR: Failed to create HTTP request: " + str(error) + "[/color]")
		status_label.text = "Request failed"
		pending_request_type = ""


func _on_http_request_completed(result: int, response_code: int, _headers: PackedStringArray, body: PackedByteArray) -> void:
	status_label.text = ""

	if result != HTTPRequest.RESULT_SUCCESS:
		_log("[color=red]ERROR: HTTP request failed (result: " + str(result) + ")[/color]")
		pending_request_type = ""
		return

	if response_code != 200:
		_log("[color=red]ERROR: HTTP " + str(response_code) + "[/color]")
		if response_code == 401:
			_log("[color=yellow]Unauthorized - check your API token[/color]")
		elif response_code == 403:
			_log("[color=yellow]Forbidden - token may be invalid[/color]")
		pending_request_type = ""
		return

	var json_string = body.get_string_from_utf8()
	var json = JSON.parse_string(json_string)

	if json == null:
		_log("[color=red]ERROR: Failed to parse JSON response[/color]")
		pending_request_type = ""
		return

	match pending_request_type:
		"datasets":
			_handle_datasets_response(json)
		"files":
			_handle_files_response(json)

	pending_request_type = ""


func _handle_datasets_response(json: Variant) -> void:
	# Expected format: {"data": [{"id": "...", "title": "...", "data": [...]}]}
	if not json is Dictionary or not json.has("data"):
		_log("[color=red]ERROR: Unexpected response format[/color]")
		return

	var groups = json["data"]
	if not groups is Array:
		_log("[color=red]ERROR: Expected array in 'data'[/color]")
		return

	_log("Found " + str(groups.size()) + " dataset groups (prefectures)")

	dataset_groups.clear()

	for group in groups:
		if not group is Dictionary:
			continue

		var group_data = {
			"id": group.get("id", ""),
			"title": group.get("title", "Unknown"),
			"datasets": []
		}

		# Parse nested datasets
		var datasets = group.get("data", [])
		if datasets is Array:
			for dataset in datasets:
				if dataset is Dictionary:
					group_data["datasets"].append({
						"id": dataset.get("id", ""),
						"title": dataset.get("title", "Unknown"),
						"description": dataset.get("description", ""),
						"feature_types": dataset.get("featureTypes", [])
					})

		dataset_groups.append(group_data)

		# Add to group list
		var count = group_data["datasets"].size()
		group_list.add_item(group_data["title"] + " (" + str(count) + ")")

	_log("[color=green]Server datasets loaded![/color]")


func _on_group_selected(index: int) -> void:
	if index < 0 or index >= dataset_groups.size():
		return

	dataset_list.clear()
	file_list.clear()

	var group = dataset_groups[index]
	_log("Selected group: " + group["title"])

	# Populate dataset list
	for dataset in group["datasets"]:
		dataset_list.add_item(dataset["title"])
		dataset_list.set_item_tooltip(dataset_list.item_count - 1, dataset["description"])


func _on_dataset_selected(index: int) -> void:
	var group_index = group_list.get_selected_items()
	if group_index.is_empty():
		return

	var group = dataset_groups[group_index[0]]
	var datasets = group["datasets"]

	if index < 0 or index >= datasets.size():
		return

	var dataset = datasets[index]
	selected_dataset_id = dataset["id"]

	_log("Selected dataset: " + dataset["title"])
	_log("  ID: " + selected_dataset_id)
	_log("  Description: " + dataset["description"])

	var feature_types = dataset["feature_types"]
	if not feature_types.is_empty():
		_log("  Features: " + ", ".join(feature_types))

	# Fetch files for this dataset
	_fetch_dataset_files(selected_dataset_id)


func _fetch_dataset_files(dataset_id: String) -> void:
	var url = _get_server_url()
	if url.is_empty():
		return

	_log("Fetching files for dataset: " + dataset_id)
	status_label.text = "Fetching files..."

	var full_url = url + "/sdk/datasets/" + dataset_id + "/files"
	var headers = _get_auth_headers()

	pending_request_type = "files"
	var error = http_request.request(full_url, headers, HTTPClient.METHOD_GET)

	if error != OK:
		_log("[color=red]ERROR: Failed to create HTTP request[/color]")
		status_label.text = ""
		pending_request_type = ""


func _handle_files_response(json: Variant) -> void:
	# Expected format: {"bldg": [{"code": "...", "url": "...", "maxLod": 2}], ...}
	if not json is Dictionary:
		_log("[color=red]ERROR: Unexpected response format[/color]")
		return

	file_list.clear()

	var total_files = 0

	for package_name in json.keys():
		var files = json[package_name]
		if not files is Array:
			continue

		for file_info in files:
			if not file_info is Dictionary:
				continue

			var code = file_info.get("code", "")
			var url = file_info.get("url", "")
			var max_lod = file_info.get("maxLod", 0)

			var display_name = package_name + " [" + code + "] (LOD" + str(max_lod) + ")"
			file_list.add_item(display_name)
			file_list.set_item_metadata(file_list.item_count - 1, url)
			total_files += 1

	_log("Found " + str(total_files) + " files")
	_log("[color=green]Files loaded![/color]")


func _on_open_local_pressed() -> void:
	if not local_dataset_path.is_empty():
		_open_local_dataset(local_dataset_path)
		return

	var dialog = FileDialog.new()
	dialog.file_mode = FileDialog.FILE_MODE_OPEN_DIR
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.dir_selected.connect(_on_local_dir_selected)
	add_child(dialog)
	dialog.popup_centered(Vector2i(800, 600))


func _on_local_dir_selected(path: String) -> void:
	local_dataset_path = path
	_open_local_dataset(path)


func _open_local_dataset(path: String) -> void:
	_log("--- Opening Local Dataset ---")
	_log("Path: " + path)

	group_list.clear()
	dataset_list.clear()
	file_list.clear()

	current_source = PLATEAUDatasetSource.create_local(path)

	if current_source == null or not current_source.is_valid():
		_log("[color=red]ERROR: Invalid dataset path or no data found[/color]")
		return

	_log("[color=green]Local dataset opened![/color]")
	_update_source_info()


func _update_source_info() -> void:
	# Show available packages
	var available = current_source.get_available_packages()
	_log("Available packages: " + _packages_to_string(available))

	# Show mesh codes
	var mesh_codes = current_source.get_mesh_codes()
	mesh_code_list.clear()
	_log("Mesh codes: " + str(mesh_codes.size()))
	for code in mesh_codes:
		mesh_code_list.add_item(code)

	# Update file list for current package
	_on_package_selected(package_option.selected)


func _on_package_selected(_index: int) -> void:
	if current_source == null or not current_source.is_valid():
		return

	file_list.clear()

	var package_flag = package_option.get_selected_id()
	var gml_files = current_source.get_gml_files(package_flag)

	_log("Found " + str(gml_files.size()) + " GML files for selected package")

	for file_info in gml_files:
		var path = file_info.get_path()
		var mesh_code = file_info.get_mesh_code()
		var max_lod = file_info.get_max_lod()

		var display_name = path.get_file()
		if not mesh_code.is_empty():
			display_name += " [" + mesh_code + "]"
		display_name += " (LOD" + str(max_lod) + ")"

		file_list.add_item(display_name)
		file_list.set_item_metadata(file_list.item_count - 1, path)


func _on_load_gml_pressed() -> void:
	var selected = file_list.get_selected_items()
	if selected.is_empty():
		_log("[color=yellow]Select a GML file first[/color]")
		return

	var path = file_list.get_item_metadata(selected[0])

	# Check if it's a URL or local path
	if path.begins_with("http"):
		_log("[color=yellow]This is a remote file. Download it first.[/color]")
		return

	_log("Loading GML: " + path)

	# Clear previous meshes
	_clear_meshes()

	# Load and display the GML
	var city_model = PLATEAUCityModel.new()
	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = 9  # Tokyo
	options.mesh_granularity = 1  # Primary
	options.export_appearance = true

	var root_mesh_data = city_model.extract_meshes(options)
	var mesh_data_array = _flatten_mesh_data(Array(root_mesh_data))

	_log("Extracted " + str(mesh_data_array.size()) + " meshes")

	# Create mesh instances and calculate bounds
	var bounds_min = Vector3.INF
	var bounds_max = -Vector3.INF

	for mesh_data in mesh_data_array:
		var mi = MeshInstance3D.new()
		mi.mesh = mesh_data.get_mesh()
		mi.transform = mesh_data.get_transform()
		mi.name = mesh_data.get_name()

		mesh_container.add_child(mi)
		mesh_instances.append(mi)

		# Calculate bounds
		var aabb = mi.get_aabb()
		if aabb.size.length() > 0.001:
			var global_aabb = mi.transform * aabb
			bounds_min = bounds_min.min(global_aabb.position)
			bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	# Position camera to view all meshes
	_fit_camera_to_bounds(bounds_min, bounds_max)

	_log("[color=green]Loaded! Meshes: " + str(mesh_instances.size()) + "[/color]")


func _on_download_pressed() -> void:
	var selected = file_list.get_selected_items()
	if selected.is_empty():
		_log("[color=yellow]Select a GML file first[/color]")
		return

	var url = file_list.get_item_metadata(selected[0])
	if not url.begins_with("http"):
		_log("[color=yellow]Selected file is local, no download needed[/color]")
		return

	_log("Downloading: " + url)
	status_label.text = "Downloading..."

	var dest_dir = OS.get_user_data_dir() + "/downloads"
	DirAccess.make_dir_recursive_absolute(dest_dir)

	# Extract filename from URL
	var filename = url.get_file()
	if filename.is_empty():
		filename = "download_" + str(Time.get_unix_time_from_system()) + ".gml"

	download_dest = dest_dir + "/" + filename
	download_url = url

	# For file downloads, we typically don't need auth (CMS server rejects it)
	var headers: PackedStringArray = []

	download_request.download_file = download_dest
	var error = download_request.request(url, headers, HTTPClient.METHOD_GET)

	if error != OK:
		_log("[color=red]ERROR: Failed to start download: " + str(error) + "[/color]")
		status_label.text = ""


func _on_download_completed(result: int, response_code: int, _headers: PackedStringArray, _body: PackedByteArray) -> void:
	status_label.text = ""

	if result != HTTPRequest.RESULT_SUCCESS:
		_log("[color=red]ERROR: Download failed (result: " + str(result) + ")[/color]")
		return

	if response_code != 200:
		_log("[color=red]ERROR: HTTP " + str(response_code) + "[/color]")
		return

	_log("[color=green]Downloaded to: " + download_dest + "[/color]")

	# Update the file list item to use local path
	var selected = file_list.get_selected_items()
	if not selected.is_empty():
		file_list.set_item_metadata(selected[0], download_dest)


func _packages_to_string(flags: int) -> String:
	var names: PackedStringArray = []
	if flags & PLATEAUDatasetSource.PACKAGE_BUILDING:
		names.append("Building")
	if flags & PLATEAUDatasetSource.PACKAGE_ROAD:
		names.append("Road")
	if flags & PLATEAUDatasetSource.PACKAGE_RELIEF:
		names.append("Relief")
	if flags & PLATEAUDatasetSource.PACKAGE_LAND_USE:
		names.append("LandUse")
	if flags & PLATEAUDatasetSource.PACKAGE_VEGETATION:
		names.append("Vegetation")
	if flags & PLATEAUDatasetSource.PACKAGE_CITY_FURNITURE:
		names.append("Furniture")
	if flags & PLATEAUDatasetSource.PACKAGE_BRIDGE:
		names.append("Bridge")
	if flags & PLATEAUDatasetSource.PACKAGE_FLOOD:
		names.append("Flood")
	if flags & PLATEAUDatasetSource.PACKAGE_TSUNAMI:
		names.append("Tsunami")
	return ", ".join(names) if not names.is_empty() else "None"


func _count_meshes(mesh_data_array: Array) -> int:
	var count = 0
	for mesh_data in mesh_data_array:
		if mesh_data.get_mesh() != null:
			count += 1
		var children = mesh_data.get_children()
		if not children.is_empty():
			count += _count_meshes(Array(children))
	return count


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)


func _clear_meshes() -> void:
	for mi in mesh_instances:
		if is_instance_valid(mi):
			mi.queue_free()
	mesh_instances.clear()


func _flatten_mesh_data(mesh_data_array: Array) -> Array:
	var result: Array = []
	for mesh_data in mesh_data_array:
		if mesh_data.get_mesh() != null:
			result.append(mesh_data)
		var children = mesh_data.get_children()
		if not children.is_empty():
			result.append_array(_flatten_mesh_data(Array(children)))
	return result


func _fit_camera_to_bounds(bounds_min: Vector3, bounds_max: Vector3) -> void:
	if bounds_min == Vector3.INF or bounds_max == -Vector3.INF:
		_log("[color=yellow]No valid bounds to fit camera[/color]")
		return

	var center = (bounds_min + bounds_max) / 2.0
	var size = bounds_max - bounds_min
	var max_extent = max(size.x, max(size.y, size.z))

	# Calculate distance based on FOV (60 degrees)
	var fov_rad = deg_to_rad(camera.fov)
	var distance = (max_extent / 2.0) / tan(fov_rad / 2.0)
	distance = max(distance, 50.0)  # Minimum distance

	# Position camera at 45 degrees looking at center
	var offset = Vector3(0, distance * 0.707, distance * 0.707)
	camera.position = center + offset
	camera.look_at(center)

	_log("Camera positioned at: " + str(camera.position))
