@tool
extends RefCounted

## PLATEAU Export Dialog
## Creates a dialog for configuring and executing mesh export to glTF/GLB/OBJ.

# Settings file path
const SETTINGS_PATH := "user://plateau_exporter_settings.cfg"

# Japan coordinate zone names (JGD2011 / Japan Plane Rectangular CS)
const ZONE_NAMES := {
	1: "Zone 1 (長崎)",
	2: "Zone 2 (福岡, 佐賀)",
	3: "Zone 3 (山口, 島根)",
	4: "Zone 4 (鹿児島, 宮崎)",
	5: "Zone 5 (熊本, 大分)",
	6: "Zone 6 (大阪, 京都, 兵庫, 奈良)",
	7: "Zone 7 (愛知, 岐阜, 三重)",
	8: "Zone 8 (石川, 富山, 福井)",
	9: "Zone 9 (東京, 神奈川, 埼玉, 千葉)",
	10: "Zone 10 (新潟, 長野, 山梨)",
	11: "Zone 11 (北海道 中央)",
	12: "Zone 12 (北海道 北部)",
	13: "Zone 13 (北海道 東部)",
	14: "Zone 14 (青森, 秋田)",
	15: "Zone 15 (岩手, 宮城)",
	16: "Zone 16 (山形, 福島)",
	17: "Zone 17 (栃木, 群馬)",
	18: "Zone 18 (茨城)",
	19: "Zone 19 (沖縄)",
}

# Zone origin coordinates (latitude, longitude) for auto-detection
const ZONE_ORIGINS := {
	1: [33.0, 129.5],
	2: [33.0, 131.0],
	3: [36.0, 132.167],
	4: [33.0, 131.0],
	5: [33.0, 131.0],
	6: [36.0, 136.0],
	7: [36.0, 137.167],
	8: [36.0, 138.5],
	9: [36.0, 139.833],
	10: [40.0, 140.833],
	11: [44.0, 140.25],
	12: [44.0, 142.25],
	13: [44.0, 144.25],
	14: [40.0, 140.0],
	15: [38.0, 140.833],
	16: [36.0, 140.0],
	17: [36.0, 139.833],
	18: [36.0, 140.5],
	19: [26.0, 127.5],
}

# Mesh granularity options
const GRANULARITY_OPTIONS := {
	0: "Per City Model Area",
	1: "Per Primary Feature",
	2: "Per Atomic Feature",
}

# Coordinate system options (matches PLATEAUMeshExtractOptions)
const COORDINATE_SYSTEM_OPTIONS := {
	3: "EUN (Godot標準)",  # 東(X)上(Y)北(Z) - Godot default
	0: "ENU (PLATEAU標準)", # 東(X)北(Y)上(Z) - PLATEAU standard
	1: "WUN (glTF標準)",   # 西(X)上(Y)北(Z) - glTF standard
}

# Transform type options
const TRANSFORM_TYPE_OPTIONS := {
	0: "Local (相対座標)",
	1: "PlaneCartesian (平面直角座標)",
}

# Export format options
const FORMAT_OPTIONS := {
	0: "glTF (.gltf)",
	1: "GLB (.glb)",
	2: "OBJ (.obj)",
}

# Texture packing resolution options
const TEXTURE_RESOLUTION_OPTIONS := [512, 1024, 2048, 4096, 8192]

# Package flags
const PACKAGE_BUILDING := 1 << 0
const PACKAGE_ROAD := 1 << 1
const PACKAGE_URBAN_PLANNING := 1 << 2
const PACKAGE_LAND_USE := 1 << 3
const PACKAGE_CITY_FURNITURE := 1 << 4
const PACKAGE_VEGETATION := 1 << 5
const PACKAGE_RELIEF := 1 << 6
const PACKAGE_FLOOD := 1 << 7
const PACKAGE_TSUNAMI := 1 << 8
const PACKAGE_RAILWAY := 1 << 12
const PACKAGE_WATERWAY := 1 << 13
const PACKAGE_WATER_BODY := 1 << 14
const PACKAGE_BRIDGE := 1 << 15
const PACKAGE_TUNNEL := 1 << 18
const PACKAGE_GENERIC := 1 << 23
const PACKAGE_UNKNOWN := 1 << 24
const PACKAGE_ALL := 0x1FFFFFF

const PACKAGE_OPTIONS := [
	["Building (建物)", PACKAGE_BUILDING],
	["Road (道路)", PACKAGE_ROAD],
	["Relief (地形)", PACKAGE_RELIEF],
	["Land Use (土地利用)", PACKAGE_LAND_USE],
	["Vegetation (植生)", PACKAGE_VEGETATION],
	["City Furniture (都市設備)", PACKAGE_CITY_FURNITURE],
	["Bridge (橋)", PACKAGE_BRIDGE],
	["Water Body (水域)", PACKAGE_WATER_BODY],
	["Other (その他)", PACKAGE_URBAN_PLANNING | PACKAGE_FLOOD | PACKAGE_TSUNAMI | PACKAGE_RAILWAY | PACKAGE_WATERWAY | PACKAGE_TUNNEL | PACKAGE_GENERIC | PACKAGE_UNKNOWN],
]

# Prefecture code to Zone ID mapping
const PREFECTURE_TO_ZONE := {
	1: 11, 2: 14, 3: 15, 4: 15, 5: 14, 6: 16, 7: 16, 8: 18, 9: 17, 10: 17,
	11: 9, 12: 9, 13: 9, 14: 9, 15: 10, 16: 8, 17: 8, 18: 8, 19: 10, 20: 10,
	21: 7, 22: 7, 23: 7, 24: 7, 25: 6, 26: 6, 27: 6, 28: 6, 29: 6, 30: 6,
	31: 3, 32: 3, 33: 3, 34: 3, 35: 3, 36: 4, 37: 4, 38: 4, 39: 4, 40: 2,
	41: 2, 42: 1, 43: 2, 44: 2, 45: 2, 46: 2, 47: 19,
}


# Estimate zone ID from dataset folder name
static func _estimate_zone_from_folder_name(folder_path: String) -> int:
	var folder_name = folder_path.get_file()
	if folder_name.is_empty():
		folder_name = folder_path.get_base_dir().get_file()
	var regex = RegEx.new()
	regex.compile("^(\\d{2})\\d{3}")
	var result = regex.search(folder_name)
	if result:
		var pref_code = result.get_string(1).to_int()
		if pref_code in PREFECTURE_TO_ZONE:
			return PREFECTURE_TO_ZONE[pref_code]
	return 0


static func _estimate_zone_from_path_hierarchy(folder_path: String) -> int:
	var current_path = folder_path
	for i in range(5):
		var zone = _estimate_zone_from_folder_name(current_path)
		if zone > 0:
			return zone
		var parent = current_path.get_base_dir()
		if parent == current_path or parent.is_empty():
			break
		current_path = parent
	return 0


static func _estimate_zone_from_mesh_code(mesh_code: String) -> int:
	if mesh_code.length() < 4:
		return 0
	var lat_code = mesh_code.substr(0, 2).to_int()
	var lon_code = mesh_code.substr(2, 2).to_int()
	var lat = lat_code / 1.5 + 0.33
	var lon = lon_code + 100.0 + 0.5
	return _find_nearest_zone(lat, lon)


static func _find_nearest_zone(lat: float, lon: float) -> int:
	var best_zone = 9
	var min_distance = INF
	for zone_id in ZONE_ORIGINS:
		var origin = ZONE_ORIGINS[zone_id]
		var dlat = lat - origin[0]
		var dlon = lon - origin[1]
		var distance = dlat * dlat + dlon * dlon
		if distance < min_distance:
			min_distance = distance
			best_zone = zone_id
	return best_zone


static func _load_settings() -> Dictionary:
	var config = ConfigFile.new()
	var err = config.load(SETTINGS_PATH)
	if err != OK:
		return {}
	var settings = {}
	for key in config.get_section_keys("paths"):
		settings[key] = config.get_value("paths", key, "")
	return settings


static func _save_setting(key: String, value: String) -> void:
	var config = ConfigFile.new()
	config.load(SETTINGS_PATH)
	config.set_value("paths", key, value)
	config.save(SETTINGS_PATH)


