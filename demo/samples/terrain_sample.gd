extends Node3D
## Terrain Sample
##
## Demonstrates terrain-related features:
## - Generate heightmap from DEM/terrain mesh
## - Align buildings to terrain height
## - Save heightmap as image
##
## Usage:
## 1. Load a terrain GML file (dem, tin, etc.)
## 2. Generate heightmap from terrain
## 3. Load building GML file
## 4. Align buildings to terrain

@export_file("*.gml") var terrain_gml_path: String = ""
@export_file("*.gml") var building_gml_path: String = ""
@export var zone_id: int = 9  ## Japan Plane Rectangular CS (1-19)

var terrain_city_model: PLATEAUCityModel
var building_city_model: PLATEAUCityModel
var terrain_mesh_data: Array = []
var building_mesh_data: Array = []
var heightmap_data: PLATEAUHeightMapData
var aligner: PLATEAUHeightMapAligner

var terrain_instances: Array[MeshInstance3D] = []
var building_instances: Array[MeshInstance3D] = []

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var heightmap_preview: TextureRect = $UI/HeightmapPanel/HeightmapPreview


func _ready() -> void:
	$UI/TerrainPanel/LoadTerrainButton.pressed.connect(_on_load_terrain_pressed)
	$UI/TerrainPanel/GenerateHeightmapButton.pressed.connect(_on_generate_heightmap)
	$UI/TerrainPanel/SaveHeightmapButton.pressed.connect(_on_save_heightmap)

	$UI/BuildingPanel/LoadBuildingButton.pressed.connect(_on_load_building_pressed)
	$UI/BuildingPanel/AlignButton.pressed.connect(_on_align_buildings)

	_log("Terrain Sample ready.")
	_log("1. Load terrain GML (DEM/TIN)")
	_log("2. Generate heightmap")
	_log("3. Load building GML")
	_log("4. Align buildings to terrain")


func _on_load_terrain_pressed() -> void:
	if terrain_gml_path.is_empty():
		var dialog = FileDialog.new()
		dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
		dialog.access = FileDialog.ACCESS_FILESYSTEM
		dialog.filters = ["*.gml ; CityGML Files"]
		dialog.file_selected.connect(_on_terrain_file_selected)
		add_child(dialog)
		dialog.popup_centered(Vector2i(800, 600))
	else:
		_load_terrain(terrain_gml_path)


func _on_terrain_file_selected(path: String) -> void:
	terrain_gml_path = path
	_load_terrain(path)


func _load_terrain(path: String) -> void:
	_log("--- Loading Terrain ---")
	_log("File: " + path.get_file())

	# Clear previous terrain
	for mi in terrain_instances:
		mi.queue_free()
	terrain_instances.clear()
	terrain_mesh_data.clear()
	heightmap_data = null

	# Load terrain CityGML
	terrain_city_model = PLATEAUCityModel.new()
	if not terrain_city_model.load(path):
		_log("[color=red]ERROR: Failed to load terrain GML[/color]")
		return

	# Extract terrain meshes
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.min_lod = 0
	options.max_lod = 2
	options.mesh_granularity = 2  # Area (merged terrain)
	options.export_appearance = false

	var root_meshes = terrain_city_model.extract_meshes(options)
	terrain_mesh_data = _flatten_mesh_data(root_meshes)
	_log("Terrain meshes: " + str(terrain_mesh_data.size()) + " (from " + str(root_meshes.size()) + " root nodes)")

	if terrain_mesh_data.is_empty():
		_log("[color=yellow]WARNING: No terrain meshes found[/color]")
		return

	# Create visual representation
	var bounds_min = Vector3.INF
	var bounds_max = -Vector3.INF

	for mesh_data in terrain_mesh_data:
		var mi = MeshInstance3D.new()
		mi.mesh = mesh_data.get_mesh()
		mi.transform = mesh_data.get_transform()
		mi.name = "Terrain_" + mesh_data.get_name()

		# Green material for terrain
		var material = StandardMaterial3D.new()
		material.albedo_color = Color(0.3, 0.6, 0.3)
		material.roughness = 0.9
		mi.material_override = material

		add_child(mi)
		terrain_instances.append(mi)

		var aabb = mi.get_aabb()
		var global_aabb = mi.transform * aabb
		bounds_min = bounds_min.min(global_aabb.position)
		bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	_position_camera(bounds_min, bounds_max)
	_log("[color=green]Terrain loaded successfully[/color]")


