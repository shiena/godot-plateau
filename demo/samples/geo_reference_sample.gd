extends Node3D
## GeoReference Sample
##
## Demonstrates coordinate conversion APIs:
## - PLATEAUGeoReference: Convert between lat/lon and local XYZ coordinates
## - Japan Plane Rectangular Coordinate System (1-19 zones)
##
## Usage:
## 1. Select a coordinate zone (1-19)
## 2. Enter lat/lon/height to convert to local XYZ
## 3. Or enter local XYZ to convert to lat/lon

@export_file("*.gml") var gml_path: String = ""

var geo_reference: PLATEAUGeoReference
var city_model: PLATEAUCityModel
var mesh_instances: Array[MeshInstance3D] = []
var pending_options: PLATEAUMeshExtractOptions

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var zone_spin: SpinBox = $UI/ConversionPanel/ZoneSpin
@onready var lat_spin: SpinBox = $UI/ConversionPanel/LatSpin
@onready var lon_spin: SpinBox = $UI/ConversionPanel/LonSpin
@onready var height_spin: SpinBox = $UI/ConversionPanel/HeightSpin
@onready var x_spin: SpinBox = $UI/ConversionPanel/XSpin
@onready var y_spin: SpinBox = $UI/ConversionPanel/YSpin
@onready var z_spin: SpinBox = $UI/ConversionPanel/ZSpin
@onready var marker: MeshInstance3D = $Marker
@onready var loading_panel: Panel = $UI/LoadingPanel
@onready var loading_label: Label = $UI/LoadingPanel/LoadingLabel


func _ready() -> void:
	geo_reference = PLATEAUGeoReference.new()
	geo_reference.zone_id = 9  # Tokyo default

	# Connect UI
	$UI/ConversionPanel/ProjectButton.pressed.connect(_on_project_pressed)
	$UI/ConversionPanel/UnprojectButton.pressed.connect(_on_unproject_pressed)
	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/ImportPanel/ShowLocationButton.pressed.connect(_on_show_location_pressed)
	zone_spin.value_changed.connect(_on_zone_changed)

	# Setup marker
	var sphere = SphereMesh.new()
	sphere.radius = 5.0
	sphere.height = 10.0
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color.RED
	sphere.material = mat
	marker.mesh = sphere
	marker.visible = false

	# Preset locations
	_setup_preset_locations()

	# Hide loading panel initially
	loading_panel.visible = false

	# Setup city model with async handlers
	city_model = PLATEAUCityModel.new()
	city_model.load_completed.connect(_on_load_completed)
	city_model.extract_completed.connect(_on_extract_completed)

	_log("GeoReference Sample ready.")
	_log("Zone " + str(zone_spin.value) + " selected.")
	_log("")
	_log("Enter lat/lon/height and click 'Project' to get local XYZ.")
	_log("Or enter local XYZ and click 'Unproject' to get lat/lon.")


func _setup_preset_locations() -> void:
	var preset_option = $UI/ConversionPanel/PresetOption as OptionButton
	preset_option.clear()
	preset_option.add_item("-- Presets --")
	preset_option.add_item("Tokyo Station")
	preset_option.add_item("Shibuya Station")
	preset_option.add_item("Shinjuku Station")
	preset_option.add_item("Tokyo Tower")
	preset_option.add_item("Tokyo Skytree")
	preset_option.add_item("Osaka Station")
	preset_option.add_item("Nagoya Station")
	preset_option.item_selected.connect(_on_preset_selected)


func _on_preset_selected(index: int) -> void:
	var presets = [
		{},  # placeholder
		{"name": "Tokyo Station", "lat": 35.6812, "lon": 139.7671, "zone": 9},
		{"name": "Shibuya Station", "lat": 35.6580, "lon": 139.7016, "zone": 9},
		{"name": "Shinjuku Station", "lat": 35.6896, "lon": 139.7006, "zone": 9},
		{"name": "Tokyo Tower", "lat": 35.6586, "lon": 139.7454, "zone": 9},
		{"name": "Tokyo Skytree", "lat": 35.7101, "lon": 139.8107, "zone": 9},
		{"name": "Osaka Station", "lat": 34.7024, "lon": 135.4959, "zone": 6},
		{"name": "Nagoya Station", "lat": 35.1709, "lon": 136.8815, "zone": 7},
	]

	if index <= 0 or index >= presets.size():
		return

	var preset = presets[index]
	lat_spin.value = preset["lat"]
	lon_spin.value = preset["lon"]
	zone_spin.value = preset["zone"]
	height_spin.value = 0

	_log("Preset: " + preset["name"])
	_on_project_pressed()


