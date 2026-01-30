<p align="center">
  <img src="project/addons/plateau/icon.png" alt="PLATEAU SDK for Godot logo" width="128" height="128">
</p>

<h1 align="center">PLATEAU SDK for Godot</h1>

<p align="center">
  A GDExtension for Godot Engine 4.x that enables loading 3D city models (CityGML) provided by Japan's Ministry of Land, Infrastructure, Transport and Tourism (MLIT).
</p>

<p align="center">
  <a href="./README.ja.md">日本語版 README</a>
</p>

## Features

- Load CityGML files
- Generate and display 3D meshes
- LOD (Level of Detail) selection
- Coordinate conversion (Geographic ↔ Local coordinates)
- Texture loading with caching
- PBR material support (Metallic/Roughness/Emission)
- Smooth shading with auto-generated normals
- Attribute information access (GML ID, CityObjectType, attributes)
- Raycast-based CityObject identification via UV coordinates
- Dynamic tile loading based on camera distance
- Road network data structures (lanes, intersections, sidewalks)
- CityObjectType hierarchy with Japanese display names
- GML file metadata access (grid code, EPSG, feature type)
- Structured city model import with PLATEAUInstancedCityModel

## Classes

### PLATEAUImporter (Node3D)
Import CityGML files and place them in the scene.

```gdscript
var importer = PLATEAUImporter.new()
importer.gml_path = "path/to/your.gml"
importer.import_gml()
add_child(importer)
```

### PLATEAUCityModel (RefCounted)
Load CityGML files and extract meshes.

```gdscript
var city_model = PLATEAUCityModel.new()
city_model.load("path/to/your.gml")

var options = PLATEAUMeshExtractOptions.new()
options.min_lod = 1
options.max_lod = 2

var mesh_data_array = city_model.extract_meshes(options)
```

### PLATEAUMeshData (RefCounted)
Represents extracted mesh data with attribute information.

```gdscript
# Access mesh and transform
var mesh: ArrayMesh = mesh_data.get_mesh()
var transform: Transform3D = mesh_data.get_transform()

# Access CityObject attributes
var gml_id: String = mesh_data.get_gml_id()
var type: int = mesh_data.get_city_object_type()
var type_name: String = mesh_data.get_city_object_type_name()
var attributes: Dictionary = mesh_data.get_attributes()

# Get specific attribute (supports nested keys with "/" separator)
var building_id = mesh_data.get_attribute("bldg:buildingID")
var height = mesh_data.get_attribute("bldg:measuredHeight")

# Get GML ID from raycast hit UV (for CityObject identification)
var gml_id_from_hit = mesh_data.get_gml_id_from_uv(hit_uv)
```

### PLATEAUGeoReference (RefCounted)
Coordinate conversion between geographic and local coordinates.

```gdscript
var geo_ref = PLATEAUGeoReference.new()
geo_ref.zone_id = 9  # Tokyo
var local_pos = geo_ref.project(Vector3(35.6762, 139.6503, 0))  # lat, lon, height
```

### PLATEAUMeshExtractOptions (RefCounted)
Options for mesh extraction (LOD, granularity, textures, etc.).

```gdscript
var options = PLATEAUMeshExtractOptions.new()
options.min_lod = 0
options.max_lod = 3
options.mesh_granularity = 1  # 0=atomic, 1=primary, 2=area
options.export_appearance = true
```

### PLATEAUCityModelScene (Node3D)
Root node for imported PLATEAU city model scenes. Manages GeoReference and imported GML transforms.

```gdscript
var scene = PLATEAUCityModelScene.new()
scene.geo_reference = geo_ref
add_child(scene)
scene.import_gml("path/to/file.gml", import_options)

# Get latitude/longitude of origin
print("Origin: ", scene.latitude, ", ", scene.longitude)

# Get all GML transforms
for gml_transform in scene.get_gml_transforms():
    print(gml_transform.name)
```

### PLATEAUFilterCondition (Resource)
Filter condition for city objects by type, LOD, and package.

```gdscript
var filter = PLATEAUFilterCondition.new()
filter.city_object_types = COT_Building | COT_Road
filter.min_lod = 1
filter.max_lod = 2

if filter.matches(mesh_data):
    # Process this mesh
    pass
```

### PLATEAUCityObjectTypeHierarchy (RefCounted)
Static hierarchy of city object types with Japanese display names.

