@tool
extends RefCounted

## PLATEAU Import Dialog
## Creates a dialog for configuring and executing CityGML import.

# Japan coordinate zone names (JGD2011 / Japan Plane Rectangular CS)
const ZONE_NAMES := {
	1: "Zone 1 (Nagasaki)",
	2: "Zone 2 (Fukuoka, Saga)",
	3: "Zone 3 (Yamaguchi, Shimane)",
	4: "Zone 4 (Kagoshima, Miyazaki)",
	5: "Zone 5 (Kumamoto, Oita)",
	6: "Zone 6 (Ehime, Kochi)",
	7: "Zone 7 (Okayama, Hiroshima)",
	8: "Zone 8 (Hyogo, Tottori)",
	9: "Zone 9 (Tokyo, Kanagawa, Saitama, Chiba)",
	10: "Zone 10 (Niigata, Nagano)",
	11: "Zone 11 (Hokkaido Central)",
	12: "Zone 12 (Hokkaido North)",
	13: "Zone 13 (Hokkaido East)",
	14: "Zone 14 (Aomori, Akita)",
	15: "Zone 15 (Iwate, Miyagi)",
	16: "Zone 16 (Yamagata, Fukushima)",
	17: "Zone 17 (Tochigi, Gunma)",
	18: "Zone 18 (Ibaraki, Saitama)",
	19: "Zone 19 (Okinawa)",
}

# Mesh granularity options
const GRANULARITY_OPTIONS := {
	0: "Per City Model Area",
	1: "Per Primary Feature",
	2: "Per Atomic Feature",
}

static func create_dialog(gml_path: String, editor_interface: EditorInterface) -> AcceptDialog:
	var dialog = AcceptDialog.new()
	dialog.title = "Import PLATEAU CityGML"
	dialog.ok_button_text = "Import"
	dialog.size = Vector2i(500, 600)

	# Main container
	var main_vbox = VBoxContainer.new()
	main_vbox.set_anchors_preset(Control.PRESET_FULL_RECT)
	dialog.add_child(main_vbox)

	# Scroll container for options
	var scroll = ScrollContainer.new()
	scroll.custom_minimum_size = Vector2(480, 500)
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	main_vbox.add_child(scroll)

	var options_vbox = VBoxContainer.new()
	options_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.add_child(options_vbox)

	# File path display
	var file_section = _create_section("File", options_vbox)
	var file_label = Label.new()
	file_label.text = gml_path.get_file()
	file_label.tooltip_text = gml_path
	file_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	file_section.add_child(file_label)

	# Coordinate Zone
	var zone_section = _create_section("Coordinate Zone", options_vbox)
	var zone_dropdown = OptionButton.new()
	zone_dropdown.name = "ZoneDropdown"
	for zone_id in range(1, 20):
		if zone_id in ZONE_NAMES:
			zone_dropdown.add_item(ZONE_NAMES[zone_id], zone_id)
	zone_dropdown.select(8)  # Default: Zone 9 (Tokyo)
	zone_section.add_child(zone_dropdown)

	# LOD Range
	var lod_section = _create_section("LOD Range", options_vbox)
	var lod_hbox = HBoxContainer.new()

	var lod_min_label = Label.new()
	lod_min_label.text = "Min:"
	lod_hbox.add_child(lod_min_label)

	var lod_min_spin = SpinBox.new()
	lod_min_spin.name = "LODMinSpin"
	lod_min_spin.min_value = 0
	lod_min_spin.max_value = 4
	lod_min_spin.value = 0
	lod_hbox.add_child(lod_min_spin)

	var lod_spacer = Control.new()
	lod_spacer.custom_minimum_size = Vector2(20, 0)
	lod_hbox.add_child(lod_spacer)

	var lod_max_label = Label.new()
	lod_max_label.text = "Max:"
	lod_hbox.add_child(lod_max_label)

	var lod_max_spin = SpinBox.new()
	lod_max_spin.name = "LODMaxSpin"
	lod_max_spin.min_value = 0
	lod_max_spin.max_value = 4
	lod_max_spin.value = 4
	lod_hbox.add_child(lod_max_spin)

	lod_section.add_child(lod_hbox)

	# Mesh Granularity
	var granularity_section = _create_section("Mesh Granularity", options_vbox)
	var granularity_dropdown = OptionButton.new()
	granularity_dropdown.name = "GranularityDropdown"
	for id in GRANULARITY_OPTIONS.keys():
		granularity_dropdown.add_item(GRANULARITY_OPTIONS[id], id)
	granularity_dropdown.select(1)  # Default: Per Primary Feature
	granularity_section.add_child(granularity_dropdown)

	# Import Options
	var options_section = _create_section("Import Options", options_vbox)

	var texture_check = CheckBox.new()
	texture_check.name = "TextureCheck"
	texture_check.text = "Include Textures"
	texture_check.button_pressed = true
	options_section.add_child(texture_check)

	var collision_check = CheckBox.new()
	collision_check.name = "CollisionCheck"
	collision_check.text = "Generate Collision"
	collision_check.button_pressed = false
	options_section.add_child(collision_check)

	# Progress section (hidden initially)
	var progress_section = _create_section("Progress", options_vbox)
	progress_section.name = "ProgressSection"
	progress_section.visible = false

	var progress_bar = ProgressBar.new()
	progress_bar.name = "ProgressBar"
	progress_bar.custom_minimum_size = Vector2(0, 20)
	progress_section.add_child(progress_bar)

	var progress_label = Label.new()
	progress_label.name = "ProgressLabel"
	progress_label.text = "Preparing..."
	progress_section.add_child(progress_label)

	# Connect confirmed signal
	dialog.confirmed.connect(func():
		_on_import_confirmed(dialog, gml_path, editor_interface)
	)

	return dialog

