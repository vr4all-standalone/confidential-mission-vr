/*
	The player's own models (hotd2-vr, Task 14b): guns and controllers the player puts in a folder,
	each a GLB (glTF 2.0 binary, read by glb_mesh.h) with a JSON sidecar of the same name. The app ships
	no such model; the player answers for the rights to the files they add.

	One root, two folders, "guns" and "controllers": next to flycast.exe on the PC, else (and on the
	Quest) next to the game's zip (the folder of the content path: Download/ on the Quest). Each
	folder is taken from the first root that has it.

	guns/<name>.glb + guns/<name>.json, in file-name order, the menu's choices after the arcade gun; the one
	chosen is vr.Gun 4 with vr.GunFile its file name without .glb (xr_hands.h):
		{ "name": "Zapper",			the menu's label, at most 24 bytes of UTF-8 (else the file name, cut to 24)
		  "grip": [x, y, z],		the top of the grip, in the GLB's space: the hand's point
		  "muzzle": [x, y, z],		the end of the barrel, in the GLB's space
		  "up": [x, y, z],			optional (default +y): away from the grip
		  "length": 0.25 }			metres, 0.05 to 1.5: the model's extent along the barrel
	Gun space (xr_hands.h) from the GLB's: the grip's top the origin, the barrel (muzzle - grip
	without its part along "up") -z, "up" +y, scaled so the model spans "length" along the barrel.
	Every number finite as a float and within +-MaxCoordinate; "grip" and "muzzle" each at most twice
	the model's size (its bounds' longest side) from its bounds. Each GLB is looked at when it changes
	(its triangles, its textures' sizes, its bounds: nothing decoded); only the chosen gun is read
	(xr_hands.cpp). The guns' textures together at most GunTextureBudget, in file-name order: a gun
	that would go over it is left out (one after it that fits is not).

	controllers/left.glb, right.glb + left.json, right.json (each hand on its own):
		{ "aim": { "position": [x, y, z], "rotation": [qx, qy, qz, qw] },	the aim origin (OpenXR's aim
				pose: -z along the ray, +y up) in the GLB's space and units
		  "scale": 0.01 }			metres per GLB unit (0.01: a model in cm), 0.0001 to 100
	Drawn with that node on the aim pose: room <- aim pose * scale * inverse(aim node) <- GLB, the GLB
	in its scene's space (its node transforms applied), as the aim node is.

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
	std::string name;		// the menu's label: the sidecar's "name", else the stem (at most 24 bytes)
	std::string stamp;		// the GLB's and the sidecar's sizes and times when looked at
	bool usable = false;	// offered: the sidecar, the GLB and the budget all fine (else why not)
	std::string why;
	std::string fault;		// what's wrong with the sidecar or the GLB (empty: nothing; the budget aside)
	glm::vec3 grip { 0.f }, muzzle { 0.f }, up { 0.f, 1.f, 0.f };
	float length = 0.f;
	size_t triangles = 0;	// the GLB's, looked at
	int textures = 0;
	size_t textureBytes = 0;	// its textures' RGBA as they'd be read (fitted to MaxTextureSide)
	std::string logged;		// its last "XR: own gun" line from scan()
};

struct ControllerFile
{
	bool present = false;	// controllers/<hand>.glb is there
	std::string path, shown;
	std::string stamp;		// the GLB's and the sidecar's sizes and times
	bool usable = false;	// the sidecar read and in range (else why not)
	std::string why;
	glm::mat4 modelToAim { 1.f };	// GLB space -> aim space: scale * inverse(aim node)
};

// The folders read again: guns/*.glb, controllers/left.glb and right.glb, and their sidecars; a gun's
// files looked at again only when they changed (names, sizes, times). On the render thread: at the
// game's start (xr_hands.cpp), the session's (xr_controllers.cpp) and when the VR menu opens; no file
// watcher. Logs "XR: own models from <folder>" once per change, and for each gun whose state changed
// "XR: own gun guns/<file> offered: "<name>" (<n> triangles, <t> textures, <x.x> MB)" or "... skipped: <why>".
void scan();
// Each folder's own count of changes: 0 until the first scan().
unsigned gunsGeneration();
unsigned controllersGeneration();
const std::vector<GunFile>& guns();
// hand: 0 left, 1 right
const ControllerFile& controller(int hand);
// The whole of a file, at most maxBytes: false if missing, unreadable or bigger.
bool readFile(const std::string& path, std::vector<u8>& data, size_t maxBytes);

// The caps on a model: triangles, textures (each at most MaxTextureSide a side, halved till then).
constexpr size_t MaxTriangles = 200000;
constexpr int MaxTextures = 8, MaxTextureSide = 2048;
constexpr size_t MaxFileBytes = 256u * 1024 * 1024;
// The folder's guns' textures together, as read (level 0; mipmaps add a third on the GPU).
constexpr size_t GunTextureBudget = 256u * 1024 * 1024;
// A sidecar's numbers, either way.
constexpr float MaxCoordinate = 1e6f;

}
