extends Node3D
## Multi Model Sample - 複数GMLの同時管理
##
## このサンプルで学べること:
## - 複数のGMLファイルを同時にロード
## - 各モデルの表示/非表示切り替え
## - 共通の座標系での配置
##
## 使い方:
## 1. 複数のGMLファイルを追加
## 2. チェックボックスで表示/非表示を切り替え
## 3. 個別に削除可能

@export var zone_id: int = 9

var loaded_models: Array[Dictionary] = []  # {path, root, checkbox}
var shared_reference_point: Vector3

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var model_container: Node3D = $ModelContainer
@onready var model_list: VBoxContainer = $UI/ModelPanel/ScrollContainer/ModelList


func _ready() -> void:
	$UI/ImportPanel/AddButton.pressed.connect(_on_add_pressed)
	$UI/ImportPanel/ClearAllButton.pressed.connect(_clear_all)
	$UI/ImportPanel/FitViewButton.pressed.connect(_fit_view)

	_log("Multi Model Sample ready.")
	_log("")
	_log("Add multiple GML files to view together.")
	_log("All models share the same coordinate system.")


func _on_add_pressed() -> void:
	PLATEAUUtils.show_gml_file_dialog(self, _add_model)


func _add_model(path: String) -> void:
	# 既にロード済みか確認
	for model in loaded_models:
		if model["path"] == path:
			_log("[color=yellow]Already loaded: " + path.get_file() + "[/color]")
			return

	_log("--- Adding: " + path.get_file() + " ---")

	var city_model = PLATEAUCityModel.new()
	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load[/color]")
		return

	# オプション設定
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.mesh_granularity = 1  # Primary
	options.export_appearance = true

	# 最初のモデルの場合、参照点を設定
	if loaded_models.is_empty():
		shared_reference_point = city_model.get_center_point(zone_id)
		_log("Reference point set: " + str(shared_reference_point))

	options.reference_point = shared_reference_point

	var mesh_data = city_model.extract_meshes(options)

	# GeoReference作成
	var geo_ref = PLATEAUGeoReference.new()
	geo_ref.zone_id = zone_id
	geo_ref.reference_point = shared_reference_point

	# PLATEAUImporterでシーン作成
	var importer = PLATEAUImporter.new()
	var typed_data: Array[PLATEAUMeshData] = []
	for md in mesh_data:
		typed_data.append(md)

	var root = importer.import_to_scene(
		typed_data,
		path.get_file().get_basename(),
		geo_ref,
		options,
		path
	)

	if root == null:
		_log("[color=red]ERROR: Failed to create scene[/color]")
		return

	model_container.add_child(root)

	# UI要素を作成
	var hbox = HBoxContainer.new()
	hbox.name = "Model_" + str(loaded_models.size())

	var checkbox = CheckBox.new()
	checkbox.button_pressed = true
	checkbox.text = path.get_file()
	checkbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	checkbox.toggled.connect(func(pressed): root.visible = pressed)
	hbox.add_child(checkbox)

	var remove_btn = Button.new()
	remove_btn.text = "X"
	remove_btn.custom_minimum_size = Vector2(30, 0)
	remove_btn.pressed.connect(func(): _remove_model(path))
	hbox.add_child(remove_btn)

	model_list.add_child(hbox)

	# モデル情報を保存
	loaded_models.append({
		"path": path,
		"root": root,
		"ui": hbox
	})

	_log("Meshes: " + str(_count_meshes(root)))
	_log("[color=green]Added successfully![/color]")

	_fit_view()


func _count_meshes(node: Node) -> int:
	var count = 0
	if node is MeshInstance3D:
		count = 1

	for child in node.get_children():
		count += _count_meshes(child)

	return count


func _remove_model(path: String) -> void:
	for i in range(loaded_models.size()):
		if loaded_models[i]["path"] == path:
			var model = loaded_models[i]

			# シーンから削除
			if is_instance_valid(model["root"]):
				model["root"].queue_free()

			# UIから削除
			if is_instance_valid(model["ui"]):
				model["ui"].queue_free()

			loaded_models.remove_at(i)
			_log("Removed: " + path.get_file())

			# 最後のモデルを削除した場合、参照点をリセット
			if loaded_models.is_empty():
				shared_reference_point = Vector3.ZERO

			return


func _clear_all() -> void:
	for model in loaded_models:
		if is_instance_valid(model["root"]):
			model["root"].queue_free()
		if is_instance_valid(model["ui"]):
			model["ui"].queue_free()

	loaded_models.clear()
	shared_reference_point = Vector3.ZERO
	_log("Cleared all models")


func _fit_view() -> void:
	if loaded_models.is_empty():
		return

	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	for model in loaded_models:
		if not is_instance_valid(model["root"]) or not model["root"].visible:
			continue

		_calculate_bounds(model["root"], bounds_min, bounds_max)

	if bounds_min != Vector3.INF:
		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)


func _calculate_bounds(node: Node, bounds_min: Vector3, bounds_max: Vector3) -> void:
	if node is MeshInstance3D and node.mesh != null:
		var aabb = node.mesh.get_aabb()
		var global_aabb = node.global_transform * aabb
		bounds_min = bounds_min.min(global_aabb.position)
		bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	for child in node.get_children():
		_calculate_bounds(child, bounds_min, bounds_max)


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
