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
# Note: For production API, you need to obtain a token from PLATEAU website.
# Mock server does not require authentication.
var PRODUCTION_SERVER: String
var MOCK_SERVER: String

@export_dir var local_dataset_path: String = ""

var current_source: PLATEAUDatasetSource
var selected_dataset_id: String = ""

@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var group_list: ItemList = $UI/DatasetPanel/GroupList
@onready var dataset_list: ItemList = $UI/DatasetPanel/DatasetList
@onready var package_option: OptionButton = $UI/FilePanel/PackageOption
@onready var mesh_code_list: ItemList = $UI/FilePanel/MeshCodeList
@onready var lod_list: ItemList = $UI/FilePanel/LODList
@onready var packages_list: ItemList = $UI/FilePanel/PackagesList

@onready var server_url_edit: LineEdit = $UI/ServerPanel/ServerUrlEdit
@onready var token_edit: LineEdit = $UI/ServerPanel/TokenEdit
@onready var mock_check: CheckBox = $UI/ServerPanel/MockCheck
@onready var status_label: Label = $UI/ServerPanel/StatusLabel

@onready var http_request: HTTPRequest = $HTTPRequest
@onready var download_request: HTTPRequest = $DownloadRequest
@onready var tile_request: HTTPRequest = $TileRequest
@onready var camera: Camera3D = $Camera3D
@onready var mesh_container: Node3D = $MeshContainer

# Texture download UI
@onready var tile_source_option: OptionButton = $UI/TexturePanel/TileSourceOption
@onready var zoom_spin: SpinBox = $UI/TexturePanel/ZoomSpin
@onready var download_texture_button: Button = $UI/TexturePanel/DownloadTextureButton
@onready var apply_texture_button: Button = $UI/TexturePanel/ApplyTextureButton
@onready var texture_progress: ProgressBar = $UI/TexturePanel/TextureProgress
@onready var texture_status: Label = $UI/TexturePanel/TextureStatus

# Navigation UI
@onready var jump_center_button: Button = $UI/NavigationPanel/JumpCenterButton
@onready var fit_view_button: Button = $UI/NavigationPanel/FitViewButton
@onready var nav_lat_spin: SpinBox = $UI/NavigationPanel/LatSpin
@onready var nav_lon_spin: SpinBox = $UI/NavigationPanel/LonSpin
@onready var jump_to_location_button: Button = $UI/NavigationPanel/JumpToLocationButton
@onready var marker: MeshInstance3D = $Marker

var dataset_groups: Array = []
var city_model_root: PLATEAUInstancedCityModel
var mesh_instances: Array[MeshInstance3D] = []  # Cached mesh instances for texture application
var pending_request_type: String = ""
var download_url: String = ""
var download_dest: String = ""
var all_server_files: Array = []  # Store all file data for filtering

# GML download queue state
var gml_download_queue: Array = []  # Array of file_data dictionaries to download
var gml_download_index: int = 0

# Texture download state
var tile_downloader: PLATEAUVectorTileDownloader
var tile_queue: Array = []  # Array of tile coordinates to download
var downloaded_tiles: Array = []  # Array of PLATEAUVectorTile
var current_tile_index: int = 0
var combined_texture: ImageTexture
var geo_reference: PLATEAUGeoReference
var mesh_bounds_min: Vector3
var mesh_bounds_max: Vector3

# Actual tile extent (geographic bounds of downloaded tiles)
var tile_extent_min_lat: float = 0.0
var tile_extent_max_lat: float = 0.0
var tile_extent_min_lon: float = 0.0
var tile_extent_max_lon: float = 0.0


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
	mesh_code_list.multi_selected.connect(_on_mesh_code_selected)
	lod_list.multi_selected.connect(_on_lod_selected)
	packages_list.multi_selected.connect(_on_package_filter_selected)

	$UI/TexturePanel/LoadGMLButton.pressed.connect(_on_load_gml_pressed)
	$UI/TexturePanel/DownloadButton.pressed.connect(_on_download_pressed)

	# Connect HTTP signals
	http_request.request_completed.connect(_on_http_request_completed)
	download_request.request_completed.connect(_on_download_completed)
	tile_request.request_completed.connect(_on_tile_download_completed)

	# Setup package options
	_setup_package_options()

	# Setup tile source options
	_setup_tile_source_options()

	# Connect texture download buttons
	download_texture_button.pressed.connect(_on_download_texture_pressed)
	apply_texture_button.pressed.connect(_on_apply_texture_pressed)

	# Initialize texture download state
	texture_progress.value = 0
	texture_progress.visible = false

	# Connect navigation buttons
	jump_center_button.pressed.connect(_on_jump_center_pressed)
	fit_view_button.pressed.connect(_on_fit_view_pressed)
	jump_to_location_button.pressed.connect(_on_jump_to_location_pressed)

	# Setup marker (red sphere)
	var sphere = SphereMesh.new()
	sphere.radius = 5.0
	sphere.height = 10.0
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color.RED
	sphere.material = mat
	marker.mesh = sphere
	marker.visible = false

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


