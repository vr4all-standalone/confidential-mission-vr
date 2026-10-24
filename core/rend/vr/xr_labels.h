/*
	The how-to-play labels by the controllers' buttons (hotd2-vr): before play, and again after the
	settings menu's Show controls, a hand that holds its controller (xr_host.h holds()) shows short
	labels for the buttons that matter, each with a line to its button (controllers::anchor): the
	right hand's to its right, the left hand's to its left, the stick's above. The words are the
	controllers' own, by their interaction profile (Quest Touch, or the PC controllers' buttons from
	the bindings in xr_host.cpp); the pictures are xr_panel.h's textTexture, uploaded once each.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#include <glm/glm.hpp>

namespace vr::xr::labels
{

// One hand's labels into the bound eye framebuffer, over what's there (drawn after its controller):
// viewProj: room space -> this eye's clip space; grip: that hand's grip space -> room space, as for
// controllers::draw; profile: that hand's interaction profile ("", or one not known here: Quest
// Touch's words). hand: 0 left, 1 right. Laid out in a plane facing the eye, once for each grip pose
// and eye (the frame's first eye; the other eye, a few cm off, gets the same places), each label
// turned to face its eye. The pictures are looked up as drawn (xr_panel.h textTexture).
void draw(const glm::mat4& viewProj, int hand, const glm::mat4& grip, const char *profile);
// Forget the layouts (the session's end, with termPanel). Render thread.
void term();

}
