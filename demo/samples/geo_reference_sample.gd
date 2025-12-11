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
	if gml_path.is_empty():
		var dialog = FileDialog.new()
		dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
		dialog.access = FileDialog.ACCESS_FILESYSTEM
		dialog.filters = ["*.gml ; CityGML Files"]
		dialog.file_selected.connect(_on_file_selected)
		add_child(dialog)
		dialog.popup_centered(Vector2i(800, 600))
	else:
		_import_gml(gml_path)


func _on_file_selected(path: String) -> void:
	gml_path = path
	_import_gml(path)


func _import_gml(path: String) -> void:
	_log("--- Import GML ---")
	_log("File: " + path.get_file())

	# Clear previous
	for mi in mesh_instances:
		mi.queue_free()
	mesh_instances.clear()

	# Load CityGML
	city_model = PLATEAUCityModel.new()
	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	# Use zone_id from UI
	geo_reference.zone_id = int(zone_spin.value)
	_log("Using zone_id: " + str(geo_reference.zone_id))

	# Setup extraction options
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = int(zone_spin.value)
	options.mesh_granularity = 1  # Primary

	# Extract and display meshes
	var root_mesh_data = city_model.extract_meshes(options)
	var mesh_data_array = _flatten_mesh_data(Array(root_mesh_data))

	var bounds_min = Vector3.INF
	var bounds_max = -Vector3.INF

	for mesh_data in mesh_data_array:
		var mesh_instance = MeshInstance3D.new()
		mesh_instance.mesh = mesh_data.get_mesh()
		mesh_instance.transform = mesh_data.get_transform()
		add_child(mesh_instance)
		mesh_instances.append(mesh_instance)

		var aabb = mesh_instance.get_aabb()
		var global_aabb = mesh_instance.transform * aabb
		bounds_min = bounds_min.min(global_aabb.position)
		bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	# Position camera
	var center = (bounds_min + bounds_max) / 2
	var size = (bounds_max - bounds_min).length()
	if size > 0.001:
		camera.position = center + Vector3(0, size * 0.5, size * 0.8)
		camera.look_at(center)

	# Calculate center lat/lon
	var center_lat_lon = geo_reference.unproject(center)
	_log("Model center: lat=%.6f, lon=%.6f" % [center_lat_lon.x, center_lat_lon.y])

	# Set as default
	lat_spin.value = center_lat_lon.x
	lon_spin.value = center_lat_lon.y
	height_spin.value = center_lat_lon.z

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


func _flatten_mesh_data(mesh_data_array: Array) -> Array:
	var result: Array = []
	for mesh_data in mesh_data_array:
		if mesh_data.get_mesh() != null:
			result.append(mesh_data)
		var children = mesh_data.get_children()
		if not children.is_empty():
			result.append_array(_flatten_mesh_data(Array(children)))
	return result


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
