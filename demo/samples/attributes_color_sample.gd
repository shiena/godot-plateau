extends Node3D
## Attributes Color Sample
##
## Demonstrates how to color-code buildings based on their attributes.
## - Color by building usage (bldg:usage)
## - Color by CityObjectType
## - Color by flood risk (if available)
##
## Usage:
## 1. Set gml_path to your CityGML file path
## 2. Run the scene
## 3. Use the buttons to switch between color modes

enum ColorMode {
	ORIGINAL,      ## Original textures/materials
	BY_USAGE,      ## Color by building usage attribute
	BY_TYPE,       ## Color by CityObjectType
	BY_FLOOD_RISK, ## Color by flood risk attribute
}

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9  ## Japan Plane Rectangular CS (1-19)

var city_model: PLATEAUCityModel
var mesh_instances: Array[MeshInstance3D] = []
var mesh_data_map: Dictionary = {}  # MeshInstance3D -> PLATEAUMeshData
var original_materials: Dictionary = {}  # MeshInstance3D -> Material
var current_mode: ColorMode = ColorMode.ORIGINAL

# Color mappings for building usage (bldg:usage)
var usage_colors: Dictionary = {
	"住宅": Color(0.2, 0.8, 0.2),       # Residential - Green
	"共同住宅": Color(0.3, 0.9, 0.3),   # Apartment - Light Green
	"店舗": Color(0.9, 0.9, 0.2),       # Shop - Yellow
	"事務所": Color(0.2, 0.5, 0.9),     # Office - Blue
	"工場": Color(0.6, 0.3, 0.6),       # Factory - Purple
	"倉庫": Color(0.5, 0.5, 0.5),       # Warehouse - Gray
	"商業施設": Color(0.9, 0.6, 0.2),   # Commercial - Orange
	"公共施設": Color(0.2, 0.9, 0.9),   # Public - Cyan
}
var default_usage_color: Color = Color(0.7, 0.7, 0.7)

# Color mappings for CityObjectType
var type_colors: Dictionary = {
	"Building": Color(0.8, 0.4, 0.2),
	"Road": Color(0.3, 0.3, 0.3),
	"Railway": Color(0.5, 0.3, 0.1),
	"Bridge": Color(0.6, 0.5, 0.4),
	"Tunnel": Color(0.4, 0.4, 0.5),
	"LandUse": Color(0.4, 0.7, 0.3),
	"Vegetation": Color(0.2, 0.6, 0.2),
	"WaterBody": Color(0.2, 0.4, 0.9),
	"CityFurniture": Color(0.6, 0.6, 0.6),
	"Relief": Color(0.5, 0.4, 0.3),
}
var default_type_color: Color = Color(0.5, 0.5, 0.5)

# Color mappings for flood risk levels
var flood_risk_colors: Dictionary = {
	0: Color(0.2, 0.2, 0.9),   # < 0.5m - Blue (low risk)
	1: Color(0.2, 0.9, 0.9),   # 0.5-1m - Cyan
	2: Color(0.2, 0.9, 0.2),   # 1-2m - Green
	3: Color(0.9, 0.9, 0.2),   # 2-3m - Yellow
	4: Color(0.9, 0.6, 0.2),   # 3-5m - Orange
	5: Color(0.9, 0.2, 0.2),   # > 5m - Red (high risk)
}

@onready var camera: Camera3D = $Camera3D
@onready var info_label: Label = $UI/InfoLabel
@onready var legend_label: RichTextLabel = $UI/LegendPanel/LegendLabel


func _ready() -> void:
	$UI/ImportButton.pressed.connect(_on_import_pressed)
	$UI/ModeButtons/OriginalButton.pressed.connect(func(): _set_color_mode(ColorMode.ORIGINAL))
	$UI/ModeButtons/UsageButton.pressed.connect(func(): _set_color_mode(ColorMode.BY_USAGE))
	$UI/ModeButtons/TypeButton.pressed.connect(func(): _set_color_mode(ColorMode.BY_TYPE))
	$UI/ModeButtons/FloodButton.pressed.connect(func(): _set_color_mode(ColorMode.BY_FLOOD_RISK))
	_update_legend()


func _on_import_pressed() -> void:
	if gml_path.is_empty():
		PLATEAUUtils.show_gml_file_dialog(self, _on_file_selected)
	else:
		_import_gml(gml_path)


func _on_file_selected(path: String) -> void:
	gml_path = path
	_import_gml(path)


