/*
	The player's controllers drawn in the hands (hotd2-vr): the runtime's own models where it
	has them (XR_FB_render_model: a Quest's Touch controllers, glTF read by glb_mesh.h), else a
	built-in low-poly shape like a Quest 3 Touch Plus (controller_model.h, made by
	hotd2-vr/controller_model/make_controller.py: self-made, no Meta data). Drawn by
	xr_host.cpp into each eye, with the guns' lighting (xr_gun.h ModelLightingSource), for each
	tracked hand that holds no gun.

	Grip space is OpenXR's grip pose of that hand: metres, the origin in the middle of the
	handle, -z up the handle, -y forward. Room space as in xr_gun.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#include <glm/glm.hpp>
#include <openxr/openxr.h>
#include <string>

namespace vr::xr::controllers
{

// The buttons a label can point at. A and X (B and Y) are the same place: the lower (upper)
// face button of whichever hand. Menu: the left controller's menu button (=); on the right
// one the same place holds the Meta button.
enum Button { A, B, X, Y, Menu, Stick, Trigger, Grip, Buttons };

// Once a session, at its first FOCUSED state, on the render thread: asks the runtime for the
// controllers' models (/model_fb/controller/left and /right, glTF subset 1) when it has
// XR_FB_render_model (enabled at xrCreateInstance whenever listed; its functions are looked
// up here), each hand on its own; a hand whose model doesn't come takes the built-in shape
// (retry() asks again while that may pass). Logs a model's arrival, "XR: controller models
// from the runtime (left <n> triangles, right <n>)" ("built in" for a hand's count that has
// none yet), and, when it changes, "XR: controller models built in (<why>)".
void init(XrInstance instance, XrSession session);
// After init, on the render thread: asks again for each hand still built in whose last
// failure may pass (its model unavailable: hand tracking on, the controller asleep, no
// interaction profile bound yet; its path not listed; a load that failed). Nothing when every
// hand has the runtime's model or the runtime has no XR_FB_render_model. A hand that has the
// runtime's model keeps it. An unavailable model costs one call. xr_host.cpp calls it at each
// interaction profile change and regained focus, and every 2 s while focused (15 times a
// focus at most) while retryWanted().
void retry();
// Whether retry() may still bring a model.
bool retryWanted();
// One hand's controller into the bound eye framebuffer, at `grip` (grip space -> room space),
// with the depth test on and the depth buffer as it is (xr_host.cpp clears it first, or a
// gun's model does). viewProj: room space -> this eye's clip space; eyePos: the eye in room
// space. Draws whatever vr.ShowControllers says (the caller checks it). GL objects are made
// on the first draw.
// (aim: the hand's aim pose, room space: where the player's own model, controllers/<hand>.glb, goes; xr_own.h)
void draw(const glm::mat4& viewProj, const glm::vec3& eyePos, int hand, const glm::mat4& grip, const glm::mat4& aim);
// Grip space, metres: where that button's label points, the middle of its part of the
// built-in shape (also while the runtime's model is drawn: its parts aren't known by name).
// hand: 0 left, 1 right.
glm::vec3 anchor(int hand, Button button);
// Where that button's label line ends on the controller that hand draws (render thread, after its
// draw()): on the player's own model (controllers/<hand>.glb, xr_own.h) with a "buttons" point for it
// in its sidecar (its own name, else its twin's: a and x, b and y), that point placed as the model is
// (room <- aim * scale * inverse(aim node) <- GLB; local: in the aim pose); else (the built-in shape,
// the runtime's model, or no point) grip * anchor() (local: anchor(), in the grip pose).
struct LabelPoint
{
	glm::vec3 room { 0.f };
	glm::vec3 local { 0.f };
	bool ownModel = false;	// the hand draws the player's own model
	bool fromFile = false;	// the point is its sidecar's
};
LabelPoint labelPoint(int hand, Button button, const glm::mat4& grip, const glm::mat4& aim);
// The sidecar of the hand's own model as the log names it (controllers/left.json), "" without one.
std::string ownSidecar(int hand);
// Whether that hand's model is the runtime's.
bool fromRuntime(int hand);
// The session or the GL context is going away (render thread, context current): GL objects
// and models let go; init again for the next session.
void term();

}