func _on_generate_heightmap() -> void:
	if terrain_mesh_data.is_empty():
		_log("[color=red]ERROR: Load terrain first[/color]")
		return

	_log("--- Generating Heightmap ---")

	var terrain = PLATEAUTerrain.new()
	terrain.texture_width = $UI/TerrainPanel/WidthSpin.value
	terrain.texture_height = $UI/TerrainPanel/HeightSpin.value
	terrain.fill_edges = true
	terrain.apply_blur_filter = true

	_log("Size: " + str(terrain.texture_width) + "x" + str(terrain.texture_height))

	var start_time = Time.get_ticks_msec()

	# Convert to TypedArray
	var typed_array: Array[PLATEAUMeshData] = []
	for md in terrain_mesh_data:
		typed_array.append(md)

	heightmap_data = terrain.generate_from_meshes(typed_array)

	var gen_time = Time.get_ticks_msec() - start_time
	_log("Generation time: " + str(gen_time) + " ms")

	if heightmap_data == null:
		_log("[color=red]ERROR: Failed to generate heightmap[/color]")
		return

	var min_bounds = heightmap_data.get_min_bounds()
	var max_bounds = heightmap_data.get_max_bounds()
	_log("Min height: " + str(min_bounds.y))
	_log("Max height: " + str(max_bounds.y))
	_log("Bounds: " + str(min_bounds) + " - " + str(max_bounds))

	# Create preview image from heightmap raw data
	var raw_data = heightmap_data.get_heightmap_raw()
	if not raw_data.is_empty():
		var w = heightmap_data.get_width()
		var h = heightmap_data.get_height()
		var image = Image.create(w, h, false, Image.FORMAT_L8)
		# Convert uint16 heightmap to 8-bit grayscale
		for y in range(h):
			for x in range(w):
				var idx = (y * w + x) * 2  # uint16 = 2 bytes
				if idx + 1 < raw_data.size():
					var val = raw_data[idx] | (raw_data[idx + 1] << 8)
					var normalized = clampf(float(val) / 65535.0, 0.0, 1.0)
					image.set_pixel(x, y, Color(normalized, normalized, normalized))
		var texture = ImageTexture.create_from_image(image)
		heightmap_preview.texture = texture
		_log("[color=green]Heightmap generated (" + str(w) + "x" + str(h) + ")[/color]")
	else:
		_log("[color=yellow]WARNING: Heightmap data is empty[/color]")

	# Setup aligner
	aligner = PLATEAUHeightMapAligner.new()
	aligner.add_heightmap(heightmap_data)
	_log("Aligner ready for building alignment")


func _on_save_heightmap() -> void:
	if heightmap_data == null:
		_log("[color=red]ERROR: Generate heightmap first[/color]")
		return

	var dialog = FileDialog.new()
	dialog.file_mode = FileDialog.FILE_MODE_SAVE_FILE
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.filters = ["*.png ; PNG Image"]
	dialog.current_file = "heightmap.png"
	dialog.file_selected.connect(_do_save_heightmap)
	add_child(dialog)
	dialog.popup_centered(Vector2i(800, 600))


func _do_save_heightmap(path: String) -> void:
	if heightmap_data.save_png(path):
		_log("[color=green]Heightmap saved: " + path + "[/color]")
	else:
		_log("[color=red]ERROR: Failed to save heightmap[/color]")