func _setup_tile_source_options() -> void:
	tile_source_option.clear()
	tile_source_option.add_item("GSI Aerial Photo", PLATEAUVectorTileDownloader.TILE_SOURCE_GSI_PHOTO)
	tile_source_option.add_item("GSI Standard Map", PLATEAUVectorTileDownloader.TILE_SOURCE_GSI_STD)
	tile_source_option.add_item("GSI Pale Map", PLATEAUVectorTileDownloader.TILE_SOURCE_GSI_PALE)
	tile_source_option.add_item("OpenStreetMap", PLATEAUVectorTileDownloader.TILE_SOURCE_OSM)


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
	# Build auth headers using C++ helper
	# - custom_token: User-entered Bearer token (empty to use default for production server)
	# - use_default_token: Use built-in token when connecting to default production server
	var token = token_edit.text.strip_edges()
	var use_default = _is_default_server() and token.is_empty()
	return PLATEAUDatasetSource.build_auth_headers(token, use_default)


func _is_default_server() -> bool:
	var url = server_url_edit.text.strip_edges()
	return url == PLATEAUDatasetSource.get_default_server_url()


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
	mesh_code_list.clear()
	lod_list.clear()
	packages_list.clear()
	all_server_files.clear()
	gml_download_queue.clear()
	dataset_groups.clear()
	_clear_meshes()
	_update_file_buttons_state()

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
	mesh_code_list.clear()
	lod_list.clear()
	packages_list.clear()
	all_server_files.clear()
	gml_download_queue.clear()
	_clear_meshes()
	_update_file_buttons_state()

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

	# Clear previous state
	mesh_code_list.clear()
	lod_list.clear()
	packages_list.clear()
	all_server_files.clear()
	gml_download_queue.clear()
	_clear_meshes()
	_update_file_buttons_state()

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

	mesh_code_list.clear()
	lod_list.clear()
	packages_list.clear()
	all_server_files.clear()
	gml_download_queue.clear()

	var mesh_codes: Dictionary = {}  # Use Dictionary as a Set to collect unique codes

	for package_name in json.keys():
		var files = json[package_name]
		if not files is Array:
			continue

		for file_info in files:
			if not file_info is Dictionary:
				continue

			var code = file_info.get("code", "")
			var url = file_info.get("url", "")
			var max_lod = int(file_info.get("maxLod", 0))  # Convert to int (JSON may return float)

			# Store file data for filtering
			all_server_files.append({
				"package": package_name,
				"code": code,
				"url": url,
				"max_lod": max_lod,
				"display_name": package_name + " [" + code + "] (LOD" + str(max_lod) + ")"
			})

			# Collect unique mesh codes
			if not code.is_empty():
				mesh_codes[code] = true

	# Add sorted mesh codes to the list
	var sorted_codes = mesh_codes.keys()
	sorted_codes.sort()
	for code in sorted_codes:
		mesh_code_list.add_item(code)

	# LOD and Packages lists remain empty until Mesh Codes are selected

	_log("Found " + str(all_server_files.size()) + " files")
	_log("Mesh codes: " + str(sorted_codes.size()))
	_log("[color=green]Select Mesh Codes to continue[/color]")


## Returns filtered files based on current Mesh Codes, LOD, and Packages selection
func _get_filtered_files() -> Array:
	# Get selected mesh codes (required)
	var selected_codes: Array = []
	for i in mesh_code_list.get_selected_items():
		selected_codes.append(mesh_code_list.get_item_text(i))

	# Get selected LODs (optional)
	var selected_lods: Array = []
	for i in lod_list.get_selected_items():
		selected_lods.append(lod_list.get_item_text(i))

	# Get selected packages (optional)
	var selected_packages: Array = []
	for i in packages_list.get_selected_items():
		selected_packages.append(packages_list.get_item_text(i))

	# Return files matching all filters
	var result: Array = []
	for file_data in all_server_files:
		# Filter by mesh codes (required)
		if not selected_codes.is_empty() and not file_data["code"] in selected_codes:
			continue
		# Filter by LOD (optional)
		if not selected_lods.is_empty():
			var lod_str = "LOD" + str(file_data["max_lod"])
			if not lod_str in selected_lods:
				continue
		# Filter by packages (optional)
		if not selected_packages.is_empty() and not file_data["package"] in selected_packages:
			continue
		result.append(file_data)
	return result


## Update LOD list based on selected Mesh Codes
func _update_lod_list() -> void:
	# Remember current selection
	var prev_selected_lods: Array = []
	for i in lod_list.get_selected_items():
		prev_selected_lods.append(lod_list.get_item_text(i))

	lod_list.clear()

	# Get selected mesh codes (required)
	var selected_codes: Array = []
	for i in mesh_code_list.get_selected_items():
		selected_codes.append(mesh_code_list.get_item_text(i))

	# Don't show LODs if no mesh codes are selected
	if selected_codes.is_empty():
		return

	# Collect LODs for selected mesh codes
	var lods: Dictionary = {}
	for file_data in all_server_files:
		if file_data["code"] in selected_codes:
			var lod_str = "LOD" + str(file_data["max_lod"])
			lods[lod_str] = file_data["max_lod"]

	# Add sorted LODs to the list (sort by numeric value)
	var sorted_lods = lods.keys()
	sorted_lods.sort_custom(func(a, b): return lods[a] < lods[b])
	for lod in sorted_lods:
		lod_list.add_item(lod)
		# Restore selection
		if lod in prev_selected_lods:
			lod_list.select(lod_list.item_count - 1, false)