func _on_zone_changed(value: float) -> void:
	geo_reference.zone_id = int(value)
	_log("Zone changed to " + str(int(value)))


func _on_project_pressed() -> void:
	geo_reference.zone_id = int(zone_spin.value)

	var lat = lat_spin.value
	var lon = lon_spin.value
	var height = height_spin.value

	var lat_lon_height = Vector3(lat, lon, height)
	var xyz = geo_reference.project(lat_lon_height)

	# Update XYZ spinboxes
	x_spin.value = xyz.x
	y_spin.value = xyz.y
	z_spin.value = xyz.z

	_log("--- Project ---")
	_log("Input: lat=%.6f, lon=%.6f, height=%.2f" % [lat, lon, height])
	_log("Output: x=%.2f, y=%.2f, z=%.2f" % [xyz.x, xyz.y, xyz.z])

	# Move marker
	marker.position = xyz
	marker.visible = true


func _on_unproject_pressed() -> void:
	geo_reference.zone_id = int(zone_spin.value)

	var x = x_spin.value
	var y = y_spin.value
	var z = z_spin.value

	var xyz = Vector3(x, y, z)
	var lat_lon_height = geo_reference.unproject(xyz)

	# Update lat/lon spinboxes
	lat_spin.value = lat_lon_height.x
	lon_spin.value = lat_lon_height.y
	height_spin.value = lat_lon_height.z

	_log("--- Unproject ---")
	_log("Input: x=%.2f, y=%.2f, z=%.2f" % [x, y, z])
	_log("Output: lat=%.6f, lon=%.6f, height=%.2f" % [lat_lon_height.x, lat_lon_height.y, lat_lon_height.z])

	# Move marker
	marker.position = xyz
	marker.visible = true


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


func _import_gml(path: String) -> void:
	if city_model.is_processing():
		_log("[color=yellow]Already processing, please wait...[/color]")
		return

	_log("--- Import GML ---")
	_log("File: " + path.get_file())

	# Clear previous
	PLATEAUUtils.clear_mesh_instances(mesh_instances)

	# Use zone_id from UI
	geo_reference.zone_id = int(zone_spin.value)
	_log("Using zone_id: " + str(geo_reference.zone_id))

	# Prepare extraction options
	pending_options = PLATEAUMeshExtractOptions.new()
	pending_options.coordinate_zone_id = int(zone_spin.value)
	pending_options.mesh_granularity = 1  # Primary

	# Start async load
	_show_loading("Loading CityGML...")
	city_model.load_async(path)


func _on_load_completed(success: bool) -> void:
	if not success:
		_hide_loading()
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	_update_loading("Extracting meshes...")
	city_model.extract_meshes_async(pending_options)


func _on_extract_completed(root_mesh_data: Array) -> void:
	var mesh_data_array = PLATEAUUtils.flatten_mesh_data(root_mesh_data)

	if mesh_data_array.is_empty():
		_hide_loading()
		_log("[color=yellow]WARNING: No meshes extracted[/color]")
		return

	_update_loading("Creating mesh instances...")
	var result = await PLATEAUUtils.create_mesh_instances_async(
		mesh_data_array,
		self,
		50,
		func(percent, current, total):
			_update_loading("Creating instances: %d/%d (%d%%)" % [current, total, percent])
	)
	mesh_instances = result["instances"]

	# Position camera
	var center = (result["bounds_min"] + result["bounds_max"]) / 2
	PLATEAUUtils.fit_camera_to_bounds(camera, result["bounds_min"], result["bounds_max"])

	# Calculate center lat/lon
	var center_lat_lon = geo_reference.unproject(center)
	_log("Model center: lat=%.6f, lon=%.6f" % [center_lat_lon.x, center_lat_lon.y])

	# Set as default
	lat_spin.value = center_lat_lon.x
	lon_spin.value = center_lat_lon.y
	height_spin.value = center_lat_lon.z

	_hide_loading()
	_log("[color=green]Import complete! " + str(mesh_data_array.size()) + " meshes[/color]")


func _on_show_location_pressed() -> void:
	if mesh_instances.is_empty():
		_log("[color=yellow]Import a GML file first[/color]")
		return

	_on_project_pressed()

	# Move camera to marker
	var target = marker.position
	camera.position = target + Vector3(0, 100, 150)
	camera.look_at(target)

	_log("Camera moved to location")


func _show_loading(message: String) -> void:
	loading_label.text = message
	loading_panel.visible = true


func _update_loading(message: String) -> void:
	loading_label.text = message


func _hide_loading() -> void:
	loading_panel.visible = false


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
