## PLATEAUPortable - Runtime helper for baked PLATEAU scenes (.scn/.tscn)
##
## Works WITHOUT the PLATEAU GDExtension and without libplateau.
## All lookups use plain node metadata baked at import time by
## PLATEAUImporter (bake_metadata=true) or PLATEAUImporter.import_to_portable_scene().
##
## Typical workflow:
##   1. Desktop/editor: import GML with PLATEAUImporter, then
##      PLATEAUPortable.save_scene(root, "res://city.scn")
##   2. Mobile: load("res://city.scn").instantiate() and use this helper for
##      filtering, attribute access and raycast -> gml_id lookup.
##
## Baked metadata keys (per MeshInstance3D):
##   "gml_id": String            - CityGML ID of the feature
##   "city_object_type": int     - PLATEAUCityObjectType bit flag
##   "attributes": Dictionary    - CityGML attributes (nested Dictionaries)
##   "city_objects": Dictionary  - {Vector2i(primary, atomic): gml_id} UV2 lookup table
## Root metadata keys (portable scenes):
##   "plateau_portable", "plateau_zone_id", "plateau_reference_point",
##   "plateau_unit_scale", "plateau_coordinate_system", "plateau_min_lod",
##   "plateau_max_lod", "plateau_mesh_granularity", "plateau_gml_path"
class_name PLATEAUPortable
extends RefCounted

# CityObject types (matches PLATEAUCityObjectType in the extension /
# citygml::CityObject::CityObjectsType). Defined here so filtering works
# without the extension.
const COT_GENERIC_CITY_OBJECT := 1
const COT_BUILDING := 1 << 1
const COT_ROOM := 1 << 2
const COT_BUILDING_INSTALLATION := 1 << 3
const COT_BUILDING_FURNITURE := 1 << 4
const COT_DOOR := 1 << 5
const COT_WINDOW := 1 << 6
const COT_CITY_FURNITURE := 1 << 7
const COT_TRACK := 1 << 8
const COT_ROAD := 1 << 9
const COT_RAILWAY := 1 << 10
const COT_SQUARE := 1 << 11
const COT_PLANT_COVER := 1 << 12
const COT_SOLITARY_VEGETATION_OBJECT := 1 << 13
const COT_WATER_BODY := 1 << 14
const COT_RELIEF_FEATURE := 1 << 15
const COT_LAND_USE := 1 << 16
const COT_TUNNEL := 1 << 17
const COT_BRIDGE := 1 << 18
const COT_BRIDGE_CONSTRUCTION_ELEMENT := 1 << 19
const COT_BRIDGE_INSTALLATION := 1 << 20
const COT_BRIDGE_PART := 1 << 21
const COT_BUILDING_PART := 1 << 22
const COT_WALL_SURFACE := 1 << 23
const COT_ROOF_SURFACE := 1 << 24
const COT_GROUND_SURFACE := 1 << 25
const COT_CLOSURE_SURFACE := 1 << 26
const COT_FLOOR_SURFACE := 1 << 27
const COT_INTERIOR_WALL_SURFACE := 1 << 28
const COT_CEILING_SURFACE := 1 << 29
const COT_CITY_OBJECT_GROUP := 1 << 30
const COT_OUTER_CEILING_SURFACE := 1 << 31
const COT_OUTER_FLOOR_SURFACE := 1 << 32
const COT_TRANSPORTATION_OBJECT := 1 << 33
const COT_INT_BUILDING_INSTALLATION := 1 << 34
const COT_WATER_SURFACE := 1 << 35
const COT_RELIEF_COMPONENT := 1 << 36
const COT_TIN_RELIEF := 1 << 37
const COT_MASS_POINT_RELIEF := 1 << 38
const COT_BREAKLINE_RELIEF := 1 << 39
const COT_RASTER_RELIEF := 1 << 40
const COT_UNKNOWN := 1 << 41

## Building-related types (building itself, parts and boundary surfaces)
const MASK_BUILDING_ALL := COT_BUILDING | COT_BUILDING_PART | COT_BUILDING_INSTALLATION \
	| COT_WALL_SURFACE | COT_ROOF_SURFACE | COT_GROUND_SURFACE | COT_CLOSURE_SURFACE \
	| COT_OUTER_CEILING_SURFACE | COT_OUTER_FLOOR_SURFACE
## Terrain-related types
const MASK_RELIEF_ALL := COT_RELIEF_FEATURE | COT_RELIEF_COMPONENT | COT_TIN_RELIEF \
	| COT_MASS_POINT_RELIEF | COT_BREAKLINE_RELIEF | COT_RASTER_RELIEF
## Everything
const MASK_ALL := (1 << 42) - 1


## Save a (portable) scene root as .scn/.tscn. Assigns ownership of the whole
## subtree to root so every node is serialized. Textures created at import time
## have no resource path, so they are embedded into the saved file automatically.
## Binary .scn is recommended for large city scenes.
static func save_scene(root: Node, path: String) -> Error:
	if root == null:
		return ERR_INVALID_PARAMETER
	_set_owner_recursive(root, root)
	var packed := PackedScene.new()
	var err := packed.pack(root)
	if err != OK:
		return err
	return ResourceSaver.save(packed, path)


## True if root was created by PLATEAUImporter.import_to_portable_scene()
static func is_portable_scene(root: Node) -> bool:
	return root != null and root.has_meta("plateau_portable")


# --- Per-node accessors (work on any baked scene) ---

static func get_gml_id(node: Node) -> String:
	return String(node.get_meta("gml_id", "")) if node else ""


static func get_city_object_type(node: Node) -> int:
	return int(node.get_meta("city_object_type", 0)) if node else 0