## Update Packages list based on selected Mesh Codes and LOD
func _update_packages_list() -> void:
	# Remember current selection
	var prev_selected_packages: Array = []
	for i in packages_list.get_selected_items():
		prev_selected_packages.append(packages_list.get_item_text(i))

	packages_list.clear()

	# Get selected mesh codes (required)
	var selected_codes: Array = []
	for i in mesh_code_list.get_selected_items():
		selected_codes.append(mesh_code_list.get_item_text(i))

	# Don't show packages if no mesh codes are selected
	if selected_codes.is_empty():
		return

	# Get selected LODs (optional)
	var selected_lods: Array = []
	for i in lod_list.get_selected_items():
		selected_lods.append(lod_list.get_item_text(i))

	# Collect packages for selected mesh codes and LODs
	var packages: Dictionary = {}
	for file_data in all_server_files:
		if not file_data["code"] in selected_codes:
			continue
		if not selected_lods.is_empty():
			var lod_str = "LOD" + str(file_data["max_lod"])
			if not lod_str in selected_lods:
				continue
		packages[file_data["package"]] = true

	# Add sorted packages to the list
	var sorted_packages = packages.keys()
	sorted_packages.sort()
	for pkg in sorted_packages:
		packages_list.add_item(pkg)
		# Restore selection
		if pkg in prev_selected_packages:
			packages_list.select(packages_list.item_count - 1, false)


func _update_file_buttons_state() -> void:
	# Mesh Codes selection is required
	var has_mesh_code_selection = not mesh_code_list.get_selected_items().is_empty()

	if not has_mesh_code_selection:
		$UI/TexturePanel/DownloadButton.disabled = true
		$UI/TexturePanel/LoadGMLButton.disabled = true
		return

	# Get filtered files based on current selection
	var filtered_files = _get_filtered_files()

	# Check if any filtered file is remote (needs download) or local (can load directly)
	var has_remote_files = false
	var has_local_files = false
	for file_data in filtered_files:
		var url = file_data["url"]
		if url.begins_with("http"):
			has_remote_files = true
		else:
			has_local_files = true

	# Download GML: enabled if has mesh code selection and has remote files
	$UI/TexturePanel/DownloadButton.disabled = not has_remote_files
	# Load Selected GML: enabled if has mesh code selection and has local files
	$UI/TexturePanel/LoadGMLButton.disabled = not has_local_files


func _on_mesh_code_selected(_index: int, _selected: bool) -> void:
	_update_lod_list()
	_update_packages_list()
	_update_file_buttons_state()
	var selected_count = mesh_code_list.get_selected_items().size()
	var filtered_count = _get_filtered_files().size()
	if selected_count > 0:
		_log("Mesh codes: %d selected, %d files match" % [selected_count, filtered_count])


func _on_lod_selected(_index: int, _selected: bool) -> void:
	_update_packages_list()
	_update_file_buttons_state()
	var selected_count = lod_list.get_selected_items().size()
	var filtered_count = _get_filtered_files().size()
	if selected_count > 0:
		_log("LOD: %d selected, %d files match" % [selected_count, filtered_count])


func _on_package_filter_selected(_index: int, _selected: bool) -> void:
	_update_file_buttons_state()
	var selected_count = packages_list.get_selected_items().size()
	var filtered_count = _get_filtered_files().size()
	if selected_count > 0:
		_log("Packages: %d selected, %d files match" % [selected_count, filtered_count])


func _on_open_local_pressed() -> void:
	if not local_dataset_path.is_empty():
		_open_local_dataset(local_dataset_path)
		return

	PLATEAUUtils.show_directory_dialog(self, _on_local_dir_selected)


func _on_local_dir_selected(path: String) -> void:
	local_dataset_path = path
	_open_local_dataset(path)


func _open_local_dataset(path: String) -> void:
	_log("--- Opening Local Dataset ---")
	_log("Path: " + path)

	group_list.clear()
	dataset_list.clear()
	mesh_code_list.clear()
	lod_list.clear()
	packages_list.clear()
	all_server_files.clear()
	gml_download_queue.clear()
	_clear_meshes()
	_update_file_buttons_state()

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
	packages_list.clear()
	_log("Mesh codes: " + str(mesh_codes.size()))
	for code in mesh_codes:
		mesh_code_list.add_item(code)

	# Update file list for current package
	_on_package_selected(package_option.selected)


func _on_package_selected(_index: int) -> void:
	if current_source == null or not current_source.is_valid():
		return

	# Clear and rebuild all_server_files from local dataset
	mesh_code_list.clear()
	lod_list.clear()
	packages_list.clear()
	all_server_files.clear()
	gml_download_queue.clear()
	_update_file_buttons_state()

	var package_flag = package_option.get_selected_id()
	var gml_files = current_source.get_gml_files(package_flag)

	_log("Found " + str(gml_files.size()) + " GML files for selected package")

	var mesh_codes: Dictionary = {}

	for file_info in gml_files:
		var path = file_info.get_path()
		var mesh_code = file_info.get_mesh_code()
		var max_lod = int(file_info.get_max_lod())  # Ensure int type

		var display_name = path.get_file()
		if not mesh_code.is_empty():
			display_name += " [" + mesh_code + "]"
		display_name += " (LOD" + str(max_lod) + ")"

		# Store in all_server_files for unified handling
		all_server_files.append({
			"package": package_flag,
			"code": mesh_code,
			"url": path,  # Local path
			"max_lod": max_lod,
			"display_name": display_name
		})

		# Collect unique mesh codes
		if not mesh_code.is_empty():
			mesh_codes[mesh_code] = true

	# Add sorted mesh codes to the list
	var sorted_codes = mesh_codes.keys()
	sorted_codes.sort()
	for code in sorted_codes:
		mesh_code_list.add_item(code)

	# LOD and Packages lists remain empty until Mesh Codes are selected

	_log("Mesh codes: " + str(sorted_codes.size()))
	_log("[color=green]Select Mesh Codes to continue[/color]")