```gdscript
var hierarchy = PLATEAUCityObjectTypeHierarchy.new()

# Get type display name in Japanese
var name = PLATEAUCityObjectTypeHierarchy.get_type_display_name(COT_Building)
# Returns "建築物"

# Convert type to package
var package = PLATEAUCityObjectTypeHierarchy.type_to_package(COT_RoofSurface)
# Returns PACKAGE_BUILDING
```

### PLATEAUDynamicTileManager (Node3D)
Dynamic tile loading manager based on camera distance. Automatically loads/unloads tiles.

```gdscript
var manager = PLATEAUDynamicTileManager.new()
add_child(manager)

# Set load distances per zoom level (min_distance, max_distance)
manager.set_load_distance(11, Vector2(-10000, 500))   # High detail, close range
manager.set_load_distance(10, Vector2(500, 1500))    # Medium detail
manager.set_load_distance(9, Vector2(1500, 10000))   # Low detail, far range

# Initialize with metadata store
manager.initialize(meta_store)
manager.tile_base_path = "res://tiles/"
manager.camera = $Camera3D
manager.auto_update = true

# Signals
manager.tile_loaded.connect(_on_tile_loaded)
manager.tile_unloaded.connect(_on_tile_unloaded)
```

### PLATEAUGmlFile (RefCounted)
GML file information and utilities for accessing file metadata.

```gdscript
var gml = PLATEAUGmlFile.create("C:/path/to/udx/bldg/53394601_bldg_6697_op.gml")
if gml.is_valid():
    print("Grid code: ", gml.get_grid_code())        # 53394601
    print("Feature type: ", gml.get_feature_type())  # bldg
    print("EPSG: ", gml.get_epsg())                  # 6697
    print("Dataset root: ", gml.get_dataset_root_path())
    print("Max LOD: ", gml.get_max_lod())

    # Get texture paths referenced in GML
    var textures = gml.search_image_paths()
    for tex_path in textures:
        print("Texture: ", tex_path)

    # Get geographic extent of grid code
    var extent = gml.get_grid_extent()
    print("Lat: ", extent.min_lat, " - ", extent.max_lat)
```

### PLATEAUInstancedCityModel (Node3D)
Root node for imported PLATEAU city model. Holds metadata and provides access to child nodes.

```gdscript
# Created by PLATEAUImporter.import_to_scene()
var city_model: PLATEAUInstancedCityModel = importer.import_to_scene(
    mesh_data_array, "MyCity", geo_reference, options, gml_path
)
add_child(city_model)

# Access metadata
print("Zone ID: ", city_model.zone_id)
print("Latitude: ", city_model.get_latitude())
print("Longitude: ", city_model.get_longitude())
print("GML Path: ", city_model.gml_path)

# Get GeoReference for coordinate conversion
var geo_ref = city_model.get_geo_reference()

# Access child transforms
for gml_transform in city_model.get_gml_transforms():
    print("GML: ", gml_transform.name)
    var lods = city_model.get_lods(gml_transform)
    print("Available LODs: ", lods)
```

### PLATEAURnModel (RefCounted)
Root container for road network data including roads, intersections, and sidewalks.

```gdscript
var model = PLATEAURnModel.new()

# Add roads and intersections
var road = PLATEAURnRoad.new()
model.add_road(road)

var intersection = PLATEAURnIntersection.new()
model.add_intersection(intersection)

# Build from mesh data (road package)
var model2 = PLATEAURnModel.create_from_mesh_data(road_mesh_data_array)

# Generate visualization mesh
var mesh = model.generate_mesh()
```

## Samples

The `project/samples/` directory contains example scenes demonstrating SDK features.

### Attributes Display Sample (`attributes_display_sample.tscn`)
Demonstrates how to display CityObject attributes when clicking on buildings.
- Load CityGML file and extract meshes
- Left-click on a building to show its attributes
- Displays GML ID, CityObjectType, and all attributes

### Attributes Color Sample (`attributes_color_sample.tscn`)
Demonstrates how to color-code buildings based on their attributes.
- Color by building usage (`bldg:usage`)
- Color by CityObjectType
- Color by flood risk (if available in data)

