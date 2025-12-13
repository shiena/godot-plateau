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
PLATEAU SDKの主要APIを示します。
- **インポート**: 各種オプション（LOD、粒度、テクスチャ）でCityGMLを読み込み
- **エクスポート**: glTF/GLB/OBJ形式でメッシュを保存
- **粒度変換**: 最小地物/主要地物/地域単位の変換

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
```

ビルドされたライブラリは `bin/<platform>/` と `demo/bin/<platform>/` にコピーされます。

#### ビルドオプション

```bash
# libplateauのビルドをスキップ（ビルド済みの場合）
scons platform=windows target=template_release skip_libplateau_build=yes

# macOS: Homebrewのカスタムプレフィックス（デフォルト: /opt/homebrew）
scons platform=macos target=template_release homebrew_prefix=/usr/local
```

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
│       ├── plateau_city_model.cpp/h      # CityModel, MeshDataクラス
│       ├── plateau_geo_reference.cpp/h   # 座標変換
│       ├── plateau_importer.cpp/h        # シーン構築
│       └── plateau_mesh_extract_options.cpp/h
├── demo/
│   └── bin/
│       └── godot-plateau.gdextension
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

### モバイルプラットフォーム (iOS/Android/visionOS)

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
