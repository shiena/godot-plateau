extends Node3D
## Flood Sample - 浸水シミュレーション可視化
##
## このサンプルで学べること:
## - 浸水データ (fld) のロードと表示
## - 浸水深に応じた色分け
## - 建物データとの重ね合わせ
##
## 浸水データについて:
## - PLATEAUでは洪水浸水想定区域データが提供されています
## - fld/ フォルダに格納されています
## - 浸水深の属性を持つメッシュデータです
##
## 使い方:
## 1. 建物GMLをロード (オプション)
## 2. 浸水GMLをロード
## 3. 浸水深に応じた色分けで表示

@export_file("*.gml") var building_gml_path: String = ""
@export_file("*.gml") var flood_gml_path: String = ""
@export var zone_id: int = 9

var city_model: PLATEAUCityModel

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var building_container: Node3D = $BuildingContainer
@onready var flood_container: Node3D = $FloodContainer
@onready var legend_container: VBoxContainer = $UI/LegendPanel/LegendContainer

# 浸水深の色分け
var depth_colors: Array[Dictionary] = [
	{"min": 0.0, "max": 0.5, "color": Color(0.7, 0.85, 1.0, 0.5), "label": "0-0.5m"},
	{"min": 0.5, "max": 1.0, "color": Color(0.5, 0.7, 1.0, 0.6), "label": "0.5-1m"},
	{"min": 1.0, "max": 2.0, "color": Color(0.3, 0.5, 1.0, 0.7), "label": "1-2m"},
	{"min": 2.0, "max": 5.0, "color": Color(0.1, 0.3, 0.9, 0.8), "label": "2-5m"},
	{"min": 5.0, "max": 999.0, "color": Color(0.0, 0.1, 0.6, 0.9), "label": "5m+"},
]


func _ready() -> void:
	city_model = PLATEAUCityModel.new()

	$UI/ImportPanel/LoadBuildingButton.pressed.connect(_on_load_building_pressed)
	$UI/ImportPanel/LoadFloodButton.pressed.connect(_on_load_flood_pressed)
	$UI/ImportPanel/ClearButton.pressed.connect(_clear_all)
	$UI/ImportPanel/FloodOpacitySlider.value_changed.connect(_on_opacity_changed)

	_setup_legend()

	_log("Flood Sample ready.")
	_log("")
	_log("This sample demonstrates flood visualization.")
	_log("")
	_log("Steps:")
	_log("1. (Optional) Load building GML for context")
	_log("2. Load flood GML (fld/*.gml)")
	_log("3. Flood areas are colored by depth")


func _setup_legend() -> void:
	# 凡例を作成
	for depth_info in depth_colors:
		var hbox = HBoxContainer.new()

		var color_rect = ColorRect.new()
		color_rect.custom_minimum_size = Vector2(20, 20)
		color_rect.color = depth_info["color"]
		hbox.add_child(color_rect)

		var label = Label.new()
		label.text = "  " + depth_info["label"]
		hbox.add_child(label)

		legend_container.add_child(hbox)


func _on_load_building_pressed() -> void:
	if building_gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _load_building)
	else:
		_load_building(building_gml_path)


func _load_building(path: String) -> void:
	building_gml_path = path
	_log("--- Loading Building GML ---")
	_log("File: " + path.get_file())

	# 建物をクリア
	for child in building_container.get_children():
		child.queue_free()

	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 2  # Area (建物を結合して軽量化)
	options.export_appearance = true

	var mesh_data = city_model.extract_meshes(options)

	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	_create_meshes(mesh_data, building_container, null, bounds_min, bounds_max)

	PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)
	_log("[color=green]Building loaded![/color]")


func _on_load_flood_pressed() -> void:
	if flood_gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _load_flood)
	else:
		_load_flood(flood_gml_path)


func _load_flood(path: String) -> void:
	flood_gml_path = path
	_log("--- Loading Flood GML ---")
	_log("File: " + path.get_file())

	# 浸水データをクリア
	for child in flood_container.get_children():
		child.queue_free()

	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 1  # Primary
	options.export_appearance = false  # 色は自分で設定

	var mesh_data = city_model.extract_meshes(options)

	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	# 浸水メッシュを作成 (半透明で表示)
	var flood_count = 0
	for root in mesh_data:
		flood_count += _create_flood_meshes(root, flood_container, bounds_min, bounds_max)

	_log("Flood meshes: " + str(flood_count))

	if building_container.get_child_count() == 0:
		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	_log("[color=green]Flood data loaded![/color]")