static func create_dialog() -> AcceptDialog:
	var dialog = AcceptDialog.new()
	dialog.title = "PLATEAU Converter"
	dialog.ok_button_text = "Close"
	dialog.size = Vector2i(550, 700)

	# Main container
	var main_vbox = VBoxContainer.new()
	main_vbox.set_anchors_preset(Control.PRESET_FULL_RECT)
	main_vbox.offset_left = 8
	main_vbox.offset_top = 8
	main_vbox.offset_right = -8
	main_vbox.offset_bottom = -50
	dialog.add_child(main_vbox)

	# Scroll container for options
	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	main_vbox.add_child(scroll)

	var options_vbox = VBoxContainer.new()
	options_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	options_vbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.add_child(options_vbox)

	# === Input Source Section ===
	var input_section = _create_section("Input Source", options_vbox)

	# Radio buttons for source type
	var radio_hbox = HBoxContainer.new()
	var source_radio_dataset = CheckBox.new()
	source_radio_dataset.name = "SourceRadioDataset"
	source_radio_dataset.text = "Dataset Folder"
	source_radio_dataset.button_pressed = true
	radio_hbox.add_child(source_radio_dataset)

	var radio_spacer = Control.new()
	radio_spacer.custom_minimum_size = Vector2(20, 0)
	radio_hbox.add_child(radio_spacer)

	var source_radio_gml = CheckBox.new()
	source_radio_gml.name = "SourceRadioGml"
	source_radio_gml.text = "GML File"
	source_radio_gml.button_pressed = false
	radio_hbox.add_child(source_radio_gml)

	input_section.add_child(radio_hbox)

	# Input path with browse button
	var path_hbox = HBoxContainer.new()
	var input_path = LineEdit.new()
	input_path.name = "InputPath"
	input_path.placeholder_text = "Select a dataset folder..."
	input_path.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	input_path.editable = false
	path_hbox.add_child(input_path)

	var browse_btn = Button.new()
	browse_btn.name = "BrowseButton"
	browse_btn.text = "Browse..."
	path_hbox.add_child(browse_btn)

	input_section.add_child(path_hbox)

	# Info label
	var input_info_label = Label.new()
	input_info_label.name = "InputInfoLabel"
	input_info_label.text = ""
	input_info_label.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
	input_section.add_child(input_info_label)

	# === Package Filter Section (for dataset mode) ===
	var filter_section = _create_section("Package Filter", options_vbox)
	filter_section.name = "FilterSection"

	var filter_container = VBoxContainer.new()
	filter_container.name = "FilterContainer"
	filter_section.add_child(filter_container)

	for pkg_info in PACKAGE_OPTIONS:
		var check = CheckBox.new()
		check.name = "PkgCheck_" + str(pkg_info[1])
		check.text = pkg_info[0]
		check.button_pressed = (pkg_info[0].begins_with("Building"))
		check.set_meta("package_flag", pkg_info[1])
		check.toggled.connect(func(_pressed): _on_package_filter_changed(dialog))
		filter_container.add_child(check)

	# === Mesh Code Filter Section (for dataset mode) ===
	var mesh_filter_section = _create_section("Mesh Code Filter (地域)", options_vbox)
	mesh_filter_section.name = "MeshSection"

	var mesh_info_label = Label.new()
	mesh_info_label.name = "MeshInfoLabel"
	mesh_info_label.text = "Select a dataset to see available mesh codes"
	mesh_info_label.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
	mesh_filter_section.add_child(mesh_info_label)

	# Buttons for select all / deselect all
	var mesh_btn_hbox = HBoxContainer.new()
	var mesh_select_all_btn = Button.new()
	mesh_select_all_btn.name = "MeshSelectAllBtn"
	mesh_select_all_btn.text = "Select All"
	mesh_select_all_btn.disabled = true
	mesh_btn_hbox.add_child(mesh_select_all_btn)

	var mesh_deselect_all_btn = Button.new()
	mesh_deselect_all_btn.name = "MeshDeselectAllBtn"
	mesh_deselect_all_btn.text = "Deselect All"
	mesh_deselect_all_btn.disabled = true
	mesh_btn_hbox.add_child(mesh_deselect_all_btn)

	mesh_filter_section.add_child(mesh_btn_hbox)

	# Scrollable list for mesh codes
	var mesh_scroll = ScrollContainer.new()
	mesh_scroll.custom_minimum_size = Vector2(0, 120)
	mesh_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	mesh_filter_section.add_child(mesh_scroll)

	var mesh_grid = GridContainer.new()
	mesh_grid.name = "MeshCodeGrid"
	mesh_grid.columns = 4
	mesh_grid.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	mesh_scroll.add_child(mesh_grid)

	# Connect mesh code buttons
	mesh_select_all_btn.pressed.connect(func():
		for child in mesh_grid.get_children():
			if child is CheckBox:
				child.button_pressed = true
		_update_mesh_info_label(dialog)
	)

	mesh_deselect_all_btn.pressed.connect(func():
		for child in mesh_grid.get_children():
			if child is CheckBox:
				child.button_pressed = false
		_update_mesh_info_label(dialog)
	)

	# === Coordinate System Section ===
	var coord_section = _create_section("Coordinate System", options_vbox)

	var zone_desc = Label.new()
	zone_desc.text = "日本測地系2011の平面直角座標系ゾーン。データの地域に合わせて選択してください。"
	zone_desc.autowrap_mode = TextServer.AUTOWRAP_WORD
	zone_desc.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
	coord_section.add_child(zone_desc)

	var zone_hbox = HBoxContainer.new()

	var zone_label = Label.new()
	zone_label.text = "Zone ID:"
	zone_hbox.add_child(zone_label)

	var zone_dropdown = OptionButton.new()
	zone_dropdown.name = "ZoneDropdown"
	for zone_id in range(1, 20):
		if zone_id in ZONE_NAMES:
			zone_dropdown.add_item(ZONE_NAMES[zone_id], zone_id)
	zone_dropdown.select(8)  # Default: Zone 9 (Tokyo)
	zone_hbox.add_child(zone_dropdown)

	coord_section.add_child(zone_hbox)

	# Coordinate Axis
	var axis_hbox = HBoxContainer.new()

	var axis_label = Label.new()
	axis_label.text = "Axis:"
	axis_hbox.add_child(axis_label)

	var axis_dropdown = OptionButton.new()
	axis_dropdown.name = "AxisDropdown"
	# EUN (Godot), ENU, WUN の順で追加
	axis_dropdown.add_item(COORDINATE_SYSTEM_OPTIONS[3], 3)  # EUN
	axis_dropdown.add_item(COORDINATE_SYSTEM_OPTIONS[0], 0)  # ENU
	axis_dropdown.add_item(COORDINATE_SYSTEM_OPTIONS[1], 1)  # WUN
	axis_dropdown.select(0)  # Default: EUN (Godot標準)
	axis_hbox.add_child(axis_dropdown)

	coord_section.add_child(axis_hbox)

	# Transform Type
	var transform_hbox = HBoxContainer.new()

	var transform_label = Label.new()
	transform_label.text = "Transform:"
	transform_hbox.add_child(transform_label)

	var transform_dropdown = OptionButton.new()
	transform_dropdown.name = "TransformDropdown"
	transform_dropdown.add_item(TRANSFORM_TYPE_OPTIONS[0], 0)  # Local
	transform_dropdown.add_item(TRANSFORM_TYPE_OPTIONS[1], 1)  # PlaneCartesian
	transform_dropdown.select(0)  # Default: Local
	transform_hbox.add_child(transform_dropdown)

	coord_section.add_child(transform_hbox)

	var transform_desc = Label.new()
	transform_desc.text = "Local: ルート位置からの相対座標\nPlaneCartesian: 基準点からの絶対座標（GIS連携用）"
	transform_desc.autowrap_mode = TextServer.AUTOWRAP_WORD
	transform_desc.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
	coord_section.add_child(transform_desc)

	# === Mesh Options Section ===
	var mesh_options_section = _create_section("Mesh Options", options_vbox)

	# Granularity
	var granularity_hbox = HBoxContainer.new()
	var granularity_label = Label.new()
	granularity_label.text = "Granularity:"
	granularity_hbox.add_child(granularity_label)

	var granularity_dropdown = OptionButton.new()
	granularity_dropdown.name = "GranularityDropdown"
	for id in GRANULARITY_OPTIONS.keys():
		granularity_dropdown.add_item(GRANULARITY_OPTIONS[id], id)
	granularity_dropdown.select(1)  # Default: Per Primary Feature
	granularity_hbox.add_child(granularity_dropdown)

	mesh_options_section.add_child(granularity_hbox)

	# LOD Range
	var lod_hbox = HBoxContainer.new()
	var lod_range_label = Label.new()
	lod_range_label.text = "LOD Range:"
	lod_hbox.add_child(lod_range_label)

	var lod_min_label = Label.new()
	lod_min_label.text = "Min"
	lod_hbox.add_child(lod_min_label)

	var lod_min_spin = SpinBox.new()
	lod_min_spin.name = "LODMinSpin"
	lod_min_spin.min_value = 0
	lod_min_spin.max_value = 4
	lod_min_spin.value = 0
	lod_hbox.add_child(lod_min_spin)

	var lod_spacer = Control.new()
	lod_spacer.custom_minimum_size = Vector2(10, 0)
	lod_hbox.add_child(lod_spacer)

	var lod_max_label = Label.new()
	lod_max_label.text = "Max"
	lod_hbox.add_child(lod_max_label)

	var lod_max_spin = SpinBox.new()
	lod_max_spin.name = "LODMaxSpin"
	lod_max_spin.min_value = 0
	lod_max_spin.max_value = 4
	lod_max_spin.value = 4
	lod_hbox.add_child(lod_max_spin)

	mesh_options_section.add_child(lod_hbox)

	# Highest LOD Only
	var highest_lod_check = CheckBox.new()
	highest_lod_check.name = "HighestLODCheck"
	highest_lod_check.text = "Highest LOD Only (推奨)"
	highest_lod_check.button_pressed = true
	mesh_options_section.add_child(highest_lod_check)

	# === Texture Section ===
	var texture_section = _create_section("Texture", options_vbox)

	var texture_check = CheckBox.new()
	texture_check.name = "TextureCheck"
	texture_check.text = "Export Textures"
	texture_check.button_pressed = true
	texture_section.add_child(texture_check)

	var packing_hbox = HBoxContainer.new()
	var texture_packing_check = CheckBox.new()
	texture_packing_check.name = "TexturePackingCheck"
	texture_packing_check.text = "Enable Texture Packing"
	texture_packing_check.button_pressed = false
	packing_hbox.add_child(texture_packing_check)

	var packing_spacer = Control.new()
	packing_spacer.custom_minimum_size = Vector2(10, 0)
	packing_hbox.add_child(packing_spacer)

	var res_label = Label.new()
	res_label.text = "Resolution:"
	packing_hbox.add_child(res_label)

	var texture_res_dropdown = OptionButton.new()
	texture_res_dropdown.name = "TextureResDropdown"
	for res in TEXTURE_RESOLUTION_OPTIONS:
		texture_res_dropdown.add_item(str(res), res)
	texture_res_dropdown.select(2)  # Default: 2048
	texture_res_dropdown.disabled = true
	packing_hbox.add_child(texture_res_dropdown)

	texture_section.add_child(packing_hbox)

	# === Export Section ===
	var export_section = _create_section("Export (External)", options_vbox)

	var export_desc = Label.new()
	export_desc.text = "外部アプリやiOS/Android向けにglTF/GLB/OBJ形式で出力"
	export_desc.autowrap_mode = TextServer.AUTOWRAP_WORD
	export_desc.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
	export_section.add_child(export_desc)

	# Per-file export option (memory optimization)
	var per_file_check = CheckBox.new()
	per_file_check.name = "PerFileExportCheck"
	per_file_check.text = "GMLファイル単位でエクスポート (メモリ節約)"
	per_file_check.button_pressed = true
	export_section.add_child(per_file_check)

	var per_file_desc = Label.new()
	per_file_desc.text = "大量のGMLファイルを処理する際のメモリ不足を防止します"
	per_file_desc.autowrap_mode = TextServer.AUTOWRAP_WORD
	per_file_desc.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
	export_section.add_child(per_file_desc)

	# Format selection
	var format_hbox = HBoxContainer.new()
	var format_label = Label.new()
	format_label.text = "Format:"
	format_hbox.add_child(format_label)

	var format_dropdown = OptionButton.new()
	format_dropdown.name = "FormatDropdown"
	# GLB, glTF, OBJ の順で追加（GLBがデフォルト）
	format_dropdown.add_item(FORMAT_OPTIONS[1], 1)  # GLB
	format_dropdown.add_item(FORMAT_OPTIONS[0], 0)  # glTF
	format_dropdown.add_item(FORMAT_OPTIONS[2], 2)  # OBJ
	format_dropdown.select(0)  # Default: GLB (first item)
	format_hbox.add_child(format_dropdown)

	export_section.add_child(format_hbox)

	# Output path
	var output_path_hbox = HBoxContainer.new()
	var output_path = LineEdit.new()
	output_path.name = "OutputPath"
	output_path.placeholder_text = "Select output file..."
	output_path.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	output_path.editable = false
	output_path_hbox.add_child(output_path)

	var output_browse_btn = Button.new()
	output_browse_btn.name = "OutputBrowseButton"
	output_browse_btn.text = "Browse..."
	output_path_hbox.add_child(output_browse_btn)

	export_section.add_child(output_path_hbox)

	# Export button
	var export_btn = Button.new()
	export_btn.name = "ExportButton"
	export_btn.text = "Export to File"
	export_section.add_child(export_btn)

	# === Import Section ===
	var import_section = _create_section("Import (Project)", options_vbox)

	var import_desc = Label.new()
	import_desc.text = "PC向けにプロジェクト内に.tscnシーンとして取り込み"
	import_desc.autowrap_mode = TextServer.AUTOWRAP_WORD
	import_desc.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
	import_section.add_child(import_desc)

	# LOD auto-switch option
	var lod_switch_check = CheckBox.new()
	lod_switch_check.name = "LODSwitchCheck"
	lod_switch_check.text = "LOD自動切り替え (距離ベース)"
	lod_switch_check.button_pressed = true
	import_section.add_child(lod_switch_check)

	# LOD distance settings
	var lod_distance_hbox = HBoxContainer.new()
	lod_distance_hbox.name = "LODDistanceHBox"

	var lod_dist_label = Label.new()
	lod_dist_label.text = "LOD2距離:"
	lod_distance_hbox.add_child(lod_dist_label)

	var lod2_dist_spin = SpinBox.new()
	lod2_dist_spin.name = "LOD2DistSpin"
	lod2_dist_spin.min_value = 50
	lod2_dist_spin.max_value = 2000
	lod2_dist_spin.value = 200
	lod2_dist_spin.suffix = "m"
	lod_distance_hbox.add_child(lod2_dist_spin)

	var lod1_label = Label.new()
	lod1_label.text = " LOD1:"
	lod_distance_hbox.add_child(lod1_label)

	var lod1_dist_spin = SpinBox.new()
	lod1_dist_spin.name = "LOD1DistSpin"
	lod1_dist_spin.min_value = 100
	lod1_dist_spin.max_value = 5000
	lod1_dist_spin.value = 500
	lod1_dist_spin.suffix = "m"
	lod_distance_hbox.add_child(lod1_dist_spin)

	import_section.add_child(lod_distance_hbox)

	# Import output path
	var import_path_hbox = HBoxContainer.new()
	var import_path = LineEdit.new()
	import_path.name = "ImportPath"
	import_path.placeholder_text = "Select project folder... (res://)"
	import_path.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	import_path.editable = false
	import_path_hbox.add_child(import_path)

	var import_browse_btn = Button.new()
	import_browse_btn.name = "ImportBrowseButton"
	import_browse_btn.text = "Browse..."
	import_path_hbox.add_child(import_browse_btn)

	import_section.add_child(import_path_hbox)

	# Import button
	var import_btn = Button.new()
	import_btn.name = "ImportButton"
	import_btn.text = "Import to Project"
	import_section.add_child(import_btn)

	# Connect LOD switch checkbox
	lod_switch_check.toggled.connect(func(pressed):
		lod_distance_hbox.visible = pressed
	)

	# === Progress Section ===
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

	# Load saved settings
	var saved_settings = _load_settings()

	# Store file dialogs as metadata
	var input_folder_dialog = FileDialog.new()
	input_folder_dialog.name = "InputFolderDialog"
	input_folder_dialog.title = "Select Dataset Folder"
	input_folder_dialog.file_mode = FileDialog.FILE_MODE_OPEN_DIR
	input_folder_dialog.access = FileDialog.ACCESS_FILESYSTEM
	if saved_settings.has("last_input_folder"):
		input_folder_dialog.current_dir = saved_settings["last_input_folder"]
	dialog.add_child(input_folder_dialog)

	var input_file_dialog = FileDialog.new()
	input_file_dialog.name = "InputFileDialog"
	input_file_dialog.title = "Select GML File"
	input_file_dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	input_file_dialog.access = FileDialog.ACCESS_FILESYSTEM
	input_file_dialog.filters = PackedStringArray(["*.gml ; CityGML Files"])
	if saved_settings.has("last_input_file_folder"):
		input_file_dialog.current_dir = saved_settings["last_input_file_folder"]
	dialog.add_child(input_file_dialog)

	var output_file_dialog = FileDialog.new()
	output_file_dialog.name = "OutputFileDialog"
	output_file_dialog.title = "Save Export File"
	output_file_dialog.file_mode = FileDialog.FILE_MODE_SAVE_FILE
	output_file_dialog.access = FileDialog.ACCESS_FILESYSTEM
	if saved_settings.has("last_output_folder"):
		output_file_dialog.current_dir = saved_settings["last_output_folder"]
	dialog.add_child(output_file_dialog)

	# Import folder dialog (project-internal)
	var import_folder_dialog = FileDialog.new()
	import_folder_dialog.name = "ImportFolderDialog"
	import_folder_dialog.title = "Select Import Folder (Project)"
	import_folder_dialog.file_mode = FileDialog.FILE_MODE_OPEN_DIR
	import_folder_dialog.access = FileDialog.ACCESS_RESOURCES  # res:// only
	import_folder_dialog.current_dir = "res://"
	dialog.add_child(import_folder_dialog)

	# Connect signals
	var filter_section_parent = filter_section.get_parent().get_parent()
	var mesh_section_parent = mesh_filter_section.get_parent().get_parent()

	source_radio_dataset.toggled.connect(func(pressed):
		if pressed:
			source_radio_gml.button_pressed = false
			input_path.text = ""
			input_path.placeholder_text = "Select a dataset folder..."
			input_info_label.text = ""
			filter_section_parent.visible = true
			mesh_section_parent.visible = true
			_clear_mesh_code_grid(dialog)
	)

	source_radio_gml.toggled.connect(func(pressed):
		if pressed:
			source_radio_dataset.button_pressed = false
			input_path.text = ""
			input_path.placeholder_text = "Select a GML file..."
			input_info_label.text = ""
			filter_section_parent.visible = false
			mesh_section_parent.visible = false
	)

	browse_btn.pressed.connect(func():
		if source_radio_dataset.button_pressed:
			input_folder_dialog.popup_centered(Vector2i(800, 600))
		else:
			input_file_dialog.popup_centered(Vector2i(800, 600))
	)

	input_folder_dialog.dir_selected.connect(func(path):
		input_path.text = path
		_update_dataset_info(dialog, path)
		_save_setting("last_input_folder", path)
	)

	input_file_dialog.file_selected.connect(func(path):
		input_path.text = path
		var filename = path.get_file()
		_save_setting("last_input_file_folder", path.get_base_dir())

		# Try to detect zone from filename mesh code first
		var detected_zone = 0
		var regex = RegEx.new()
		regex.compile("^(\\d{4,8})")
		var result = regex.search(filename)
		if result:
			detected_zone = _estimate_zone_from_mesh_code(result.get_string(1))

		# If not detected, try parent folders
		if detected_zone == 0:
			detected_zone = _estimate_zone_from_path_hierarchy(path.get_base_dir())

		if detected_zone > 0:
			for i in range(zone_dropdown.item_count):
				if zone_dropdown.get_item_id(i) == detected_zone:
					zone_dropdown.select(i)
					break
			input_info_label.text = "Selected: %s (Zone %d detected)" % [filename, detected_zone]
			input_info_label.add_theme_color_override("font_color", Color(0.4, 0.8, 0.4))
		else:
			input_info_label.text = "Selected: " + filename
			input_info_label.remove_theme_color_override("font_color")
	)

	texture_packing_check.toggled.connect(func(pressed):
		texture_res_dropdown.disabled = not pressed
	)

	format_dropdown.item_selected.connect(func(_idx):
		_update_output_filter(dialog)
	)

	output_browse_btn.pressed.connect(func():
		_update_output_filter(dialog)
		output_file_dialog.popup_centered(Vector2i(800, 600))
	)

	output_file_dialog.file_selected.connect(func(path):
		output_path.text = path
		_save_setting("last_output_folder", path.get_base_dir())
	)

	# Connect import browse button
	import_browse_btn.pressed.connect(func():
		import_folder_dialog.popup_centered(Vector2i(800, 600))
	)

	import_folder_dialog.dir_selected.connect(func(path):
		import_path.text = path
	)

	# Connect import button
	import_btn.pressed.connect(func():
		_on_import_confirmed(dialog)
	)

	# Connect export button
	export_btn.pressed.connect(func():
		_on_export_confirmed(dialog)
	)

	return dialog


