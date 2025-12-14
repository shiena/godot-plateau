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
var pending_options: PLATEAUMeshExtractOptions
var pending_mesh_data: Array = []

@onready var camera: Camera3D = $Camera3D
@onready var attributes_label: RichTextLabel = $UI/Panel/ScrollContainer/AttributesLabel
@onready var info_label: Label = $UI/InfoLabel
@onready var import_button: Button = $UI/ImportButton
@onready var loading_panel: Panel = $UI/LoadingPanel
@onready var loading_label: Label = $UI/LoadingPanel/LoadingLabel


func _ready() -> void:
	import_button.pressed.connect(_on_import_pressed)
	_update_info("Click 'Import GML' or set gml_path and run")

	# Hide loading panel initially
	loading_panel.visible = false

	# Setup city model with async handlers
	city_model = PLATEAUCityModel.new()
	city_model.load_completed.connect(_on_load_completed)
	city_model.extract_completed.connect(_on_extract_completed)


func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
			_handle_click(event.position)


func _on_import_pressed() -> void:
	if city_model.is_processing():
		_update_info("Already processing, please wait...")
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
		_update_info("Already processing, please wait...")
		return

	_update_info("Loading: " + path.get_file() + "...")

	# Clear previous data
	for child in get_children():
		if child is MeshInstance3D:
			child.queue_free()
	mesh_data_map.clear()

	# Prepare extraction options
	pending_options = PLATEAUMeshExtractOptions.new()
	pending_options.coordinate_zone_id = zone_id
	pending_options.min_lod = 1
	pending_options.max_lod = 2
	pending_options.mesh_granularity = 1  # Primary feature (per building)
	pending_options.export_appearance = true

	# Start async load
	_show_loading("Loading CityGML...")
	city_model.load_async(path)


func _on_load_completed(success: bool) -> void:
	if not success:
		_hide_loading()
		_update_info("Failed to load: " + gml_path)
		return

	_update_loading("Extracting meshes...")
	city_model.extract_meshes_async(pending_options)


func _on_extract_completed(root_mesh_data: Array) -> void:
	if root_mesh_data.is_empty():
		_hide_loading()
		_update_info("No meshes extracted from: " + gml_path)
		return

	# Flatten hierarchy to get actual building meshes (children of LOD nodes)
	pending_mesh_data = PLATEAUUtils.flatten_mesh_data(root_mesh_data)

	# Create MeshInstance3D for each mesh (async with collision)
	_update_loading("Creating mesh instances...")
	await _create_mesh_instances_with_collision()

	_hide_loading()
	_update_info("Loaded " + str(pending_mesh_data.size()) + " meshes. Click on buildings to see attributes.")


func _create_mesh_instances_with_collision() -> void:
	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF
	var total := pending_mesh_data.size()
	var batch_size := 50

	for i in range(0, total, batch_size):
		var batch_end := mini(i + batch_size, total)

		for j in range(i, batch_end):
			var mesh_data = pending_mesh_data[j]
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

		var percent := int((batch_end * 100) / total) if total > 0 else 100
		_update_loading("Creating instances: %d/%d (%d%%)" % [batch_end, total, percent])
		await get_tree().process_frame

	# Position camera to view all meshes
	PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)


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


func _show_loading(message: String) -> void:
	loading_label.text = message
	loading_panel.visible = true


func _update_loading(message: String) -> void:
	loading_label.text = message


func _hide_loading() -> void:
	loading_panel.visible = false


func _update_info(message: String) -> void:
	info_label.text = message
	print(message)
