# PLATEAU SDK for Godot

Godot Engine 4.x 向けの PLATEAU SDK GDExtension です。国土交通省が提供する3D都市モデル（CityGML）をGodotで利用できます。

## 機能

- CityGMLファイルの読み込み
- 3Dメッシュの生成と表示
- LOD（Level of Detail）の選択
- 座標変換（地理座標 ↔ ローカル座標）
- テクスチャの読み込み（キャッシュ対応）
- PBRマテリアル対応（Metallic/Roughness/Emission）
- スムーズシェーディング（法線自動生成）
- 属性情報アクセス（GML ID、CityObjectType、属性）
- UV座標によるRaycastベースのCityObject識別
- カメラ距離に基づく動的タイル読み込み
- 道路ネットワークデータ構造（車線、交差点、歩道）
- 日本語表示名付きCityObjectType階層
- GMLファイルメタデータアクセス（グリッドコード、EPSG、地物タイプ）
- PLATEAUInstancedCityModelによる構造化された都市モデルインポート

## 主要クラス

### PLATEAUImporter (Node3D)
CityGMLファイルをインポートしてシーンに配置するノード。

```gdscript
var importer = PLATEAUImporter.new()
importer.gml_path = "path/to/your.gml"
importer.import_gml()
add_child(importer)
```

### PLATEAUCityModel (RefCounted)
CityGMLファイルの読み込みとメッシュ抽出を行うクラス。

```gdscript
var city_model = PLATEAUCityModel.new()
city_model.load("path/to/your.gml")

var options = PLATEAUMeshExtractOptions.new()
options.min_lod = 1
options.max_lod = 2

var mesh_data_array = city_model.extract_meshes(options)
```

### PLATEAUGeoReference (RefCounted)
座標変換を行うクラス。

```gdscript
var geo_ref = PLATEAUGeoReference.new()
geo_ref.zone_id = 9  # 東京
var local_pos = geo_ref.project(Vector3(35.6762, 139.6503, 0))  # 緯度, 経度, 高さ
```

### PLATEAUMeshExtractOptions (RefCounted)
メッシュ抽出オプション。

```gdscript
var options = PLATEAUMeshExtractOptions.new()
options.min_lod = 0
options.max_lod = 3
options.mesh_granularity = 1  # 0=最小地物, 1=主要地物, 2=地域単位
options.export_appearance = true
```

### PLATEAUMeshData (RefCounted)
抽出されたメッシュデータと属性情報を保持するクラス。

```gdscript
# メッシュとトランスフォームへのアクセス
var mesh: ArrayMesh = mesh_data.get_mesh()
var transform: Transform3D = mesh_data.get_transform()

# CityObject属性へのアクセス
var gml_id: String = mesh_data.get_gml_id()
var type: int = mesh_data.get_city_object_type()
var type_name: String = mesh_data.get_city_object_type_name()
var attributes: Dictionary = mesh_data.get_attributes()

# 特定の属性を取得（"/"区切りでネスト対応）
var building_id = mesh_data.get_attribute("bldg:buildingID")
var height = mesh_data.get_attribute("bldg:measuredHeight")

# RaycastヒットのUV座標からGML IDを取得（CityObject識別用）
var gml_id_from_hit = mesh_data.get_gml_id_from_uv(hit_uv)
```

### PLATEAUCityModelScene (Node3D)
PLATEAUシティモデルのルートノード。GeoReferenceとインポートしたGMLトランスフォームを管理。

```gdscript
var scene = PLATEAUCityModelScene.new()
scene.geo_reference = geo_ref
add_child(scene)
scene.import_gml("path/to/file.gml", import_options)

# 原点の緯度経度を取得
print("Origin: ", scene.latitude, ", ", scene.longitude)

# 全GMLトランスフォームを取得
for gml_transform in scene.get_gml_transforms():
    print(gml_transform.name)
```

### PLATEAUFilterCondition (Resource)
タイプ、LOD、パッケージによるCityObjectのフィルター条件。

```gdscript
var filter = PLATEAUFilterCondition.new()
filter.city_object_types = COT_Building | COT_Road
filter.min_lod = 1
filter.max_lod = 2

if filter.matches(mesh_data):
    # このメッシュを処理
    pass
```

### PLATEAUCityObjectTypeHierarchy (RefCounted)
日本語表示名付きのCityObjectType階層。

```gdscript
var hierarchy = PLATEAUCityObjectTypeHierarchy.new()

# 日本語の表示名を取得
var name = PLATEAUCityObjectTypeHierarchy.get_type_display_name(COT_Building)
# "建築物" を返す

# タイプからパッケージへ変換
var package = PLATEAUCityObjectTypeHierarchy.type_to_package(COT_RoofSurface)
# PACKAGE_BUILDING を返す
```

