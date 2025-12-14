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

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var zone_spin: SpinBox = $UI/OptionsPanel/ZoneSpin
@onready var min_lod_spin: SpinBox = $UI/OptionsPanel/MinLODSpin
@onready var max_lod_spin: SpinBox = $UI/OptionsPanel/MaxLODSpin
@onready var granularity_option: OptionButton = $UI/OptionsPanel/GranularityOption
@onready var texture_check: CheckBox = $UI/OptionsPanel/TextureCheck
@onready var collision_check: CheckBox = $UI/OptionsPanel/CollisionCheck
@onready var stats_label: Label = $UI/StatsLabel


func _ready() -> void:
	# PLATEAUImporter is a Node3D - add it as a child
	importer = PLATEAUImporter.new()
	importer.name = "PLATEAUImporter"
	add_child(importer)

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

	_log("PLATEAUImporter Sample ready.")
	_log("")
	_log("PLATEAUImporter provides a high-level API for importing CityGML.")
	_log("It automatically builds scene hierarchy as child nodes.")
	_log("")
	_log("Click 'Import GML' to start.")


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
	_log("--- Import with PLATEAUImporter ---")
	_log("File: " + path.get_file())

	# Configure extract options
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = int(zone_spin.value)
	options.min_lod = int(min_lod_spin.value)
	options.max_lod = int(max_lod_spin.value)
	options.mesh_granularity = granularity_option.get_selected_id()
	options.export_appearance = texture_check.button_pressed

	_log("Options:")
	_log("  Zone: " + str(options.coordinate_zone_id))
	_log("  LOD: " + str(options.min_lod) + "-" + str(options.max_lod))
	_log("  Granularity: " + granularity_option.get_item_text(granularity_option.selected))
	_log("  Textures: " + str(options.export_appearance))
	_log("  Collision: " + str(collision_check.button_pressed))

	# Configure importer
	importer.extract_options = options
	importer.generate_collision = collision_check.button_pressed

	# Configure geo reference (optional - importer creates default)
	var geo_ref = PLATEAUGeoReference.new()
	geo_ref.zone_id = int(zone_spin.value)
	importer.geo_reference = geo_ref

	# Import!
	var start_time = Time.get_ticks_msec()
	var success = importer.import_from_path(path)
	var elapsed = Time.get_ticks_msec() - start_time

	if success:
		_log("[color=green]Import successful! (" + str(elapsed) + " ms)[/color]")

		# Get imported city model for additional info
		var city_model = importer.get_city_model()
		if city_model:
			_log("CityModel loaded: " + str(city_model))

		# Count imported nodes
		var mesh_count = _count_mesh_instances(importer)
		var collision_count = _count_collisions(importer)
		_log("Created " + str(mesh_count) + " mesh instances")
		if collision_check.button_pressed:
			_log("Created " + str(collision_count) + " collision bodies")

		_update_stats()
		_fit_camera_to_scene()
	else:
		_log("[color=red]Import failed![/color]")


func _on_clear_pressed() -> void:
	_log("--- Clear ---")
	importer.clear_meshes()
	_log("All meshes cleared")
	_update_stats()


func _on_save_scene_pressed() -> void:
	if not importer.is_imported():
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


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
