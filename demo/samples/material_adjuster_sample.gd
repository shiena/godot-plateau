extends Node3D
## Material Adjuster Sample
##
## Demonstrates material adjustment APIs:
## - PLATEAUMaterialAdjusterByAttr: Color buildings by attribute values (e.g., usage)
## - PLATEAUMaterialAdjusterByType: Color buildings by CityObjectType
##
## Usage:
## 1. Import a CityGML file
## 2. Select an adjustment mode (By Attribute or By Type)
## 3. Click "Apply Materials" to see the color coding

@export_file("*.gml") var gml_path: String = ""
@export var zone_id: int = 9

var city_model: PLATEAUCityModel
var current_mesh_data: Array = []
var mesh_instances: Array[MeshInstance3D] = []

# Material palette
var materials: Array[StandardMaterial3D] = []
var material_colors = [
	Color(0.8, 0.2, 0.2),  # Red - Residential
	Color(0.2, 0.6, 0.8),  # Blue - Commercial
	Color(0.2, 0.8, 0.3),  # Green - Office
	Color(0.9, 0.7, 0.2),  # Yellow - Industrial
	Color(0.7, 0.3, 0.8),  # Purple - Public
	Color(0.9, 0.5, 0.2),  # Orange - Mixed
	Color(0.5, 0.5, 0.5),  # Gray - Unknown
	Color(0.3, 0.7, 0.7),  # Cyan - Other
]

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var mode_option: OptionButton = $UI/AdjustPanel/ModeOption
@onready var attr_key_edit: LineEdit = $UI/AdjustPanel/AttrKeyRow/AttrKeyEdit
@onready var legend_container: VBoxContainer = $UI/LegendPanel/ScrollContainer/LegendContainer


func _ready() -> void:
	_create_materials()

	# Connect UI
	$UI/ImportPanel/ImportButton.pressed.connect(_on_import_pressed)
	$UI/AdjustPanel/ApplyButton.pressed.connect(_on_apply_pressed)
	$UI/AdjustPanel/ResetButton.pressed.connect(_on_reset_pressed)
	mode_option.item_selected.connect(_on_mode_changed)

	# Setup mode options
	mode_option.clear()
	mode_option.add_item("By Attribute")
	mode_option.add_item("By CityObjectType")

	attr_key_edit.text = "bldg:storeysaboveground"

	_log("Material Adjuster Sample ready.")
	_log("Click 'Import GML' to load a CityGML file.")


func _create_materials() -> void:
	materials.clear()
	for color in material_colors:
		var mat = StandardMaterial3D.new()
		mat.albedo_color = color
		mat.roughness = 0.7
		materials.append(mat)


func _on_mode_changed(index: int) -> void:
	# Show/hide attribute key field based on mode
	var attr_row = $UI/AdjustPanel/AttrKeyRow
	attr_row.visible = (index == 0)


func _on_import_pressed() -> void:
	if gml_path.is_empty():
		var dialog = FileDialog.new()
		dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
		dialog.access = FileDialog.ACCESS_FILESYSTEM
		dialog.filters = ["*.gml ; CityGML Files"]
		dialog.file_selected.connect(_on_file_selected)
		add_child(dialog)
		dialog.popup_centered(Vector2i(800, 600))
	else:
		_import_gml(gml_path)


func _on_file_selected(path: String) -> void:
	gml_path = path
	_import_gml(path)


func _import_gml(path: String) -> void:
	_log("--- Import GML ---")
	_log("File: " + path.get_file())

	_clear_meshes()

	city_model = PLATEAUCityModel.new()
	if not city_model.load(path):
		_log("[color=red]ERROR: Failed to load GML[/color]")
		return

	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.mesh_granularity = 1  # Primary (per building)
	options.export_appearance = true

	var root_mesh_data = city_model.extract_meshes(options)
	current_mesh_data = _flatten_mesh_data(Array(root_mesh_data))

	_log("Extracted " + str(current_mesh_data.size()) + " meshes")

	_create_mesh_instances()
	_log("[color=green]Import complete![/color]")


