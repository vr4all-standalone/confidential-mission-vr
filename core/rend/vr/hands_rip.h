/*
	The game's own model in the player's hand, made on the spot from the player's own game
	(hotd2-vr): the agent's hands and pistol in The House of the Dead 2, the hero's staff in
	The Maze of the Kings. Each game keeps its own file, and never shows another game's.

	With no hands model yet, the first start of The House of the Dead 2 runs the game by
	itself, fast and silent, to the game over scene: Start through the title and Arcade mode,
	then no input, so the agent loses and kneels with his pistol in his hand. That frame is
	taken apart into the model (hands_build.h) and saved; the game goes on as usual. On the
	headset a panel says what's going on, and the gun's B skips it (the arcade gun then, and
	another go at the next start). Whenever the scene comes up in play, it's taken too.

	The Maze of the Kings goes the same way to its attract demo, with no input at all: the
	first frame with the hero and his staff, whole, in it (about half a minute of game).

	Confidential Mission draws its guns by themselves, with no hand on them (Source::CmGuns,
	gun-cm-*.bin, gun only): Start once at 30 s of game takes its SEGA logo to the reload
	tutorial, where its cabinet gun is turned about on screen; the first frame with all of it
	is the cabinet gun (given up at 120 s of game, with the fullest capture seen if there is
	one). Then Start every 2 s for 16 s takes it to the stage 1 intro, where the agents hold
	their pistol, never whole for long: every pass with enough of it is captured, and 20 s of
	game after the last (or at 150 s) they're merged into the agents' pistol, flagged complete
	when all of it came together. It runs while any of the guns' files is missing; B skips it
	here too, and nothing more is made that session.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#include "hands_build.h"
#include <string>

struct rend_context;
struct RenderPass;

namespace vr::hands
{

// What a game's own model is made from (its profile, vr_reproject.cpp: gameModelSource()),
// and the file it's kept in, in the data directory.
struct Source
{
	enum Kind { None, AgentsPistol, HerosStaff, CmGuns } kind = None;
	const char *file = "";
	Parts pistol {};		// AgentsPistol: the textures in the game over scene
	StaffParts staff {};	// HerosStaff: the textures in the attract demo and the story's intro
	GunTarget guns[3] {};	// CmGuns: the guns, each its own file (the cabinet gun first; unused: no file)
	// The file modelPath() names (CmGuns: the cabinet gun's, until Task 13 chooses)
	const char *modelFile() const { return kind == CmGuns ? guns[0].file : file; }
	const char *name() const {
		return kind == CmGuns ? "the guns" : kind == HerosStaff ? "the hero's staff" : "the agent's hands and pistol";
	}
};

// Where the running game's model is kept (the user data directory; none for a game without
// one), and where older builds put the agent's hands.
std::string modelPath();
std::string legacyModelPath();

// Every parsed render pass (vr::dropShotMarker): looks for the scene while there's no
// model, and makes it.
void observePass(const rend_context& ctx, const RenderPass& pass, const RenderPass& prev);

// The model to draw changed since the last call: one was just made, or another game started
// (the renderer loads it again then).
bool takeNewModel();

// The game is being run to the scene: no player input, a panel instead of the game.
bool preparing();
// How far along that is, 0..1 (a guess from the time the run usually takes).
float prepProgress();
// The player doesn't want to wait: the arcade gun this time.
void skipPrep();

}