static func _create_section(title: String, parent: Control) -> VBoxContainer:
	var section = VBoxContainer.new()

	var label = Label.new()
	label.text = title + ":"
	section.add_child(label)

	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 16)
	section.add_child(margin)

	var content = VBoxContainer.new()
	margin.add_child(content)

	var spacer = Control.new()
	spacer.custom_minimum_size = Vector2(0, 8)
	section.add_child(spacer)

	parent.add_child(section)
	return content


static func _update_dataset_info(dialog: AcceptDialog, folder_path: String) -> void:
	var info_label: Label = dialog.find_child("InputInfoLabel", true, false)
	var zone_dropdown: OptionButton = dialog.find_child("ZoneDropdown", true, false)
	if not info_label:
		return

	var source = PLATEAUDatasetSource.create_local(folder_path)
	if not source.is_valid():
		info_label.text = "Invalid dataset folder"
		info_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
		dialog.set_meta("dataset_source", null)
		return

	# ソースをダイアログに保存
	dialog.set_meta("dataset_source", source)

	var gml_files = source.get_gml_files(PACKAGE_ALL)
	var count = gml_files.size()

	# Auto-detect zone
	var detected_zone = _estimate_zone_from_folder_name(folder_path)
	var detection_source = "folder"

	if detected_zone == 0 and gml_files.size() > 0:
		var mesh_code = gml_files[0].mesh_code
		if not mesh_code.is_empty():
			detected_zone = _estimate_zone_from_mesh_code(mesh_code)
			detection_source = "mesh code"

	if detected_zone == 0:
		detected_zone = 9
		detection_source = "default"

	if zone_dropdown:
		for i in range(zone_dropdown.item_count):
			if zone_dropdown.get_item_id(i) == detected_zone:
				zone_dropdown.select(i)
				break

	if detection_source == "default":
		info_label.text = "Dataset: %d GML file%s found" % [count, "s" if count != 1 else ""]
	else:
		info_label.text = "Dataset: %d GML file%s found (Zone %d detected)" % [count, "s" if count != 1 else "", detected_zone]
	info_label.add_theme_color_override("font_color", Color(0.4, 0.8, 0.4))

	# 選択されたパッケージでメッシュコードグリッドを更新
	var package_flags = _get_selected_package_flags(dialog)
	_update_mesh_code_grid(dialog, source, package_flags)


