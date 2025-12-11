extends Node3D
## Basemap Sample
##
## Demonstrates map tile download features:
## - Download map tiles from GSI (Geospatial Information Authority of Japan)
## - Download tiles from OpenStreetMap
## - Create combined texture from multiple tiles
## - Display tiles as ground plane
##
## Usage:
## 1. Set the center coordinates (latitude/longitude)
## 2. Select tile source and zoom level
## 3. Download and display map tiles

@export var center_latitude: float = 35.6812  ## Tokyo Station
@export var center_longitude: float = 139.7671
@export var zoom_level: int = 16
@export var tile_range: int = 2  ## Number of tiles around center (NxN grid)

var downloader: PLATEAUVectorTileDownloader
var downloaded_tiles: Array[PLATEAUVectorTile] = []
var ground_mesh: MeshInstance3D
var combined_texture: ImageTexture

# For async downloads
var pending_downloads: Array[Dictionary] = []
var current_http: HTTPRequest = null

@onready var camera: Camera3D = $Camera3D
@onready var log_label: RichTextLabel = $UI/LogPanel/ScrollContainer/LogLabel
@onready var preview_rect: TextureRect = $UI/PreviewPanel/PreviewTexture


func _ready() -> void:
	$UI/ControlPanel/DownloadGSIButton.pressed.connect(_on_download_gsi)
	$UI/ControlPanel/DownloadOSMButton.pressed.connect(_on_download_osm)
	$UI/ControlPanel/ApplyToGroundButton.pressed.connect(_on_apply_to_ground)
	$UI/ControlPanel/SaveTextureButton.pressed.connect(_on_save_texture)

	$UI/ControlPanel/LatSpin.value = center_latitude
	$UI/ControlPanel/LonSpin.value = center_longitude
	$UI/ControlPanel/ZoomSpin.value = zoom_level
	$UI/ControlPanel/RangeSpin.value = tile_range

	_log("Basemap Sample ready.")
	_log("Default: Tokyo Station (35.6812, 139.7671)")
	_log("Select source and click Download.")


func _on_download_gsi() -> void:
	_download_tiles(0)  # GSI_PHOTO = 0


func _on_download_osm() -> void:
	_download_tiles(3)  # OSM = 3


func _download_tiles(source: int) -> void:
	_log("--- Downloading Tiles ---")

	center_latitude = $UI/ControlPanel/LatSpin.value
	center_longitude = $UI/ControlPanel/LonSpin.value
	zoom_level = int($UI/ControlPanel/ZoomSpin.value)
	tile_range = int($UI/ControlPanel/RangeSpin.value)

	var source_name = "GSI Photo" if source == 0 else "OpenStreetMap"
	_log("Source: " + source_name)
	_log("Center: %.6f, %.6f" % [center_latitude, center_longitude])
	_log("Zoom: " + str(zoom_level) + ", Range: " + str(tile_range))

	# Create downloader for URL generation
	downloader = PLATEAUVectorTileDownloader.new()
	downloader.destination = "user://map_tiles"
	downloader.tile_source = source
	downloader.zoom_level = zoom_level

	# Calculate center tile coordinates
	var center_tile = _lat_lon_to_tile(center_latitude, center_longitude, zoom_level)
	_log("Center tile: " + str(center_tile.x) + ", " + str(center_tile.y))

	# Build download queue
	downloaded_tiles.clear()
	pending_downloads.clear()

	for dx in range(-tile_range, tile_range + 1):
		for dy in range(-tile_range, tile_range + 1):
			var tx = center_tile.x + dx
			var ty = center_tile.y + dy

			var coord = PLATEAUTileCoordinate.new()
			coord.column = tx
			coord.row = ty
			coord.zoom_level = zoom_level

			var url = downloader.get_tile_url(coord)
			var file_path = downloader.get_tile_file_path(coord)

			pending_downloads.append({
				"coord": coord,
				"url": url,
				"file_path": file_path
			})

	var total = pending_downloads.size()
	_log("Queued " + str(total) + " tiles for download")

	# Start downloading
	_download_next_tile()


func _download_next_tile() -> void:
	if pending_downloads.is_empty():
		_log("All downloads complete!")
		_on_downloads_finished()
		return

	var item = pending_downloads.pop_front()
	var coord: PLATEAUTileCoordinate = item["coord"]
	var url: String = item["url"]
	var file_path: String = item["file_path"]

	# Ensure directory exists
	var dir_path = file_path.get_base_dir()
	DirAccess.make_dir_recursive_absolute(dir_path)

	# Check if file already exists (cache)
	if FileAccess.file_exists(file_path):
		_log("Cached: " + str(coord.column) + "/" + str(coord.row))
		var tile = PLATEAUVectorTile.new()
		tile.set_coordinate(coord)
		tile.set_image_path(file_path)
		tile.set_success(true)
		downloaded_tiles.append(tile)
		_download_next_tile()
		return

	# Create HTTP request
	if current_http != null:
		current_http.queue_free()

	current_http = HTTPRequest.new()
	current_http.download_file = file_path
	add_child(current_http)

	# Store metadata for callback
	current_http.set_meta("coord", coord)
	current_http.set_meta("file_path", file_path)

	current_http.request_completed.connect(_on_http_completed)

	_log("Downloading: " + str(coord.column) + "/" + str(coord.row))
	var error = current_http.request(url)
	if error != OK:
		_log("[color=red]HTTP request error: " + str(error) + "[/color]")
		_download_next_tile()


