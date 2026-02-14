#pragma once

#include "plateau_platform.h"

#ifndef PLATEAU_MOBILE_PLATFORM
#include <plateau/polygon_mesh/model.h>
#include <plateau/polygon_mesh/mesh.h>

namespace godot {

// Type aliases to avoid name collision with godot::Node and godot::Mesh
using PlateauModel = plateau::polygonMesh::Model;
using PlateauNode = plateau::polygonMesh::Node;
using PlateauMesh = plateau::polygonMesh::Mesh;

} // namespace godot
#endif