static func _get_selected_package_flags(dialog: AcceptDialog) -> int:
	var filter_container: VBoxContainer = dialog.find_child("FilterContainer", true, false)
	var flags: int = 0
	if filter_container:
		for child in filter_container.get_children():
			if child is CheckBox and child.button_pressed:
				flags |= child.get_meta("package_flag", 0)
	return flags if flags > 0 else PACKAGE_ALL


static func _on_package_filter_changed(dialog: AcceptDialog) -> void:
	var source = dialog.get_meta("dataset_source") if dialog.has_meta("dataset_source") else null
	if source == null or not source.is_valid():
		return

	var package_flags = _get_selected_package_flags(dialog)
	_update_mesh_code_grid(dialog, source, package_flags)


static func _clear_mesh_code_grid(dialog: AcceptDialog) -> void:
	var mesh_grid: GridContainer = dialog.find_child("MeshCodeGrid", true, false)
	var mesh_info_label: Label = dialog.find_child("MeshInfoLabel", true, false)
	var select_btn: Button = dialog.find_child("MeshSelectAllBtn", true, false)
	var deselect_btn: Button = dialog.find_child("MeshDeselectAllBtn", true, false)

	if mesh_grid:
		for child in mesh_grid.get_children():
			child.queue_free()

	if mesh_info_label:
		mesh_info_label.text = "Select a dataset to see available mesh codes"

	if select_btn:
		select_btn.disabled = true
	if deselect_btn:
		deselect_btn.disabled = true


static func _update_mesh_code_grid(dialog: AcceptDialog, source, package_flags: int = PACKAGE_ALL) -> void:
	var mesh_grid: GridContainer = dialog.find_child("MeshCodeGrid", true, false)
	var mesh_info_label: Label = dialog.find_child("MeshInfoLabel", true, false)
	var select_btn: Button = dialog.find_child("MeshSelectAllBtn", true, false)
	var deselect_btn: Button = dialog.find_child("MeshDeselectAllBtn", true, false)

	if not mesh_grid:
		return

	for child in mesh_grid.get_children():
		child.queue_free()

	# 選択されたパッケージに対応するGMLファイルからメッシュコードを抽出
	var gml_files = source.get_gml_files(package_flags)
	var mesh_code_set = {}
	for file_info in gml_files:
		var mc = file_info.mesh_code
		if not mc.is_empty():
			mesh_code_set[mc] = true

	var mesh_codes = mesh_code_set.keys()
	mesh_codes.sort()

	if mesh_codes.is_empty():
		if mesh_info_label:
			mesh_info_label.text = "No mesh codes found"
		return

	if select_btn:
		select_btn.disabled = false
	if deselect_btn:
		deselect_btn.disabled = false

	var max_display = 100
	var displayed = 0

	for mesh_code in mesh_codes:
		if displayed >= max_display:
			break

		var check = CheckBox.new()
		check.text = mesh_code
		check.button_pressed = true
		check.set_meta("mesh_code", mesh_code)
		check.toggled.connect(func(_pressed): _update_mesh_info_label(dialog))
		mesh_grid.add_child(check)
		displayed += 1

	_update_mesh_info_label(dialog)

	if mesh_codes.size() > max_display and mesh_info_label:
		mesh_info_label.text += " (showing first %d of %d)" % [max_display, mesh_codes.size()]