func _import_gml(path: String) -> void:
	_update_info("Loading: " + path.get_file() + "...")

	# Clear previous data
	PLATEAUUtils.clear_mesh_instances(mesh_instances)
	mesh_data_map.clear()
	original_materials.clear()

	# Load CityGML
	city_model = PLATEAUCityModel.new()
	if not city_model.load(path):
		_update_info("Failed to load: " + path)
		return

	# Setup extraction options
	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.min_lod = 1
	options.max_lod = 2
	options.mesh_granularity = 1  # Primary feature
	options.export_appearance = true

	# Extract meshes
	var root_mesh_data = city_model.extract_meshes(options)

	if root_mesh_data.is_empty():
		_update_info("No meshes extracted from: " + path)
		return

	# Flatten hierarchy to get actual building meshes (children of LOD nodes)
	var mesh_data_array = PLATEAUUtils.flatten_mesh_data(root_mesh_data)

	# Create MeshInstance3D for each mesh
	var result = PLATEAUUtils.create_mesh_instances(mesh_data_array, self)
	mesh_instances = result["instances"]

	# Build mesh_data_map and store original materials
	for i in range(mesh_instances.size()):
		var mi = mesh_instances[i]
		var mesh_data = mesh_data_array[i]
		mesh_data_map[mi] = mesh_data
		if mi.mesh.get_surface_count() > 0:
			original_materials[mi] = mi.mesh.surface_get_material(0)

	# Position camera
	PLATEAUUtils.fit_camera_to_bounds(camera, result["bounds_min"], result["bounds_max"])

	_update_info("Loaded " + str(mesh_data_array.size()) + " meshes. Use buttons to change color mode.")
	_apply_color_mode()


func _set_color_mode(mode: ColorMode) -> void:
	current_mode = mode
	_apply_color_mode()
	_update_legend()


func _apply_color_mode() -> void:
	for mesh_instance in mesh_instances:
		if not mesh_data_map.has(mesh_instance):
			continue

		var mesh_data: PLATEAUMeshData = mesh_data_map[mesh_instance]
		var color: Color

		match current_mode:
			ColorMode.ORIGINAL:
				_restore_original_material(mesh_instance)
				continue
			ColorMode.BY_USAGE:
				color = _get_usage_color(mesh_data)
			ColorMode.BY_TYPE:
				color = _get_type_color(mesh_data)
			ColorMode.BY_FLOOD_RISK:
				color = _get_flood_risk_color(mesh_data)

		_set_mesh_color(mesh_instance, color)


func _get_usage_color(mesh_data: PLATEAUMeshData) -> Color:
	var usage = mesh_data.get_attribute("bldg:usage")
	if usage == null:
		usage = mesh_data.get_attribute("uro:buildingDetails/uro:buildingUse")

	if usage != null:
		var usage_str = str(usage)
		for key in usage_colors.keys():
			if usage_str.contains(key):
				return usage_colors[key]

	return default_usage_color


func _get_type_color(mesh_data: PLATEAUMeshData) -> Color:
	var type_name = mesh_data.get_city_object_type_name()

	for key in type_colors.keys():
		if type_name.contains(key):
			return type_colors[key]

	return default_type_color


func _get_flood_risk_color(mesh_data: PLATEAUMeshData) -> Color:
	# Try various flood risk attribute paths
	var risk = mesh_data.get_attribute("uro:floodingRiskAttribute/uro:rank")
	if risk == null:
		risk = mesh_data.get_attribute("uro:HighTideRiskAttribute/uro:rank")
	if risk == null:
		risk = mesh_data.get_attribute("fld:rank")

	if risk != null:
		var rank = int(risk)
		if flood_risk_colors.has(rank):
			return flood_risk_colors[rank]

	return default_usage_color


func _set_mesh_color(mesh_instance: MeshInstance3D, color: Color) -> void:
	var material = StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = 0.8
	mesh_instance.material_override = material


func _restore_original_material(mesh_instance: MeshInstance3D) -> void:
	mesh_instance.material_override = null


func _update_legend() -> void:
	var text = ""

	match current_mode:
		ColorMode.ORIGINAL:
			text = "[b]Original Materials[/b]\nShowing original textures and materials."

		ColorMode.BY_USAGE:
			text = "[b]Building Usage[/b]\n"
			for usage in usage_colors.keys():
				var c = usage_colors[usage]
				text += "[color=#%s]■[/color] %s\n" % [c.to_html(false), usage]
			text += "[color=#%s]■[/color] Other\n" % default_usage_color.to_html(false)

		ColorMode.BY_TYPE:
			text = "[b]CityObject Type[/b]\n"
			for type_name in type_colors.keys():
				var c = type_colors[type_name]
				text += "[color=#%s]■[/color] %s\n" % [c.to_html(false), type_name]

		ColorMode.BY_FLOOD_RISK:
			text = "[b]Flood Risk Level[/b]\n"
			text += "[color=#%s]■[/color] < 0.5m (Low)\n" % flood_risk_colors[0].to_html(false)
			text += "[color=#%s]■[/color] 0.5-1m\n" % flood_risk_colors[1].to_html(false)
			text += "[color=#%s]■[/color] 1-2m\n" % flood_risk_colors[2].to_html(false)
			text += "[color=#%s]■[/color] 2-3m\n" % flood_risk_colors[3].to_html(false)
			text += "[color=#%s]■[/color] 3-5m\n" % flood_risk_colors[4].to_html(false)
			text += "[color=#%s]■[/color] > 5m (High)\n" % flood_risk_colors[5].to_html(false)

	legend_label.text = text


func _update_info(message: String) -> void:
	info_label.text = message
	print(message)
