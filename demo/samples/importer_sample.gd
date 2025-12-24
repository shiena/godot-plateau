extends Node3D
## Importer Sample
##
## Demonstrates PLATEAUImporter high-level API:
## - Simplified GML import with automatic scene hierarchy
## - Extract options configuration
## - Collision generation
## - Scene export
##
## PLATEAUImporter automatically:
## - Loads GML files
## - Extracts meshes with configured options
## - Builds scene hierarchy as child nodes
## - Optionally generates collision shapes

@export_file("*.gml") var gml_path: String = ""

var importer: PLATEAUImporter
var city_model: PLATEAUCityModel
var pending_options: PLATEAUMeshExtractOptions
var import_start_time: int = 0

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var zone_spin: SpinBox = $UI/OptionsPanel/ZoneSpin
@onready var min_lod_spin: SpinBox = $UI/OptionsPanel/MinLODSpin
@onready var max_lod_spin: SpinBox = $UI/OptionsPanel/MaxLODSpin
@onready var granularity_option: OptionButton = $UI/OptionsPanel/GranularityOption
@onready var texture_check: CheckBox = $UI/OptionsPanel/TextureCheck
@onready var collision_check: CheckBox = $UI/OptionsPanel/CollisionCheck
@onready var stats_label: Label = $UI/StatsLabel
@onready var loading_panel: Panel = $UI/LoadingPanel
@onready var loading_label: Label = $UI/LoadingPanel/LoadingLabel


func _ready() -> void:
	# PLATEAUImporter is a Node3D - add it as a child
	importer = PLATEAUImporter.new()
	importer.name = "PLATEAUImporter"
	add_child(importer)

	# Setup city model with async handlers
	city_model = PLATEAUCityModel.new()
	city_model.load_completed.connect(_on_load_completed)
	city_model.extract_completed.connect(_on_extract_completed)

	# Connect UI
	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/ImportPanel/ClearButton.pressed.connect(_on_clear_pressed)
	$UI/ImportPanel/SaveSceneButton.pressed.connect(_on_save_scene_pressed)

	# Setup granularity options
	granularity_option.clear()
	granularity_option.add_item("Atomic (finest)", 0)
	granularity_option.add_item("Primary (per building)", 1)
	granularity_option.add_item("Area (merged)", 2)
	granularity_option.select(1)

	# Hide loading panel initially
	loading_panel.visible = false

	_log("PLATEAUImporter Sample ready.")
	_log("")
	_log("PLATEAUImporter provides a high-level API for importing CityGML.")
	_log("It automatically builds scene hierarchy as child nodes.")
	_log("")
	_log("Click 'Import GML' to start.")


func _on_import_pressed() -> void:
	if city_model.is_processing():
		_log("[color=yellow]Already processing, please wait...[/color]")
		return

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
	if city_model.is_processing():
		_log("[color=yellow]Already processing, please wait...[/color]")
		return

	_log("--- Import with PLATEAUImporter ---")
	_log("File: " + path.get_file())

	# Use PLATEAUGmlFile to get metadata
	var gml_file = PLATEAUGmlFile.create(path)
	if gml_file.is_valid():
		_log("[color=cyan]GML File Info:[/color]")
		_log("  Grid Code: " + gml_file.get_grid_code())
		_log("  Feature Type: " + gml_file.get_feature_type())
		_log("  EPSG: " + str(gml_file.get_epsg()))
		_log("  Dataset Root: " + gml_file.get_dataset_root_path())

		# Search for textures in GML (like Unity)
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

	# Clear previous import
	importer.clear_meshes()

	# Configure extract options
	pending_options = PLATEAUMeshExtractOptions.new()
	pending_options.coordinate_zone_id = int(zone_spin.value)
	pending_options.min_lod = int(min_lod_spin.value)
	pending_options.max_lod = int(max_lod_spin.value)
	pending_options.mesh_granularity = granularity_option.get_selected_id()
	pending_options.export_appearance = texture_check.button_pressed
	pending_options.highest_lod_only = true

	_log("[color=cyan]Import Options:[/color]")
	_log("  Zone: " + str(pending_options.coordinate_zone_id))
	_log("  LOD: " + str(pending_options.min_lod) + "-" + str(pending_options.max_lod))
	_log("  Granularity: " + granularity_option.get_item_text(granularity_option.selected))
	_log("  Textures: " + str(pending_options.export_appearance))
	_log("  Collision: " + str(collision_check.button_pressed))

	# Configure importer for scene building later
	importer.extract_options = pending_options
	importer.generate_collision = collision_check.button_pressed

	# Configure geo reference
	var geo_ref = PLATEAUGeoReference.new()
	geo_ref.zone_id = int(zone_spin.value)
	importer.geo_reference = geo_ref

	# Start async import
	import_start_time = Time.get_ticks_msec()
	_show_loading("Loading CityGML...")
	city_model.load_async(path)