### PLATEAUDynamicTileManager (Node3D)
カメラ距離に基づく動的タイルローディングマネージャー。タイルを自動的にロード/アンロード。

```gdscript
var manager = PLATEAUDynamicTileManager.new()
add_child(manager)

# ズームレベルごとのロード距離を設定 (min_distance, max_distance)
manager.set_load_distance(11, Vector2(-10000, 500))   # 高詳細、近距離
manager.set_load_distance(10, Vector2(500, 1500))    # 中詳細
manager.set_load_distance(9, Vector2(1500, 10000))   # 低詳細、遠距離

# メタデータストアで初期化
manager.initialize(meta_store)
manager.tile_base_path = "res://tiles/"
manager.camera = $Camera3D
manager.auto_update = true

# シグナル
manager.tile_loaded.connect(_on_tile_loaded)
manager.tile_unloaded.connect(_on_tile_unloaded)
```

### PLATEAUGmlFile (RefCounted)
GMLファイルのメタデータにアクセスするためのクラス。

```gdscript
var gml = PLATEAUGmlFile.create("C:/path/to/udx/bldg/53394601_bldg_6697_op.gml")
if gml.is_valid():
    print("グリッドコード: ", gml.get_grid_code())        # 53394601
    print("地物タイプ: ", gml.get_feature_type())  # bldg
    print("EPSG: ", gml.get_epsg())                  # 6697
    print("データセットルート: ", gml.get_dataset_root_path())
    print("最大LOD: ", gml.get_max_lod())

    # GMLで参照されているテクスチャパスを取得
    var textures = gml.search_image_paths()
    for tex_path in textures:
        print("テクスチャ: ", tex_path)

    # グリッドコードの地理的範囲を取得
    var extent = gml.get_grid_extent()
    print("緯度: ", extent.min_lat, " - ", extent.max_lat)
```

### PLATEAUInstancedCityModel (Node3D)
インポートされたPLATEAU都市モデルのルートノード。メタデータを保持し、子ノードへのアクセスを提供。

```gdscript
# PLATEAUImporter.import_to_scene()で作成
var city_model: PLATEAUInstancedCityModel = importer.import_to_scene(
    mesh_data_array, "MyCity", geo_reference, options, gml_path
)
add_child(city_model)

# メタデータへのアクセス
print("Zone ID: ", city_model.zone_id)
print("緯度: ", city_model.get_latitude())
print("経度: ", city_model.get_longitude())
print("GMLパス: ", city_model.gml_path)

# 座標変換用のGeoReferenceを取得
var geo_ref = city_model.get_geo_reference()

# 子トランスフォームへのアクセス
for gml_transform in city_model.get_gml_transforms():
    print("GML: ", gml_transform.name)
    var lods = city_model.get_lods(gml_transform)
    print("利用可能なLOD: ", lods)
```

### PLATEAURnModel (RefCounted)
道路ネットワークデータのルートコンテナ。道路、交差点、歩道を含む。

```gdscript
var model = PLATEAURnModel.new()

# 道路と交差点を追加
var road = PLATEAURnRoad.new()
model.add_road(road)

var intersection = PLATEAURnIntersection.new()
model.add_intersection(intersection)

# メッシュデータから構築（道路パッケージ）
var model2 = PLATEAURnModel.create_from_mesh_data(road_mesh_data_array)

# 可視化メッシュを生成
var mesh = model.generate_mesh()
```

## サンプル

`demo/samples/` ディレクトリにSDKの機能を示すサンプルシーンがあります。

### 属性表示サンプル (`attributes_display_sample.tscn`)
建物クリック時に属性情報を表示する方法を示します。
- CityGMLファイルを読み込んでメッシュを抽出
- 建物を左クリックで属性を表示
- GML ID、CityObjectType、全属性を表示

### 属性色分けサンプル (`attributes_color_sample.tscn`)
属性に基づいて建物を色分けする方法を示します。
- 建物用途（`bldg:usage`）で色分け
- CityObjectTypeで色分け
- 浸水リスク（データに含まれる場合）で色分け

### APIサンプル (`api_sample.tscn`)
PLATEAU SDKの主要APIを示します（データセットフォルダ選択版）。
- **データセット選択**: PLATEAUデータセットフォルダを選択、パッケージとメッシュコードを選択
- **インポート**: 各種オプション（LOD、粒度、テクスチャ）で複数GMLを読み込み
- **エクスポート**: glTF/GLB/OBJ形式でメッシュを保存
- **粒度変換**: 最小地物/主要地物/地域単位の変換

