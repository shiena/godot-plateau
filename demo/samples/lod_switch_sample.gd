extends Node3D
## LOD Switch Sample - カメラ距離によるLOD切り替え
##
## このサンプルで学べること:
## - LOD0/1/2/3の違いと使い分け
## - カメラ距離に応じた動的LOD切り替え
## - RID使用量の最適化
##
## 使い方:
## 1. GMLファイルをロード
## 2. カメラを移動してLODの切り替わりを確認
## 3. LOD閾値をスライダーで調整

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9

## LOD切り替え距離の閾値 (メートル)
@export var lod_thresholds: Array[float] = [100.0, 300.0, 800.0]

var city_model: PLATEAUCityModel
var city_model_root: PLATEAUInstancedCityModel
var lod_nodes: Dictionary = {}  # {lod_level: Array[Node3D]}
var current_lod: int = -1

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var lod_label: Label = $UI/LODPanel/CurrentLODLabel
@onready var distance_label: Label = $UI/LODPanel/DistanceLabel
@onready var threshold_slider: HSlider = $UI/LODPanel/ThresholdSlider
@onready var auto_switch_check: CheckBox = $UI/LODPanel/AutoSwitchCheck


func _ready() -> void:
	city_model = PLATEAUCityModel.new()

	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/LODPanel/LOD0Button.pressed.connect(func(): _set_lod(0))
	$UI/LODPanel/LOD1Button.pressed.connect(func(): _set_lod(1))
	$UI/LODPanel/LOD2Button.pressed.connect(func(): _set_lod(2))
	$UI/LODPanel/LOD3Button.pressed.connect(func(): _set_lod(3))
	threshold_slider.value_changed.connect(_on_threshold_changed)

	_log("LOD Switch Sample ready.")
	_log("Click 'Import GML' to load a CityGML file.")
	_log("")
	_log("LOD levels:")
	_log("  LOD0: 簡略化された形状 (箱)")
	_log("  LOD1: 基本的な形状")
	_log("  LOD2: 詳細な形状 + テクスチャ")
	_log("  LOD3: 最高詳細 (利用可能な場合)")


func _process(_delta: float) -> void:
	if city_model_root == null:
		return

	# カメラからモデル中心までの距離を計算
	var center = city_model_root.global_position
	var distance = camera.global_position.distance_to(center)
	distance_label.text = "Distance: %.1f m" % distance

	# 自動LOD切り替え
	if auto_switch_check.button_pressed:
		var target_lod = _get_lod_for_distance(distance)
		if target_lod != current_lod:
			_set_lod(target_lod)


func _get_lod_for_distance(distance: float) -> int:
	# 距離に応じたLODを決定
	# 近い: LOD2, 中間: LOD1, 遠い: LOD0
	var base_threshold = threshold_slider.value

	if distance < base_threshold:
		return 2  # 近距離: 最高詳細
	elif distance < base_threshold * 3:
		return 1  # 中距離
	else:
		return 0  # 遠距離: 簡略化


func _on_import_pressed() -> void:
	if gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _import_gml)
	else:
		_import_gml(gml_path)


func _import_gml(path: String) -> void:
	gml_path = path
	_log("--- Loading GML ---")
	_log("File: " + path.get_file())

	# 既存のモデルをクリア
	if city_model_root != null:
		city_model_root.queue_free()
		city_model_root = null
	lod_nodes.clear()

	# GMLロード
	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	# 全LODを含めて抽出 (LOD0-3)
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 1  # Primary
	options.export_appearance = true
	options.min_lod = 0
	options.max_lod = 3
	options.highest_lod_only = false  # 全LODを含める

	var mesh_data = city_model.extract_meshes(options)
	_log("Extracted root nodes: " + str(mesh_data.size()))

	# GeoReference作成
	var geo_ref = PLATEAUGeoReference.new()
	geo_ref.zone_id = zone_id
	geo_ref.reference_point = options.reference_point

	# PLATEAUImporterでシーン作成
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

	# LODノードを収集
	_collect_lod_nodes(city_model_root)

	# 利用可能なLODを表示
	var available_lods = lod_nodes.keys()
	available_lods.sort()
	_log("Available LODs: " + str(available_lods))

	for lod in available_lods:
		_log("  LOD%d: %d nodes" % [lod, lod_nodes[lod].size()])

	# カメラ調整
	_fit_camera()

	# 初期LODを設定
	_set_lod(2 if lod_nodes.has(2) else available_lods[-1])

	_log("[color=green]Load complete![/color]")


func _collect_lod_nodes(node: Node) -> void:
	# LODノードを再帰的に収集
	for child in node.get_children():
		if child is Node3D:
			var lod = PLATEAUUtils.parse_lod_from_name(child.name)
			if lod >= 0:
				if not lod_nodes.has(lod):
					lod_nodes[lod] = []
				lod_nodes[lod].append(child)
		_collect_lod_nodes(child)


func _set_lod(target_lod: int) -> void:
	if not lod_nodes.has(target_lod):
		# 指定LODがない場合、最も近いLODを探す
		var available = lod_nodes.keys()
		if available.is_empty():
			return
		available.sort()
		target_lod = available[-1]  # 最高LOD
		for lod in available:
			if lod >= target_lod:
				target_lod = lod
				break

	if target_lod == current_lod:
		return

	# 全LODを非表示にしてから、ターゲットLODのみ表示
	for lod in lod_nodes.keys():
		for node in lod_nodes[lod]:
			node.visible = (lod == target_lod)

	current_lod = target_lod
	lod_label.text = "Current LOD: %d" % current_lod
	_log("Switched to LOD%d" % current_lod)

	# RID使用量を表示
	_log_rid_usage()


func _fit_camera() -> void:
	if city_model_root == null:
		return

	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	for child in city_model_root.get_children():
		if child is Node3D:
			_calculate_bounds(child, bounds_min, bounds_max)

	PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)


func _calculate_bounds(node: Node3D, bounds_min: Vector3, bounds_max: Vector3) -> void:
	if node is MeshInstance3D and node.mesh != null:
		var aabb = node.mesh.get_aabb()
		var global_aabb = node.global_transform * aabb
		bounds_min = bounds_min.min(global_aabb.position)
		bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	for child in node.get_children():
		if child is Node3D:
			_calculate_bounds(child, bounds_min, bounds_max)


func _on_threshold_changed(value: float) -> void:
	_log("LOD threshold: %.0f m" % value)


func _log_rid_usage() -> void:
	var objects = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME)
	var primitives = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_TOTAL_PRIMITIVES_IN_FRAME)
	var video_mem = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_VIDEO_MEM_USED)
	_log("[color=cyan]RID: Objects=%d, Primitives=%d, VRAM=%.1fMB[/color]" % [
		objects, primitives, video_mem / 1048576.0
	])


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
