@tool
extends EditorPlugin

## PLATEAU SDK Editor Plugin
## Provides GUI for importing CityGML data from PLATEAU into Godot.

const PLUGIN_NAME := "PLATEAU SDK"
const IMPORT_DIALOG_SCRIPT := preload("res://addons/plateau/plateau_import_dialog.gd")
const EXPORT_DIALOG_SCRIPT := preload("res://addons/plateau/plateau_export_dialog.gd")

var dock_panel: Control
var import_dialog: AcceptDialog
var export_dialog: AcceptDialog
var file_dialog: FileDialog

func _enter_tree() -> void:
	# Create the dock panel
	dock_panel = _create_dock_panel()
	add_control_to_dock(EditorPlugin.DOCK_SLOT_RIGHT_UL, dock_panel)

	# Create file dialog for GML selection
	file_dialog = FileDialog.new()
	file_dialog.title = "Select CityGML File"
	file_dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	file_dialog.access = FileDialog.ACCESS_FILESYSTEM
	file_dialog.filters = PackedStringArray(["*.gml ; CityGML Files"])
	file_dialog.file_selected.connect(_on_file_selected)
	add_child(file_dialog)

	# Add tool menu item
	add_tool_menu_item("PLATEAU Exporter...", _on_exporter_menu_pressed)

	print("[PLATEAU] Plugin enabled")

func _exit_tree() -> void:
	# Clean up
	remove_tool_menu_item("PLATEAU Exporter...")

	if dock_panel:
		remove_control_from_docks(dock_panel)
		dock_panel.queue_free()

	if file_dialog:
		file_dialog.queue_free()

	if import_dialog:
		import_dialog.queue_free()

	if export_dialog:
		export_dialog.queue_free()

	print("[PLATEAU] Plugin disabled")

func _create_dock_panel() -> Control:
	var panel = MarginContainer.new()
	panel.name = "PLATEAU"
	panel.custom_minimum_size = Vector2(200, 100)

	# Main container
	var vbox = VBoxContainer.new()
	panel.add_child(vbox)

	# Title
	var title_label = Label.new()
	title_label.text = "PLATEAU SDK"
	title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	var title_settings = LabelSettings.new()
	title_settings.font_size = 18
	title_label.label_settings = title_settings
	vbox.add_child(title_label)

	# Separator
	var sep1 = HSeparator.new()
	vbox.add_child(sep1)

	# Description
	var desc_label = Label.new()
	desc_label.text = "Import 3D city models from\nPLATEAU CityGML data."
	desc_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	desc_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	vbox.add_child(desc_label)

	# Spacer
	var spacer = Control.new()
	spacer.custom_minimum_size = Vector2(0, 10)
	vbox.add_child(spacer)

	# Import button
	var import_btn = Button.new()
	import_btn.text = "Import CityGML..."
	import_btn.pressed.connect(_on_import_pressed)
	vbox.add_child(import_btn)

	# Spacer
	var spacer2 = Control.new()
	spacer2.custom_minimum_size = Vector2(0, 10)
	vbox.add_child(spacer2)

	# Quick actions section
	var actions_label = Label.new()
	actions_label.text = "Quick Actions"
	var actions_settings = LabelSettings.new()
	actions_settings.font_size = 14
	actions_label.label_settings = actions_settings
	vbox.add_child(actions_label)

	var sep2 = HSeparator.new()
	vbox.add_child(sep2)

	# Add GeoReference button
	var georef_btn = Button.new()
	georef_btn.text = "Add GeoReference"
	georef_btn.pressed.connect(_on_add_georef_pressed)
	vbox.add_child(georef_btn)

	return panel

func _on_import_pressed() -> void:
	file_dialog.popup_centered(Vector2i(800, 600))

func _on_file_selected(path: String) -> void:
	# Open import dialog
	_show_import_dialog(path)

func _show_import_dialog(gml_path: String) -> void:
	if import_dialog:
		import_dialog.queue_free()

	import_dialog = IMPORT_DIALOG_SCRIPT.create_dialog(gml_path, get_editor_interface())
	add_child(import_dialog)
	import_dialog.popup_centered(Vector2i(500, 600))

func _on_add_georef_pressed() -> void:
	var editor_interface = get_editor_interface()
	var edited_scene_root = editor_interface.get_edited_scene_root()

	if edited_scene_root == null:
		push_error("[PLATEAU] No scene is currently open")
		return

	# Create GeoReference node
	var geo_ref = PLATEAUGeoReference.new()
	geo_ref.name = "PLATEAUGeoReference"
	geo_ref.coordinate_zone_id = 9  # Default: Tokyo area

	edited_scene_root.add_child(geo_ref)
	geo_ref.owner = edited_scene_root

	print("[PLATEAU] Added GeoReference node (Zone: 9)")

func _on_exporter_menu_pressed() -> void:
	if export_dialog:
		export_dialog.queue_free()
	export_dialog = EXPORT_DIALOG_SCRIPT.create_dialog()
	add_child(export_dialog)
	export_dialog.popup_centered(Vector2i(550, 700))
