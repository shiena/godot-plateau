extends Node3D
## Minimal Sample - 最小限のGMLロード例
##
## このサンプルで学べること:
## - PLATEAUCityModelによるGMLファイルのロード
## - PLATEAUMeshExtractOptionsの基本設定
## - メッシュの抽出と表示
##
## 使い方:
## 1. gml_pathにGMLファイルのパスを設定
## 2. シーンを実行
## 3. 自動的にGMLがロードされて表示される

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9  ## 平面直角座標系 (1-19, 東京は9)

@onready var camera: Camera3D = $Camera3D


func _ready() -> void:
	if gml_path.is_empty():
		# ファイル選択ダイアログを表示
		PLATEAUUtils.show_gml_file_dialog(self, _load_gml)
	else:
		_load_gml(gml_path)


func _load_gml(path: String) -> void:
	print("Loading: ", path)

	# 1. CityModelを作成してGMLをロード
	var city_model = PLATEAUCityModel.new()
	if not city_model.load(path):
		push_error("Failed to load GML: " + path)
		return

	# 2. メッシュ抽出オプションを設定
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.reference_point = city_model.get_center_point(zone_id)
	options.mesh_granularity = 1  # Primary (建物単位)
	options.export_appearance = true  # テクスチャを含める

	# 3. メッシュを抽出
	var mesh_data_array = city_model.extract_meshes(options)
	print("Extracted meshes: ", mesh_data_array.size())

	# 4. MeshInstance3Dを作成して表示
	var bounds_min := Vector3.INF
	var bounds_max := -Vector3.INF

	for root_data in mesh_data_array:
		_create_mesh_instances(root_data, self, bounds_min, bounds_max)

	# 5. カメラを調整
	PLATEAUUtils.fit_camera_to_bounds(camera, bounds_min, bounds_max)
	print("Load complete!")


func _create_mesh_instances(mesh_data: PLATEAUMeshData, parent: Node3D, bounds_min: Vector3, bounds_max: Vector3) -> void:
	var mesh = mesh_data.get_mesh()
	if mesh != null:
		var instance = MeshInstance3D.new()
		instance.name = mesh_data.get_name()
		instance.mesh = mesh
		instance.transform = mesh_data.get_transform()
		parent.add_child(instance)

		# AABB更新
		var aabb = mesh.get_aabb()
		bounds_min = bounds_min.min(instance.transform * aabb.position)
		bounds_max = bounds_max.max(instance.transform * (aabb.position + aabb.size))

	# 子ノードを再帰処理
	for child in mesh_data.get_children():
		_create_mesh_instances(child, parent, bounds_min, bounds_max)