static func _update_mesh_info_label(dialog: AcceptDialog) -> void:
	var mesh_grid: GridContainer = dialog.find_child("MeshCodeGrid", true, false)
	var mesh_info_label: Label = dialog.find_child("MeshInfoLabel", true, false)

	if not mesh_grid or not mesh_info_label:
		return

	var total = 0
	var selected = 0
	for child in mesh_grid.get_children():
		if child is CheckBox:
			total += 1
			if child.button_pressed:
				selected += 1

	mesh_info_label.text = "%d / %d mesh codes selected" % [selected, total]


static func _update_output_filter(dialog: AcceptDialog) -> void:
	var format_dropdown: OptionButton = dialog.find_child("FormatDropdown", true, false)
	var output_file_dialog: FileDialog = dialog.find_child("OutputFileDialog", true, false)
	var output_path: LineEdit = dialog.find_child("OutputPath", true, false)
	if not format_dropdown or not output_file_dialog:
		return

	var format_id = format_dropdown.get_selected_id()
	var ext = PLATEAUMeshExporter.get_format_extension(format_id)
	var format_name = PLATEAUMeshExporter.get_supported_formats()[format_id]
	output_file_dialog.filters = PackedStringArray(["*." + ext + " ; " + format_name + " Files"])
	output_file_dialog.current_file = "export." + ext

	# 既存の出力パスがあれば拡張子を更新
	if output_path and not output_path.text.is_empty():
		var current_path = output_path.text
		var base_name = current_path.get_basename()
		output_path.text = base_name + "." + ext


static func _on_export_confirmed(dialog: AcceptDialog) -> void:
	var source_radio_dataset: CheckBox = dialog.find_child("SourceRadioDataset", true, false)
	var input_path: LineEdit = dialog.find_child("InputPath", true, false)
	var zone_dropdown: OptionButton = dialog.find_child("ZoneDropdown", true, false)
	var granularity_dropdown: OptionButton = dialog.find_child("GranularityDropdown", true, false)
	var lod_min_spin: SpinBox = dialog.find_child("LODMinSpin", true, false)
	var lod_max_spin: SpinBox = dialog.find_child("LODMaxSpin", true, false)
	var highest_lod_check: CheckBox = dialog.find_child("HighestLODCheck", true, false)
	var texture_check: CheckBox = dialog.find_child("TextureCheck", true, false)
	var texture_packing_check: CheckBox = dialog.find_child("TexturePackingCheck", true, false)
	var texture_res_dropdown: OptionButton = dialog.find_child("TextureResDropdown", true, false)
	var format_dropdown: OptionButton = dialog.find_child("FormatDropdown", true, false)
	var output_path: LineEdit = dialog.find_child("OutputPath", true, false)
	var progress_section: Control = dialog.find_child("ProgressSection", true, false)
	var progress_bar: ProgressBar = dialog.find_child("ProgressBar", true, false)
	var progress_label: Label = dialog.find_child("ProgressLabel", true, false)

	# Validation
	if input_path.text.is_empty():
		progress_section.visible = true
		progress_bar.value = 0
		progress_label.text = "Error: No input path specified"
		progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
		return

	if output_path.text.is_empty():
		progress_section.visible = true
		progress_bar.value = 0
		progress_label.text = "Error: No output path specified"
		progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
		return

	var axis_dropdown: OptionButton = dialog.find_child("AxisDropdown", true, false)
	var transform_dropdown: OptionButton = dialog.find_child("TransformDropdown", true, false)

	var per_file_check: CheckBox = dialog.find_child("PerFileExportCheck", true, false)

	var is_dataset = source_radio_dataset.button_pressed
	var zone_id = zone_dropdown.get_selected_id()
	var coordinate_system = axis_dropdown.get_selected_id()
	var transform_type = transform_dropdown.get_selected_id()
	var granularity = granularity_dropdown.get_selected_id()
	var lod_min = int(lod_min_spin.value)
	var lod_max = int(lod_max_spin.value)
	var highest_lod_only = highest_lod_check.button_pressed
	var export_textures = texture_check.button_pressed
	var texture_packing = texture_packing_check.button_pressed
	var texture_resolution = texture_res_dropdown.get_selected_id()
	var export_format = format_dropdown.get_selected_id()
	var per_file_export = per_file_check.button_pressed if per_file_check else true

	# Collect package filter flags and mesh codes
	var package_flags: int = 0
	var selected_mesh_codes: PackedStringArray = PackedStringArray()

	if is_dataset:
		var filter_container: VBoxContainer = dialog.find_child("FilterContainer", true, false)
		if filter_container:
			for child in filter_container.get_children():
				if child is CheckBox and child.button_pressed:
					package_flags |= child.get_meta("package_flag", 0)
		if package_flags == 0:
			progress_section.visible = true
			progress_bar.value = 0
			progress_label.text = "Error: No package type selected"
			progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
			return

		var mesh_grid: GridContainer = dialog.find_child("MeshCodeGrid", true, false)
		if mesh_grid:
			for child in mesh_grid.get_children():
				if child is CheckBox and child.button_pressed:
					selected_mesh_codes.append(child.get_meta("mesh_code", ""))
		if selected_mesh_codes.is_empty():
			progress_section.visible = true
			progress_bar.value = 0
			progress_label.text = "Error: No mesh code selected"
			progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
			return
	else:
		package_flags = PACKAGE_ALL

	# Show progress
	progress_section.visible = true
	progress_bar.value = 0
	progress_label.text = "Preparing..."
	progress_label.remove_theme_color_override("font_color")

	var export_btn: Button = dialog.find_child("ExportButton", true, false)
	if export_btn:
		export_btn.disabled = true

	print("[PLATEAU Converter] Starting export:")
	print("  Input: ", input_path.text)
	print("  Is Dataset: ", is_dataset)
	print("  Package Flags: ", package_flags)
	print("  Mesh Codes: ", selected_mesh_codes.size())
	print("  Zone: ", zone_id)
	print("  Coordinate System: ", coordinate_system)
	print("  Transform Type: ", transform_type)
	print("  Granularity: ", granularity)
	print("  LOD: ", lod_min, " - ", lod_max)
	print("  Highest LOD Only: ", highest_lod_only)
	print("  Textures: ", export_textures)
	print("  Texture Packing: ", texture_packing)
	print("  Output: ", output_path.text)
	print("  Format: ", export_format)
	print("  Per-file Export: ", per_file_export)

	_do_export.call_deferred(
		dialog, is_dataset, input_path.text, package_flags, selected_mesh_codes,
		zone_id, coordinate_system, transform_type, granularity, lod_min, lod_max,
		highest_lod_only, export_textures, texture_packing, texture_resolution,
		export_format, output_path.text, per_file_export, progress_bar, progress_label
	)