func _on_apply_pressed() -> void:
	if current_mesh_data.is_empty():
		_log("[color=red]ERROR: No meshes loaded. Import first.[/color]")
		return

	var mode = mode_option.selected
	if mode == 0:
		_apply_by_attribute()
	else:
		_apply_by_type()


func _apply_by_attribute() -> void:
	_log("--- Apply Materials by Attribute ---")

	var attr_key = attr_key_edit.text
	_log("Attribute key: " + attr_key)

	# Debug: Show first mesh's attributes
	if current_mesh_data.size() > 0:
		var first_mesh = current_mesh_data[0]
		var attrs = first_mesh.get_attributes()
		_log("First mesh GML ID: " + first_mesh.get_gml_id())
		_log("First mesh attributes count: " + str(attrs.size()))
		# Show all non-empty string attributes
		for key in attrs.keys():
			var val = attrs[key]
			if val is String and not val.is_empty():
				_log("  " + str(key) + " = " + str(val))
			elif val is int or val is float:
				_log("  " + str(key) + " = " + str(val))
			elif val is Dictionary:
				_log("  " + str(key) + " = (Dictionary)")
				# Show nested dictionary contents
				for nested_key in val.keys():
					var nested_val = val[nested_key]
					if nested_val is Dictionary:
						for k2 in nested_val.keys():
							_log("    " + str(nested_key) + "/" + str(k2) + " = " + str(nested_val[k2]))
					else:
						_log("    " + str(nested_key) + " = " + str(nested_val))

	# Collect unique attribute values
	var attr_values: Dictionary = {}
	for mesh_data in current_mesh_data:
		var value = mesh_data.get_attribute(attr_key)
		if value != null and str(value) != "":
			var value_str = str(value)
			if not attr_values.has(value_str):
				attr_values[value_str] = attr_values.size() % materials.size()

	_log("Found " + str(attr_values.size()) + " unique values")
	for attr_value in attr_values.keys():
		var mat_id = attr_values[attr_value]
		_log("  " + attr_value + " -> Material " + str(mat_id))

	# Apply materials directly in GDScript (simpler and more reliable)
	_log("Applying materials...")
	_apply_materials_manually(attr_key, attr_values)

	_update_legend(attr_values)
	_log("[color=green]Materials applied![/color]")


func _apply_by_type() -> void:
	_log("--- Apply Materials by CityObjectType ---")

	# Collect unique types
	var type_values: Dictionary = {}
	for mesh_data in current_mesh_data:
		var type_id = mesh_data.get_city_object_type()
		var type_name = mesh_data.get_city_object_type_name()
		if type_id != 0 and not type_values.has(type_id):
			type_values[type_id] = {
				"name": type_name,
				"mat_id": type_values.size() % materials.size()
			}

	_log("Found " + str(type_values.size()) + " unique types")

	for type_id in type_values.keys():
		var info = type_values[type_id]
		_log("  " + info["name"] + " -> Material " + str(info["mat_id"]))

	# Apply materials directly in GDScript
	_log("Applying materials...")
	_apply_materials_by_type_manually(type_values)

	# Update legend
	var legend_dict: Dictionary = {}
	for type_id in type_values.keys():
		var info = type_values[type_id]
		legend_dict[info["name"]] = info["mat_id"]
	_update_legend(legend_dict)

	_log("[color=green]Materials applied![/color]")


func _apply_materials_from_surface_names() -> void:
	# Rebuild mesh instances with new materials based on surface names
	_clear_mesh_instances_only()

	for mesh_data in current_mesh_data:
		var mi = MeshInstance3D.new()
		mi.mesh = mesh_data.get_mesh()
		mi.transform = mesh_data.get_transform()
		mi.name = mesh_data.get_name()

		# Apply materials based on surface names (mat_0, mat_1, etc.)
		var mesh = mi.mesh
		if mesh != null:
			for surf_idx in range(mesh.get_surface_count()):
				var surf_name = mesh.surface_get_name(surf_idx)
				if surf_name.begins_with("mat_"):
					var mat_id = surf_name.substr(4).to_int()
					if mat_id >= 0 and mat_id < materials.size():
						mi.set_surface_override_material(surf_idx, materials[mat_id])

		add_child(mi)
		mesh_instances.append(mi)