### API Sample (`api_sample.tscn`)
Demonstrates the main PLATEAU SDK APIs with dataset folder selection.
- **Dataset Selection**: Select PLATEAU dataset folder, choose packages and mesh codes
- **Import**: Load multiple GML files with various options (LOD, granularity, textures)
- **Export**: Save meshes to glTF/GLB/OBJ formats
- **Granularity Conversion**: Convert between Atomic/Primary/Area mesh levels

### API Sample Simple (`api_sample_simple.tscn`)
Simplified version of API Sample for single GML file import.
- **Single File Import**: Select and load one CityGML file
- **Export**: Save meshes to glTF/GLB/OBJ formats
- **Granularity Conversion**: Convert between mesh granularity levels
- Uses PLATEAUInstancedCityModel for scene organization

### Terrain Sample (`terrain_sample.tscn`)
Demonstrates terrain-related features.
- Load terrain GML (DEM/TIN data)
- Generate heightmap from terrain mesh
- Align buildings to terrain height
- Save heightmap as PNG image

### Basemap Sample (`basemap_sample.tscn`)
Demonstrates map tile download features.
- Download map tiles from GSI (Geospatial Information Authority of Japan)
- Download tiles from OpenStreetMap
- Create combined texture from multiple tiles
- Apply texture to ground mesh

### Using the Samples

1. Open the sample project in Godot
2. Run one of the sample scenes
3. Use "Import GML..." or "Load..." buttons to select your CityGML file
4. Interact with the UI to test various features