func _on_load_gml_pressed() -> void:
	# Get filtered files (respects GML Files, Mesh Codes, and Packages selection)
	var filtered_files = _get_filtered_files()

	# Filter to only local files
	var local_files: Array = []
	for file_data in filtered_files:
		if not file_data["url"].begins_with("http"):
			local_files.append(file_data)

	if local_files.is_empty():
		_log("[color=yellow]No local files to load. Download files first.[/color]")
		return

	# Get selected LODs from UI (for visibility control after import)
	var selected_lod_values: Array[int] = []
	for i in lod_list.get_selected_items():
		var lod_text = lod_list.get_item_text(i)  # e.g., "LOD1"
		var lod_num = PLATEAUUtils.parse_lod_from_name(lod_text)
		if lod_num >= 0:
			selected_lod_values.append(lod_num)

	_log("--- Loading %d GML file(s) ---" % local_files.size())

	# Show loading status
	texture_status.text = "Loading GML..."
	await get_tree().process_frame

	# Clear previous meshes
	_clear_meshes()

	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = 9  # Tokyo
	options.mesh_granularity = 1  # Primary
	options.export_appearance = true

	# Restrict LOD range based on user selection to reduce RID usage
	if not selected_lod_values.is_empty():
		var lod_min = selected_lod_values.min()
		var lod_max = selected_lod_values.max()
		options.min_lod = lod_min
		options.max_lod = lod_max
		_log("LOD range restricted to: %d-%d" % [lod_min, lod_max])

	# Initialize combined bounds
	var combined_bounds_min = Vector3(INF, INF, INF)
	var combined_bounds_max = Vector3(-INF, -INF, -INF)
	var first_file = true
	var total_mesh_count = 0

	for file_data in local_files:
		var path = file_data["url"]
		_log("Loading: " + path.get_file())

		texture_status.text = "Loading: " + path.get_file()
		await get_tree().process_frame

		# Load and display the GML
		var city_model = PLATEAUCityModel.new()
		city_model.log_level = PLATEAUCityModel.LOG_LEVEL_NONE
		if not city_model.load(path):
			_log("[color=red]ERROR: Failed to load " + path.get_file() + "[/color]")
			continue

		# Get center point from first file for reference
		if first_file:
			var center_point = city_model.get_center_point(options.coordinate_zone_id)
			options.reference_point = center_point
			_log("Reference point: " + str(center_point))

			# Create GeoReference for coordinate conversion
			geo_reference = PLATEAUGeoReference.new()
			geo_reference.zone_id = options.coordinate_zone_id
			geo_reference.reference_point = center_point
			first_file = false

		texture_status.text = "Extracting meshes: " + path.get_file()
		await get_tree().process_frame

		var root_mesh_data = city_model.extract_meshes(options)

		texture_status.text = "Creating scene: " + path.get_file()
		await get_tree().process_frame

		# Use PLATEAUUtils to import to city model
		var result = PLATEAUUtils.import_to_city_model(
			Array(root_mesh_data),
			mesh_container,
			path.get_file().get_basename(),
			geo_reference,
			options,
			path
		)

		# Track city model root (use last one for info display)
		city_model_root = result["city_model_root"]
		var flat_mesh_data: Array = result["flat_mesh_data"]
		total_mesh_count += flat_mesh_data.size()

		# Collect mesh instances
		if city_model_root != null:
			_collect_mesh_instances(city_model_root, mesh_instances)

		# Expand combined bounds
		var file_bounds_min: Vector3 = result["bounds_min"]
		var file_bounds_max: Vector3 = result["bounds_max"]
		combined_bounds_min.x = minf(combined_bounds_min.x, file_bounds_min.x)
		combined_bounds_min.y = minf(combined_bounds_min.y, file_bounds_min.y)
		combined_bounds_min.z = minf(combined_bounds_min.z, file_bounds_min.z)
		combined_bounds_max.x = maxf(combined_bounds_max.x, file_bounds_max.x)
		combined_bounds_max.y = maxf(combined_bounds_max.y, file_bounds_max.y)
		combined_bounds_max.z = maxf(combined_bounds_max.z, file_bounds_max.z)

	# Save combined bounds for texture download
	mesh_bounds_min = combined_bounds_min
	mesh_bounds_max = combined_bounds_max

	_log("Extracted " + str(total_mesh_count) + " meshes total")

	# Position camera to view all meshes
	PLATEAUUtils.fit_camera_to_bounds(camera, mesh_bounds_min, mesh_bounds_max)

	# Display PLATEAUInstancedCityModel info
	if city_model_root != null:
		_log("[color=cyan]PLATEAUInstancedCityModel Info:[/color]")
		_log("  Zone ID: " + str(city_model_root.zone_id))
		_log("  Latitude: %.6f" % city_model_root.get_latitude())
		_log("  Longitude: %.6f" % city_model_root.get_longitude())

	# Apply user-selected LOD visibility to ALL imported city models
	# (override PLATEAUImporter's default max LOD behavior)
	if not selected_lod_values.is_empty():
		_log("Applying LOD filter to all imported models...")
		for child in mesh_container.get_children():
			if child is PLATEAUInstancedCityModel:
				_apply_selected_lod_visibility(child, selected_lod_values)
		# Re-collect mesh instances after visibility change (only visible ones)
		mesh_instances.clear()
		_collect_visible_mesh_instances(mesh_container, mesh_instances)
		_log("After LOD filter: %d visible meshes" % mesh_instances.size())

	_log("[color=green]Loaded! Files: %d, Meshes: %d[/color]" % [local_files.size(), mesh_instances.size()])

	texture_status.text = ""

	# Enable texture download button now that meshes are loaded
	download_texture_button.disabled = mesh_instances.is_empty()

	# Calculate center lat/lon and set navigation UI
	if geo_reference != null:
		var center = (mesh_bounds_min + mesh_bounds_max) / 2
		var center_geo = geo_reference.unproject(center)
		nav_lat_spin.value = center_geo.x
		nav_lon_spin.value = center_geo.y
		_log("Model center: lat=%.6f, lon=%.6f" % [center_geo.x, center_geo.y])

	# Reset texture state
	apply_texture_button.disabled = true
	combined_texture = null