static func get_attributes(node: Node) -> Dictionary:
	if node == null:
		return {}
	var attrs: Dictionary = node.get_meta("attributes", {})
	return attrs


## Get attribute value by key. Nested attribute sets can be addressed with "/"
## (e.g. "uro:buildingDetails/uro:totalFloorArea"). Returns null when missing.
static func get_attribute(node: Node, key: String) -> Variant:
	var current: Variant = get_attributes(node)
	for part in key.split("/"):
		if current is Dictionary and current.has(part):
			current = current[part]
		else:
			return null
	return current


# --- Filtering ---

## Show only features whose city_object_type matches type_mask.
## Nodes without baked type metadata are left untouched (containers, LOD nodes).
## Returns the number of nodes whose visibility was updated.
static func filter_by_type(root: Node, type_mask: int) -> int:
	var count := 0
	for node in _walk(root):
		var n3d := node as Node3D
		if n3d and n3d.has_meta("city_object_type"):
			n3d.visible = (int(n3d.get_meta("city_object_type")) & type_mask) != 0
			count += 1
	return count


## Filter with a custom predicate: predicate.call(node) -> bool (true = visible).
## Only nodes carrying PLATEAU metadata are passed to the predicate.
## Useful for attribute-based filtering, e.g.:
##   PLATEAUPortable.filter_custom(root, func(n):
##       return int(PLATEAUPortable.get_attribute(n, "bldg:storeysAboveGround")) >= 10)
static func filter_custom(root: Node, predicate: Callable) -> int:
	var count := 0
	for node in _walk(root):
		var n3d := node as Node3D
		if n3d and (n3d.has_meta("gml_id") or n3d.has_meta("city_object_type")):
			n3d.visible = bool(predicate.call(n3d))
			count += 1
	return count


## Make every filtered node visible again.
static func show_all(root: Node) -> void:
	for node in _walk(root):
		var n3d := node as Node3D
		if n3d and (n3d.has_meta("gml_id") or n3d.has_meta("city_object_type")):
			n3d.visible = true


## Find the MeshInstance3D carrying the given gml_id (linear search).
static func find_by_gml_id(root: Node, gml_id: String) -> Node:
	for node in _walk(root):
		if node.has_meta("gml_id") and String(node.get_meta("gml_id")) == gml_id:
			return node
	return null


# --- Raycast -> feature lookup ---

## Resolve a gml_id from a UV2 value (CityObjectIndex is baked into UV2).
## Falls back to the primary feature when the atomic entry is missing.
static func get_gml_id_from_uv2(node: Node, uv2: Vector2) -> String:
	if node == null or not node.has_meta("city_objects"):
		return ""
	var map: Dictionary = node.get_meta("city_objects")
	var key := Vector2i(roundi(uv2.x), roundi(uv2.y))
	if map.has(key):
		return String(map[key])
	# Primary fallback (atomic_index = -1)
	var primary_key := Vector2i(key.x, -1)
	if map.has(primary_key):
		return String(map[primary_key])
	return ""


## Resolve a gml_id from a physics raycast hit against collision generated by
## PLATEAUImporter (generate_collision=true). face_index is the value from the
## raycast result dictionary; faces are concatenated over surfaces in order,
## matching the collision shape layout.
static func get_gml_id_from_face(mesh_instance: MeshInstance3D, face_index: int) -> String:
	if mesh_instance == null or mesh_instance.mesh == null or face_index < 0:
		return ""
	var mesh := mesh_instance.mesh
	var remaining := face_index
	for s in mesh.get_surface_count():
		var arrays := mesh.surface_get_arrays(s)
		if arrays.is_empty():
			continue
		var indices: PackedInt32Array = arrays[Mesh.ARRAY_INDEX]
		var vertices: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
		@warning_ignore("integer_division")
		var face_count := indices.size() / 3 if indices.size() > 0 else vertices.size() / 3
		if remaining < face_count:
			var uv2s: PackedVector2Array = arrays[Mesh.ARRAY_TEX_UV2]
			if uv2s.is_empty():
				return ""
			var vertex_index := indices[remaining * 3] if indices.size() > 0 else remaining * 3
			if vertex_index >= uv2s.size():
				return ""
			return get_gml_id_from_uv2(mesh_instance, uv2s[vertex_index])
		remaining -= face_count
	return ""


# --- Root (geo reference) accessors for portable scenes ---

static func get_zone_id(root: Node) -> int:
	return int(root.get_meta("plateau_zone_id", 0)) if root else 0


static func get_reference_point(root: Node) -> Vector3:
	if root == null:
		return Vector3.ZERO
	var point: Vector3 = root.get_meta("plateau_reference_point", Vector3.ZERO)
	return point


static func get_unit_scale(root: Node) -> float:
	return float(root.get_meta("plateau_unit_scale", 1.0)) if root else 1.0


static func get_coordinate_system(root: Node) -> int:
	return int(root.get_meta("plateau_coordinate_system", 0)) if root else 0


static func get_gml_path(root: Node) -> String:
	return String(root.get_meta("plateau_gml_path", "")) if root else ""


# --- Internal ---

static func _walk(node: Node) -> Array[Node]:
	var result: Array[Node] = []
	if node == null:
		return result
	var stack: Array[Node] = [node]
	while not stack.is_empty():
		var current: Node = stack.pop_back()
		result.append(current)
		for child in current.get_children():
			stack.push_back(child)
	return result


static func _set_owner_recursive(node: Node, owner: Node) -> void:
	if node != owner:
		node.owner = owner
	for child in node.get_children():
		_set_owner_recursive(child, owner)
