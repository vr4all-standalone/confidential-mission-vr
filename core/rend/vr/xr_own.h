/*
	The player's own models (hotd2-vr, Task 14b): guns and controllers the player puts in a folder,
	each a GLB (glTF 2.0 binary, read by glb_mesh.h) with a JSON sidecar of the same name. The app ships
	no such model; the player answers for the rights to the files they add.

	One root, two folders, "guns" and "controllers": next to flycast.exe on the PC, else (and on the
	Quest) next to the game's zip (the folder of the content path: Download/ on the Quest). Each
	folder is taken from the first root that has it.

	guns/<name>.glb + guns/<name>.json, in file-name order (vr.Gun 4, 5, ...; xr_hands.h):
		{ "name": "Zapper",			the menu's label, at most 24 characters (else the file name)
		  "grip": [x, y, z],		the top of the grip, in the GLB's space: the hand's point
		  "muzzle": [x, y, z],		the end of the barrel, in the GLB's space
		  "up": [x, y, z],			optional (default +y): away from the grip
		  "length": 0.25 }			metres, 0.05 to 1.5: the model's extent along the barrel
	Gun space (xr_hands.h) from the GLB's: the grip's top the origin, the barrel (muzzle - grip
	without its part along "up") -z, "up" +y, scaled so the model spans "length" along the barrel.

	controllers/left.glb, right.glb + left.json, right.json (each hand on its own):
		{ "aim": { "position": [x, y, z], "rotation": [qx, qy, qz, qw] },	the aim origin (OpenXR's aim
				pose: -z along the ray, +y up) in the GLB's space and units
		  "scale": 0.01 }			metres per GLB unit (0.01: a model in cm), 0.0001 to 100
	Drawn with that node on the aim pose: room <- aim pose * scale * inverse(aim node) <- GLB.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#include "types.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace vr::xr::own
{

struct GunFile
{
	std::string path;		// the GLB
	std::string shown;		// as logged: guns/<file>.glb
	std::string stem;		// its file name without .glb
	std::string name;		// the menu's label: the sidecar's "name", else the stem (at most 24 characters)
	bool usable = false;	// the sidecar read and in range (else why not)
	std::string why;
	glm::vec3 grip { 0.f }, muzzle { 0.f }, up { 0.f, 1.f, 0.f };
	float length = 0.f;
};

struct ControllerFile
{
	bool present = false;	// controllers/<hand>.glb is there
	std::string path, shown;
	bool usable = false;	// the sidecar read and in range (else why not)
	std::string why;
	glm::mat4 modelToAim { 1.f };	// GLB space -> aim space: scale * inverse(aim node)
};

// The folders read again: guns/*.glb, controllers/left.glb and right.glb, and their sidecars.
// generation() goes up when what's there changed (names, sizes, times, sidecars). On the render
// thread: at the game's start (xr_hands.cpp), the session's (xr_controllers.cpp) and when the VR menu
// opens; no file watcher. Logs "XR: own models from <folder>" once per change.
void scan();
// 0 until the first scan().
unsigned generation();
const std::vector<GunFile>& guns();
// hand: 0 left, 1 right
const ControllerFile& controller(int hand);
// The whole of a file, at most maxBytes: false if missing, unreadable or bigger.
bool readFile(const std::string& path, std::vector<u8>& data, size_t maxBytes);

// The caps on a model: triangles, textures (each at most MaxTextureSide a side, halved till then).
constexpr size_t MaxTriangles = 200000;
constexpr int MaxTextures = 8, MaxTextureSide = 2048;

}