static func _do_export(
	dialog: AcceptDialog,
	is_dataset: bool,
	input_path: String,
	package_flags: int,
	selected_mesh_codes: PackedStringArray,
	zone_id: int,
	coordinate_system: int,
	transform_type: int,
	granularity: int,
	lod_min: int,
	lod_max: int,
	highest_lod_only: bool,
	export_textures: bool,
	texture_packing: bool,
	texture_resolution: int,
	export_format: int,
	output_path: String,
	per_file_export: bool,
	progress_bar: ProgressBar,
	progress_label: Label
) -> void:

	var start_time := Time.get_ticks_msec()
	var gml_paths: Array[String] = []

	if is_dataset:
		progress_label.text = "Scanning dataset folder..."
		var source = PLATEAUDatasetSource.create_local(input_path)
		if not source.is_valid():
			push_error("[PLATEAU Converter] Invalid dataset folder: " + input_path)
			_export_finished(dialog, false, progress_label)
			return

		# パッケージタイプでGMLファイルを取得
		var gml_files = source.get_gml_files(package_flags)

		# メッシュコードでフィルタリング（GDScript側で実行）
		for file_info in gml_files:
			var file_mesh_code = file_info.mesh_code
			if selected_mesh_codes.is_empty() or selected_mesh_codes.has(file_mesh_code):
				gml_paths.append(file_info.get_path())

		if gml_paths.is_empty():
			progress_label.text = "Error: No GML files match the selected filters"
			progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
			push_error("[PLATEAU Converter] No GML files found for selected package/mesh code combination")
			var export_btn_err: Button = dialog.find_child("ExportButton", true, false)
			if export_btn_err:
				export_btn_err.disabled = false
			return

		print("[PLATEAU Converter] Found %d GML files for selected filters" % gml_paths.size())
	else:
		gml_paths.append(input_path)

	progress_bar.value = 5

	var total_files = gml_paths.size()
	var total_meshes = 0
	var export_count = 0
	var success = true

	# Per-file export mode: export each GML to a separate file
	if per_file_export and total_files > 1:
		var output_dir = output_path.get_base_dir()
		var output_ext = output_path.get_extension()

		for i in range(total_files):
			var gml_path = gml_paths[i]
			var file_name = gml_path.get_file()
			progress_label.text = "Processing: %s (%d/%d)" % [file_name, i + 1, total_files]

			var mesh_data = _extract_meshes_from_gml(
				gml_path, zone_id, coordinate_system, transform_type, granularity,
				lod_min, lod_max, highest_lod_only, export_textures, texture_packing,
				texture_resolution
			)

			if mesh_data.is_empty():
				var progress = 5 + int(90.0 * (i + 1) / total_files)
				progress_bar.value = progress
				continue

			total_meshes += mesh_data.size()

			# Create output path for this file
			var file_output_name = file_name.get_basename() + "." + output_ext
			var file_output_path = output_dir.path_join(file_output_name)

			progress_label.text = "Exporting: %s (%d/%d)" % [file_name, i + 1, total_files]

			var exporter = PLATEAUMeshExporter.new()

			# Set texture directory
			var gml_dir = gml_path.get_base_dir()
			var parent_dir = gml_dir.get_base_dir()
			exporter.texture_directory = parent_dir

			var file_success = exporter.export_to_file(mesh_data, file_output_path, export_format)

			if file_success:
				export_count += 1
				print("[PLATEAU Converter] Exported: %s (%d meshes)" % [file_output_path, mesh_data.size()])
			else:
				push_error("[PLATEAU Converter] Failed to export: " + file_output_path)
				success = false

			# Clear references to release memory
			mesh_data.clear()
			exporter = null

			var progress = 5 + int(90.0 * (i + 1) / total_files)
			progress_bar.value = progress

		progress_bar.value = 100

		var elapsed := (Time.get_ticks_msec() - start_time) / 1000.0

		if export_count > 0:
			print("[PLATEAU Converter] Export completed in %.2f seconds" % elapsed)
			print("[PLATEAU Converter] Total files: %d, Total meshes: %d" % [export_count, total_meshes])
			print("[PLATEAU Converter] Output directory: %s" % output_dir)
		else:
			push_error("[PLATEAU Converter] No files exported")
			success = false

	else:
		# Single file export mode (original behavior)
		var all_mesh_data: Array = []

		for i in range(total_files):
			var gml_path = gml_paths[i]
			var file_name = gml_path.get_file()
			progress_label.text = "Processing: %s (%d/%d)" % [file_name, i + 1, total_files]

			var mesh_data = _extract_meshes_from_gml(
				gml_path, zone_id, coordinate_system, transform_type, granularity,
				lod_min, lod_max, highest_lod_only, export_textures, texture_packing,
				texture_resolution
			)

			for md in mesh_data:
				all_mesh_data.append(md)

			var progress = 5 + int(75.0 * (i + 1) / total_files)
			progress_bar.value = progress

		if all_mesh_data.is_empty():
			push_warning("[PLATEAU Converter] No meshes extracted from GML files")

		progress_bar.value = 85
		progress_label.text = "Exporting to file..."

		var exporter = PLATEAUMeshExporter.new()

		# Set texture directory
		if gml_paths.size() > 0:
			var gml_dir = gml_paths[0].get_base_dir()
			var parent_dir = gml_dir.get_base_dir()
			exporter.texture_directory = parent_dir

		success = exporter.export_to_file(all_mesh_data, output_path, export_format)

		progress_bar.value = 100

		var elapsed := (Time.get_ticks_msec() - start_time) / 1000.0

		if success:
			print("[PLATEAU Converter] Export completed in %.2f seconds" % elapsed)
			print("[PLATEAU Converter] Total meshes: %d" % all_mesh_data.size())
			print("[PLATEAU Converter] Output: %s" % output_path)
		else:
			push_error("[PLATEAU Converter] Export failed")

	_export_finished(dialog, success, progress_label)


static func _extract_meshes_from_gml(
	gml_path: String,
	zone_id: int,
	coordinate_system: int,
	transform_type: int,
	granularity: int,
	lod_min: int,
	lod_max: int,
	highest_lod_only: bool,
	export_textures: bool,
	texture_packing: bool,
	texture_resolution: int
) -> Array:

	var result: Array = []

	var city_model = PLATEAUCityModel.new()
	if not city_model.load(gml_path):
		push_error("[PLATEAU Converter] Failed to load: " + gml_path)
		return result

	var options = PLATEAUMeshExtractOptions.new()
	options.coordinate_zone_id = zone_id
	# coordinate_systemはデフォルト（EUN=3）を使用
	# options.coordinate_system = coordinate_system

	# Transform Type: 0=Local (相対座標), 1=PlaneCartesian (絶対座標)
	if transform_type == 0:
		# Local: モデル中心を原点とした相対座標
		# get_center_point()は緯度・経度を返すので、平面直角座標に変換する
		var center_latlon = city_model.get_center_point(zone_id)
		var geo_ref = PLATEAUGeoReference.new()
		geo_ref.zone_id = zone_id
		options.reference_point = geo_ref.project(center_latlon)
	else:
		# PlaneCartesian: 平面直角座標系の基準点からの絶対座標
		options.reference_point = Vector3.ZERO

	options.mesh_granularity = granularity
	options.min_lod = lod_min
	options.max_lod = lod_max
	options.highest_lod_only = highest_lod_only
	options.export_appearance = export_textures
	options.enable_texture_packing = texture_packing
	options.unit_scale = 1.0  # メートル単位
	options.texture_packing_resolution = texture_resolution

	var extracted = city_model.extract_meshes(options)

	var all_meshes: Array = []
	for root_mesh in extracted:
		_flatten_mesh_data(root_mesh, all_meshes)

	# 重複除去: 同じ名前のメッシュがある場合、頂点数が多い方を保持
	var mesh_by_name: Dictionary = {}
	for md in all_meshes:
		var name = md.get_name()
		var mesh = md.get_mesh()
		var vertex_count = 0
		if mesh != null:
			for surf_idx in range(mesh.get_surface_count()):
				var arrays = mesh.surface_get_arrays(surf_idx)
				if arrays.size() > Mesh.ARRAY_VERTEX:
					var vertices = arrays[Mesh.ARRAY_VERTEX] as PackedVector3Array
					vertex_count += vertices.size()

		if name in mesh_by_name:
			var existing = mesh_by_name[name]
			if vertex_count > existing["vertex_count"]:
				mesh_by_name[name] = {"mesh_data": md, "vertex_count": vertex_count}
		else:
			mesh_by_name[name] = {"mesh_data": md, "vertex_count": vertex_count}

	for name in mesh_by_name:
		result.append(mesh_by_name[name]["mesh_data"])

	return result


static func _flatten_mesh_data(mesh_data, result: Array) -> void:
	if mesh_data.get_mesh() != null:
		result.append(mesh_data)

	for child in mesh_data.get_children():
		_flatten_mesh_data(child, result)


static func _export_finished(dialog: AcceptDialog, success: bool, progress_label: Label) -> void:
	var export_btn: Button = dialog.find_child("ExportButton", true, false)
	if export_btn:
		export_btn.disabled = false

	if success:
		progress_label.text = "Export completed!"
		progress_label.add_theme_color_override("font_color", Color(0.4, 0.8, 0.4))
	else:
		progress_label.text = "Export failed. Check console for details."
		progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))