**Note**: You need to prepare your own PLATEAU CityGML data. Download from [G空間情報センター](https://www.geospatial.jp/ckan/dataset/plateau).

## Editor Plugin

### PLATEAU Converter

An editor tool for importing CityGML into your project and exporting to external formats.

**Opening the Tool**: Tool menu → PLATEAU Converter...

#### Import to Project

Import CityGML files as `.tscn` scenes into your Godot project.

- **Scene Structure**: Creates `PLATEAUInstancedCityModel` as root node
- **Mesh Resources**: Saves meshes as separate `.res` files for better performance
- **Texture Handling**: Copies textures from external PLATEAU dataset to project folder
- **LOD Auto-Switch**: Optional distance-based LOD switching using `visibility_range`

Usage:
1. Click "Import GML..." to select a CityGML file
2. Configure import options (LOD range, granularity, textures)
3. Enable "LOD auto-switch" if you want distance-based LOD switching
4. Click "Import to Project" and select output folder
5. The scene is saved as `.tscn` with mesh resources in a `meshes/` subfolder

#### Export to File

Export loaded meshes to external 3D formats for use in other applications.

- **Supported Formats**: glTF (.gltf), GLB (.glb), OBJ (.obj)
- **Use Cases**: External 3D software, mobile apps, web viewers

Usage:
1. Load a CityGML file using "Import GML..."
2. Select export format (glTF/GLB/OBJ)
3. Click "Export to File" and choose destination

## Building

### Requirements

- Python 3.8+
- SCons 4.0+
- CMake 3.17+
- Visual Studio 2022 (Windows) / Ninja + Xcode (macOS)
- Git

#### Linux Additional Requirements

Install the following packages:

```bash
# Ubuntu/Debian
sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev
```

#### macOS Additional Requirements

Install the following Homebrew packages:

```bash
brew install mesa-glu xz libdeflate ninja
```

### Build with SCons (Recommended)

SCons automatically builds libplateau and godot-plateau together.

#### 1. Clone repository

```bash
git clone --recursive https://github.com/shiena/godot-plateau.git
cd godot-plateau
```

#### 2. Build

```bash
# Windows (template_debug)
scons platform=windows target=template_debug

# Windows (template_release)
scons platform=windows target=template_release

# macOS
scons platform=macos target=template_release

# Linux
scons platform=linux target=template_release

# Android (requires ANDROID_HOME or ANDROID_SDK_ROOT environment variable)
scons platform=android arch=arm64 target=template_release

# iOS (requires macOS with Xcode)
scons platform=ios arch=arm64 target=template_release
```

The built library will be copied to `bin/<platform>/` and `project/addons/plateau/<platform>/`.

#### Build Options

```bash
# Skip libplateau build (if already built)
scons platform=windows target=template_release skip_libplateau_build=yes

# macOS: Custom Homebrew prefix (default: /opt/homebrew)
scons platform=macos target=template_release homebrew_prefix=/usr/local

# Linux: Use Clang instead of GCC (for GCC 15+ environments)
scons platform=linux target=template_release use_clang=yes
```

#### Android Build Requirements

- `ANDROID_HOME` or `ANDROID_SDK_ROOT` environment variable set to Android SDK path
- Android NDK installed (default version: 23.1.7779620)
- Ninja build tool

```bash
# Example with custom NDK version
scons platform=android arch=arm64 target=template_release ndk_version=23.1.7779620
```

#### iOS Build Requirements

- macOS with Xcode and Command Line Tools installed
- Ninja build tool (`brew install ninja`)

### Build with CMake (Alternative)

#### 1. Clone repository

```bash
git clone --recursive https://github.com/shiena/godot-plateau.git
cd godot-plateau
```

#### 2. Build libplateau (first time only)

```bash
cmake -S libplateau -B build/windows/libplateau -G "Visual Studio 17 2022" -DPLATEAU_USE_FBX=OFF -DCMAKE_BUILD_TYPE:STRING="Release" -DBUILD_LIB_TYPE=static -DRUNTIME_LIB_TYPE=MT
cmake --build build/windows/libplateau --config Release
```

#### 3. Build godot-plateau

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

The built DLL will be copied to `bin/windows/` and `project/bin/windows/`.

## Project Structure

```
godot-plateau/
├── src/
│   ├── register_types.cpp/h
│   └── plateau/
│       ├── plateau_city_model.cpp/h           # CityModel, MeshData classes
│       ├── plateau_geo_reference.cpp/h        # Coordinate conversion
│       ├── plateau_importer.cpp/h             # Scene builder
│       ├── plateau_mesh_extract_options.cpp/h
│       ├── plateau_gml_file.cpp/h             # GML file metadata
│       ├── plateau_instanced_city_model.cpp/h # Imported city model root
│       ├── plateau_city_model_scene.cpp/h     # CityModelScene, FilterCondition
│       ├── plateau_city_object_type.cpp/h     # Type hierarchy
│       ├── plateau_dynamic_tile.cpp/h         # Dynamic tile loading
│       └── plateau_road_network.cpp/h         # Road network data
├── doc_classes/        # API documentation XML files
├── project/
│   ├── bin/
│   │   └── godot-plateau.gdextension
│   └── samples/
│       └── plateau_utils.gd    # Sample utility functions
├── libplateau/         # Submodule
├── godot-cpp/          # Submodule
├── SConstruct          # SCons build script (recommended)
├── CMakeLists.txt      # CMake build script (alternative)
└── methods.py          # SCons helper functions
```

## Known Limitations

### OBJ Export

When exporting to OBJ format, libplateau creates one group per CityObject (building). If you have many buildings (e.g., 1990), the exported OBJ will have that many groups. When re-importing such OBJ files into Godot, you may encounter `MAX_MESH_SURFACES` errors because Godot limits meshes to 256 surfaces.

**Workaround**: Use AREA granularity (`mesh_granularity = 2`) before exporting to merge buildings into fewer meshes.

### Mobile Platforms (iOS/Android)

The following classes are **not available** on mobile platforms due to libpng dependency in height map generation:

- `PLATEAUTerrain` - Height map generation from terrain meshes
- `PLATEAUHeightMapData` - Height map data container
- `PLATEAUHeightMapAligner` - Building height alignment to terrain
- `PLATEAUVectorTileDownloader` - Map tile downloading
- `PLATEAUTileCoordinate` - Tile coordinate utilities
- `PLATEAUVectorTile` - Downloaded tile information

Core functionality (CityGML loading, mesh extraction, coordinate conversion) works on all platforms.

## Disclaimer

This project is not affiliated with, endorsed by, or sponsored by the [Godot Foundation](https://godot.foundation/) or Japan's [Ministry of Land, Infrastructure, Transport and Tourism (MLIT)](https://www.mlit.go.jp/).

- "Godot" and the Godot logo are registered trademarks of the Godot Foundation.
- "PLATEAU" is a project by the Ministry of Land, Infrastructure, Transport and Tourism of Japan.

## License

- godot-plateau: MIT License
- libplateau: See libplateau/LICENSE

## References

- [Project PLATEAU](https://www.mlit.go.jp/plateau/)
- [PLATEAU SDK for Unity](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unity)
- [PLATEAU SDK for Unreal](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unreal)
- [3D-Tiles-For-Godot](https://github.com/Battle-Road-Labs/3D-Tiles-For-Godot)
- [Godot GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/index.html)
