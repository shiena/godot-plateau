extends Node3D
## CityObject Select Sample - 建物選択とハイライト
##
## このサンプルで学べること:
## - レイキャストによる建物の選択
## - PLATEAUMeshDataから属性情報を取得
## - 選択した建物のハイライト表示
##
## 使い方:
## 1. GMLファイルをロード
## 2. 建物をクリックして選択
## 3. 属性情報がログに表示される

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9

var city_model: PLATEAUCityModel
var mesh_data_map: Dictionary = {}  # MeshInstance3D -> PLATEAUMeshData
var selected_mesh: MeshInstance3D
var original_materials: Array = []  # 選択前のマテリアルを保存
var highlight_material: StandardMaterial3D

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var info_label: Label = $UI/InfoPanel/InfoLabel


func _ready() -> void:
	city_model = PLATEAUCityModel.new()
	city_model.load_completed.connect(_on_load_completed)
	city_model.extract_completed.connect(_on_extract_completed)

	# ハイライト用マテリアル作成
	highlight_material = StandardMaterial3D.new()
	highlight_material.albedo_color = Color(1.0, 0.5, 0.0, 1.0)  # オレンジ
	highlight_material.emission_enabled = true
	highlight_material.emission = Color(1.0, 0.5, 0.0)
	highlight_material.emission_energy_multiplier = 0.5

	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/ImportPanel/ClearSelectionButton.pressed.connect(_clear_selection)

	_log("CityObject Select Sample ready.")
	_log("Click 'Import GML' to load a CityGML file.")
	_log("Then click on buildings to select them.")


func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_raycast_select(event.position)


func _raycast_select(screen_pos: Vector2) -> void:
	if mesh_data_map.is_empty():
		return

	# レイキャスト用のパラメータを設定
	var from = camera.project_ray_origin(screen_pos)
	var to = from + camera.project_ray_normal(screen_pos) * 10000

	var space_state = get_world_3d().direct_space_state
	var query = PhysicsRayQueryParameters3D.create(from, to)
	var result = space_state.intersect_ray(query)

	if result.is_empty():
		# 何もヒットしなかった場合は選択解除
		_clear_selection()
		return

	# ヒットしたオブジェクトを取得
	var collider = result.collider
	if collider == null:
		return

	# StaticBody3Dの親からMeshInstance3Dを探す
	var mesh_instance: MeshInstance3D = null
	if collider is StaticBody3D:
		mesh_instance = collider.get_parent() as MeshInstance3D

	if mesh_instance == null or not mesh_data_map.has(mesh_instance):
		return

	_select_mesh(mesh_instance)


func _select_mesh(mesh_instance: MeshInstance3D) -> void:
	# 同じメッシュなら何もしない
	if mesh_instance == selected_mesh:
		return

	# 前の選択を解除
	_clear_selection()

	# 新しい選択を設定
	selected_mesh = mesh_instance

	# 元のマテリアルを保存してハイライト適用
	original_materials.clear()
	if mesh_instance.mesh != null:
		for i in range(mesh_instance.mesh.get_surface_count()):
			original_materials.append(mesh_instance.get_surface_override_material(i))
			mesh_instance.set_surface_override_material(i, highlight_material)

	_log("--- Selected: " + mesh_instance.name + " ---")

	# PLATEAUMeshDataから属性情報を取得して表示
	_display_attributes(mesh_instance)


func _display_attributes(mesh_instance: MeshInstance3D) -> void:
	if not mesh_data_map.has(mesh_instance):
		info_label.text = "Name: " + mesh_instance.name + "\n(No data)"
		return

	var mesh_data: PLATEAUMeshData = mesh_data_map[mesh_instance]

	# 基本情報
	var gml_id = mesh_data.get_gml_id()
	var type_name = mesh_data.get_city_object_type_name()
	var attributes = mesh_data.get_attributes()

	var info_text = "Name: " + mesh_instance.name + "\n"
	info_text += "GML ID: " + gml_id + "\n"
	info_text += "Type: " + type_name + "\n"

	_log("GML ID: " + gml_id)
	_log("Type: " + type_name)
	_log("Attributes: " + str(attributes.size()) + " items")

	# 属性を表示 (最初の5つ)
	var attr_count = 0
	for key in attributes.keys():
		var value = attributes[key]
		var value_str = _format_attribute_value(value)
		_log("  " + key + ": " + value_str)
		if attr_count < 5:
			info_text += key + ": " + value_str + "\n"
		attr_count += 1

	if attr_count > 5:
		info_text += "... and " + str(attr_count - 5) + " more"

	info_label.text = info_text


func _format_attribute_value(value: Variant) -> String:
	if value is Dictionary:
		return "(nested: " + str(value.size()) + " items)"
	elif value is float:
		return "%.2f" % value
	else:
		return str(value)


func _clear_selection() -> void:
	if selected_mesh != null and is_instance_valid(selected_mesh):
		# 元のマテリアルを復元
		if selected_mesh.mesh != null:
			for i in range(mini(original_materials.size(), selected_mesh.mesh.get_surface_count())):
				selected_mesh.set_surface_override_material(i, original_materials[i])
	selected_mesh = null
	original_materials.clear()
	info_label.text = "Click on a building to select"


func _on_import_pressed() -> void:
	if city_model.is_processing():
		_log("[color=yellow]Already processing...[/color]")
		return

	if gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _import_gml)
	else:
		_import_gml(gml_path)


func _import_gml(path: String) -> void:
	gml_path = path
	_log("--- Loading GML ---")
	_log("File: " + path.get_file())

	# 既存をクリア
	_clear_selection()
	for mesh_instance in mesh_data_map.keys():
		if is_instance_valid(mesh_instance):
			mesh_instance.queue_free()
	mesh_data_map.clear()

	# 非同期ロード開始
	city_model.load_async(path)


func _on_load_completed(success: bool) -> void:
	if not success:
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	_log("GML loaded, extracting meshes...")

	# メッシュ抽出オプション (Primary粒度で建物単位)
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 1  # Primary
	options.export_appearance = true

	city_model.extract_meshes_async(options)


func _on_extract_completed(mesh_data_array: Array) -> void:
	if mesh_data_array.is_empty():
		_log("[color=red]ERROR: No meshes extracted[/color]")
		return

	_log("Creating mesh instances...")

	# メッシュデータをフラット化
	var flat_data = PLATEAUUtils.flatten_mesh_data(mesh_data_array)

	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	for mesh_data in flat_data:
		var mesh = mesh_data.get_mesh()
		if mesh == null:
			continue

		var mesh_instance = MeshInstance3D.new()
		mesh_instance.mesh = mesh
		mesh_instance.transform = mesh_data.get_transform()
		mesh_instance.name = mesh_data.get_name()
		add_child(mesh_instance)

		# コリジョンを追加 (レイキャスト用)
		mesh_instance.create_trimesh_collision()

		# MeshInstance3D -> PLATEAUMeshData のマッピングを保存
		mesh_data_map[mesh_instance] = mesh_data

		# バウンディングボックス計算
		var aabb = mesh.get_aabb()
		var global_aabb = mesh_instance.transform * aabb
		bounds_min = bounds_min.min(global_aabb.position)
		bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	# カメラ調整
	if bounds_min != Vector3.INF:
		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	_log("[color=green]Load complete! " + str(mesh_data_map.size()) + " meshes.[/color]")
	_log("Click on buildings to select them.")


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