static func _on_import_confirmed(dialog: AcceptDialog) -> void:
	var source_radio_dataset: CheckBox = dialog.find_child("SourceRadioDataset", true, false)
	var input_path: LineEdit = dialog.find_child("InputPath", true, false)
	var zone_dropdown: OptionButton = dialog.find_child("ZoneDropdown", true, false)
	var granularity_dropdown: OptionButton = dialog.find_child("GranularityDropdown", true, false)
	var lod_min_spin: SpinBox = dialog.find_child("LODMinSpin", true, false)
	var lod_max_spin: SpinBox = dialog.find_child("LODMaxSpin", true, false)
	var highest_lod_check: CheckBox = dialog.find_child("HighestLODCheck", true, false)
	var texture_check: CheckBox = dialog.find_child("TextureCheck", true, false)
	var texture_packing_check: CheckBox = dialog.find_child("TexturePackingCheck", true, false)
	var texture_res_dropdown: OptionButton = dialog.find_child("TextureResDropdown", true, false)
	var import_path: LineEdit = dialog.find_child("ImportPath", true, false)
	var lod_switch_check: CheckBox = dialog.find_child("LODSwitchCheck", true, false)
	var lod2_dist_spin: SpinBox = dialog.find_child("LOD2DistSpin", true, false)
	var lod1_dist_spin: SpinBox = dialog.find_child("LOD1DistSpin", true, false)
	var progress_section: Control = dialog.find_child("ProgressSection", true, false)
	var progress_bar: ProgressBar = dialog.find_child("ProgressBar", true, false)
	var progress_label: Label = dialog.find_child("ProgressLabel", true, false)

	# Validation
	if input_path.text.is_empty():
		progress_section.visible = true
		progress_bar.value = 0
		progress_label.text = "Error: No input path specified"
		progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
		return

	if import_path.text.is_empty():
		progress_section.visible = true
		progress_bar.value = 0
		progress_label.text = "Error: No import folder specified"
		progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
		return

	var axis_dropdown: OptionButton = dialog.find_child("AxisDropdown", true, false)
	var transform_dropdown: OptionButton = dialog.find_child("TransformDropdown", true, false)

	var is_dataset = source_radio_dataset.button_pressed
	var zone_id = zone_dropdown.get_selected_id()
	var coordinate_system = axis_dropdown.get_selected_id()
	var transform_type = transform_dropdown.get_selected_id()
	var granularity = granularity_dropdown.get_selected_id()
	var lod_min = int(lod_min_spin.value)
	var lod_max = int(lod_max_spin.value)
	var highest_lod_only = highest_lod_check.button_pressed
	var export_textures = texture_check.button_pressed
	var texture_packing = texture_packing_check.button_pressed
	var texture_resolution = texture_res_dropdown.get_selected_id()
	var lod_auto_switch = lod_switch_check.button_pressed
	var lod2_distance = lod2_dist_spin.value
	var lod1_distance = lod1_dist_spin.value

	# Collect package filter flags and mesh codes
	var package_flags: int = 0
	var selected_mesh_codes: PackedStringArray = PackedStringArray()

	if is_dataset:
		var filter_container: VBoxContainer = dialog.find_child("FilterContainer", true, false)
		if filter_container:
			for child in filter_container.get_children():
				if child is CheckBox and child.button_pressed:
					package_flags |= child.get_meta("package_flag", 0)
		if package_flags == 0:
			progress_section.visible = true
			progress_bar.value = 0
			progress_label.text = "Error: No package type selected"
			progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
			return

		var mesh_grid: GridContainer = dialog.find_child("MeshCodeGrid", true, false)
		if mesh_grid:
			for child in mesh_grid.get_children():
				if child is CheckBox and child.button_pressed:
					selected_mesh_codes.append(child.get_meta("mesh_code", ""))
		if selected_mesh_codes.is_empty():
			progress_section.visible = true
			progress_bar.value = 0
			progress_label.text = "Error: No mesh code selected"
			progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
			return
	else:
		package_flags = PACKAGE_ALL

	# Show progress
	progress_section.visible = true
	progress_bar.value = 0
	progress_label.text = "Preparing import..."
	progress_label.remove_theme_color_override("font_color")

	var import_btn: Button = dialog.find_child("ImportButton", true, false)
	if import_btn:
		import_btn.disabled = true

	print("[PLATEAU Converter] Starting import:")
	print("  Input: ", input_path.text)
	print("  Output: ", import_path.text)
	print("  Is Dataset: ", is_dataset)
	print("  Zone: ", zone_id)
	print("  Granularity: ", granularity)
	print("  LOD: ", lod_min, " - ", lod_max)
	print("  Highest LOD Only: ", highest_lod_only)
	print("  Textures: ", export_textures)
	print("  LOD Auto Switch: ", lod_auto_switch)

	_do_import.call_deferred(
		dialog, is_dataset, input_path.text, package_flags, selected_mesh_codes,
		zone_id, coordinate_system, transform_type, granularity, lod_min, lod_max,
		highest_lod_only, export_textures, texture_packing, texture_resolution,
		import_path.text, lod_auto_switch, lod2_distance, lod1_distance,
		progress_bar, progress_label
	)


static func _do_import(
	dialog: AcceptDialog,
	is_dataset: bool,
	input_path: String,
	package_flags: int,
	selected_mesh_codes: PackedStringArray,
	zone_id: int,
	coordinate_system: int,
	transform_type: int,
	granularity: int,
	lod_min: int,
	lod_max: int,
	highest_lod_only: bool,
	export_textures: bool,
	texture_packing: bool,
	texture_resolution: int,
	import_folder: String,
	lod_auto_switch: bool,
	lod2_distance: float,
	lod1_distance: float,
	progress_bar: ProgressBar,
	progress_label: Label
) -> void:

	var start_time := Time.get_ticks_msec()
	var gml_paths: Array[String] = []

	if is_dataset:
		progress_label.text = "Scanning dataset folder..."
		var source = PLATEAUDatasetSource.create_local(input_path)
		if not source.is_valid():
			push_error("[PLATEAU Converter] Invalid dataset folder: " + input_path)
			_import_finished(dialog, false, progress_label)
			return

		var gml_files = source.get_gml_files(package_flags)

		for file_info in gml_files:
			var file_mesh_code = file_info.mesh_code
			if selected_mesh_codes.is_empty() or selected_mesh_codes.has(file_mesh_code):
				gml_paths.append(file_info.get_path())

		if gml_paths.is_empty():
			progress_label.text = "Error: No GML files match the selected filters"
			progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
			var import_btn_err: Button = dialog.find_child("ImportButton", true, false)
			if import_btn_err:
				import_btn_err.disabled = false
			return

		print("[PLATEAU Converter] Found %d GML files for selected filters" % gml_paths.size())
	else:
		gml_paths.append(input_path)

	progress_bar.value = 5

	# Create output directories
	var scene_name = input_path.get_file().get_basename() if not is_dataset else "plateau_import"
	var output_base = import_folder.path_join(scene_name)
	var meshes_dir = output_base.path_join("meshes")
	var textures_dir = output_base.path_join("textures")

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(meshes_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(textures_dir))

	print("[PLATEAU Converter] Output directories:")
	print("  Base: ", output_base)
	print("  Meshes: ", meshes_dir)
	print("  Textures: ", textures_dir)

	# Create root node
	var root = PLATEAUInstancedCityModel.new()
	root.name = scene_name
	root.zone_id = zone_id

	# Set LOD Auto-Switch settings
	root.lod_auto_switch_enabled = lod_auto_switch
	root.lod2_distance = lod2_distance
	root.lod1_distance = lod1_distance
	root.lod_disable_in_editor = true

	# Track copied textures to avoid duplicates
	var copied_textures: Dictionary = {}

	var total_files = gml_paths.size()
	var first_file = true
	var reference_point = Vector3.ZERO

	for i in range(total_files):
		var gml_path = gml_paths[i]
		var file_name = gml_path.get_file()
		progress_label.text = "Processing: %s (%d/%d)" % [file_name, i + 1, total_files]

		var city_model = PLATEAUCityModel.new()
		city_model.log_level = PLATEAUCityModel.LOG_LEVEL_NONE
		if not city_model.load(gml_path):
			push_error("[PLATEAU Converter] Failed to load: " + gml_path)
			continue

		var options = PLATEAUMeshExtractOptions.new()
		options.coordinate_zone_id = zone_id

		if first_file:
			if transform_type == 0:  # Local
				var center_latlon = city_model.get_center_point(zone_id)
				var geo_ref = PLATEAUGeoReference.new()
				geo_ref.zone_id = zone_id
				reference_point = geo_ref.project(center_latlon)
			else:
				reference_point = Vector3.ZERO

			root.set_reference_point(reference_point)

			# Store latitude/longitude in root
			var center = city_model.get_center_point(zone_id)
			var geo_ref = PLATEAUGeoReference.new()
			geo_ref.zone_id = zone_id
			var latlon = geo_ref.unproject(reference_point)
			# Note: get_center_point returns Vector3(lat, lon, 0) in some implementations
			first_file = false

		options.reference_point = reference_point
		options.mesh_granularity = granularity
		options.min_lod = lod_min
		options.max_lod = lod_max
		options.highest_lod_only = highest_lod_only
		options.export_appearance = export_textures
		options.enable_texture_packing = texture_packing
		options.texture_packing_resolution = texture_resolution
		options.unit_scale = 1.0

		var extracted = city_model.extract_meshes(options)

		# Create GML transform node
		var gml_node = Node3D.new()
		gml_node.name = file_name.get_basename()
		root.add_child(gml_node)
		gml_node.owner = root

		# Process extracted meshes and organize by LOD
		var lod_meshes: Dictionary = {}  # LOD number -> Array of mesh data

		for root_mesh in extracted:
			_collect_meshes_by_lod(root_mesh, lod_meshes)

		# Create LOD nodes and save meshes
		var lod_keys = lod_meshes.keys()
		lod_keys.sort()

		for lod in lod_keys:
			var lod_node = Node3D.new()
			lod_node.name = "LOD%d" % lod
			gml_node.add_child(lod_node)
			lod_node.owner = root

			var mesh_array = lod_meshes[lod]
			for j in range(mesh_array.size()):
				var mesh_data = mesh_array[j]
				var mesh = mesh_data.get_mesh()
				if mesh == null:
					continue

				# Save mesh as .res file
				var mesh_name = mesh_data.get_name()
				if mesh_name.is_empty():
					mesh_name = "mesh_%d" % mesh_data.get_instance_id()

				var mesh_filename = "%s_LOD%d_%s.res" % [file_name.get_basename(), lod, mesh_name.replace("/", "_")]
				var mesh_path = meshes_dir.path_join(mesh_filename)

				# Copy textures if needed
				if export_textures:
					_copy_mesh_data_textures(mesh_data, mesh, gml_path, textures_dir, copied_textures)

				ResourceSaver.save(mesh, mesh_path)

				# Get transform before clearing reference
				var mesh_transform = mesh_data.get_transform()

				# Clear reference to release memory
				mesh_array[j] = null

				# Create MeshInstance3D with loaded mesh (new reference)
				var mi = MeshInstance3D.new()
				mi.name = mesh_name.replace("/", "_")
				mi.mesh = load(mesh_path)
				mi.transform = mesh_transform

				# Note: LOD visibility ranges are applied later via root.apply_lod_settings()
				# This allows LOD settings to be changed in the Inspector after import

				lod_node.add_child(mi)
				mi.owner = root

			# Clear array after processing each LOD
			lod_meshes[lod].clear()

		# Clear all references for this GML file to release memory
		lod_meshes.clear()
		extracted.clear()
		city_model = null

		var progress = 5 + int(85.0 * (i + 1) / total_files)
		progress_bar.value = progress

	# Apply LOD settings to all MeshInstance3D nodes
	progress_label.text = "Applying LOD settings..."
	root.apply_lod_settings()

	# Save the scene
	progress_label.text = "Saving scene..."
	progress_bar.value = 95

	var packed_scene = PackedScene.new()
	var result = packed_scene.pack(root)

	if result != OK:
		push_error("[PLATEAU Converter] Failed to pack scene: " + str(result))
		_import_finished(dialog, false, progress_label)
		root.queue_free()
		return

	var scene_path = output_base.path_join(scene_name + ".tscn")
	result = ResourceSaver.save(packed_scene, scene_path)

	if result != OK:
		push_error("[PLATEAU Converter] Failed to save scene: " + str(result))
		_import_finished(dialog, false, progress_label)
		root.queue_free()
		return

	root.queue_free()

	progress_bar.value = 100

	var elapsed := (Time.get_ticks_msec() - start_time) / 1000.0

	print("[PLATEAU Converter] Import completed in %.2f seconds" % elapsed)
	print("[PLATEAU Converter] Scene saved to: %s" % scene_path)
	print("[PLATEAU Converter] Textures copied: %d" % copied_textures.size())

	_import_finished(dialog, true, progress_label)


