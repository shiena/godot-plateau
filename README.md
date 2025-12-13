# PLATEAU SDK for Godot

A GDExtension for Godot Engine 4.x that enables loading 3D city models (CityGML) provided by Japan's Ministry of Land, Infrastructure, Transport and Tourism (MLIT).

[日本語版 README](./README.ja.md)

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

## Samples

The `demo/samples/` directory contains example scenes demonstrating SDK features.

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
Demonstrates the main PLATEAU SDK APIs.
- **Import**: Load CityGML files with various options (LOD, granularity, textures)
- **Export**: Save meshes to glTF/GLB/OBJ formats
- **Granularity Conversion**: Convert between Atomic/Primary/Area mesh levels

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

1. Open the demo project in Godot
2. Run one of the sample scenes
3. Use "Import GML..." or "Load..." buttons to select your CityGML file
4. Interact with the UI to test various features

**Note**: You need to prepare your own PLATEAU CityGML data. Download from [G空間情報センター](https://www.geospatial.jp/ckan/dataset/plateau).

## Building

### Requirements

- Python 3.8+
- SCons 4.0+
- CMake 3.17+
- Visual Studio 2022 (Windows) / Ninja + Xcode (macOS)
- Git

#### macOS Additional Requirements

Install the following Homebrew packages:

```bash
brew install mesa-glu xz libdeflate
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
```

The built library will be copied to `bin/<platform>/` and `demo/bin/<platform>/`.

#### Build Options

```bash
# Skip libplateau build (if already built)
scons platform=windows target=template_release skip_libplateau_build=yes

# macOS: Custom Homebrew prefix (default: /opt/homebrew)
scons platform=macos target=template_release homebrew_prefix=/usr/local
```

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

The built DLL will be copied to `bin/windows/` and `demo/bin/windows/`.

## Project Structure

```
godot-plateau/
├── src/
│   ├── register_types.cpp/h
│   └── plateau/
│       ├── plateau_city_model.cpp/h      # CityModel, MeshData classes
│       ├── plateau_geo_reference.cpp/h   # Coordinate conversion
│       ├── plateau_importer.cpp/h        # Scene builder
│       └── plateau_mesh_extract_options.cpp/h
├── demo/
│   └── bin/
│       └── godot-plateau.gdextension
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

### Mobile Platforms (iOS/Android/visionOS)

The following classes are **not available** on mobile platforms due to libpng dependency in height map generation:

- `PLATEAUTerrain` - Height map generation from terrain meshes
- `PLATEAUHeightMapData` - Height map data container
- `PLATEAUHeightMapAligner` - Building height alignment to terrain
- `PLATEAUVectorTileDownloader` - Map tile downloading
- `PLATEAUTileCoordinate` - Tile coordinate utilities
- `PLATEAUVectorTile` - Downloaded tile information

Core functionality (CityGML loading, mesh extraction, coordinate conversion) works on all platforms.

## License

- godot-plateau: MIT License
- libplateau: See libplateau/LICENSE

## References

- [PLATEAU SDK for Unity](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unity)
- [PLATEAU SDK for Unreal](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unreal)
- [3D-Tiles-For-Godot](https://github.com/pslehisl/3D-Tiles-For-Godot)
- [Godot GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/index.html)
- [Project PLATEAU](https://www.mlit.go.jp/plateau/)
