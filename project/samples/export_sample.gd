extends Node3D
## Export Sample - glTF/OBJエクスポート
##
## このサンプルで学べること:
## - PLATEAUMeshExporterによるエクスポート
## - glTF/GLB/OBJ形式の違い
## - エクスポートオプションの設定
##
## 使い方:
## 1. GMLファイルをロード
## 2. エクスポート形式を選択
## 3. ファイルを保存

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9

var city_model: PLATEAUCityModel
var current_mesh_data: Array[PLATEAUMeshData] = []

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var mesh_container: Node3D = $MeshContainer
@onready var stats_label: Label = $UI/StatsLabel


func _ready() -> void:
	city_model = PLATEAUCityModel.new()

	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/ExportPanel/ExportGLTFButton.pressed.connect(func(): _export(PLATEAUMeshExporter.EXPORT_FORMAT_GLTF))
	$UI/ExportPanel/ExportGLBButton.pressed.connect(func(): _export(PLATEAUMeshExporter.EXPORT_FORMAT_GLB))
	$UI/ExportPanel/ExportOBJButton.pressed.connect(func(): _export(PLATEAUMeshExporter.EXPORT_FORMAT_OBJ))

	_log("Export Sample ready.")
	_log("Click 'Import GML' to load a CityGML file.")
	_log("")
	_log("Supported formats:")
	_log("  glTF: テキスト形式、外部ファイル参照")
	_log("  GLB: バイナリ形式、単一ファイル")
	_log("  OBJ: 広くサポートされる形式")


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
	for child in mesh_container.get_children():
		child.queue_free()
	current_mesh_data.clear()

	# GMLロード
	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	# メッシュ抽出
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 1  # Primary
	options.export_appearance = true

	var mesh_data = city_model.extract_meshes(options)

	# PLATEAUMeshDataをフラット化して保存
	for root in mesh_data:
		_flatten_mesh_data(root)

	_log("Meshes extracted: " + str(current_mesh_data.size()))

	# 表示用にシーン作成
	var geo_ref = PLATEAUGeoReference.new()
	geo_ref.zone_id = zone_id
	geo_ref.reference_point = options.reference_point

	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	for md in current_mesh_data:
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

	PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)
	_update_stats()
	_log("[color=green]Load complete! Ready to export.[/color]")


func _flatten_mesh_data(mesh_data: PLATEAUMeshData) -> void:
	if mesh_data.get_mesh() != null:
		current_mesh_data.append(mesh_data)

	for child in mesh_data.get_children():
		_flatten_mesh_data(child)


func _export(format: int) -> void:
	if current_mesh_data.is_empty():
		_log("[color=red]ERROR: No meshes to export. Import first.[/color]")
		return

	var ext = PLATEAUMeshExporter.get_format_extension(format)
	var format_name = PLATEAUMeshExporter.get_supported_formats()[format]

	PLATEAUUtils.show_save_file_dialog(
		self,
		PackedStringArray(["*." + ext + " ; " + format_name + " Files"]),
		"export." + ext,
		func(path): _do_export(path, format)
	)


func _do_export(path: String, format: int) -> void:
	_log("--- Export Start ---")
	_log("Format: " + PLATEAUMeshExporter.get_supported_formats()[format])
	_log("Path: " + path)

	var exporter = PLATEAUMeshExporter.new()
	var start_time = Time.get_ticks_msec()

	var success = exporter.export_to_file(current_mesh_data, path, format)
	var export_time = Time.get_ticks_msec() - start_time

	if success:
		_log("Export time: " + str(export_time) + " ms")

		# ファイルサイズを取得
		var file = FileAccess.open(path, FileAccess.READ)
		if file != null:
			var size = file.get_length()
			file.close()
			_log("File size: " + _format_size(size))

		_log("[color=green]Export complete: " + path + "[/color]")
	else:
		_log("[color=red]ERROR: Export failed[/color]")


func _format_size(bytes: int) -> String:
	if bytes < 1024:
		return str(bytes) + " B"
	elif bytes < 1024 * 1024:
		return "%.1f KB" % (bytes / 1024.0)
	else:
		return "%.1f MB" % (bytes / 1048576.0)


func _update_stats() -> void:
	var total_vertices = 0
	var total_triangles = 0

	for mesh_data in current_mesh_data:
		var mesh = mesh_data.get_mesh()
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
		current_mesh_data.size(), total_vertices, total_triangles
	]


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
