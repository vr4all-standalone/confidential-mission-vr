/*
	The how-to-play labels by the controllers' buttons (hotd2-vr). See xr_labels.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_labels.h"
#include "xr_controllers.h"
#include "xr_panel.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace vr::xr::labels
{
namespace
{

using controllers::Button;

struct Label
{
	Button button;
	const char *text;
};

// Each controller kind's labels by its interaction profile, each hand's from the top of its column
// down, the stick's (above them) last. The words follow Task 2's bindings; the built-in shape is a
// Quest controller's, so a PC controller's button points at the place doing the same job: Start at
// A (X on the left), the reload at B (Y), the menu at the menu button. Roboto Bold, the panels'
// font, has no triple bar (U+2261): Touch's menu button says "Menu".
struct Words
{
	const char *profile;
	std::vector<Label> hand[2];	// left, right
};
const Words Kinds[] {
	{ "/interaction_profiles/oculus/touch_controller", {
		{ { controllers::Y, "Y Reload P2" }, { controllers::X, "X Player 2" }, { controllers::Menu, "Menu" },
			{ controllers::Stick, "Stick click Recenter" } },
		{ { controllers::B, "B Reload" }, { controllers::A, "A Start" }, { controllers::Trigger, "Trigger Shoot" },
			{ controllers::Stick, "Stick click Laser" } } } },
	{ "/interaction_profiles/valve/index_controller", {
		{ { controllers::Y, "B Reload P2" }, { controllers::X, "A Player 2" }, { controllers::Menu, "Pad squeeze Menu" },
			{ controllers::Stick, "Stick click Recenter" } },
		{ { controllers::B, "B Reload" }, { controllers::A, "A Start" }, { controllers::Trigger, "Trigger Shoot" },
			{ controllers::Stick, "Stick click Laser" } } } },
	{ "/interaction_profiles/htc/vive_controller", {
		{ { controllers::Y, "Pad click Reload P2" }, { controllers::X, "Menu Player 2 (hold: Menu)" } },
		{ { controllers::B, "Pad click Reload" }, { controllers::A, "Menu Start" }, { controllers::Trigger, "Trigger Shoot" } } } },
	{ "/interaction_profiles/microsoft/motion_controller", {
		{ { controllers::Y, "Pad click Reload P2" }, { controllers::X, "Menu Player 2 (hold: Menu)" },
			{ controllers::Stick, "Stick click Recenter" } },
		{ { controllers::B, "Pad click Reload" }, { controllers::A, "Menu Start" }, { controllers::Trigger, "Trigger Shoot" },
			{ controllers::Stick, "Stick click Laser" } } } },
};

// A label's letters (its picture's pixels) and its height in the room: letters ~7.5 mm high, about
// as sharp as a Quest 3 shows them at 40 cm. The column: from the hand's outermost button, and from
// one label to the next at least. The stick's label: over the column's top and the stick.
constexpr float Px = 40.f;
constexpr float Height = 0.016f;
constexpr float Gap = 0.025f;
constexpr float Pitch = 0.021f;
constexpr float StickRise = 0.026f;
constexpr float LineWidth = 0.0012f, Dot = 0.0024f;
const glm::vec3 TextColour(0.96f);
const glm::vec4 LineColour(0.92f, 0.92f, 0.92f, 0.85f);

// A label as laid out (room space): its picture, its middle and half sizes, and its line from the
// button (from) to the label's near edge (to).
struct Placed
{
	unsigned texture;
	glm::vec3 centre;
	float hx, hy;
	glm::vec3 from, to;
};
struct Layout
{
	glm::mat4 grip { 0.f };
	const Words *words = nullptr;
	std::vector<Placed> placed;
};
Layout layouts[2];

const Words& wordsFor(const char *profile)
{
	for (const Words& words : Kinds)
		if (profile != nullptr && !strcmp(profile, words.profile))
			return words;
	return Kinds[0];
}

// The eye of a projection (room space): the point it sends to x = y = w = 0.
glm::vec3 eyeOf(const glm::mat4& viewProj)
{
	const int rows[3] { 0, 1, 3 };
	glm::mat3 m;
	glm::vec3 b;
	for (int i = 0; i < 3; i++)
	{
		for (int c = 0; c < 3; c++)
			m[c][i] = viewProj[c][rows[i]];
		b[i] = -viewProj[3][rows[i]];
	}
	return glm::inverse(m) * b;
}

// Across (to the right as seen) and up, in a plane facing `eye` from `at`, the room's up kept up
// (looking straight down: across along `fallback`).
void facing(const glm::vec3& at, const glm::vec3& eye, const glm::vec3& fallback, glm::vec3& across, glm::vec3& up, glm::vec3& toEye)
{
	toEye = eye - at;
	toEye = glm::length(toEye) > 1e-4f ? glm::normalize(toEye) : glm::vec3(0.f, 0.f, 1.f);
	across = glm::cross(glm::vec3(0.f, 1.f, 0.f), toEye);
	if (glm::length(across) < 1e-3f)
		across = fallback - toEye * glm::dot(fallback, toEye);
	across = glm::normalize(across);
	up = glm::cross(toEye, across);
}

// Where one hand's labels go, seen from `eye`: a column beside the hand's outermost button (the
// right hand's to its right as seen, the left's to its left), in the list's order, Pitch apart at
// least and centred on their buttons' height; the stick's above the column's top, over the stick.
void layOut(Layout& layout, int hand, const glm::mat4& grip, const Words& words, const glm::vec3& eye)
{
	layout.placed.clear();
	const std::vector<Label>& list = words.hand[hand];
	if (list.empty())
		return;
	std::vector<glm::vec3> at;
	glm::vec3 middle(0.f);
	for (const Label& label : list)
	{
		at.push_back(glm::vec3(grip * glm::vec4(controllers::anchor(hand, label.button), 1.f)));
		middle += at.back();
	}
	middle /= (float)at.size();
	glm::vec3 across, up, toEye;
	facing(middle, eye, glm::vec3(grip[0]), across, up, toEye);
	auto planeX = [&](const glm::vec3& p) { return glm::dot(p - middle, across); };
	auto planeY = [&](const glm::vec3& p) { return glm::dot(p - middle, up); };
	auto room = [&](float x, float y) { return middle + across * x + up * y; };
	const float side = hand == 1 ? 1.f : -1.f;
	float outer = 0.f;
	for (const glm::vec3& p : at)
		outer = std::max(outer, side * planeX(p));
	const float columnX = side * (outer + Gap);

	std::vector<size_t> column;
	std::vector<float> ys;
	float buttonsY = 0.f;
	for (size_t i = 0; i < list.size(); i++)
		if (list[i].button != controllers::Stick)
		{
			float y = planeY(at[i]);
			buttonsY += y;
			if (!ys.empty())
				y = std::min(y, ys.back() - Pitch);
			column.push_back(i);
			ys.push_back(y);
		}
	if (!ys.empty())
	{
		float labelsY = 0.f;
		for (float y : ys)
			labelsY += y;
		const float shift = (buttonsY - labelsY) / ys.size();
		for (float& y : ys)
			y += shift;
	}
	auto place = [&](size_t i, float x, float y, float alignX, const glm::vec3& to) {
		int w, h;
		const unsigned texture = textTexture(list[i].text, Px, TextColour, w, h);
		if (texture == 0 || h == 0)
			return;
		const float hy = Height * 0.5f, hx = hy * w / h;
		layout.placed.push_back({ texture, room(x + alignX * hx, y), hx, hy, at[i], to });
	};
	for (size_t k = 0; k < column.size(); k++)
		place(column[k], columnX, ys[k], side, room(columnX, ys[k]));
	for (size_t i = 0; i < list.size(); i++)
		if (list[i].button == controllers::Stick)
		{
			const float x = planeX(at[i]);
			const float y = std::max(planeY(at[i]), ys.empty() ? -1.f : ys.front()) + StickRise;
			place(i, x, y, 0.f, room(x, y - Height * 0.5f));
		}
}

// A quad facing `eye` at `centre`, its top towards the room's up.
glm::mat4 billboard(const glm::vec3& centre, const glm::vec3& eye)
{
	glm::vec3 across, up, toEye;
	facing(centre, eye, glm::vec3(1.f, 0.f, 0.f), across, up, toEye);
	return glm::mat4(glm::vec4(across, 0.f), glm::vec4(up, 0.f), glm::vec4(toEye, 0.f), glm::vec4(centre, 1.f));
}

}	// namespace

void draw(const glm::mat4& viewProj, int hand, const glm::mat4& grip, const char *profile)
{
	if (hand != 0 && hand != 1)
		return;
	const glm::vec3 eye = eyeOf(viewProj);
	const Words& words = wordsFor(profile);
	Layout& layout = layouts[hand];
	if (layout.grip != grip || layout.words != &words || layout.placed.empty())
	{
		layout.grip = grip;
		layout.words = &words;
		layOut(layout, hand, grip, words, eye);
	}
	// the lines and their dots first, the labels over them
	for (const Placed& p : layout.placed)
	{
		const glm::vec3 along = p.to - p.from;
		const float length = glm::length(along);
		if (length > 1e-4f)
		{
			const glm::vec3 middle = (p.from + p.to) * 0.5f;
			const glm::vec3 x = along / length;
			glm::vec3 y = glm::cross(x, eye - middle);
			if (glm::length(y) > 1e-6f)
			{
				y = glm::normalize(y);
				const glm::mat4 model(glm::vec4(x, 0.f), glm::vec4(y, 0.f), glm::vec4(glm::cross(x, y), 0.f), glm::vec4(middle, 1.f));
				drawTexture(viewProj, model, 0, length * 0.5f, LineWidth * 0.5f, LineColour);
			}
		}
		drawTexture(viewProj, billboard(p.from, eye), 0, Dot * 0.5f, Dot * 0.5f, LineColour);
	}
	for (const Placed& p : layout.placed)
		drawTexture(viewProj, billboard(p.centre, eye), p.texture, p.hx, p.hy);
}

}
