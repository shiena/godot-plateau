extends Node3D
## Streaming Sample - カメラ位置に応じた段階的ロード
##
## このサンプルで学べること:
## - メッシュコードによる領域分割
## - カメラ位置に応じたGMLの動的ロード/アンロード
## - メモリ・RID使用量の最適化
##
## 仕組み:
## - データセットフォルダを指定
## - カメラ位置からメッシュコードを計算
## - 周辺のGMLファイルを自動ロード
## - 遠方のGMLファイルをアンロード
##
## 使い方:
## 1. PLATEAUデータセットフォルダを選択
## 2. カメラを移動すると周辺データが自動ロード

@export_dir var dataset_path: String = ""
@export var zone_id: int = 9
@export var load_radius: int = 1  ## メッシュコード単位でのロード半径

var dataset_source: PLATEAUDatasetSource
var loaded_tiles: Dictionary = {}  # {mesh_code: Node3D}
var current_mesh_code: String = ""
var shared_reference_point: Vector3
var geo_reference: PLATEAUGeoReference
var is_loading: bool = false

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var tile_container: Node3D = $TileContainer
@onready var status_label: Label = $UI/StatusPanel/StatusLabel
@onready var mesh_code_label: Label = $UI/StatusPanel/MeshCodeLabel
@onready var loaded_label: Label = $UI/StatusPanel/LoadedLabel


func _ready() -> void:
	$UI/ImportPanel/SelectFolderButton.pressed.connect(_on_select_folder_pressed)
	$UI/ImportPanel/RadiusSpin.value_changed.connect(func(v): load_radius = int(v))
	$UI/ImportPanel/ClearButton.pressed.connect(_clear_all)

	_log("Streaming Sample ready.")
	_log("")
	_log("This sample demonstrates progressive loading.")
	_log("Select a PLATEAU dataset folder to begin.")
	_log("")
	_log("As you move the camera, nearby tiles will be")
	_log("automatically loaded and distant tiles unloaded.")


func _process(_delta: float) -> void:
	if dataset_source == null or not dataset_source.is_valid():
		return

	if is_loading:
		return

	# カメラ位置からメッシュコードを計算
	var new_code = _get_mesh_code_at_camera()
	if new_code != current_mesh_code:
		current_mesh_code = new_code
		mesh_code_label.text = "Current: " + current_mesh_code
		_update_loaded_tiles()


func _on_select_folder_pressed() -> void:
	if dataset_path.is_empty():
		PLATEAUUtils.show_directory_dialog(self, _open_dataset)
	else:
		_open_dataset(dataset_path)


func _open_dataset(path: String) -> void:
	dataset_path = path
	_log("--- Opening Dataset ---")
	_log("Path: " + path)

	dataset_source = PLATEAUDatasetSource.create_local(path)
	if dataset_source == null or not dataset_source.is_valid():
		_log("[color=red]ERROR: Invalid dataset[/color]")
		return

	var mesh_codes = dataset_source.get_mesh_codes()
	_log("Available mesh codes: " + str(mesh_codes.size()))

	if mesh_codes.is_empty():
		_log("[color=yellow]No mesh codes found[/color]")
		return

	# 最初のメッシュコードを中心に設定
	var first_code = mesh_codes[0]
	var latlon = _mesh_code_to_latlon(first_code)

	if not latlon.is_empty():
		# GeoReferenceで座標変換
		geo_reference = PLATEAUGeoReference.new()
		geo_reference.zone_id = zone_id
		geo_reference.reference_point = Vector3.ZERO

		var pos = geo_reference.project(Vector3(latlon["lat"], latlon["lon"], 0))
		shared_reference_point = pos

		# カメラを最初の位置に移動
		camera.position = Vector3(0, 500, 500)
		camera.look_at(Vector3.ZERO)

	_log("[color=green]Dataset opened![/color]")
	_log("Move the camera to load nearby tiles.")

	# 最初のロード
	current_mesh_code = first_code
	mesh_code_label.text = "Current: " + current_mesh_code
	_update_loaded_tiles()


func _get_mesh_code_at_camera() -> String:
	# カメラ位置から最も近いメッシュコードを取得
	if dataset_source == null or geo_reference == null:
		return ""

	var mesh_codes = dataset_source.get_mesh_codes()
	if mesh_codes.is_empty():
		return ""

	var cam_pos = camera.global_position
	var best_code = mesh_codes[0]
	var best_dist = INF

	for code in mesh_codes:
		var latlon = _mesh_code_to_latlon(code)
		if latlon.is_empty():
			continue

		var local_pos = geo_reference.project(Vector3(latlon["lat"], latlon["lon"], 0))
		var dist = Vector2(cam_pos.x, cam_pos.z).distance_to(Vector2(local_pos.x, local_pos.z))
		if dist < best_dist:
			best_dist = dist
			best_code = code

	return best_code


