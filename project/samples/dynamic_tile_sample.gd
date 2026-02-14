extends Node3D
## Dynamic Tile Sample - PLATEAUDynamicTileManager を使った動的タイルロード
##
## このサンプルで学べること:
## - PLATEAUDynamicTileMetaStore の生成
## - PLATEAUDynamicTileManager の初期化と設定
## - カメラ距離に応じた自動ロード/アンロード
## - ズームレベルごとのロード範囲設定
##
## 仕組み:
## 1. データセットフォルダを選択
## 2. GMLファイルからタイルシーン(.tscn)を自動生成
## 3. PLATEAUDynamicTileManager がカメラ位置に応じてタイルを自動ロード/アンロード
##
## 使い方:
## 1. PLATEAUデータセットフォルダを選択
## 2. 「Generate Tiles」でタイルシーンを生成
## 3. カメラを移動すると周辺タイルが自動ロード

@export_dir var dataset_path: String = ""
@export var zone_id: int = 9

## ズームレベルごとのロード範囲 (min_distance, max_distance)
@export var zoom11_range: Vector2 = Vector2(-10000, 500)
@export var zoom10_range: Vector2 = Vector2(500, 1500)
@export var zoom9_range: Vector2 = Vector2(1500, 10000)

var dataset_source: PLATEAUDatasetSource
var geo_reference: PLATEAUGeoReference
var tile_manager: PLATEAUDynamicTileManager
var tile_output_dir: String = "user://dynamic_tiles/"

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var status_label: Label = $UI/StatusPanel/StatusLabel
@onready var loaded_label: Label = $UI/StatusPanel/LoadedLabel
@onready var grid_code_label: Label = $UI/StatusPanel/GridCodeLabel


func _ready() -> void:
	$UI/ImportPanel/SelectFolderButton.pressed.connect(_on_select_folder_pressed)
	$UI/ImportPanel/GenerateButton.pressed.connect(_on_generate_pressed)
	$UI/ImportPanel/ClearButton.pressed.connect(_clear_all)

	# GenerateButton は初期状態で無効
	$UI/ImportPanel/GenerateButton.disabled = true

	_log("Dynamic Tile Sample ready.")
	_log("")
	_log("This sample demonstrates PLATEAUDynamicTileManager.")
	_log("1. Select a PLATEAU dataset folder")
	_log("2. Click 'Generate Tiles' to create tile scenes")
	_log("3. Move the camera to auto-load nearby tiles")


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

	var grid_codes = dataset_source.get_grid_codes()
	_log("Grid codes: " + str(grid_codes.size()))

	for gc in grid_codes:
		var extent = gc.get_extent()
		_log("  " + gc.get_code() + " (level " + str(gc.get_level()) + ")" +
			" lat=[" + str(snapped(extent.get("min_lat", 0), 0.001)) +
			", " + str(snapped(extent.get("max_lat", 0), 0.001)) + "]")

	var packages = dataset_source.get_available_packages()
	_log("Packages: " + str(packages))

	# GeoReferenceの設定
	if not grid_codes.is_empty():
		var first_gc: PLATEAUGridCode = grid_codes[0]
		var extent = first_gc.get_extent()
		if not extent.is_empty():
			geo_reference = PLATEAUGeoReference.new()
			geo_reference.zone_id = zone_id
			geo_reference.reference_point = Vector3.ZERO

			var center_lat = (extent["min_lat"] + extent["max_lat"]) / 2.0
			var center_lon = (extent["min_lon"] + extent["max_lon"]) / 2.0
			var center_pos = geo_reference.project(Vector3(center_lat, center_lon, 0))

			# カメラを中心に配置
			camera.position = Vector3(0, 500, 500)
			camera.look_at(Vector3.ZERO)
			_log("Center position: " + str(center_pos))

	$UI/ImportPanel/GenerateButton.disabled = false
	_log("[color=green]Dataset opened! Click 'Generate Tiles' to proceed.[/color]")


func _on_generate_pressed() -> void:
	if dataset_source == null or not dataset_source.is_valid():
		_log("[color=red]ERROR: No dataset loaded[/color]")
		return

	$UI/ImportPanel/GenerateButton.disabled = true
	status_label.text = "Generating tiles..."
	_log("--- Generating Tile Scenes ---")

	# タイルシーンをバッチ生成
	await _generate_tile_scenes()

	# DynamicTileManager を初期化
	_setup_tile_manager()


func _generate_tile_scenes() -> void:
	# 出力ディレクトリの確認
	DirAccess.make_dir_recursive_absolute(tile_output_dir)
	_log("Output dir: " + tile_output_dir)

	# 建物GMLを取得
	var gml_files = dataset_source.get_gml_files(PLATEAUDatasetSource.PACKAGE_BUILDING)
	_log("Building GML files: " + str(gml_files.size()))

	var tile_count := 0
	for file_info in gml_files:
		var mesh_code = file_info.get_mesh_code()
		_log("Processing: " + mesh_code + " (" + file_info.get_path() + ")")
		status_label.text = "Loading: " + mesh_code

		var city_model = PLATEAUCityModel.new()
		if not city_model.load(file_info.get_path()):
			_log("[color=yellow]  Skipped (load failed)[/color]")
			continue

		var options = PLATEAUMeshExtractOptions.new()
		options.coordinate_zone_id = zone_id
		options.reference_point = Vector3.ZERO
		options.mesh_granularity = 2  # Area
		options.export_appearance = true
		options.max_lod = 2

		var mesh_data = city_model.extract_meshes(options)
		if mesh_data.is_empty():
			_log("[color=yellow]  Skipped (no mesh)[/color]")
			continue

		# タイルシーンを作成して保存
		var tile_root = Node3D.new()
		tile_root.name = "Tile_" + mesh_code
		_create_mesh_instances(mesh_data, tile_root)

		var scene = PackedScene.new()
		scene.pack(tile_root)
		var save_path = tile_output_dir + "tile_zoom_11_grid_" + mesh_code + "_bldg.tscn"
		ResourceSaver.save(scene, save_path)
		tile_root.queue_free()

		tile_count += 1
		_log("  Saved: " + save_path)
		await get_tree().process_frame

	_log("[color=green]Generated " + str(tile_count) + " tile scenes[/color]")
	status_label.text = ""