func _on_http_completed(result: int, response_code: int, _headers: PackedStringArray, _body: PackedByteArray) -> void:
	var coord: PLATEAUTileCoordinate = current_http.get_meta("coord")
	var file_path: String = current_http.get_meta("file_path")

	var tile = PLATEAUVectorTile.new()
	tile.set_coordinate(coord)
	tile.set_image_path(file_path)

	if result == HTTPRequest.RESULT_SUCCESS and response_code == 200:
		tile.set_success(true)
		_log("[color=green]OK[/color]: " + str(coord.column) + "/" + str(coord.row))
	else:
		tile.set_success(false)
		_log("[color=red]Failed[/color]: " + str(coord.column) + "/" + str(coord.row) + " (code: " + str(response_code) + ")")

	downloaded_tiles.append(tile)
	_download_next_tile()


func _on_downloads_finished() -> void:
	var success_count = 0
	for tile in downloaded_tiles:
		if tile.is_success():
			success_count += 1

	_log("Success: " + str(success_count) + "/" + str(downloaded_tiles.size()))

	if downloaded_tiles.is_empty():
		_log("[color=red]ERROR: No tiles downloaded[/color]")
		return

	# Create combined texture
	_log("Creating combined texture...")
	combined_texture = downloader.create_combined_texture(downloaded_tiles)

	if combined_texture != null:
		preview_rect.texture = combined_texture
		_log("[color=green]Tiles downloaded successfully[/color]")
	else:
		_log("[color=yellow]WARNING: Could not create combined texture[/color]")
		# Show first tile as preview
		if downloaded_tiles.size() > 0 and downloaded_tiles[0].is_success():
			var first_texture = downloaded_tiles[0].load_texture()
			if first_texture != null:
				preview_rect.texture = first_texture


func _on_apply_to_ground() -> void:
	if combined_texture == null:
		_log("[color=red]ERROR: Download tiles first[/color]")
		return

	_log("--- Applying to Ground ---")

	# Remove existing ground
	if ground_mesh != null:
		ground_mesh.queue_free()

	# Calculate ground size based on tile coverage
	var tiles_per_side = (tile_range * 2 + 1)
	var meters_per_tile = _get_meters_per_tile(center_latitude, zoom_level)
	var ground_size = meters_per_tile * tiles_per_side

	_log("Ground size: %.1f x %.1f meters" % [ground_size, ground_size])

	# Create ground plane
	var plane_mesh = PlaneMesh.new()
	plane_mesh.size = Vector2(ground_size, ground_size)
	plane_mesh.subdivide_width = 1
	plane_mesh.subdivide_depth = 1

	# Create material with texture
	var material = StandardMaterial3D.new()
	material.albedo_texture = combined_texture
	material.uv1_scale = Vector3(1, 1, 1)

	ground_mesh = MeshInstance3D.new()
	ground_mesh.mesh = plane_mesh
	ground_mesh.material_override = material
	ground_mesh.name = "GroundMesh"
	add_child(ground_mesh)

	# Position camera
	camera.position = Vector3(0, ground_size * 0.8, ground_size * 0.5)
	camera.look_at(Vector3.ZERO)

	_log("[color=green]Ground mesh created[/color]")


func _on_save_texture() -> void:
	if combined_texture == null:
		_log("[color=red]ERROR: Download tiles first[/color]")
		return

	PLATEAUUtils.show_save_file_dialog(
		self,
		PackedStringArray(["*.png ; PNG Image"]),
		"basemap.png",
		_do_save_texture
	)


func _do_save_texture(path: String) -> void:
	var image = combined_texture.get_image()
	if image != null and image.save_png(path) == OK:
		_log("[color=green]Texture saved: " + path + "[/color]")
	else:
		_log("[color=red]ERROR: Failed to save texture[/color]")


func _lat_lon_to_tile(lat: float, lon: float, zoom: int) -> Vector2i:
	## Convert latitude/longitude to tile coordinates
	var n = pow(2.0, zoom)
	var x = int((lon + 180.0) / 360.0 * n)
	var lat_rad = deg_to_rad(lat)
	var y = int((1.0 - asinh(tan(lat_rad)) / PI) / 2.0 * n)
	return Vector2i(x, y)


func _get_meters_per_tile(lat: float, zoom: int) -> float:
	## Calculate meters per tile at given latitude and zoom level
	var earth_circumference = 40075016.686  # meters
	var tiles_at_zoom = pow(2.0, zoom)
	var meters_per_tile_equator = earth_circumference / tiles_at_zoom
	return meters_per_tile_equator * cos(deg_to_rad(lat))


func _log(message: String) -> void:
	var timestamp = Time.get_time_string_from_system()
	log_label.text += "[" + timestamp + "] " + message + "\n"
	await get_tree().process_frame
	var scroll = $UI/LogPanel/ScrollContainer as ScrollContainer
	scroll.scroll_vertical = scroll.get_v_scroll_bar().max_value
	print(message)
