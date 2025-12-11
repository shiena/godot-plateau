extends Node3D
## Attributes Display Sample
##
## Demonstrates how to display CityObject attributes when clicking on buildings.
## - Left-click on a building to show its attributes
## - Attributes are displayed in the UI panel on the right
##
## Usage:
## 1. Set gml_path to your CityGML file path
## 2. Run the scene
## 3. Click on buildings to see their attributes

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9  ## Japan Plane Rectangular CS (1-19), default: Tokyo

var city_model: PLATEAUCityModel
var mesh_data_map: Dictionary = {}  # MeshInstance3D -> PLATEAUMeshData

@onready var camera: Camera3D = $Camera3D
@onready var attributes_label: RichTextLabel = $UI/Panel/ScrollContainer/AttributesLabel
@onready var info_label: Label = $UI/InfoLabel
@onready var import_button: Button = $UI/ImportButton


func _ready() -> void:
	import_button.pressed.connect(_on_import_pressed)
	_update_info("Click 'Import GML' or set gml_path and run")


func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
			_handle_click(event.position)


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
	_update_info("Loading: " + path.get_file() + "...")

	# Clear previous data
	for child in get_children():
		if child is MeshInstance3D:
			child.queue_free()
	mesh_data_map.clear()

	# Load CityGML
	city_model = PLATEAUCityModel.new()
	if not city_model.load(path):
		_update_info("Failed to load: " + path)
		return

	# Setup extraction options
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.min_lod = 1
	options.max_lod = 2
	options.mesh_granularity = 1  # Primary feature (per building)
	options.export_appearance = true

	# Extract meshes
	var root_mesh_data = city_model.extract_meshes(options)

	if root_mesh_data.is_empty():
		_update_info("No meshes extracted from: " + path)
		return

	# Flatten hierarchy to get actual building meshes (children of LOD nodes)
	var mesh_data_array = _flatten_mesh_data(root_mesh_data)

	# Create MeshInstance3D for each mesh
	var bounds_min = Vector3.INF
	var bounds_max = -Vector3.INF

	for mesh_data in mesh_data_array:
		var mesh_instance = MeshInstance3D.new()
		mesh_instance.mesh = mesh_data.get_mesh()
		mesh_instance.transform = mesh_data.get_transform()
		mesh_instance.name = mesh_data.get_name()

		# Create collision for raycasting
		mesh_instance.create_trimesh_collision()

		add_child(mesh_instance)
		mesh_data_map[mesh_instance] = mesh_data

		# Calculate bounds
		var aabb = mesh_instance.get_aabb()
		var global_aabb = mesh_instance.transform * aabb
		bounds_min = bounds_min.min(global_aabb.position)
		bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	# Position camera to view all meshes
	var center = (bounds_min + bounds_max) / 2
	var size = (bounds_max - bounds_min).length()
	if size < 0.001:
		size = 100.0  # Default size if bounds are too small
	camera.position = center + Vector3(0, size * 0.5, size * 0.8)
	if not camera.position.is_equal_approx(center):
		camera.look_at(center)

	_update_info("Loaded " + str(mesh_data_array.size()) + " meshes. Click on buildings to see attributes.")


func _handle_click(screen_pos: Vector2) -> void:
	var from = camera.project_ray_origin(screen_pos)
	var to = from + camera.project_ray_normal(screen_pos) * 10000

	var space_state = get_world_3d().direct_space_state
	var query = PhysicsRayQueryParameters3D.create(from, to)
	var result = space_state.intersect_ray(query)

	if result.is_empty():
		attributes_label.text = "Click on a building to see its attributes."
		return

	# Find the MeshInstance3D from collision
	var collider = result.collider
	var mesh_instance: MeshInstance3D = null

	if collider is StaticBody3D:
		mesh_instance = collider.get_parent() as MeshInstance3D

	if mesh_instance == null or not mesh_data_map.has(mesh_instance):
		attributes_label.text = "No attribute data for this object."
		return

	var mesh_data: PLATEAUMeshData = mesh_data_map[mesh_instance]

	# Try to get specific CityObject from UV
	var hit_uv = result.get("uv", Vector2.ZERO)
	var gml_id = mesh_data.get_gml_id_from_uv(hit_uv)
	if gml_id.is_empty():
		gml_id = mesh_data.get_gml_id()

	# Build attribute display text
	var text = "[b]GML ID:[/b] " + gml_id + "\n"
	text += "[b]Type:[/b] " + mesh_data.get_city_object_type_name() + "\n"
	text += "[b]Name:[/b] " + mesh_data.get_name() + "\n\n"
	text += "[b]Attributes:[/b]\n"

	var attributes = mesh_data.get_attributes()
	text += _format_attributes(attributes, 0)

	attributes_label.text = text


func _format_attributes(attrs: Dictionary, indent: int) -> String:
	var text = ""
	var prefix = "  ".repeat(indent)

	for key in attrs.keys():
		var value = attrs[key]
		if value is Dictionary:
			text += prefix + "[color=cyan]" + str(key) + ":[/color]\n"
			text += _format_attributes(value, indent + 1)
		else:
			text += prefix + "[color=yellow]" + str(key) + ":[/color] " + str(value) + "\n"

	return text


func _update_info(message: String) -> void:
	info_label.text = message
	print(message)


func _flatten_mesh_data(mesh_data_array: Array) -> Array:
	## Recursively collect all mesh data including children
	var result: Array = []
	for mesh_data in mesh_data_array:
		if mesh_data.get_mesh() != null:
			result.append(mesh_data)
		# Recursively add children
		var children = mesh_data.get_children()
		if not children.is_empty():
			result.append_array(_flatten_mesh_data(Array(children)))
	return result