func _create_meshes(mesh_data_array: Array, parent: Node3D, material: Material, bounds_min: Vector3, bounds_max: Vector3) -> void:
	for root in mesh_data_array:
		_create_mesh_recursive(root, parent, material, bounds_min, bounds_max)


func _create_mesh_recursive(mesh_data: PLATEAUMeshData, parent: Node3D, material: Material, bounds_min: Vector3, bounds_max: Vector3) -> void:
	var mesh = mesh_data.get_mesh()
	if mesh != null:
		var instance = MeshInstance3D.new()
		instance.name = mesh_data.get_name()
		instance.mesh = mesh
		instance.transform = mesh_data.get_transform()

		if material != null:
			for i in range(mesh.get_surface_count()):
				instance.set_surface_override_material(i, material)

		parent.add_child(instance)

		var aabb = mesh.get_aabb()
		bounds_min = bounds_min.min(instance.transform * aabb.position)
		bounds_max = bounds_max.max(instance.transform * (aabb.position + aabb.size))

	for child in mesh_data.get_children():
		_create_mesh_recursive(child, parent, material, bounds_min, bounds_max)


func _create_flood_meshes(mesh_data: PLATEAUMeshData, parent: Node3D, bounds_min: Vector3, bounds_max: Vector3) -> int:
	var count = 0
	var mesh = mesh_data.get_mesh()

	if mesh != null:
		var instance = MeshInstance3D.new()
		instance.name = mesh_data.get_name()
		instance.mesh = mesh
		instance.transform = mesh_data.get_transform()

		# 浸水深を推定 (名前やY座標から)
		# 実際のデータでは属性から取得
		var depth = _estimate_depth(mesh_data)
		var material = _get_depth_material(depth)

		for i in range(mesh.get_surface_count()):
			instance.set_surface_override_material(i, material)

		# 少し上に持ち上げて建物と重なりやすくする
		instance.position.y += 0.1

		parent.add_child(instance)
		count += 1

		var aabb = mesh.get_aabb()
		bounds_min = bounds_min.min(instance.transform * aabb.position)
		bounds_max = bounds_max.max(instance.transform * (aabb.position + aabb.size))

	for child in mesh_data.get_children():
		count += _create_flood_meshes(child, parent, bounds_min, bounds_max)

	return count


func _estimate_depth(mesh_data: PLATEAUMeshData) -> float:
	# 名前から浸水深を推定 (例: "fld_depth_2m")
	var name = mesh_data.get_name().to_lower()

	if "5m" in name or "10m" in name:
		return 5.0
	elif "3m" in name or "4m" in name:
		return 3.0
	elif "2m" in name:
		return 2.0
	elif "1m" in name:
		return 1.0
	elif "0.5m" in name or "50cm" in name:
		return 0.5

	# デフォルト: 中程度の浸水
	return 1.5


func _get_depth_material(depth: float) -> StandardMaterial3D:
	var color = depth_colors[0]["color"]

	for depth_info in depth_colors:
		if depth >= depth_info["min"] and depth < depth_info["max"]:
			color = depth_info["color"]
			break

	var material = StandardMaterial3D.new()
	material.albedo_color = color
	material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	material.cull_mode = BaseMaterial3D.CULL_DISABLED  # 両面表示
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED

	return material


func _on_opacity_changed(value: float) -> void:
	# 浸水メッシュの透明度を変更
	for child in flood_container.get_children():
		if child is MeshInstance3D:
			var mat = child.get_surface_override_material(0)
			if mat is StandardMaterial3D:
				var color = mat.albedo_color
				color.a = value
				mat.albedo_color = color


func _clear_all() -> void:
	for child in building_container.get_children():
		child.queue_free()
	for child in flood_container.get_children():
		child.queue_free()
	_log("Cleared all data")


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