func _on_load_building_pressed() -> void:
	if building_gml_path.is_empty():
		var dialog = FileDialog.new()
		dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
		dialog.access = FileDialog.ACCESS_FILESYSTEM
		dialog.filters = ["*.gml ; CityGML Files"]
		dialog.file_selected.connect(_on_building_file_selected)
		add_child(dialog)
		dialog.popup_centered(Vector2i(800, 600))
	else:
		_load_buildings(building_gml_path)


func _on_building_file_selected(path: String) -> void:
	building_gml_path = path
	_load_buildings(path)


func _load_buildings(path: String) -> void:
	_log("--- Loading Buildings ---")
	_log("File: " + path.get_file())

	# Clear previous buildings
	for mi in building_instances:
		mi.queue_free()
	building_instances.clear()
	building_mesh_data.clear()

	# Load building CityGML
	building_city_model = PLATEAUCityModel.new()
	if not building_city_model.load(path):
		_log("[color=red]ERROR: Failed to load building GML[/color]")
		return

	# Extract building meshes
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.min_lod = 1
	options.max_lod = 2
	options.mesh_granularity = 1  # Primary (per building)
	options.export_appearance = true

	building_mesh_data = Array(building_city_model.extract_meshes(options))
	_log("Building meshes: " + str(building_mesh_data.size()))

	if building_mesh_data.is_empty():
		_log("[color=yellow]WARNING: No building meshes found[/color]")
		return

	_create_building_instances()
	_log("[color=green]Buildings loaded successfully[/color]")


func _create_building_instances() -> void:
	for mi in building_instances:
		mi.queue_free()
	building_instances.clear()

	# Flatten the mesh data to include children (buildings are in child nodes)
	var flattened = _flatten_mesh_data(building_mesh_data)
	for mesh_data in flattened:
		var mesh = mesh_data.get_mesh()
		if mesh == null:
			continue
		var mi = MeshInstance3D.new()
		mi.mesh = mesh
		mi.transform = mesh_data.get_transform()
		mi.name = "Building_" + mesh_data.get_name()

		add_child(mi)
		building_instances.append(mi)


func _on_align_buildings() -> void:
	if building_mesh_data.is_empty():
		_log("[color=red]ERROR: Load buildings first[/color]")
		return

	if aligner == null:
		_log("[color=red]ERROR: Generate heightmap first[/color]")
		return

	_log("--- Aligning Buildings ---")

	aligner.height_offset = $UI/BuildingPanel/OffsetSpin.value
	_log("Height offset: " + str(aligner.height_offset))

	var start_time = Time.get_ticks_msec()

	# Convert to TypedArray
	var typed_array: Array[PLATEAUMeshData] = []
	for md in building_mesh_data:
		typed_array.append(md)

	var aligned = aligner.align(typed_array)

	var align_time = Time.get_ticks_msec() - start_time
	_log("Align time: " + str(align_time) + " ms")
	_log("Aligned meshes: " + str(aligned.size()))

	# Update building mesh data and instances
	building_mesh_data = Array(aligned)
	_create_building_instances()

	# Update camera to show buildings
	if not building_instances.is_empty():
		var bounds_min = Vector3.INF
		var bounds_max = -Vector3.INF
		for mi in building_instances:
			var aabb = mi.get_aabb()
			var global_aabb_pos = mi.global_transform * aabb.position
			var global_aabb_end = mi.global_transform * (aabb.position + aabb.size)
			bounds_min = bounds_min.min(global_aabb_pos)
			bounds_max = bounds_max.max(global_aabb_end)
		_position_camera(bounds_min, bounds_max)

	_log("[color=green]Buildings aligned to terrain[/color]")


func _position_camera(bounds_min: Vector3, bounds_max: Vector3) -> void:
	var center = (bounds_min + bounds_max) / 2
	var size = (bounds_max - bounds_min).length()
	if size < 0.001:
		size = 100.0  # Default size if bounds are too small
	camera.position = center + Vector3(0, size * 0.5, size * 0.8)
	if not camera.position.is_equal_approx(center):
		camera.look_at(center)


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


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
