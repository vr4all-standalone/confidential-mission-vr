/*
	The settings menu in the headset (hotd2-vr). See xr_menu.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_menu.h"
#include "xr_host.h"
#include "xr_panel.h"
#include "cfg/cfg.h"
#include "cfg/option.h"
#include "emulator.h"
#include "log/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace vr::xr::menu
{
namespace
{

constexpr float Distance = 1.2f, BelowEyes = 0.1f;
constexpr int Left = 0, Right = 1;

const char *const Labels[Rows] { "Resume", "Recenter", "Gun", "Laser", "Haptics", "World size", "Panel distance",
		"Wider view", "Show controls", "Quit" };
// vr.Gun's choices (0 is auto)
const char *const GunNames[4] { "Auto", "Agents' pistol", "Cabinet gun", "Arcade gun" };
const char *const HapticsNames[4] { "Off", "Low", "Normal", "Strong" };

glm::mat4 model { 1.f };	// the panel: its middle, facing the player (+z)
int selected;
// The row each hand's ray is on (-1: none), and whether that's known yet: the first update after
// opening learns them without choosing (the selection follows a ray onto another row).
int hovered[2];
bool fresh;
bool pointing[2];
glm::vec3 pointAt[2];

// vr.Widen as the game started with it: it widens at the start only, so a change says "(next start)"
bool widenAtStart = true;
struct WidenRegistration
{
	WidenRegistration() {
		EventManager::listen(Event::Start, [](Event, void *) { widenAtStart = config::VrWiden; });
	}
} widenRegistration;

// The gun shown: what vr.Gun is, auto as what it comes to. (Task 13 decides auto, and draws the
// chosen gun; until then every choice draws the arcade gun, and auto is the arcade gun.)
int shownGun()
{
	const int gun = config::VrGun;
	return gun >= 1 && gun <= 3 ? gun : 3;
}

bool stepper(int row) {
	return row == Gun || row == Haptics || row == WorldSize || row == PanelDistance;
}
bool onOff(int row) {
	return row == Laser || row == WiderView;
}

std::string value(int row)
{
	char s[32];
	switch (row)
	{
	case Gun:
		return GunNames[shownGun()];
	case Laser:
		return config::VrLaser ? "On" : "Off";
	case Haptics:
		return HapticsNames[std::clamp((int)config::VrHaptics, 0, 3)];
	case WorldSize:
		std::snprintf(s, sizeof(s), "%.3f", (float)config::VrWorldScale);
		return s;
	case PanelDistance:
		std::snprintf(s, sizeof(s), "%.0f", (float)config::VrHudDepth);
		return s;
	case WiderView:
		return std::string(config::VrWiden ? "On" : "Off") + (config::VrWiden != widenAtStart ? " (next start)" : "");
	default:
		return "";
	}
}

RowsPanel panel()
{
	RowsPanel p;
	p.title = "PAUSED";
	for (int row = 0; row < Rows; row++)
	{
		PanelRow r;
		r.label = Labels[row];
		r.value = value(row);
		r.selected = row == selected;
		r.stepper = stepper(row);
		p.rows.push_back(r);
	}
	// (the font has no menu button sign)
	p.footer = "Point and pull the trigger, or stick and A.   Menu or B: back to the game";
	return p;
}

// A value row one step down (dir -1) or up (+1); an On/Off row the other way either way. Each
// change is saved at once; a change of the knock's strength is felt at once in the hand that made it.
void change(int row, int dir, int hand)
{
	switch (row)
	{
	case Gun:
	{
		const int gun = (shownGun() - 1 + dir + 3) % 3 + 1;
		config::VrGun.set(gun);
		config::saveInt("config", "vr.Gun", gun);
		break;
	}
	case Laser:
		config::VrLaser.set(!config::VrLaser);
		config::saveBool("config", "vr.Laser", config::VrLaser);
		break;
	case Haptics:
	{
		const int was = std::clamp((int)config::VrHaptics, 0, 3);
		const int strength = std::clamp(was + dir, 0, 3);
		if (strength == was)
			return;
		config::VrHaptics.set(strength);
		config::saveInt("config", "vr.Haptics", strength);
		NOTICE_LOG(INPUT, "XR: menu %s -> %s", Labels[row], value(row).c_str());
		sampleKnock(hand);
		return;
	}
	case WorldSize:
	{
		const float was = config::VrWorldScale;
		const float scale = std::clamp(dir > 0 ? was * 1.1f : was / 1.1f, 0.005f, 0.2f);
		if (scale == was)
			return;
		config::VrWorldScale.set(scale);
		config::saveFloat("config", "vr.WorldScale", scale);
		break;
	}
	case PanelDistance:
	{
		const float was = config::VrHudDepth;
		const float depth = std::clamp(was + 5.f * dir, 15.f, 120.f);
		if (depth == was)
			return;
		config::VrHudDepth.set(depth);
		config::saveFloat("config", "vr.HudDepth", depth);
		break;
	}
	case WiderView:
		config::VrWiden.set(!config::VrWiden);
		config::saveBool("config", "vr.Widen", config::VrWiden);
		break;
	default:
		return;
	}
	NOTICE_LOG(INPUT, "XR: menu %s -> %s", Labels[row], value(row).c_str());
}

// A row chosen (A, or a click away from a value row's halves): its action, or + on a value row.
void choose(int row, int hand)
{
	switch (row)
	{
	case Resume:
		NOTICE_LOG(INPUT, "XR: menu Resume");
		close();
		break;
	case Recenter:
		NOTICE_LOG(INPUT, "XR: menu Recenter");
		recenter();
		break;
	case ShowControls:
		NOTICE_LOG(INPUT, "XR: menu Show controls");
		showControls();
		close();
		break;
	case Quit:
		NOTICE_LOG(INPUT, "XR: menu quit");
		quitToStart();
		break;
	default:
		change(row, +1, hand);
		break;
	}
}

}	// namespace

bool isOpen()
{
	return menuPaused();
}

void place(const glm::mat4& head)
{
	const glm::vec3 eyes(head[3]);
	glm::vec3 forward = -glm::vec3(head[2]);
	// looking straight down (or up), the top of the head (or the chin) is where the face is turned
	if (std::abs(forward.x) + std::abs(forward.z) < 1e-3f)
		forward = glm::vec3(head[1]) * (forward.y < 0.f ? 1.f : -1.f);
	const glm::mat4 turn = glm::rotate(glm::mat4(1.f), std::atan2(-forward.x, -forward.z), glm::vec3(0.f, 1.f, 0.f));
	model = glm::translate(glm::mat4(1.f), eyes + glm::vec3(turn * glm::vec4(0.f, 0.f, -Distance, 0.f)) - glm::vec3(0.f, BelowEyes, 0.f)) * turn;
}

void open(const glm::mat4& head)
{
	place(head);
	selected = Resume;
	hovered[Left] = hovered[Right] = -1;
	pointing[Left] = pointing[Right] = false;
	fresh = true;
	const RowsLayout layout = rowsLayout(panel());
	NOTICE_LOG(INPUT, "XR: menu at %.2f m, %d rows, row pitch %.3f m, top row %+.3f m, %.2f m wide",
			Distance, (int)Rows, layout.rowPitch, layout.topRow, layout.width);
	requestPause(true);
}

void close()
{
	requestPause(false);
}

void update(const Input& input)
{
	if (!isOpen())
		return;
	if (input.toggle || input.back)
	{
		close();
		return;
	}
	const RowsPanel rows = panel();
	for (int hand : { Left, Right })
	{
		const Input::Ray& ray = input.ray[hand];
		float u = 0.5f;
		const int row = ray.valid ? rowAt(model, rows, ray.origin, ray.dir, &u) : -1;
		pointing[hand] = ray.valid && rowsPanelHit(model, rows, ray.origin, ray.dir, &pointAt[hand]);
		if (!fresh && row >= 0 && row != hovered[hand])
			selected = row;
		hovered[hand] = row;
		if (fresh || !ray.click || row < 0)
			continue;
		selected = row;
		NOTICE_LOG(INPUT, "XR: menu pointer click on %s at %.2f (%s hand)", Labels[row], u, hand == Right ? "right" : "left");
		if (stepper(row))
			change(row, u < 0.5f ? -1 : +1, hand);
		else if (onOff(row))
			change(row, +1, hand);
		else
			choose(row, hand);
	}
	fresh = false;
	if (input.up)
		selected = (selected + Rows - 1) % Rows;
	if (input.down)
		selected = (selected + 1) % Rows;
	if ((input.left || input.right) && (stepper(selected) || onOff(selected)))
		change(selected, input.left ? -1 : +1, Right);
	if (input.select)
		choose(selected, Right);
}

bool pointer(int hand, glm::vec3& at)
{
	if (!isOpen() || !pointing[hand])
		return false;
	at = pointAt[hand];
	return true;
}

void draw(const glm::mat4& viewProj)
{
	if (isOpen())
		drawRowsPanel(viewProj, model, panel());
}

}