## Recursively collect all MeshInstance3D nodes under a parent
func _collect_mesh_instances(parent: Node, instances: Array[MeshInstance3D]) -> void:
	for child in parent.get_children():
		if child is MeshInstance3D:
			instances.append(child)
		_collect_mesh_instances(child, instances)


## Recursively collect only visible MeshInstance3D nodes
## Checks if parent LOD node is visible before collecting
func _collect_visible_mesh_instances(parent: Node, instances: Array[MeshInstance3D]) -> void:
	for child in parent.get_children():
		# Skip invisible nodes and their children
		if child is Node3D and not child.visible:
			continue
		if child is MeshInstance3D:
			instances.append(child)
		_collect_visible_mesh_instances(child, instances)


## Apply LOD visibility based on user's selection
## Shows only LOD nodes that match the selected LOD values
## @param root: Root node containing LOD0, LOD1, etc. children
## @param selected_lods: Array of LOD numbers to show (e.g., [1] for LOD1 only)
func _apply_selected_lod_visibility(root: Node3D, selected_lods: Array[int]) -> void:
	if root == null:
		return

	for child in root.get_children():
		if child is Node3D:
			var lod = PLATEAUUtils.parse_lod_from_name(child.name)
			if lod >= 0:
				# Show only if LOD is in selected list
				child.visible = lod in selected_lods


func _on_download_pressed() -> void:
	# Get filtered files (respects GML Files, Mesh Codes, and Packages selection)
	var filtered_files = _get_filtered_files()

	# Filter to only remote files
	gml_download_queue.clear()
	for file_data in filtered_files:
		if file_data["url"].begins_with("http"):
			gml_download_queue.append(file_data)

	if gml_download_queue.is_empty():
		_log("[color=yellow]No remote files to download[/color]")
		return

	_log("--- Downloading %d GML file(s) ---" % gml_download_queue.size())

	# Disable buttons during download
	$UI/TexturePanel/DownloadButton.disabled = true
	$UI/TexturePanel/LoadGMLButton.disabled = true

	gml_download_index = 0
	_download_next_gml()


func _download_next_gml() -> void:
	if gml_download_index >= gml_download_queue.size():
		# All downloads complete
		_log("[color=green]All downloads complete![/color]")
		status_label.text = ""
		_update_file_buttons_state()
		return

	var file_data = gml_download_queue[gml_download_index]
	var url = file_data["url"]

	status_label.text = "Downloading %d/%d: %s" % [gml_download_index + 1, gml_download_queue.size(), url.get_file()]
	_log("Downloading %d/%d: %s" % [gml_download_index + 1, gml_download_queue.size(), url.get_file()])

	var dest_dir = OS.get_user_data_dir() + "/downloads"
	DirAccess.make_dir_recursive_absolute(dest_dir)

	var filename = url.get_file()
	if filename.is_empty():
		filename = "download_" + str(Time.get_unix_time_from_system()) + ".gml"

	download_dest = dest_dir + "/" + filename
	download_url = url

	var headers: PackedStringArray = []
	download_request.download_file = download_dest
	var error = download_request.request(url, headers, HTTPClient.METHOD_GET)

	if error != OK:
		_log("[color=red]ERROR: Failed to start download: " + str(error) + "[/color]")
		gml_download_index += 1
		call_deferred("_download_next_gml")


func _on_download_completed(result: int, response_code: int, _headers: PackedStringArray, _body: PackedByteArray) -> void:
	if result != HTTPRequest.RESULT_SUCCESS:
		_log("[color=red]ERROR: Download failed (result: " + str(result) + ")[/color]")
	elif response_code != 200:
		_log("[color=red]ERROR: HTTP " + str(response_code) + "[/color]")
	else:
		_log("[color=green]Downloaded: " + download_dest.get_file() + "[/color]")

		# Update the URL in all_server_files to local path
		for i in range(all_server_files.size()):
			if all_server_files[i]["url"] == download_url:
				all_server_files[i]["url"] = download_dest
				break

	# Continue to next file
	gml_download_index += 1
	_download_next_gml()


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
	if city_model_root != null and is_instance_valid(city_model_root):
		city_model_root.queue_free()
		city_model_root = null
	mesh_instances.clear()
	download_texture_button.disabled = true
	apply_texture_button.disabled = true