### APIサンプルシンプル版 (`api_sample_simple.tscn`)
単一GMLファイルインポート用のシンプル版APIサンプル。
- **単一ファイルインポート**: 1つのCityGMLファイルを選択して読み込み
- **エクスポート**: glTF/GLB/OBJ形式でメッシュを保存
- **粒度変換**: メッシュ粒度レベルの変換
- シーン構成にPLATEAUInstancedCityModelを使用

### 地形サンプル (`terrain_sample.tscn`)
地形関連の機能を示します。
- 地形GML（DEM/TINデータ）の読み込み
- 地形メッシュからハイトマップを生成
- 建物を地形高さに合わせる
- ハイトマップをPNG画像で保存

### ベースマップサンプル (`basemap_sample.tscn`)
地図タイルダウンロード機能を示します。
- 国土地理院の地図タイルをダウンロード
- OpenStreetMapからタイルをダウンロード
- 複数タイルを結合してテクスチャを作成
- 地面メッシュにテクスチャを適用

### サンプルの使い方

1. Godotでdemoプロジェクトを開く
2. サンプルシーンを実行
3. 「Import GML...」や「Load...」ボタンでCityGMLファイルを選択
4. UIで各機能をテスト

**注意**: PLATEAUのCityGMLデータは別途ご用意ください。[G空間情報センター](https://www.geospatial.jp/ckan/dataset/plateau)からダウンロードできます。

## エディタプラグイン

### PLATEAU Converter

CityGMLをプロジェクトにインポートしたり、外部フォーマットにエクスポートするためのエディタツールです。

**ツールを開く**: ツールメニュー → PLATEAU Converter...

#### Import to Project（プロジェクトへインポート）

CityGMLファイルを `.tscn` シーンとしてGodotプロジェクトにインポートします。

- **シーン構造**: `PLATEAUInstancedCityModel` をルートノードとして作成
- **メッシュリソース**: パフォーマンス向上のため、メッシュを個別の `.res` ファイルとして保存
- **テクスチャ処理**: 外部PLATEAUデータセットからプロジェクトフォルダにテクスチャをコピー
- **LOD自動切り替え**: `visibility_range` を使用した距離ベースのLOD切り替え（オプション）

使い方:
1. 「Import GML...」をクリックしてCityGMLファイルを選択
2. インポートオプション（LOD範囲、粒度、テクスチャ）を設定
3. 距離ベースのLOD切り替えが必要な場合は「LOD自動切り替え」を有効化
4. 「Import to Project」をクリックして出力フォルダを選択
5. シーンは `.tscn` として保存され、メッシュリソースは `meshes/` サブフォルダに配置

#### Export to File（ファイルへエクスポート）

読み込んだメッシュを外部3Dフォーマットにエクスポートして、他のアプリケーションで使用できます。

- **対応フォーマット**: glTF (.gltf)、GLB (.glb)、OBJ (.obj)
- **用途**: 外部3Dソフトウェア、モバイルアプリ、Webビューア

使い方:
1. 「Import GML...」でCityGMLファイルを読み込み
2. エクスポートフォーマット（glTF/GLB/OBJ）を選択
3. 「Export to File」をクリックして保存先を選択

## ビルド方法

### 必要条件

- Python 3.8以上
- SCons 4.0以上
- CMake 3.17以上
- Visual Studio 2022 (Windows) / Ninja + Xcode (macOS)
- Git

#### macOS 追加要件

以下のHomebrewパッケージをインストールしてください：

```bash
brew install mesa-glu xz libdeflate
```

### SConsでビルド（推奨）

SConsはlibplateauとgodot-plateauを自動的にビルドします。

#### 1. リポジトリのクローン

```bash
git clone --recursive https://github.com/shiena/godot-plateau.git
cd godot-plateau
```

#### 2. ビルド

```bash
# Windows (template_debug)
scons platform=windows target=template_debug

# Windows (template_release)
scons platform=windows target=template_release

# macOS
scons platform=macos target=template_release

# Linux
scons platform=linux target=template_release

# Android（ANDROID_HOMEまたはANDROID_SDK_ROOT環境変数が必要）
scons platform=android arch=arm64 target=template_release

# iOS（macOSとXcodeが必要）
scons platform=ios arch=arm64 target=template_release
```

ビルドされたライブラリは `bin/<platform>/` と `demo/addons/plateau/<platform>/` にコピーされます。

#### ビルドオプション

```bash
# libplateauのビルドをスキップ（ビルド済みの場合）
scons platform=windows target=template_release skip_libplateau_build=yes

# macOS: Homebrewのカスタムプレフィックス（デフォルト: /opt/homebrew）
scons platform=macos target=template_release homebrew_prefix=/usr/local

# Linux: GCCの代わりにClangを使用（GCC 15+環境向け）
scons platform=linux target=template_release use_clang=yes
```

#### Androidビルド要件

- `ANDROID_HOME`または`ANDROID_SDK_ROOT`環境変数にAndroid SDKパスを設定
- Android NDKがインストール済み（デフォルトバージョン: 23.1.7779620）
- Ninjaビルドツール

```bash
# カスタムNDKバージョンの例
scons platform=android arch=arm64 target=template_release ndk_version=23.1.7779620
```

#### iOSビルド要件

- macOSとXcode、Command Line Toolsがインストール済み
- Ninjaビルドツール（`brew install ninja`）

### CMakeでビルド（代替方法）

#### 1. リポジトリのクローン

```bash
git clone --recursive https://github.com/shiena/godot-plateau.git
cd godot-plateau
```

#### 2. libplateauのビルド（初回のみ）

```bash
cmake -S libplateau -B build/windows/libplateau -G "Visual Studio 17 2022" -DPLATEAU_USE_FBX=OFF -DCMAKE_BUILD_TYPE:STRING="Release" -DBUILD_LIB_TYPE=static -DRUNTIME_LIB_TYPE=MT
cmake --build build/windows/libplateau --config Release
```

#### 3. godot-plateauのビルド

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

ビルドされたDLLは `bin/windows/` と `demo/bin/windows/` にコピーされます。

## プロジェクト構造

```
godot-plateau/
├── src/
│   ├── register_types.cpp/h
│   └── plateau/
│       ├── plateau_city_model.cpp/h           # CityModel, MeshDataクラス
│       ├── plateau_geo_reference.cpp/h        # 座標変換
│       ├── plateau_importer.cpp/h             # シーン構築
│       ├── plateau_mesh_extract_options.cpp/h
│       ├── plateau_gml_file.cpp/h             # GMLファイルメタデータ
│       ├── plateau_instanced_city_model.cpp/h # インポートした都市モデルルート
│       ├── plateau_city_model_scene.cpp/h     # CityModelScene, FilterCondition
│       ├── plateau_city_object_type.cpp/h     # タイプ階層
│       ├── plateau_dynamic_tile.cpp/h         # 動的タイル読み込み
│       └── plateau_road_network.cpp/h         # 道路ネットワークデータ
├── doc_classes/        # APIドキュメントXMLファイル
├── demo/
│   ├── bin/
│   │   └── godot-plateau.gdextension
│   └── samples/
│       └── plateau_utils.gd    # サンプル用ユーティリティ関数
├── libplateau/         # サブモジュール
├── godot-cpp/          # サブモジュール
├── SConstruct          # SConsビルドスクリプト（推奨）
├── CMakeLists.txt      # CMakeビルドスクリプト（代替）
└── methods.py          # SConsヘルパー関数
```

## 既知の制限事項

### OBJエクスポート

OBJ形式でエクスポートする場合、libplateauはCityObject（建物）ごとに1つのグループを作成します。建物が多数（例：1990棟）ある場合、エクスポートされたOBJファイルには同数のグループが含まれます。このようなOBJファイルをGodotに再インポートすると、Godotのメッシュは256サーフェスまでに制限されているため、`MAX_MESH_SURFACES`エラーが発生する可能性があります。

**回避策**: エクスポート前にAREA粒度（`mesh_granularity = 2`）を使用して建物をより少ないメッシュに結合してください。

### モバイルプラットフォーム (iOS/Android)

以下のクラスはモバイルプラットフォームでは**利用できません**（ハイトマップ生成のlibpng依存のため）：

- `PLATEAUTerrain` - 地形メッシュからのハイトマップ生成
- `PLATEAUHeightMapData` - ハイトマップデータコンテナ
- `PLATEAUHeightMapAligner` - 建物の地形高さ合わせ
- `PLATEAUVectorTileDownloader` - 地図タイルダウンロード
- `PLATEAUTileCoordinate` - タイル座標ユーティリティ
- `PLATEAUVectorTile` - ダウンロードしたタイル情報

コア機能（CityGML読み込み、メッシュ抽出、座標変換）は全プラットフォームで動作します。

## 使用方法

1. `demo/bin/` フォルダを Godot プロジェクトの `addons/` にコピー
2. プロジェクト設定でプラグインを有効化
3. シーンに `PLATEAUImporter` ノードを追加
4. Inspector で GML ファイルパスを設定
5. `import_gml()` を呼び出してインポート

## ライセンス

- godot-plateau: MIT License
- libplateau: libplateau/LICENSE を参照

## 参考

- [PLATEAU SDK for Unity](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unity)
- [PLATEAU SDK for Unreal](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unreal)
- [3D-Tiles-For-Godot](https://github.com/pslehisl/3D-Tiles-For-Godot)
- [Godot GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/index.html)
- [Project PLATEAU](https://www.mlit.go.jp/plateau/)