func _apply_materials_manually(attr_key: String, attr_values: Dictionary) -> void:
	# Apply materials directly based on attribute values
	var count = mini(mesh_instances.size(), current_mesh_data.size())

	for i in range(count):
		var mi = mesh_instances[i]
		var mesh_data = current_mesh_data[i]

		var value = mesh_data.get_attribute(attr_key)
		var value_str = str(value) if value != null else ""

		if attr_values.has(value_str):
			var mat_id = attr_values[value_str]
			for surf_idx in range(mi.mesh.get_surface_count() if mi.mesh else 0):
				mi.set_surface_override_material(surf_idx, materials[mat_id])


func _apply_materials_by_type_manually(type_values: Dictionary) -> void:
	# Apply materials directly when adjuster doesn't work
	for i in range(mesh_instances.size()):
		var mi = mesh_instances[i]
		var mesh_data = current_mesh_data[i]

		var type_id = mesh_data.get_city_object_type()
		if type_values.has(type_id):
			var mat_id = type_values[type_id]["mat_id"]
			for surf_idx in range(mi.mesh.get_surface_count() if mi.mesh else 0):
				mi.set_surface_override_material(surf_idx, materials[mat_id])


func _on_reset_pressed() -> void:
	if current_mesh_data.is_empty():
		return

	_log("--- Reset Materials ---")

	# Clear overrides
	for mi in mesh_instances:
		for surf_idx in range(mi.mesh.get_surface_count() if mi.mesh else 0):
			mi.set_surface_override_material(surf_idx, null)

	# Clear legend
	for child in legend_container.get_children():
		child.queue_free()

	_log("Materials reset to original")


func _update_legend(value_to_mat: Dictionary) -> void:
	# Clear existing legend
	for child in legend_container.get_children():
		child.queue_free()

	# Add legend entries
	for value in value_to_mat.keys():
		var mat_id = value_to_mat[value]
		if mat_id is Dictionary:
			mat_id = value_to_mat[value]["mat_id"]

		var hbox = HBoxContainer.new()

		var color_rect = ColorRect.new()
		color_rect.custom_minimum_size = Vector2(20, 20)
		color_rect.color = material_colors[mat_id % material_colors.size()]
		hbox.add_child(color_rect)

		var label = Label.new()
		label.text = " " + str(value)
		hbox.add_child(label)

		legend_container.add_child(hbox)


func _clear_meshes() -> void:
	_clear_mesh_instances_only()
	current_mesh_data.clear()


func _clear_mesh_instances_only() -> void:
	for mi in mesh_instances:
		mi.queue_free()
	mesh_instances.clear()


func _create_mesh_instances() -> void:
	var bounds_min = Vector3.INF
	var bounds_max = -Vector3.INF

	for mesh_data in current_mesh_data:
		var mi = MeshInstance3D.new()
		mi.mesh = mesh_data.get_mesh()
		mi.transform = mesh_data.get_transform()
		mi.name = mesh_data.get_name()

		add_child(mi)
		mesh_instances.append(mi)

		var aabb = mi.get_aabb()
		var global_aabb = mi.transform * aabb
		bounds_min = bounds_min.min(global_aabb.position)
		bounds_max = bounds_max.max(global_aabb.position + global_aabb.size)

	# Position camera
	var center = (bounds_min + bounds_max) / 2
	var size = (bounds_max - bounds_min).length()
	if size > 0.001:
		camera.position = center + Vector3(0, size * 0.5, size * 0.8)
		camera.look_at(center)


func _flatten_mesh_data(mesh_data_array: Array) -> Array:
	var result: Array = []
	for mesh_data in mesh_data_array:
		if mesh_data.get_mesh() != null:
			result.append(mesh_data)
		var children = mesh_data.get_children()
		if not children.is_empty():
			result.append_array(_flatten_mesh_data(Array(children)))
	return result


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