static func _collect_meshes_by_lod(mesh_data, lod_meshes: Dictionary, current_lod: int = -1) -> void:
	var name = mesh_data.get_name()

	# Try to parse LOD from name
	var lod = current_lod
	var lod_regex = RegEx.new()
	lod_regex.compile("LOD(\\d+)")
	var match_result = lod_regex.search(name)
	if match_result:
		lod = match_result.get_string(1).to_int()

	# If this mesh has geometry, add it
	if mesh_data.get_mesh() != null:
		var target_lod = lod if lod >= 0 else 0
		if not lod_meshes.has(target_lod):
			lod_meshes[target_lod] = []
		lod_meshes[target_lod].append(mesh_data)

	# Process children
	for child in mesh_data.get_children():
		_collect_meshes_by_lod(child, lod_meshes, lod)


static func _copy_mesh_data_textures(mesh_data, mesh: ArrayMesh, gml_path: String, textures_dir: String, copied_textures: Dictionary) -> void:
	if mesh == null:
		return

	var gml_dir = gml_path.get_base_dir()

	# Get texture paths from mesh_data (stored during extraction)
	var texture_paths = mesh_data.get_texture_paths()

	for surface_idx in range(mesh.get_surface_count()):
		var mat = mesh.surface_get_material(surface_idx)
		if mat == null or not mat is StandardMaterial3D:
			continue

		var std_mat = mat as StandardMaterial3D
		var tex = std_mat.albedo_texture

		if tex == null:
			continue

		# Get texture path from mesh_data or from resource
		var tex_path = ""
		if surface_idx < texture_paths.size():
			tex_path = texture_paths[surface_idx]
		if tex_path.is_empty():
			tex_path = tex.resource_path

		if tex_path.is_empty() or tex_path.begins_with("res://"):
			continue

		# Already copied?
		if copied_textures.has(tex_path):
			# Update material to use copied texture
			var cached_tex = load(copied_textures[tex_path])
			if cached_tex:
				std_mat.albedo_texture = cached_tex
			continue

		# Resolve texture path (may be relative to GML)
		var src_global = tex_path
		if not tex_path.is_absolute_path():
			# Try relative to GML directory
			src_global = gml_dir.path_join(tex_path)
			# Also try parent directories (PLATEAU data structure: udx/bldg/xxx.gml, appearance/xxx.jpg)
			if not FileAccess.file_exists(src_global):
				var parent_dir = gml_dir.get_base_dir()
				src_global = parent_dir.path_join(tex_path)
			if not FileAccess.file_exists(src_global):
				var grandparent_dir = gml_dir.get_base_dir().get_base_dir()
				src_global = grandparent_dir.path_join(tex_path)

		if not FileAccess.file_exists(src_global):
			push_warning("[PLATEAU Converter] Texture not found: " + tex_path)
			continue

		# Copy texture file
		var tex_filename = tex_path.get_file()

		# Avoid filename collision by adding hash if needed
		var dest_path = textures_dir.path_join(tex_filename)
		var dest_global = ProjectSettings.globalize_path(dest_path)

		if FileAccess.file_exists(dest_global):
			# File exists, check if it's the same
			var existing_hash = FileAccess.get_md5(dest_global)
			var new_hash = FileAccess.get_md5(src_global)
			if existing_hash != new_hash:
				# Different file, add suffix
				var base = tex_filename.get_basename()
				var ext = tex_filename.get_extension()
				tex_filename = "%s_%d.%s" % [base, tex_path.hash(), ext]
				dest_path = textures_dir.path_join(tex_filename)
				dest_global = ProjectSettings.globalize_path(dest_path)

		if not FileAccess.file_exists(dest_global):
			var err = DirAccess.copy_absolute(src_global, dest_global)
			if err != OK:
				push_error("[PLATEAU Converter] Failed to copy texture: " + src_global + " -> " + dest_global)
				continue
			print("[PLATEAU Converter] Copied texture: ", tex_filename)

		copied_textures[tex_path] = dest_path

		# Update material to use copied texture
		var new_tex = load(dest_path)
		if new_tex:
			std_mat.albedo_texture = new_tex


static func _import_finished(dialog: AcceptDialog, success: bool, progress_label: Label) -> void:
	var import_btn: Button = dialog.find_child("ImportButton", true, false)
	if import_btn:
		import_btn.disabled = false

	if success:
		progress_label.text = "Import completed!"
		progress_label.add_theme_color_override("font_color", Color(0.4, 0.8, 0.4))

		# Refresh the FileSystem dock to show new files
		if Engine.is_editor_hint():
			var editor_interface = Engine.get_singleton("EditorInterface")
			if editor_interface:
				editor_interface.get_resource_filesystem().scan()
	else:
		progress_label.text = "Import failed. Check console for details."
		progress_label.add_theme_color_override("font_color", Color(1.0, 0.4, 0.4))
