/*
	A glTF 2.0 binary (GLB) read into one triangle mesh (hotd2-vr): the controllers' models from
	the OpenXR runtime (xr_controllers.h, XR_FB_render_model). Parsed by cgltf (core/deps/cgltf).

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#include "types.h"

#include <string>
#include <vector>

namespace vr::xr
{

// Every triangle of the default scene (else of every node), in the file's own space (glTF:
// metres, y up) with the node transforms applied: positions and normals x, y, z, texture
// coordinates u, v (v down the image, as glTF has it), indices into them. The base colour
// image of the first primitive that has one, when it is a PNG or a JPEG, as RGBA rows
// (top row first): texWidth x texHeight; else none (texWidth 0) and the mesh is untextured.
struct GlbMesh { std::vector<float> positions, normals, uvs; std::vector<u32> indices; int texWidth = 0, texHeight = 0; std::vector<u8> rgba; };

// All primitives (triangle lists, strips and fans), node transforms applied; base colour
// PNG/JPEG via stb_image; other images: untextured. Primitives with no normals get face
// normals. External files (a buffer or image by file name) are not read. False, with why in
// `error`, when the data isn't a GLB this can draw (no triangles).
bool loadGlb(const u8 *data, size_t size, GlbMesh& out, std::string& error);

}