static func _create_section(title: String, parent: Control) -> VBoxContainer:
	var section = VBoxContainer.new()

	var label = Label.new()
	label.text = title
	var settings = LabelSettings.new()
	settings.font_size = 14
	label.label_settings = settings
	section.add_child(label)

	var sep = HSeparator.new()
	section.add_child(sep)

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 10)
	section.add_child(margin)

	var content = VBoxContainer.new()
	margin.add_child(content)

	var spacer = Control.new()
	spacer.custom_minimum_size = Vector2(0, 10)
	section.add_child(spacer)

	parent.add_child(section)
	return content

static func _on_import_confirmed(dialog: AcceptDialog, gml_path: String, editor_interface: EditorInterface) -> void:
	# Get values from dialog
	var zone_dropdown: OptionButton = dialog.find_child("ZoneDropdown", true, false)
	var lod_min_spin: SpinBox = dialog.find_child("LODMinSpin", true, false)
	var lod_max_spin: SpinBox = dialog.find_child("LODMaxSpin", true, false)
	var granularity_dropdown: OptionButton = dialog.find_child("GranularityDropdown", true, false)
	var texture_check: CheckBox = dialog.find_child("TextureCheck", true, false)
	var collision_check: CheckBox = dialog.find_child("CollisionCheck", true, false)
	var progress_section: Control = dialog.find_child("ProgressSection", true, false)
	var progress_bar: ProgressBar = dialog.find_child("ProgressBar", true, false)
	var progress_label: Label = dialog.find_child("ProgressLabel", true, false)

	if not zone_dropdown or not lod_min_spin or not lod_max_spin:
		push_error("[PLATEAU] Failed to get dialog controls")
		return

	var zone_id: int = zone_dropdown.get_selected_id()
	var lod_min: int = int(lod_min_spin.value)
	var lod_max: int = int(lod_max_spin.value)
	var granularity: int = granularity_dropdown.get_selected_id()
	var include_textures: bool = texture_check.button_pressed
	var generate_collision: bool = collision_check.button_pressed

	# Show progress
	progress_section.visible = true
	progress_bar.value = 0
	progress_label.text = "Loading CityGML..."

	# Disable OK button during import
	dialog.get_ok_button().disabled = true

	print("[PLATEAU] Starting import:")
	print("  File: ", gml_path)
	print("  Zone: ", zone_id)
	print("  LOD: ", lod_min, " - ", lod_max)
	print("  Granularity: ", granularity)
	print("  Textures: ", include_textures)
	print("  Collision: ", generate_collision)

	# Perform import (deferred to allow UI update)
	_do_import.call_deferred(
		dialog, gml_path, zone_id, lod_min, lod_max, granularity,
		include_textures, generate_collision, editor_interface,
		progress_bar, progress_label
	)

static func _do_import(
	dialog: AcceptDialog,
	gml_path: String,
	zone_id: int,
	lod_min: int,
	lod_max: int,
	granularity: int,
	include_textures: bool,
	generate_collision: bool,
	editor_interface: EditorInterface,
	progress_bar: ProgressBar,
	progress_label: Label
) -> void:

	var start_time := Time.get_ticks_msec()

	# Load CityGML
	progress_label.text = "Loading CityGML..."
	progress_bar.value = 10

	var city_model := PLATEAUCityModel.new()
	if not city_model.load(gml_path):
		push_error("[PLATEAU] Failed to load CityGML: " + gml_path)
		_import_finished(dialog, false, progress_label)
		return

	progress_bar.value = 30

	# Configure options
	progress_label.text = "Configuring options..."
	var options := PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	options.lod_range_min = lod_min
	options.lod_range_max = lod_max
	options.mesh_granularity = granularity
	options.include_textures = include_textures

	progress_bar.value = 40

	# Extract meshes
	progress_label.text = "Extracting meshes..."
	var mesh_array: Array[PLATEAUMeshData] = []
	var extracted = city_model.extract_meshes(options)
	for i in range(extracted.size()):
		mesh_array.append(extracted[i])

	if mesh_array.is_empty():
		push_warning("[PLATEAU] No meshes extracted from CityGML")

	progress_bar.value = 70

	# Import to scene
	progress_label.text = "Creating scene..."
	var importer := PLATEAUImporter.new()
	importer.generate_collision = generate_collision

	var edited_scene = editor_interface.get_edited_scene_root()
	if edited_scene == null:
		push_error("[PLATEAU] No scene is currently open")
		_import_finished(dialog, false, progress_label)
		return

	var root_node: Node3D = importer.import_to_scene(mesh_array, gml_path.get_file().get_basename())
	if root_node == null:
		push_error("[PLATEAU] Failed to create scene hierarchy")
		_import_finished(dialog, false, progress_label)
		return

	progress_bar.value = 90

	# Add to scene
	edited_scene.add_child(root_node)
	root_node.owner = edited_scene
	_set_owner_recursive(root_node, edited_scene)

	progress_bar.value = 100

	var elapsed := (Time.get_ticks_msec() - start_time) / 1000.0
	print("[PLATEAU] Import completed in %.2f seconds" % elapsed)
	print("[PLATEAU] Meshes: ", mesh_array.size())

	_import_finished(dialog, true, progress_label)

static func _set_owner_recursive(node: Node, owner: Node) -> void:
	for child in node.get_children():
		child.owner = owner
		_set_owner_recursive(child, owner)

static func _import_finished(dialog: AcceptDialog, success: bool, progress_label: Label) -> void:
	dialog.get_ok_button().disabled = false

	if success:
		progress_label.text = "Import completed!"
		# Close dialog after short delay
		await dialog.get_tree().create_timer(1.0).timeout
		dialog.hide()
	else:
		progress_label.text = "Import failed. Check console for details."