# --- Texture Download Functions ---

func _on_download_texture_pressed() -> void:
	if mesh_instances.is_empty():
		_log("[color=yellow]Load a GML file first before downloading texture[/color]")
		return

	if geo_reference == null:
		_log("[color=red]ERROR: GeoReference not set[/color]")
		return

	_log("--- Downloading Terrain Texture ---")

	# Convert mesh bounds to geographic coordinates
	# PLATEAUGeoReference.unproject converts local XYZ to lat/lon/height
	var min_geo = geo_reference.unproject(mesh_bounds_min)  # Returns Vector3(lat, lon, height)
	var max_geo = geo_reference.unproject(mesh_bounds_max)

	# Ensure min/max are correct (unproject may swap them)
	var min_lat = minf(min_geo.x, max_geo.x)
	var max_lat = maxf(min_geo.x, max_geo.x)
	var min_lon = minf(min_geo.y, max_geo.y)
	var max_lon = maxf(min_geo.y, max_geo.y)

	_log("Geographic extent:")
	_log("  Lat: %.6f to %.6f" % [min_lat, max_lat])
	_log("  Lon: %.6f to %.6f" % [min_lon, max_lon])

	# Create tile downloader
	tile_downloader = PLATEAUVectorTileDownloader.new()
	tile_downloader.destination = OS.get_user_data_dir() + "/map_tiles"
	tile_downloader.zoom_level = int(zoom_spin.value)
	tile_downloader.tile_source = tile_source_option.get_selected_id()

	# Set extent
	tile_downloader.set_extent(min_lat, min_lon, max_lat, max_lon)

	var tile_count = tile_downloader.get_tile_count()
	_log("Tile count: " + str(tile_count) + " (zoom level " + str(tile_downloader.zoom_level) + ")")

	if tile_count == 0:
		_log("[color=red]ERROR: No tiles to download[/color]")
		return

	if tile_count > 100:
		_log("[color=yellow]Warning: Large number of tiles (" + str(tile_count) + "). Consider reducing zoom level.[/color]")

	# Prepare download queue
	tile_queue.clear()
	downloaded_tiles.clear()
	current_tile_index = 0

	for i in range(tile_count):
		tile_queue.append(i)

	# Setup progress bar
	texture_progress.visible = true
	texture_progress.max_value = tile_count
	texture_progress.value = 0
	texture_status.text = "Downloading tiles..."

	# Disable buttons during download
	download_texture_button.disabled = true
	apply_texture_button.disabled = true

	# Start downloading first tile
	_download_next_tile()


func _download_next_tile() -> void:
	if current_tile_index >= tile_queue.size():
		# All tiles downloaded
		_on_all_tiles_downloaded()
		return

	var tile_index = tile_queue[current_tile_index]
	var coord = tile_downloader.get_tile_coordinate(tile_index)
	var url = tile_downloader.get_tile_url(coord)
	var file_path = tile_downloader.get_tile_file_path(coord)

	# Ensure directory exists
	var dir_path = file_path.get_base_dir()
	DirAccess.make_dir_recursive_absolute(dir_path)

	# Check if tile already exists
	if FileAccess.file_exists(file_path):
		_log("Tile %d/%d cached: z%d/%d/%d" % [current_tile_index + 1, tile_queue.size(), coord.zoom_level, coord.column, coord.row])
		var tile = PLATEAUVectorTile.new()
		tile.set_coordinate(coord)
		tile.image_path = file_path
		tile.set_success(true)
		downloaded_tiles.append(tile)

		current_tile_index += 1
		texture_progress.value = current_tile_index
		_download_next_tile()
		return

	texture_status.text = "Downloading tile %d/%d (z%d/%d/%d)..." % [current_tile_index + 1, tile_queue.size(), coord.zoom_level, coord.column, coord.row]

	# Download using HTTPRequest
	tile_request.download_file = file_path
	var error = tile_request.request(url, [], HTTPClient.METHOD_GET)

	if error != OK:
		_log("[color=red]ERROR: Failed to request tile: " + str(error) + "[/color]")
		current_tile_index += 1
		texture_progress.value = current_tile_index
		call_deferred("_download_next_tile")


func _on_tile_download_completed(result: int, response_code: int, _headers: PackedStringArray, _body: PackedByteArray) -> void:
	var tile_index = tile_queue[current_tile_index]
	var coord = tile_downloader.get_tile_coordinate(tile_index)
	var file_path = tile_downloader.get_tile_file_path(coord)

	var tile = PLATEAUVectorTile.new()
	tile.set_coordinate(coord)
	tile.image_path = file_path

	if result == HTTPRequest.RESULT_SUCCESS and response_code == 200:
		tile.set_success(true)
		_log("Tile %d/%d downloaded: z%d/%d/%d" % [current_tile_index + 1, tile_queue.size(), coord.zoom_level, coord.column, coord.row])
	else:
		tile.set_success(false)
		_log("[color=yellow]Tile %d/%d failed (HTTP %d): z%d/%d/%d[/color]" % [current_tile_index + 1, tile_queue.size(), response_code, coord.zoom_level, coord.column, coord.row])

	downloaded_tiles.append(tile)

	current_tile_index += 1
	texture_progress.value = current_tile_index

	# Small delay to avoid overwhelming the server
	await get_tree().create_timer(0.1).timeout
	_download_next_tile()