func _create_mesh_instances(mesh_data_array: Array, parent: Node3D) -> void:
	for root in mesh_data_array:
		_add_mesh_recursive(root, parent)


func _add_mesh_recursive(mesh_data: PLATEAUMeshData, parent: Node3D) -> void:
	var mesh = mesh_data.get_mesh()
	if mesh != null:
		var instance = MeshInstance3D.new()
		instance.name = mesh_data.get_name()
		instance.mesh = mesh
		instance.transform = mesh_data.get_transform()
		parent.add_child(instance)
		instance.owner = parent

	for child in mesh_data.get_children():
		_add_mesh_recursive(child, parent)


func _setup_tile_manager() -> void:
	_log("--- Setting up DynamicTileManager ---")

	# メタデータストアを構築
	var meta_store = PLATEAUDynamicTileMetaStore.new()
	meta_store.reference_point = Vector3.ZERO

	# 生成したタイルシーンからメタデータを収集
	var dir = DirAccess.open(tile_output_dir)
	if dir == null:
		_log("[color=red]ERROR: Cannot open tile directory[/color]")
		return

	dir.list_dir_begin()
	var file_name = dir.get_next()
	while file_name != "":
		if file_name.ends_with(".tscn") and file_name.begins_with("tile_zoom_"):
			var parts = file_name.trim_suffix(".tscn").split("_")
			# tile_zoom_11_grid_MESHCODE_bldg
			if parts.size() >= 6:
				var zoom_level = int(parts[2])
				var mesh_code = parts[4]

				var gc = PLATEAUGridCode.parse(mesh_code)
				var extent_dict = gc.get_extent() if gc.is_valid() else {}

				var info = PLATEAUDynamicTileMetaInfo.new()
				info.address = tile_output_dir + file_name
				info.zoom_level = zoom_level
				info.group_name = "bldg"

				# 緯度経度範囲からAABBに変換
				if not extent_dict.is_empty() and geo_reference != null:
					var min_pos = geo_reference.project(Vector3(
						extent_dict["min_lat"], extent_dict["min_lon"], 0))
					var max_pos = geo_reference.project(Vector3(
						extent_dict["max_lat"], extent_dict["max_lon"], 0))
					var aabb_pos = Vector3(min(min_pos.x, max_pos.x), -100, min(min_pos.z, max_pos.z))
					var aabb_size = Vector3(abs(max_pos.x - min_pos.x), 1000, abs(max_pos.z - min_pos.z))
					info.extent = AABB(aabb_pos, aabb_size)

				meta_store.add_tile_meta_info(info)
				_log("  Registered: " + mesh_code + " zoom=" + str(zoom_level))

		file_name = dir.get_next()
	dir.list_dir_end()

	_log("Total tiles: " + str(meta_store.get_tile_count()))

	if meta_store.get_tile_count() == 0:
		_log("[color=yellow]No tiles found[/color]")
		return

	# TileManager の作成と初期化
	tile_manager = PLATEAUDynamicTileManager.new()
	tile_manager.name = "TileManager"
	add_child(tile_manager)

	tile_manager.set_load_distance(11, zoom11_range)
	tile_manager.set_load_distance(10, zoom10_range)
	tile_manager.set_load_distance(9, zoom9_range)
	tile_manager.camera = camera
	tile_manager.auto_update = true
	tile_manager.ignore_y = true
	tile_manager.tile_base_path = tile_output_dir

	# シグナル接続
	tile_manager.tile_loaded.connect(_on_tile_loaded)
	tile_manager.tile_unloaded.connect(_on_tile_unloaded)

	# 初期化
	tile_manager.initialize(meta_store)

	_log("[color=green]TileManager initialized![/color]")
	_log("Move the camera to trigger dynamic loading.")


func _on_tile_loaded(tile: PLATEAUDynamicTile) -> void:
	_log("[color=cyan]Loaded: " + tile.get_address().get_file() + "[/color]")
	_update_status()


func _on_tile_unloaded(tile: PLATEAUDynamicTile) -> void:
	_log("Unloaded: " + tile.get_address().get_file())
	_update_status()


func _process(_delta: float) -> void:
	if tile_manager == null:
		return

	# カメラ位置を表示
	grid_code_label.text = "Camera: (%.0f, %.0f, %.0f)" % [
		camera.global_position.x, camera.global_position.y, camera.global_position.z]


func _update_status() -> void:
	if tile_manager == null:
		loaded_label.text = "Loaded: 0 tiles"
		return

	var loaded = tile_manager.get_loaded_tiles()
	loaded_label.text = "Loaded: " + str(loaded.size()) + " / " + str(tile_manager.get_tiles().size()) + " tiles"

	var objects = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME)
	var video_mem = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_VIDEO_MEM_USED)
	status_label.text = "RID: %d, VRAM: %.1fMB" % [objects, video_mem / 1048576.0]


func _clear_all() -> void:
	if tile_manager != null:
		tile_manager.cleanup()
		tile_manager.queue_free()
		tile_manager = null
	_log("Cleared all tiles")
	_update_status()


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