func _on_load_completed(success: bool) -> void:
	if not success:
		_hide_loading()
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	_log("GML load completed")
	_update_loading("Extracting meshes...")
	city_model.extract_meshes_async(pending_options)


func _on_extract_completed(root_mesh_data: Array) -> void:
	if root_mesh_data.is_empty():
		_hide_loading()
		_log("[color=yellow]No meshes extracted[/color]")
		_update_stats()
		return

	_update_loading("Building scene hierarchy...")
	await _build_scene_async(root_mesh_data)

	var elapsed = Time.get_ticks_msec() - import_start_time
	_hide_loading()
	_log("[color=green]Import successful! (" + str(elapsed) + " ms)[/color]")

	# Count imported nodes
	var mesh_count = _count_mesh_instances(importer)
	var collision_count = _count_collisions(importer)
	_log("Created " + str(mesh_count) + " mesh instances")
	if collision_check.button_pressed:
		_log("Created " + str(collision_count) + " collision bodies")

	_update_stats()
	_fit_camera_to_scene()


func _build_scene_async(root_mesh_data: Array) -> void:
	# Build scene hierarchy using PLATEAUImporter.import_to_scene
	# Process in batches to keep UI responsive
	var total = root_mesh_data.size()
	var batch_size = 20

	for i in range(0, total, batch_size):
		var batch_end = mini(i + batch_size, total)

		for j in range(i, batch_end):
			var mesh_data = root_mesh_data[j]
			var node = _create_node_from_mesh_data(mesh_data)
			if node:
				importer.add_child(node)
				node.owner = importer

		_update_loading("Building scene: %d/%d" % [batch_end, total])
		await get_tree().process_frame


func _create_node_from_mesh_data(mesh_data: PLATEAUMeshData) -> Node3D:
	if mesh_data == null:
		return null

	var mesh = mesh_data.get_mesh()
	var node: Node3D

	if mesh != null and mesh.get_surface_count() > 0:
		# Create MeshInstance3D for nodes with meshes
		var mi = MeshInstance3D.new()
		mi.name = mesh_data.get_name()
		mi.mesh = mesh
		mi.transform = mesh_data.get_transform()

		# Generate collision if enabled
		if collision_check.button_pressed:
			_create_collision_for_mesh(mi)

		node = mi
	else:
		# Create Node3D for nodes without meshes (containers)
		node = Node3D.new()
		node.name = mesh_data.get_name()
		node.transform = mesh_data.get_transform()

	# Recursively add children
	var children = mesh_data.get_children()
	for child in children:
		var child_node = _create_node_from_mesh_data(child)
		if child_node:
			node.add_child(child_node)
			child_node.owner = node

	return node


func _create_collision_for_mesh(mesh_instance: MeshInstance3D) -> void:
	if mesh_instance == null or mesh_instance.mesh == null:
		return

	var mesh = mesh_instance.mesh

	# Create trimesh collision shape from mesh
	var shape = ConcavePolygonShape3D.new()

	# Get faces from all surfaces
	var faces = PackedVector3Array()
	for surface_idx in range(mesh.get_surface_count()):
		var arrays = mesh.surface_get_arrays(surface_idx)
		if arrays.size() == 0:
			continue

		var vertices: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
		var indices: PackedInt32Array = arrays[Mesh.ARRAY_INDEX] if arrays.size() > Mesh.ARRAY_INDEX else PackedInt32Array()

		if indices.size() > 0:
			# Indexed mesh
			for idx in indices:
				faces.push_back(vertices[idx])
		else:
			# Non-indexed mesh
			for v in vertices:
				faces.push_back(v)

	if faces.size() < 3:
		return

	shape.set_faces(faces)

	# Create StaticBody3D and CollisionShape3D
	var static_body = StaticBody3D.new()
	static_body.name = mesh_instance.name + "_collision"

	var collision_shape = CollisionShape3D.new()
	collision_shape.name = "CollisionShape3D"
	collision_shape.shape = shape

	static_body.add_child(collision_shape)
	mesh_instance.add_child(static_body)