func _on_all_tiles_downloaded() -> void:
	_log("--- All Tiles Downloaded ---")

	# Count successful downloads and find tile bounds
	var success_count = 0
	var min_col = 999999
	var max_col = -999999
	var min_row = 999999
	var max_row = -999999

	for tile in downloaded_tiles:
		if tile.is_success():
			success_count += 1
			var coord = tile.get_coordinate()
			if coord != null:
				min_col = mini(min_col, coord.column)
				max_col = maxi(max_col, coord.column)
				min_row = mini(min_row, coord.row)
				max_row = maxi(max_row, coord.row)

	_log("Successfully downloaded: %d/%d tiles" % [success_count, downloaded_tiles.size()])
	_log("Tile grid: col %d-%d, row %d-%d" % [min_col, max_col, min_row, max_row])

	if success_count == 0:
		_log("[color=red]ERROR: No tiles downloaded successfully[/color]")
		texture_status.text = "Download failed"
		download_texture_button.disabled = false
		texture_progress.visible = false
		return

	# Calculate actual geographic extent from tile bounds
	# Use PLATEAUTileCoordinate.get_extent() to get geographic bounds for each corner tile
	# Top-left tile (min_col, min_row) gives us max_lat (north) and min_lon (west)
	# Bottom-right tile (max_col, max_row) gives us min_lat (south) and max_lon (east)
	var zoom = tile_downloader.zoom_level

	var top_left_coord = PLATEAUTileCoordinate.new()
	top_left_coord.column = min_col
	top_left_coord.row = min_row
	top_left_coord.zoom_level = zoom
	var top_left_extent = top_left_coord.get_extent()

	var bottom_right_coord = PLATEAUTileCoordinate.new()
	bottom_right_coord.column = max_col
	bottom_right_coord.row = max_row
	bottom_right_coord.zoom_level = zoom
	var bottom_right_extent = bottom_right_coord.get_extent()

	# In web mercator: row increases from north to south
	# So min_row is north (has max_lat), max_row is south (has min_lat)
	tile_extent_max_lat = top_left_extent["max_lat"]      # North edge of top-left tile
	tile_extent_min_lat = bottom_right_extent["min_lat"]  # South edge of bottom-right tile
	tile_extent_min_lon = top_left_extent["min_lon"]      # West edge of top-left tile
	tile_extent_max_lon = bottom_right_extent["max_lon"]  # East edge of bottom-right tile

	_log("Actual tile extent (geographic):")
	_log("  Lat: %.6f to %.6f" % [tile_extent_min_lat, tile_extent_max_lat])
	_log("  Lon: %.6f to %.6f" % [tile_extent_min_lon, tile_extent_max_lon])

	# Create combined texture
	texture_status.text = "Creating combined texture..."
	_log("Creating combined texture...")

	# Convert downloaded_tiles array to TypedArray
	var tiles_array: Array[PLATEAUVectorTile] = []
	for tile in downloaded_tiles:
		tiles_array.append(tile)

	combined_texture = tile_downloader.create_combined_texture(tiles_array)

	if combined_texture != null:
		_log("[color=green]Combined texture created: %dx%d[/color]" % [combined_texture.get_width(), combined_texture.get_height()])
		apply_texture_button.disabled = false
		texture_status.text = "Texture ready. Click 'Apply to Terrain' to apply."
	else:
		_log("[color=red]ERROR: Failed to create combined texture[/color]")
		texture_status.text = "Failed to create texture"

	download_texture_button.disabled = false
	texture_progress.visible = false


