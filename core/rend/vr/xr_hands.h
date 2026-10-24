/*
	The agent's own hands and pistol in the headset (hotd2-vr), taken from the player's copy
	of the game: hands.bin in the data directory, made by the app (hands_rip.h) or on the PC
	by hotd2-vr/assets/rip_hands.py from a rip of the game over scene. In The Maze of the
	Kings the hero's staff in his fist, and his other glove, from staff-mok.bin. Each game
	has its own file; nothing of the game is in this source; without the file the arcade gun
	of xr_gun.h is drawn instead.

	Gun space is that of xr_gun.h (barrel along -z, y up, metres) with its origin in the fist
	around the grip. Hand space holds the open left hand: fingers along -z, palm towards +x,
	thumb up, origin just off the palm, where the controller's grip goes. Mirrored in x, both
	serve the other side. (The staff: its head along -z, origin on its axis in the fist; hand
	space is staff space mirrored, with the left glove a fist like the right one.)

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace vr::xr
{

struct HandsModel
{
	glm::vec3 muzzle;		// gun space
	glm::vec3 grab;			// gun space: the back of the slide, where the other hand takes it
	float travel;			// metres the slide goes back (+z); 0: no slide (the staff)
	glm::mat4 onSlide;		// hand space -> gun space: the open hand racking the slide (at rest)
	bool gunOnly = false;	// no hands (HND1 v2): the other hand keeps its controller
};

// The running game's model, loaded on first use (again once another game starts); nullptr
// without one. (Confidential Mission's guns are each their own: gunModel(); here the one
// chosenGun() gives.)
const HandsModel *handsModel();

// The guns a hand can hold in Confidential Mission: the arcade gun (xr_gun.h's own, always
// there), and the game's cabinet gun, agents' pistol and assault rifle, each its own file made
// from the player's game (hands_rip.h; vr_reproject.cpp's guns[], in that order).
enum class GunKind { Arcade, Cabinet, Pistol, Rifle };
// Its model, loaded on first use from its file (again once one is made or another game starts);
// nullptr if missing or unreadable, for the arcade gun, and in a game without these guns.
const HandsModel *gunModel(GunKind kind);
// All of it was in the game's frame (HND1 v2 flag 2); the arcade gun always.
bool gunComplete(GunKind kind);
// The gun vr.Gun gives: 0 auto, the agents' pistol if complete, else the cabinet gun if there,
// else the arcade gun; 1-3 as chosen (the agents' pistol, the cabinet gun, the arcade gun), a
// missing one falling back the same way. The arcade gun with vr.GameHands off, and in a game
// without these guns. Logs "XR: gun: <gun> (<auto|chosen>[: <why the fallback>])" at the
// start and at each change.
GunKind chosenGun();
// The gun in a player's (0, 1) hand: chosenGun() now (Task 14 adds the rifle).
GunKind handGun(int player);
// "agents' pistol", "cabinet gun", "arcade gun", "assault rifle"
const char *gunName(GunKind kind);
// The model a hand holding `kind` draws, with vr.GameHands on: Confidential Mission's gun of
// that kind (gunModel(), none for the arcade gun), another game's own whatever the kind
// (handsModel()); nullptr: the arcade gun.
const HandsModel *handModel(GunKind kind);

// vr.Gun's choices, in the menu's order, round and round (Task 14b appends the player's own).
// The game's own models off: only Auto and the arcade gun are made. Another game: one choice
// (its own model or the arcade gun), so the row can't change vr.Gun.
struct GunChoice
{
	int setting;		// vr.Gun
	const char *label;	// the menu's: "Agents' pistol"
	bool made;			// its model is there (Auto and the arcade gun: always): skipped if not
	bool complete;		// ...all of it (the menu says "(incomplete)" if not)
};
std::vector<GunChoice> gunChoices();

struct HandsView
{
	const HandsModel *model = nullptr;	// which: handsModel() or a gunModel() (handModel())
	glm::mat4 gunPose;		// gun space -> room space
	float slide = 0.f;		// metres the slide is back
	bool otherHand = false;	// draw the open hand...
	glm::mat4 handPose;		// ...hand space -> room space
	glm::vec4 muzzleLight { 0.f };	// room position, strength (the muzzle flash)
};

// Draws the view's model (the pistol in its hand, and the other hand) into the bound eye framebuffer.
void drawHands(const glm::mat4& viewProj, const glm::vec3& eyePos, const HandsView& view);
// The GL context is going away.
void termHands();

}