func _on_clear_pressed() -> void:
	_log("--- Clear ---")
	importer.clear_meshes()
	_log("All meshes cleared")
	_update_stats()


func _on_save_scene_pressed() -> void:
	if importer.get_child_count() == 0:
		_log("[color=red]ERROR: No import to save. Import first.[/color]")
		return

	var dialog = FileDialog.new()
	dialog.file_mode = FileDialog.FILE_MODE_SAVE_FILE
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.filters = ["*.tscn ; Godot Scene Files", "*.scn ; Binary Scene Files"]
	dialog.current_file = "plateau_import.tscn"
	dialog.file_selected.connect(_on_save_file_selected)
	add_child(dialog)
	dialog.popup_centered(Vector2i(800, 600))


func _on_save_file_selected(path: String) -> void:
	_log("--- Save Scene ---")
	_log("Path: " + path)

	# Create a packed scene from importer's children
	var scene_root = Node3D.new()
	scene_root.name = "PLATEAUScene"

	# Copy importer's children to new root
	for child in importer.get_children():
		var duplicate = child.duplicate()
		scene_root.add_child(duplicate)
		duplicate.owner = scene_root
		_set_owner_recursive(duplicate, scene_root)

	var packed_scene = PackedScene.new()
	var result = packed_scene.pack(scene_root)

	if result == OK:
		var error = ResourceSaver.save(packed_scene, path)
		if error == OK:
			_log("[color=green]Scene saved successfully![/color]")
		else:
			_log("[color=red]Failed to save scene: " + str(error) + "[/color]")
	else:
		_log("[color=red]Failed to pack scene: " + str(result) + "[/color]")

	scene_root.queue_free()


func _set_owner_recursive(node: Node, owner: Node) -> void:
	for child in node.get_children():
		child.owner = owner
		_set_owner_recursive(child, owner)


func _count_mesh_instances(node: Node) -> int:
	var count = 0
	if node is MeshInstance3D:
		count = 1
	for child in node.get_children():
		count += _count_mesh_instances(child)
	return count


func _count_collisions(node: Node) -> int:
	var count = 0
	if node is StaticBody3D:
		count = 1
	for child in node.get_children():
		count += _count_collisions(child)
	return count


func _update_stats() -> void:
	var mesh_count = _count_mesh_instances(importer)
	var collision_count = _count_collisions(importer)
	var total_vertices = 0
	var total_triangles = 0

	_count_geometry(importer, total_vertices, total_triangles)

	stats_label.text = "Meshes: %d | Vertices: %d | Triangles: %d | Collisions: %d" % [
		mesh_count, total_vertices, total_triangles, collision_count
	]


func _count_geometry(node: Node, vertices: int, triangles: int) -> void:
	# Note: GDScript doesn't support pass-by-reference, so this is simplified
	pass


func _fit_camera_to_scene() -> void:
	var bounds = _calculate_bounds(importer)

	if bounds.size.length() > 0.001:
		var center = bounds.position + bounds.size / 2
		var size = bounds.size.length()
		camera.position = center + Vector3(0, size * 0.5, size * 0.8)
		camera.look_at(center)
		_log("Camera positioned at center: " + str(center))


func _calculate_bounds(node: Node) -> AABB:
	var bounds = AABB()
	var first = true

	for child in node.get_children():
		if child is MeshInstance3D:
			var mi = child as MeshInstance3D
			var aabb = mi.get_aabb()
			var global_aabb = mi.global_transform * aabb
			if first:
				bounds = global_aabb
				first = false
			else:
				bounds = bounds.merge(global_aabb)

		# Recursively check children
		var child_bounds = _calculate_bounds(child)
		if child_bounds.size.length() > 0:
			if first:
				bounds = child_bounds
				first = false
			else:
				bounds = bounds.merge(child_bounds)

	return bounds


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