func _on_apply_texture_pressed() -> void:
	if combined_texture == null:
		_log("[color=yellow]Download texture first[/color]")
		return

	if mesh_instances.is_empty():
		_log("[color=yellow]No meshes to apply texture to[/color]")
		return

	# Show processing status
	texture_status.text = "Applying texture to meshes..."
	await get_tree().process_frame

	_log("--- Applying Texture to Meshes ---")
	_log("[color=yellow]Note: This feature is designed for terrain (Relief/DEM) data.[/color]")
	_log("[color=yellow]Meshes with existing textures will be skipped.[/color]")

	# Use actual tile extent for UV calculation (not the requested extent)
	# This is the geographic bounds of the downloaded tiles
	var min_lat = tile_extent_min_lat
	var max_lat = tile_extent_max_lat
	var min_lon = tile_extent_min_lon
	var max_lon = tile_extent_max_lon

	_log("Using actual tile extent for UV mapping:")
	_log("  Lat: %.6f to %.6f" % [min_lat, max_lat])
	_log("  Lon: %.6f to %.6f" % [min_lon, max_lon])
	_log("  Reference point: " + str(geo_reference.reference_point))

	var applied_count = 0
	var skipped_count = 0

	for mi in mesh_instances:
		var mesh = mi.mesh
		if mesh == null:
			continue

		# Skip meshes that already have textures (likely buildings with textures)
		var has_existing_texture = false
		if mesh.get_surface_count() > 0:
			var existing_mat = mesh.surface_get_material(0)
			if existing_mat != null and existing_mat is StandardMaterial3D:
				var std_mat = existing_mat as StandardMaterial3D
				if std_mat.albedo_texture != null:
					has_existing_texture = true

		# Also check override material
		var override_mat = mi.get_surface_override_material(0)
		if override_mat != null and override_mat is StandardMaterial3D:
			var std_mat = override_mat as StandardMaterial3D
			if std_mat.albedo_texture != null:
				has_existing_texture = true

		if has_existing_texture:
			skipped_count += 1
			continue

		# Create material with combined texture
		var material = StandardMaterial3D.new()
		material.albedo_texture = combined_texture
		material.cull_mode = BaseMaterial3D.CULL_BACK

		# Remap UVs based on geographic coordinates
		var new_mesh = _remap_mesh_uvs_to_geographic(mesh, min_lat, max_lat, min_lon, max_lon, mi.global_transform)

		if new_mesh != null:
			mi.mesh = new_mesh
			mi.set_surface_override_material(0, material)
			applied_count += 1
		else:
			# If UV remap failed, still try to apply texture with existing UVs
			mi.set_surface_override_material(0, material)
			applied_count += 1

	_log("[color=green]Texture applied to %d meshes (skipped %d with existing textures)[/color]" % [applied_count, skipped_count])
	texture_status.text = ""


func _remap_mesh_uvs_to_geographic(mesh: Mesh, min_lat: float, max_lat: float, min_lon: float, max_lon: float, transform: Transform3D) -> ArrayMesh:
	if mesh.get_surface_count() == 0:
		return null

	var arrays = mesh.surface_get_arrays(0)
	if arrays.is_empty():
		return null

	var vertices: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
	if vertices.is_empty():
		return null

	# Calculate new UVs based on vertex positions converted to geographic coordinates
	# Using libplateau's Extent::uvAt() formula:
	#   - Southwest corner (min_lat, min_lon) = UV(0, 0)
	#   - Northeast corner (max_lat, max_lon) = UV(1, 1)
	#   - U = (lon - min_lon) / (max_lon - min_lon)
	#   - V = (lat - min_lat) / (max_lat - min_lat)
	#
	# However, in the combined tile texture:
	#   - North (max_lat) is at the top (V=0 in texture coords)
	#   - South (min_lat) is at the bottom (V=1 in texture coords)
	# So we invert V: V_texture = 1.0 - V_extent

	var new_uvs = PackedVector2Array()
	new_uvs.resize(vertices.size())

	var lat_range = max_lat - min_lat
	var lon_range = max_lon - min_lon

	if lat_range <= 0 or lon_range <= 0:
		return null

	for i in range(vertices.size()):
		var world_pos = transform * vertices[i]
		var geo = geo_reference.unproject(world_pos)  # Returns Vector3(lat, lon, height)

		# Calculate UV using Extent::uvAt() formula with V inverted for texture coords
		var u = (geo.y - min_lon) / lon_range  # Longitude -> U
		var v = 1.0 - (geo.x - min_lat) / lat_range  # Latitude -> V (inverted for texture)

		new_uvs[i] = Vector2(u, v)

	# Create new mesh with updated UVs
	arrays[Mesh.ARRAY_TEX_UV] = new_uvs

	var new_mesh = ArrayMesh.new()
	new_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)

	return new_mesh


# --- Navigation Functions ---

func _on_jump_center_pressed() -> void:
	if mesh_instances.is_empty():
		_log("[color=yellow]Load a GML file first[/color]")
		return

	var center = (mesh_bounds_min + mesh_bounds_max) / 2
	var center_geo = geo_reference.unproject(center)

	# Update UI
	nav_lat_spin.value = center_geo.x
	nav_lon_spin.value = center_geo.y

	# Move marker
	marker.position = center
	marker.visible = true

	# Move camera to center
	camera.position = center + Vector3(0, 100, 150)
	camera.look_at(center)

	_log("Jumped to center: lat=%.6f, lon=%.6f" % [center_geo.x, center_geo.y])


func _on_fit_view_pressed() -> void:
	if mesh_instances.is_empty():
		_log("[color=yellow]Load a GML file first[/color]")
		return

	PLATEAUUtils.fit_camera_to_bounds(camera, mesh_bounds_min, mesh_bounds_max)
	marker.visible = false
	_log("Fit view to model bounds")


func _on_jump_to_location_pressed() -> void:
	if geo_reference == null:
		_log("[color=yellow]Load a GML file first to set coordinate zone[/color]")
		return

	var lat = nav_lat_spin.value
	var lon = nav_lon_spin.value
	var height = 0.0

	# Convert lat/lon to local XYZ
	var lat_lon_height = Vector3(lat, lon, height)
	var xyz = geo_reference.project(lat_lon_height)

	# Move marker
	marker.position = xyz
	marker.visible = true

	# Move camera to location
	camera.position = xyz + Vector3(0, 100, 150)
	camera.look_at(xyz)

	_log("Jumped to: lat=%.6f, lon=%.6f -> xyz=(%.2f, %.2f, %.2f)" % [lat, lon, xyz.x, xyz.y, xyz.z])