func _update_loaded_tiles() -> void:
	if current_mesh_code.is_empty():
		return

	# 周辺のメッシュコードを計算
	var target_codes = _get_surrounding_codes(current_mesh_code, load_radius)

	# 不要なタイルをアンロード
	var codes_to_remove: Array = []
	for code in loaded_tiles.keys():
		if not code in target_codes:
			codes_to_remove.append(code)

	for code in codes_to_remove:
		_unload_tile(code)

	# 必要なタイルをロード
	for code in target_codes:
		if not loaded_tiles.has(code):
			await _load_tile(code)

	_update_status()


func _get_surrounding_codes(center_code: String, radius: int) -> Array:
	# 中心コードの周辺メッシュコードを取得
	var result: Array = [center_code]

	if center_code.length() < 8:
		return result

	# メッシュコードをパース
	var base = center_code.substr(0, 6)  # 2次メッシュまで
	var r = center_code.substr(6, 1).to_int()
	var c = center_code.substr(7, 1).to_int()

	for dr in range(-radius, radius + 1):
		for dc in range(-radius, radius + 1):
			var nr = r + dr
			var nc = c + dc
			if nr >= 0 and nr <= 9 and nc >= 0 and nc <= 9:
				var code = base + str(nr) + str(nc)
				if not code in result:
					result.append(code)

	return result


func _load_tile(mesh_code: String) -> void:
	if loaded_tiles.has(mesh_code):
		return

	# このメッシュコードのGMLファイルを検索
	var gml_files = dataset_source.get_gml_files(PLATEAUDatasetSource.PACKAGE_BUILDING)
	var target_file: PLATEAUGmlFileInfo = null

	for file_info in gml_files:
		if file_info.get_mesh_code() == mesh_code:
			target_file = file_info
			break

	if target_file == null:
		return

	is_loading = true
	status_label.text = "Loading: " + mesh_code

	_log("Loading tile: " + mesh_code)

	var city_model = PLATEAUCityModel.new()
	if not city_model.load(target_file.get_path()):
		_log("[color=yellow]Failed to load: " + mesh_code + "[/color]")
		is_loading = false
		return

	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = shared_reference_point
	options.mesh_granularity = 2  # Area (軽量化)
	options.export_appearance = true
	options.max_lod = 1  # LOD1まで (軽量化)

	var mesh_data = city_model.extract_meshes(options)

	# コンテナノードを作成
	var container = Node3D.new()
	container.name = "Tile_" + mesh_code

	for root in mesh_data:
		_create_mesh_instances(root, container)

	tile_container.add_child(container)
	loaded_tiles[mesh_code] = container

	is_loading = false
	status_label.text = ""
	await get_tree().process_frame


func _create_mesh_instances(mesh_data: PLATEAUMeshData, parent: Node3D) -> void:
	var mesh = mesh_data.get_mesh()
	if mesh != null:
		var instance = MeshInstance3D.new()
		instance.name = mesh_data.get_name()
		instance.mesh = mesh
		instance.transform = mesh_data.get_transform()
		parent.add_child(instance)

	for child in mesh_data.get_children():
		_create_mesh_instances(child, parent)


func _unload_tile(mesh_code: String) -> void:
	if not loaded_tiles.has(mesh_code):
		return

	_log("Unloading tile: " + mesh_code)

	var container = loaded_tiles[mesh_code]
	if is_instance_valid(container):
		container.queue_free()

	loaded_tiles.erase(mesh_code)


func _update_status() -> void:
	loaded_label.text = "Loaded: " + str(loaded_tiles.size()) + " tiles"

	var objects = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME)
	var video_mem = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_VIDEO_MEM_USED)
	_log("[color=cyan]RID: %d, VRAM: %.1fMB[/color]" % [objects, video_mem / 1048576.0])


func _clear_all() -> void:
	for code in loaded_tiles.keys():
		_unload_tile(code)
	loaded_tiles.clear()
	_log("Cleared all tiles")


func _mesh_code_to_latlon(mesh_code: String) -> Dictionary:
	if mesh_code.length() < 4:
		return {}

	var p = mesh_code.substr(0, 2).to_int()
	var u = mesh_code.substr(2, 2).to_int()

	var lat = p * 2.0 / 3.0
	var lon = u + 100.0

	if mesh_code.length() >= 6:
		var q = mesh_code.substr(4, 1).to_int()
		var v = mesh_code.substr(5, 1).to_int()
		lat += q * 5.0 / 60.0
		lon += v * 7.5 / 60.0

	if mesh_code.length() >= 8:
		var r = mesh_code.substr(6, 1).to_int()
		var w = mesh_code.substr(7, 1).to_int()
		lat += r * 30.0 / 3600.0 + 15.0 / 3600.0
		lon += w * 45.0 / 3600.0 + 22.5 / 3600.0

	return {"lat": lat, "lon": lon}


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
