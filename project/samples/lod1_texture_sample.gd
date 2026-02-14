extends Node3D
## LOD1 Texture Projection Sample - LOD2テクスチャをLOD1に投影
##
## このサンプルで学べること:
## - LOD2モデルの5方向正射影キャプチャ
## - トライプラナーシェーダによるLOD1テクスチャ投影
## - SubViewportを使った動的テクスチャ生成
##
## 仕組み:
## 1. GMLファイルからLOD1とLOD2を抽出
## 2. LOD2モデルをSubViewportで5方向(前後左右上)からキャプチャ
## 3. キャプチャしたテクスチャをトライプラナーシェーダでLOD1に投影
##
## 使い方:
## 1. LOD2を含むGMLファイルをロード
## 2. 「Capture & Apply」でLOD2テクスチャをLOD1に投影
## 3. 表示モードを切り替えて結果を確認

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9
@export var capture_resolution: int = 1024
@export var blend_sharpness: float = 4.0

const TRIPLANAR_SHADER_PATH = "res://addons/plateau/shaders/plateau_lod1_triplanar.gdshader"
const CAPTURE_MARGIN = 1.01

var city_model: PLATEAUCityModel
var city_model_root: PLATEAUInstancedCityModel
var lod1_nodes: Array[Node3D] = []
var lod2_nodes: Array[Node3D] = []
var captured_material: ShaderMaterial
var lod1_material_applied := false

enum ViewMode { LOD2_ORIGINAL, LOD1_TEXTURED, LOD1_ORIGINAL }
var current_view: ViewMode = ViewMode.LOD2_ORIGINAL

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var status_label: Label = $UI/StatusPanel/StatusLabel


func _ready() -> void:
	city_model = PLATEAUCityModel.new()

	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/ControlPanel/CaptureButton.pressed.connect(_on_capture_pressed)
	$UI/ControlPanel/LOD2Button.pressed.connect(func(): _set_view(ViewMode.LOD2_ORIGINAL))
	$UI/ControlPanel/LOD1TexturedButton.pressed.connect(func(): _set_view(ViewMode.LOD1_TEXTURED))
	$UI/ControlPanel/LOD1OriginalButton.pressed.connect(func(): _set_view(ViewMode.LOD1_ORIGINAL))

	$UI/ControlPanel/CaptureButton.disabled = true
	$UI/ControlPanel/LOD1TexturedButton.disabled = true

	_log("LOD1 Texture Projection Sample ready.")
	_log("")
	_log("This sample captures LOD2 textures and projects")
	_log("them onto LOD1 using a triplanar shader.")
	_log("")
	_log("1. Import a GML file with LOD1 & LOD2 data")
	_log("2. Click 'Capture & Apply'")
	_log("3. Switch views to compare results")


func _on_import_pressed() -> void:
	if gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _import_gml)
	else:
		_import_gml(gml_path)


func _import_gml(path: String) -> void:
	gml_path = path
	_log("--- Loading GML ---")
	_log("File: " + path.get_file())

	# Clear previous
	if city_model_root != null:
		city_model_root.queue_free()
		city_model_root = null
	lod1_nodes.clear()
	lod2_nodes.clear()
	captured_material = null
	lod1_material_applied = false

	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	# Extract with both LOD1 and LOD2
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 1  # Primary
	options.export_appearance = true
	options.min_lod = 1
	options.max_lod = 2
	options.highest_lod_only = false  # Include both LOD1 and LOD2

	var mesh_data = city_model.extract_meshes(options)
	_log("Extracted root nodes: " + str(mesh_data.size()))

	var geo_ref = PLATEAUGeoReference.new()
	geo_ref.zone_id = zone_id
	geo_ref.reference_point = options.reference_point

	var importer = PLATEAUImporter.new()
	var typed_data: Array[PLATEAUMeshData] = []
	for md in mesh_data:
		typed_data.append(md)

	city_model_root = importer.import_to_scene(
		typed_data,
		path.get_file().get_basename(),
		geo_ref,
		options,
		path
	)

	if city_model_root == null:
		_log("[color=red]ERROR: Failed to create scene[/color]")
		return

	add_child(city_model_root)

	# Collect LOD nodes
	_collect_lod_nodes(city_model_root)

	_log("LOD1 nodes: " + str(lod1_nodes.size()))
	_log("LOD2 nodes: " + str(lod2_nodes.size()))

	if lod2_nodes.is_empty():
		_log("[color=yellow]WARNING: No LOD2 data found. Need LOD2 for capture.[/color]")
	if lod1_nodes.is_empty():
		_log("[color=yellow]WARNING: No LOD1 data found. Need LOD1 for projection target.[/color]")

	# Fit camera
	var visible = lod2_nodes if not lod2_nodes.is_empty() else lod1_nodes
	var aabb := _calculate_combined_aabb(visible)
	if aabb.size.length() > 0.001:
		PLATEAUUtils.fit_camera_to_bounds(camera, aabb.position, aabb.position + aabb.size)

	# Enable capture button
	$UI/ControlPanel/CaptureButton.disabled = lod1_nodes.is_empty() or lod2_nodes.is_empty()

	# Show LOD2 by default
	_set_view(ViewMode.LOD2_ORIGINAL)

	_log("[color=green]Load complete![/color]")


