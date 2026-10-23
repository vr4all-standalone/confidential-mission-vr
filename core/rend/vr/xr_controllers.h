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

namespace vr::xr::controllers
{

// The buttons a label can point at. A and X (B and Y) are the same place: the lower (upper)
// face button of whichever hand. Menu: the left controller's menu button (=); on the right
// one the same place holds the Meta button.
enum Button { A, B, X, Y, Menu, Stick, Trigger, Grip, Buttons };

// Once a session, at its first FOCUSED state, on the render thread: asks the runtime for the
// controllers' models (/model_fb/controller/left and /right, glTF subset 1) when it has
// XR_FB_render_model (enabled at xrCreateInstance whenever listed; its functions are looked
// up here), else, or if any load fails, takes the built-in shape for both. Logs which:
// "XR: controller models from the runtime (left <n> triangles, right <n>)" or
// "XR: controller models built in (<why>)".
void init(XrInstance instance, XrSession session);
// One hand's controller into the bound eye framebuffer, at `grip` (grip space -> room space),
// with the depth test on and the depth buffer as it is (xr_host.cpp clears it first, or a
// gun's model does). viewProj: room space -> this eye's clip space; eyePos: the eye in room
// space. Draws whatever vr.ShowControllers says (the caller checks it). GL objects are made
// on the first draw.
void draw(const glm::mat4& viewProj, const glm::vec3& eyePos, int hand, const glm::mat4& grip);
// Grip space, metres: where that button's label points, the middle of its part of the
// built-in shape (also while the runtime's model is drawn: its parts aren't known by name).
// hand: 0 left, 1 right.
glm::vec3 anchor(int hand, Button button);
// Whether that hand's model is the runtime's.
bool fromRuntime(int hand);
// The session or the GL context is going away (render thread, context current): GL objects
// and models let go; init again for the next session.
void term();

}
