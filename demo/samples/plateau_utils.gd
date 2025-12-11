class_name PLATEAUUtils
## PLATEAU SDK Sample Utilities
##
## Common utility functions for PLATEAU SDK samples.
## Reduces code duplication across sample files.


## Recursively flatten mesh data hierarchy to a flat array
## @param mesh_data_array: Array of PLATEAUMeshData (may contain nested children)
## @return: Flat array of PLATEAUMeshData with non-null meshes
static func flatten_mesh_data(mesh_data_array: Array) -> Array:
	var result: Array = []
	for mesh_data in mesh_data_array:
		if mesh_data.get_mesh() != null:
			result.append(mesh_data)
		var children = mesh_data.get_children()
		if not children.is_empty():
			result.append_array(flatten_mesh_data(Array(children)))
	return result


## Create MeshInstance3D nodes from mesh data array
## @param mesh_data_array: Flat array of PLATEAUMeshData
## @param parent: Parent node to add mesh instances to
## @return: Dictionary with "instances", "bounds_min", "bounds_max"
static func create_mesh_instances(mesh_data_array: Array, parent: Node) -> Dictionary:
	var instances: Array[MeshInstance3D] = []
	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	for mesh_data in mesh_data_array:
		var mesh = mesh_data.get_mesh()
		if mesh == null:
			continue

		var mi := MeshInstance3D.new()
		mi.mesh = mesh
		mi.transform = mesh_data.get_transform()
		mi.name = mesh_data.get_name()

		parent.add_child(mi)
		instances.append(mi)

		# Calculate bounds
		var aabb := mi.get_aabb()
		if aabb.size.length() > 0.001:
			var global_aabb := mi.transform * aabb
			bounds_min = bounds_min.min(global_aabb.position)
			bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	return {
		"instances": instances,
		"bounds_min": bounds_min,
		"bounds_max": bounds_max
	}


## Position camera to view bounds
## @param camera: Camera3D to position
## @param bounds_min: Minimum corner of AABB
## @param bounds_max: Maximum corner of AABB
## @param height_factor: Camera height relative to size (default 0.5)
## @param distance_factor: Camera distance relative to size (default 0.8)
static func fit_camera_to_bounds(
	camera: Camera3D,
	bounds_min: Vector3,
	bounds_max: Vector3,
	height_factor: float = 0.5,
	distance_factor: float = 0.8
) -> void:
	if bounds_min == Vector3.INF or bounds_max == -Vector3.INF:
		return

	var center := (bounds_min + bounds_max) / 2.0
	var size := (bounds_max - bounds_min).length()

	if size < 0.001:
		size = 100.0  # Default size if bounds are too small

	camera.position = center + Vector3(0, size * height_factor, size * distance_factor)
	if not camera.position.is_equal_approx(center):
		camera.look_at(center)


## Clear and free mesh instances
## @param instances: Array of MeshInstance3D to free
static func clear_mesh_instances(instances: Array[MeshInstance3D]) -> void:
	for mi in instances:
		if is_instance_valid(mi):
			mi.queue_free()
	instances.clear()


## Show GML file dialog
## @param parent: Parent node for the dialog
## @param callback: Callable to call with selected path (func(path: String))
static func show_gml_file_dialog(parent: Node, callback: Callable) -> void:
	var dialog := FileDialog.new()
	dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.filters = ["*.gml ; CityGML Files"]
	dialog.file_selected.connect(callback)
	parent.add_child(dialog)
	dialog.popup_centered(Vector2i(800, 600))


## Show save file dialog
## @param parent: Parent node for the dialog
## @param filters: Array of filter strings (e.g., ["*.png ; PNG Image"])
## @param default_name: Default filename
## @param callback: Callable to call with selected path
static func show_save_file_dialog(
	parent: Node,
	filters: PackedStringArray,
	default_name: String,
	callback: Callable
) -> void:
	var dialog := FileDialog.new()
	dialog.file_mode = FileDialog.FILE_MODE_SAVE_FILE
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.filters = filters
	dialog.current_file = default_name
	dialog.file_selected.connect(callback)
	parent.add_child(dialog)
	dialog.popup_centered(Vector2i(800, 600))


## Show directory selection dialog
## @param parent: Parent node for the dialog
## @param callback: Callable to call with selected path
static func show_directory_dialog(parent: Node, callback: Callable) -> void:
	var dialog := FileDialog.new()
	dialog.file_mode = FileDialog.FILE_MODE_OPEN_DIR
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.dir_selected.connect(callback)
	parent.add_child(dialog)
	dialog.popup_centered(Vector2i(800, 600))


## Log message to RichTextLabel with timestamp and auto-scroll
## @param log_label: RichTextLabel to append to
## @param scroll_container: ScrollContainer to auto-scroll
## @param message: Message to log
## @param tree: SceneTree for process_frame await (pass get_tree())
static func log_message(
	log_label: RichTextLabel,
	scroll_container: ScrollContainer,
	message: String,
	tree: SceneTree
) -> void:
	var timestamp := Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	print(message)
	# Auto-scroll (requires await, so caller must handle async)
	if tree != null:
		await tree.process_frame
		scroll_container.scroll_vertical = int(scroll_container.get_v_scroll_bar().max_value)


## Create MeshInstance3D nodes from mesh data array with batched processing (async version)
## Prevents UI freeze by yielding control back to main thread periodically
## @param mesh_data_array: Flat array of PLATEAUMeshData
## @param parent: Parent node to add mesh instances to
## @param batch_size: Number of meshes to process per frame (default 50)
## @param progress_callback: Optional callback func(percent: int, current: int, total: int)
## @return: Dictionary with "instances", "bounds_min", "bounds_max"
static func create_mesh_instances_async(
	mesh_data_array: Array,
	parent: Node,
	batch_size: int = 50,
	progress_callback: Callable = Callable()
) -> Dictionary:
	var instances: Array[MeshInstance3D] = []
	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF
	var total := mesh_data_array.size()

	for i in range(0, total, batch_size):
		var batch_end := mini(i + batch_size, total)

		for j in range(i, batch_end):
			var mesh_data = mesh_data_array[j]
			var mesh = mesh_data.get_mesh()
			if mesh == null:
				continue

			var mi := MeshInstance3D.new()
			mi.mesh = mesh
			mi.transform = mesh_data.get_transform()
			mi.name = mesh_data.get_name()

			parent.add_child(mi)
			instances.append(mi)

			# Calculate bounds
			var aabb := mi.get_aabb()
			if aabb.size.length() > 0.001:
				var global_aabb := mi.transform * aabb
				bounds_min = bounds_min.min(global_aabb.position)
				bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

		# Report progress and yield to main thread
		if progress_callback.is_valid():
			var percent := int((batch_end * 100) / total) if total > 0 else 100
			progress_callback.call(percent, batch_end, total)

		# Yield to prevent UI freeze
		await parent.get_tree().process_frame

	return {
		"instances": instances,
		"bounds_min": bounds_min,
		"bounds_max": bounds_max
	}