func _collect_lod_nodes(node: Node) -> void:
	for child in node.get_children():
		if child is Node3D:
			var lod = PLATEAUUtils.parse_lod_from_name(child.name)
			if lod == 1:
				lod1_nodes.append(child)
			elif lod == 2:
				lod2_nodes.append(child)
		_collect_lod_nodes(child)


func _on_capture_pressed() -> void:
	if lod1_nodes.is_empty() or lod2_nodes.is_empty():
		_log("[color=red]ERROR: Need both LOD1 and LOD2 data[/color]")
		return

	$UI/ControlPanel/CaptureButton.disabled = true
	status_label.text = "Capturing..."
	_log("--- Capturing LOD2 Textures ---")
	_log("Resolution: " + str(capture_resolution) + "x" + str(capture_resolution))

	# Show LOD2 for capture, hide LOD1
	_show_lods(false, true)
	await get_tree().process_frame

	# Calculate AABB of all LOD2 nodes
	var aabb := _calculate_combined_aabb(lod2_nodes)
	_log("AABB: pos=" + str(aabb.position.snapped(Vector3.ONE * 0.1))
		+ " size=" + str(aabb.size.snapped(Vector3.ONE * 0.1)))

	# Capture 5 directions
	var dir_names := ["Front (+Z)", "Back (-Z)", "Left (-X)", "Right (+X)", "Top (+Y)"]
	var textures: Array[ImageTexture] = []

	for dir_idx in range(5):
		_log("  Capturing: " + dir_names[dir_idx])
		status_label.text = "Capturing: " + dir_names[dir_idx]
		var tex := await _capture_direction(dir_idx, aabb)
		textures.append(tex)

	# Create ShaderMaterial
	var shader = load(TRIPLANAR_SHADER_PATH) as Shader
	if shader == null:
		_log("[color=red]ERROR: Failed to load shader: " + TRIPLANAR_SHADER_PATH + "[/color]")
		$UI/ControlPanel/CaptureButton.disabled = false
		return

	captured_material = ShaderMaterial.new()
	captured_material.shader = shader
	captured_material.set_shader_parameter("texture_front", textures[0])
	captured_material.set_shader_parameter("texture_back", textures[1])
	captured_material.set_shader_parameter("texture_left", textures[2])
	captured_material.set_shader_parameter("texture_right", textures[3])
	captured_material.set_shader_parameter("texture_top", textures[4])
	captured_material.set_shader_parameter("aabb_position", aabb.position)
	captured_material.set_shader_parameter("aabb_size", aabb.size)
	captured_material.set_shader_parameter("blend_sharpness", blend_sharpness)

	# Apply to LOD1
	for n in lod1_nodes:
		_apply_material_recursive(n)
	lod1_material_applied = true

	$UI/ControlPanel/CaptureButton.disabled = false
	$UI/ControlPanel/LOD1TexturedButton.disabled = false
	status_label.text = ""

	# Switch to textured LOD1 view
	_set_view(ViewMode.LOD1_TEXTURED)

	_log("[color=green]Capture complete! Textures applied to LOD1.[/color]")


