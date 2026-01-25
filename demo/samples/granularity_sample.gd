extends Node3D
## Granularity Sample - メッシュ粒度の比較
##
## このサンプルで学べること:
## - Atomic/Primary/Area粒度の違い
## - 粒度によるメッシュ数・パフォーマンスへの影響
## - PLATEAUGranularityConverterによる粒度変換
##
## 粒度の説明:
## - Atomic: 最小単位 (壁、屋根などの部品単位)
## - Primary: 建物単位 (一般的な用途に最適)
## - Area: 地域全体を1つのメッシュに結合
##
## 使い方:
## 1. GMLファイルをロード
## 2. 粒度を選択して違いを確認

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9

var city_model: PLATEAUCityModel
var original_mesh_data: Array[PLATEAUMeshData] = []
var current_granularity: int = 1  # Primary

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var mesh_container: Node3D = $MeshContainer
@onready var stats_label: Label = $UI/StatsLabel
@onready var granularity_label: Label = $UI/GranularityPanel/CurrentLabel


func _ready() -> void:
	city_model = PLATEAUCityModel.new()

	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/GranularityPanel/AtomicButton.pressed.connect(func(): _set_granularity(0))
	$UI/GranularityPanel/PrimaryButton.pressed.connect(func(): _set_granularity(1))
	$UI/GranularityPanel/AreaButton.pressed.connect(func(): _set_granularity(2))

	_log("Granularity Sample ready.")
	_log("Click 'Import GML' to load a CityGML file.")
	_log("")
	_log("Granularity levels:")
	_log("  [color=cyan]Atomic[/color]: 最小単位 (部品ごと)")
	_log("  [color=cyan]Primary[/color]: 建物単位 (推奨)")
	_log("  [color=cyan]Area[/color]: 地域全体を結合")


func _on_import_pressed() -> void:
	if gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _import_gml)
	else:
		_import_gml(gml_path)


func _import_gml(path: String) -> void:
	gml_path = path
	_log("--- Loading GML ---")
	_log("File: " + path.get_file())

	# 既存をクリア
	_clear_meshes()

	# GMLロード
	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	# Atomic粒度で抽出 (後で変換可能)
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 0  # Atomic
	options.export_appearance = true

	var mesh_data = city_model.extract_meshes(options)

	# オリジナルデータを保存
	original_mesh_data.clear()
	for root in mesh_data:
		_flatten_mesh_data(root, original_mesh_data)

	_log("Original meshes (Atomic): " + str(original_mesh_data.size()))

	# Primaryで表示
	_set_granularity(1)

	_log("[color=green]Load complete![/color]")


func _flatten_mesh_data(mesh_data: PLATEAUMeshData, result: Array[PLATEAUMeshData]) -> void:
	if mesh_data.get_mesh() != null:
		result.append(mesh_data)

	for child in mesh_data.get_children():
		_flatten_mesh_data(child, result)


func _set_granularity(granularity: int) -> void:
	if original_mesh_data.is_empty():
		_log("[color=yellow]Import a GML file first[/color]")
		return

	_clear_meshes()

	var granularity_names = ["Atomic", "Primary", "Area"]
	_log("--- Converting to " + granularity_names[granularity] + " ---")

	var start_time = Time.get_ticks_msec()

	var display_data: Array[PLATEAUMeshData] = []

	if granularity == 0:
		# Atomicはそのまま
		display_data = original_mesh_data
	else:
		# 粒度変換
		var converter = PLATEAUGranularityConverter.new()
		var convert_target = PLATEAUGranularityConverter.CONVERT_GRANULARITY_PRIMARY if granularity == 1 else PLATEAUGranularityConverter.CONVERT_GRANULARITY_AREA
		display_data = converter.convert(original_mesh_data, convert_target)

	var convert_time = Time.get_ticks_msec() - start_time

	_log("Conversion time: " + str(convert_time) + " ms")
	_log("Result meshes: " + str(display_data.size()))

	# 表示
	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	for md in display_data:
		var mesh = md.get_mesh()
		if mesh != null:
			var instance = MeshInstance3D.new()
			instance.name = md.get_name()
			instance.mesh = mesh
			instance.transform = md.get_transform()
			mesh_container.add_child(instance)

			var aabb = mesh.get_aabb()
			bounds_min = bounds_min.min(instance.transform * aabb.position)
			bounds_max = bounds_max.max(instance.transform * (aabb.position + aabb.size))

	if current_granularity != granularity:
		PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)

	current_granularity = granularity
	granularity_label.text = "Current: " + granularity_names[granularity]
	_update_stats(display_data)


func _clear_meshes() -> void:
	for child in mesh_container.get_children():
		child.queue_free()


func _update_stats(mesh_data: Array[PLATEAUMeshData]) -> void:
	var total_vertices = 0
	var total_triangles = 0

	for md in mesh_data:
		var mesh = md.get_mesh()
		if mesh != null:
			for surface_idx in range(mesh.get_surface_count()):
				var arrays = mesh.surface_get_arrays(surface_idx)
				if arrays.size() > Mesh.ARRAY_VERTEX:
					var vertices = arrays[Mesh.ARRAY_VERTEX] as PackedVector3Array
					total_vertices += vertices.size()
				if arrays.size() > Mesh.ARRAY_INDEX:
					var indices = arrays[Mesh.ARRAY_INDEX] as PackedInt32Array
					@warning_ignore("integer_division")
					total_triangles += indices.size() / 3

	stats_label.text = "Meshes: %d | Vertices: %d | Triangles: %d" % [
		mesh_data.size(), total_vertices, total_triangles
	]

	# RID使用量
	var objects = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME)
	_log("[color=cyan]RID Objects: %d[/color]" % objects)


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
