/*
	OpenXR host (hotd2-vr): turns the Android build into an immersive Quest app.

	The emulator keeps producing frames at the game's own rate. The render thread paces
	itself on the headset instead (xrWaitFrame), lets those frames through, and draws the
	latest one once per eye with the reprojection from rend/vr_reproject.h, so head
	movement is tracked at the display rate. The right controller is the light gun.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#include <glm/glm.hpp>

namespace vr::xr
{

// What a hand holds (holds()): its controller (drawn, with the how-to-play labels before play), or a gun.
enum class Holds { Controller, Gun };

struct Eye
{
	unsigned fbo;
	int width;
	int height;
	// Rebuilt game eye space (renderer y convention, game units) -> this eye's clip space.
	glm::mat4 viewProj;
};

#ifdef USE_OPENXR
// Built with OpenXR and switched on (vr.Xr).
bool enabled();
// One headset frame, called by the render thread in place of the usual wait-and-present.
// Returns false when nothing was shown (session not running).
bool frame();
// The eye being drawn right now, or nullptr outside an eye pass.
const Eye *currentEye();
// The render context is going away (app in the background, renderer restart): drop the
// session bound to it. The next frame() starts a new one on the new context.
void term();
// The controllers in use: the runtime's interaction profile for the right hand (the left's
// when the right has none), e.g. "/interaction_profiles/oculus/touch_controller"; "" before
// the runtime names one. Render thread only (the string changes there when the runtime names
// other controllers): copy the result if you keep it.
const char *controllerProfile();
// The VR menu holds the emulator stopped: from its stop until the menu closes again. Paused,
// Emulator::render still calls frame(), which draws the last game frame again for each head pose.
bool menuPaused();
// The VR menu opens (on) or closes: asked on the render thread, done at the top of the next
// Emulator::render (applyPause), between headset frames.
void requestPause(bool on);
// Emulator::render, render thread, first thing: stops or starts the emulator as requestPause asked.
void applyPause();
// What the left stick click does: the game camera goes where the head is (its yaw) on the next frame.
void recenter();
// Quit to the start screen: on Android BaseGLActivity.quitToStart through JNI (the start screen
// opens, the game's activity finishes); on the PC Flycast exits (dc_exit, at the next applyPause).
void quitToStart();
// What a hand holds now (0 left, 1 right). A game starts with a controller in each hand and the
// labels on (xr_labels.h). Player 1's gun hand (the right, or the hand whose trigger was pulled:
// the single gun's move) holds the gun from the first right Start or trigger pull, the labels going
// then; the left holds player 2's gun while player 2 is in. In single play the other hand holds its
// controller. Haptics go to the gun's hand, so this is where a gun is drawn. Render thread.
Holds holds(int hand);
// The menu's Show controls: both hands controllers with their labels again, until the next trigger
// pull in the game, which gives back what they held and fires as usual.
void showControls();
// A shot's knock in a hand, at vr.Haptics' strength as it is now: the menu's sample of it.
void sampleKnock(int hand);
#else
inline bool enabled() { return false; }
inline bool frame() { return false; }
inline const Eye *currentEye() { return nullptr; }
inline void term() {}
inline const char *controllerProfile() { return ""; }
inline bool menuPaused() { return false; }
inline void requestPause(bool) {}
inline void applyPause() {}
inline void recenter() {}
inline void quitToStart() {}
inline Holds holds(int) { return Holds::Gun; }
inline void showControls() {}
inline void sampleKnock(int) {}
#endif

}