func _capture_direction(dir_idx: int, aabb: AABB) -> ImageTexture:
	var viewport := SubViewport.new()
	viewport.size = Vector2i(capture_resolution, capture_resolution)
	viewport.transparent_bg = false
	viewport.render_target_update_mode = SubViewport.UPDATE_ONCE
	viewport.own_world_3d = true

	var cam := Camera3D.new()
	cam.projection = Camera3D.PROJECTION_ORTHOGONAL
	viewport.add_child(cam)

	var light := DirectionalLight3D.new()
	light.shadow_enabled = false
	viewport.add_child(light)

	# Clone LOD2 nodes into viewport
	for lod2 in lod2_nodes:
		var clone := lod2.duplicate()
		viewport.add_child(clone)
		clone.global_transform = lod2.global_transform
		clone.visible = true

	# Setup camera for direction
	var center := aabb.get_center()
	var half := aabb.size / 2.0
	var far_dist := aabb.size.length()

	match dir_idx:
		0:  # Front (+Z): right=+X, up=+Y
			cam.position = center + Vector3(0, 0, half.z + far_dist)
			cam.look_at(center)
			cam.size = maxf(aabb.size.x, aabb.size.y) * CAPTURE_MARGIN
			light.rotation_degrees = Vector3(-30, 0, 0)
		1:  # Back (-Z): right=-X, up=+Y
			cam.position = center - Vector3(0, 0, half.z + far_dist)
			cam.look_at(center)
			cam.size = maxf(aabb.size.x, aabb.size.y) * CAPTURE_MARGIN
			light.rotation_degrees = Vector3(-30, 180, 0)
		2:  # Left (-X): right=+Z, up=+Y
			cam.position = center - Vector3(half.x + far_dist, 0, 0)
			cam.look_at(center)
			cam.size = maxf(aabb.size.z, aabb.size.y) * CAPTURE_MARGIN
			light.rotation_degrees = Vector3(-30, 90, 0)
		3:  # Right (+X): right=-Z, up=+Y
			cam.position = center + Vector3(half.x + far_dist, 0, 0)
			cam.look_at(center)
			cam.size = maxf(aabb.size.z, aabb.size.y) * CAPTURE_MARGIN
			light.rotation_degrees = Vector3(-30, -90, 0)
		4:  # Top (+Y): right=+X, up=+Z
			cam.position = center + Vector3(0, half.y + far_dist, 0)
			cam.rotation_degrees = Vector3(-90, 0, 0)
			cam.size = maxf(aabb.size.x, aabb.size.z) * CAPTURE_MARGIN
			light.rotation_degrees = Vector3(-90, 0, 0)

	cam.far = far_dist * 3.0
	cam.near = 0.1

	add_child(viewport)

	# Wait for render
	await get_tree().process_frame
	await get_tree().process_frame

	# Get texture
	var image := viewport.get_texture().get_image()
	var tex := ImageTexture.create_from_image(image)

	viewport.queue_free()

	return tex


func _apply_material_recursive(node: Node) -> void:
	if node is MeshInstance3D and node.mesh != null:
		for surf_idx in range(node.mesh.get_surface_count()):
			node.set_surface_override_material(surf_idx, captured_material)
	for child in node.get_children():
		_apply_material_recursive(child)


func _clear_material_overrides(node: Node) -> void:
	if node is MeshInstance3D:
		for surf_idx in range(node.mesh.get_surface_count() if node.mesh else 0):
			node.set_surface_override_material(surf_idx, null)
	for child in node.get_children():
		_clear_material_overrides(child)


func _set_view(mode: ViewMode) -> void:
	current_view = mode
	match mode:
		ViewMode.LOD2_ORIGINAL:
			_show_lods(false, true)
		ViewMode.LOD1_TEXTURED:
			if captured_material != null and not lod1_material_applied:
				for n in lod1_nodes:
					_apply_material_recursive(n)
				lod1_material_applied = true
			_show_lods(true, false)
		ViewMode.LOD1_ORIGINAL:
			if lod1_material_applied:
				for n in lod1_nodes:
					_clear_material_overrides(n)
				lod1_material_applied = false
			_show_lods(true, false)


func _show_lods(show_lod1: bool, show_lod2: bool) -> void:
	for n in lod1_nodes:
		n.visible = show_lod1
	for n in lod2_nodes:
		n.visible = show_lod2


func _calculate_combined_aabb(nodes: Array[Node3D]) -> AABB:
	var result := AABB()
	var initialized := false

	for node in nodes:
		var node_aabb := _get_aabb_recursive(node)
		if node_aabb.size.length() < 0.001:
			continue
		if not initialized:
			result = node_aabb
			initialized = true
		else:
			result = result.merge(node_aabb)

	return result


func _get_aabb_recursive(node: Node) -> AABB:
	var result := AABB()
	var initialized := false

	if node is MeshInstance3D and node.mesh != null:
		result = node.global_transform * node.mesh.get_aabb()
		initialized = true

	for child in node.get_children():
		var child_aabb := _get_aabb_recursive(child)
		if child_aabb.size.length() < 0.001:
			continue
		if not initialized:
			result = child_aabb
			initialized = true
		else:
			result = result.merge(child_aabb)

	return result


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
