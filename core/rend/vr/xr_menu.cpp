/*
	The settings menu in the headset (hotd2-vr). See xr_menu.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_menu.h"
#include "xr_hands.h"
#include "xr_gun.h"
#include "xr_own.h"
#include "xr_host.h"
#include "xr_panel.h"
#include "view_settings.h"
#include "cm_game_options.h"
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

const char *const Labels[Rows] { "Gun model", "Gun angle", "Reload", "Two players", "Laser", "Crosshair", "Crosshair min size", "Crosshair max size", "Crosshair shrink distance", "Haptics",
 "World size", "Panel distance", "HUD size", "Wider view", "Game zoom", "How to play", "Open Service Menu", "Reset to defaults", "Quit", "Advanced", "Advanced", "Cancel", "Confirm", "Apply settings and restart", "Gun size", "Game difficulty", "Boss difficulty", "Shifting difficulty", "Collision size", "Life" };
const char *const HapticsNames[4] { "Off", "Light", "Medium", "Strong" };
// vr.Haptics' words, saved by the Haptics row (hapticsLevel() reads them, and 1.2's numbers)
const char *const HapticsWords[4] { "off", "light", "medium", "strong" };
const char *const ReloadNames[2] { "Shoot off-screen", "Point off-screen" };
// vr.TwoPlayerP1Hand by twoPlayerP1Hand() (0 left, the default; 1 right), the user's words: the game shows P1 on
// the left, P2 on the right. It applies once player 2 is in; playing alone either hand's trigger takes the gun.
const char *const HandsNames[2] { "P1 left / P2 right", "P1 right / P2 left" };
constexpr int GunAngleMax = 30;	// vr.GunAngle's range, degrees either way

glm::mat4 model { 1.f };	// the panel: its middle, facing the player (+z)
enum class Page { Gun, World, System, Game, GunAdvanced, WorldAdvanced };
enum class Confirmation { None, Reset, Quit };
Page page = Page::Gun;
Confirmation confirmation = Confirmation::None;
SettingsHit focus { SettingsHit::Kind::Row, 0 };
int activeHand = -1;
SettingsHit hovered;
bool fresh, gunsTurned, quitting, activeTracked;
bool pointing[2] {};
glm::vec3 pointAt[2];

int mainTab() {
 return page == Page::GunAdvanced ? 0 : page == Page::WorldAdvanced ? 1 : (int)page;
}
bool advanced() { return page == Page::GunAdvanced || page == Page::WorldAdvanced; }
bool nativeRow(int row) { return row >= NativeGameDifficulty && row <= NativeLife; }
cm::options::Field nativeField(int row) { return static_cast<cm::options::Field>(row - NativeGameDifficulty); }
std::vector<int> pageRows() {
 if (confirmation != Confirmation::None) return { Cancel, Confirm };
 switch (page) {
 case Page::Gun: return { Gun, GunAngle, Laser, Reticle, Haptics, AdvancedGun };
 case Page::GunAdvanced: return { GunSize, ReticleMinSize, ReticleMaxSize, ReticleShrinkDistance, Reload };
 case Page::World: return { WiderView, GameZoom, AdvancedWorld };
 case Page::WorldAdvanced: return { WorldSize, HudSize, PanelDistance };
 case Page::System: return { TwoPlayers, ServiceMenu, HowToPlay, ResetDefaults, Quit };
 case Page::Game: {
  const auto state = cm::options::snapshot();
  std::vector<int> rows;
  for (unsigned field = 0; field < cm::options::FieldCount; ++field)
   if (state.supportedMask & (1u << field)) rows.push_back(NativeGameDifficulty + field);
  if (rows.empty()) return { ServiceMenu };
  rows.push_back(ApplyGame); return rows;
 }
 }
 return {};
}
void setPage(Page next) {
 page = next; confirmation = Confirmation::None;
 focus = { SettingsHit::Kind::Row, 0 }; hovered = {}; fresh = true;
}
void back() {
 if (confirmation != Confirmation::None) { confirmation = Confirmation::None; focus = { SettingsHit::Kind::Row, 0 }; }
 else if (advanced()) setPage(mainTab() == 0 ? Page::Gun : Page::World);
 else close();
}

// vr.Widen as the game started with it: it widens at the start only, so a change says "(next start)"
bool widenAtStart = true;
struct WidenRegistration
{
	WidenRegistration() {
		EventManager::listen(Event::Start, [](Event, void *) { widenAtStart = config::VrWiden; });
	}
} widenRegistration;

// vr.Gun's place in its choices (xr_hands.h gunChoices()): a folder gun by vr.GunFile (4 and over all
// meaning that, its name in any case); any other setting, 1.1's 0, 2 and 3 too, the in-game pistol's, as
// chosenGun() reads them; not there: no selected pill
int gunAt(const std::vector<GunChoice>& choices)
{
	const int setting = config::VrGun >= OwnGuns ? OwnGuns : 1;
	for (size_t i = 0; i < choices.size(); i++)
		if (choices[i].setting == setting && (choices[i].file == nullptr || own::sameStem(config::VrGunFile.get(), choices[i].file)))
			return (int)i;
	return -1;
}

bool stepper(int row) {
	return nativeRow(row) || row == GunSize || row == Gun || row == GunAngle || row == Reload || row == Reticle || row == ReticleMinSize || row == ReticleMaxSize || row == ReticleShrinkDistance || row == Haptics || row == WorldSize || row == PanelDistance || row == HudSize;
}
// The rows a stick held left or right steps again and again (Input::repeat)
bool repeats(int row) {
	return row == GunSize || row == GunAngle || row == ReticleMinSize || row == ReticleMaxSize || row == ReticleShrinkDistance || row == WorldSize || row == PanelDistance || row == HudSize;
}
int gunAngle() {
	return std::clamp((int)config::VrGunAngle, -GunAngleMax, GunAngleMax);
}
// (Two players too: either way, its other value)
bool onOff(int row) {
	return row == TwoPlayers || row == Laser || row == WiderView || row == GameZoom || row == HowToPlay;
}

config::Option<float>& gunSize() { return gameGun(chosenGun()) ? config::VrHandScale : config::VrGunScale; }
const char* gunSizeKey() { return gameGun(chosenGun()) ? "vr.HandScale" : "vr.GunScale"; }

std::string value(int row)
{
 if (nativeRow(row)) {
  const auto state = cm::options::snapshot(); const unsigned field = row - NativeGameDifficulty;
  const char* label = cm::options::valueLabel(nativeField(row), state.value[field]);
  return label ? label : std::to_string(state.value[field]);
 }
 char s[32];
	switch (row)
	{
	case Gun:
	{
		// a gun not made yet shows dimmed (only when vr.Gun names it: the row skips it)
		const std::vector<GunChoice> choices = gunChoices();
		const int at = gunAt(choices);
        if (at < 0) return config::VrGun >= OwnGuns ? config::VrGunFile.get() + " (not there)" : "No model available";
		const GunChoice& gun = choices[at];
		return std::string(gun.label) + (!gun.made ? (gun.setting >= 4 ? " (not there)" : config::VrGameHands ? " (not made yet)" : " (game's models off)")
				: !gun.complete ? " (incomplete)" : "");
	}
	case GunSize:
  std::snprintf(s, sizeof(s), "%.0f%%", gunSize().get() * 100.f); return s;
	case GunAngle:
		// (with a sign, no degree sign: the panel adds it, the log stays ASCII)
		std::snprintf(s, sizeof(s), gunAngle() == 0 ? "%d" : "%+d", gunAngle());
		return s;
	case Reload:
		return ReloadNames[std::clamp((int)config::VrReload, 0, 1)];
	case TwoPlayers:
		return HandsNames[twoPlayerP1Hand()];
	case Laser:
		return vr::laser::choiceLabel(config::VrLaser, config::VrLaserWidth);
	case Reticle:
        return vr::reticle::modeLabel(config::VrReticleMode);
    case ReticleMinSize:
        std::snprintf(s,sizeof(s),"%.0f%%",vr::reticle::minimum(config::VrReticleMinSize)*100.f);return s;
    case ReticleMaxSize:
        std::snprintf(s,sizeof(s),"%.0f%%",vr::reticle::maximum(config::VrReticleMaxSize)*100.f);return s;
    case ReticleShrinkDistance:
        std::snprintf(s,sizeof(s),"%.1f m",vr::reticle::distance(config::VrReticleShrinkDistance));return s;
	case Haptics:
		return HapticsNames[hapticsLevel()];
	case WorldSize:
		std::snprintf(s, sizeof(s), "%.3f", (float)config::VrWorldScale);
		return s;
	case PanelDistance:
		std::snprintf(s, sizeof(s), "%.0f", (float)config::VrHudDepth);
		return s;
	case HudSize:
        std::snprintf(s,sizeof(s),"%.0f%%",vr::hud::size(config::VrHudSize)*100.f);return s;
	case WiderView:
		return std::string(vr::view::label(config::VrWidenMode)) + (config::VrWiden != widenAtStart ? " (next start)" : "");
	case GameZoom:
		return gameZoomOn()?"On":"Off";
	case HowToPlay:
		return howToPlayOn() ? "On" : "Off";
	case ServiceMenu:
		return serviceMenuAvailable() ? "" : "Offline / after loading";
	default:
		return "";
	}
}

SettingsPanel panel()
{
 SettingsPanel p;
 const auto native = cm::options::snapshot();
 p.locked = native.busy;
 p.activeTab = mainTab();
 p.selectedTab = focus.kind == SettingsHit::Kind::Tab ? focus.index : -1;
 p.back = advanced(); p.backSelected = focus.kind == SettingsHit::Kind::Back;
 p.closeSelected = focus.kind == SettingsHit::Kind::Close;
 p.dialog = confirmation != Confirmation::None;
 const char* titles[] = { "Gun", "World", "System", "In-game", "Advanced gun settings", "Advanced world settings" };
 p.title = titles[(int)page];
 p.subtitle = page == Page::GunAdvanced ? "Fine tune crosshair sizing and CM reload behavior." :
  page == Page::WorldAdvanced ? "Exact saved sizes; HUD resizing applies to Wider gameplay." :
  page == Page::Game ? native.message ? native.message : "Native operator settings" :
  "Confidential Mission / VR settings";
 if (p.dialog) {
  p.title = confirmation == Confirmation::Reset ? "Reset VR settings?" : "Quit to the launcher?";
  p.subtitle = confirmation == Confirmation::Reset ? "Restore VR preferences. Native operator settings and game data are preserved." : "Leave the current game and return to the launcher.";
 }
 const auto rows = pageRows();
 for (int index = 0; index < (int)rows.size(); ++index)
 {
  const int row = rows[index];
  SettingsRow r;
  r.label = nativeRow(row) ? cm::options::label(nativeField(row)) : row == Confirm ? confirmation == Confirmation::Reset ? "Reset VR settings" : "Quit" : Labels[row];
  if (row == ServiceMenu && serviceMenuExitAvailable()) r.label = "Exit Service Menu";
  r.value = value(row);
  if (row == GunAngle) r.value += "\xB0";
  r.selected = focus.kind == SettingsHit::Kind::Row && focus.index == index;
  r.stepper = stepper(row);
  r.action = row == ApplyGame || row == AdvancedGun || row == AdvancedWorld || row == ServiceMenu || row == ResetDefaults || row == Quit || row == Cancel || row == Confirm;
  if (row == ServiceMenu) r.dim = !serviceMenuAvailable();
  if (row == Gun) {
   const auto choices = gunChoices();
   r.choice = gunAt(choices); r.stepper = false;
   r.dim = std::none_of(choices.begin(), choices.end(), [](const GunChoice& gun) { return gun.made; });
   for (const auto& gun : choices) { r.choices.push_back(gun.label); r.choiceEnabled.push_back(gun.made); }
   if (r.choice < 0 || !choices[r.choice].made || !choices[r.choice].complete) r.note = r.value;
   r.value.clear();
  }
  if (row == Laser) { r.choices = { "Off", "Normal", "5x", "10x" }; r.choice = vr::laser::choice(config::VrLaser, config::VrLaserWidth); }
  if (row == WiderView) { r.choices = { "Off", "On", "Fade" }; r.choice = vr::view::mode(config::VrWidenMode); if (config::VrWiden != widenAtStart) r.note = "Applies next start"; }
  if (row == Reticle) { r.choices = { "Custom", "None" }; r.choice = vr::reticle::mode(config::VrReticleMode) - 1; r.stepper = false; }
  if (row == Haptics) { r.choices = { "Off", "Light", "Medium", "Strong" }; r.choice = hapticsLevel(); r.stepper = false; }
  if (row == GameZoom || row == HowToPlay) { r.choices = { "Off", "On" }; r.choice = row == GameZoom ? gameZoomOn() : howToPlayOn(); }
  if (row == GameZoom && gameZoomOn() && gameZoomPaused()) r.note = "Paused for cutscene";
  if (row == TwoPlayers) { r.choices = { HandsNames[0], HandsNames[1] }; r.choice = twoPlayerP1Hand(); r.stepper = false; r.value.clear(); }
  if (row == Reload) { r.note = r.value; r.value = "Change"; }
  if (nativeRow(row)) {
   r.dim = !native.available;
   r.note = native.pendingMask & (1u << (row - NativeGameDifficulty)) ? "Pending restart" : "Native operator setting";
  }
  if (row == ApplyGame) r.dim = !native.available || native.pendingMask == 0;
  r.dim = r.dim || p.locked;
  p.rows.push_back(r);
 }
 p.footer = page == Page::Game ? native.busy ? "Saving native settings; please wait for the restarted game." : native.pendingMask ? "Pending changes survive closing. Apply settings and restart to save." : "Native changes are staged until Apply settings and restart." :
  page == Page::World || howToPlayOn() ? "During gameplay: tap either grip to toggle zoom. Starts Off." :
  page == Page::GunAdvanced ? "Gun size applies to the current model. Aim and trigger; B goes back." :
  "Aim + trigger to select | Stick to adjust | B to go back";
 return p;
}

// Both direct model pills and row/stick adjustment use the same file identity and availability.
void selectGun(const std::vector<GunChoice>& choices, int at)
{
 if (at < 0 || at >= (int)choices.size() || !choices[at].made || at == gunAt(choices)) return;
 const auto& gun = choices[at];
 config::VrGun.set(gun.setting); config::saveInt("config", "vr.Gun", gun.setting);
 if (gun.file != nullptr) {
  config::VrGunFile.set(gun.file); config::saveStr("config", "vr.GunFile", gun.file);
 }
 chosenGun(); gunsTurned = true; // The new model can move the ray; consume that transition frame.
 NOTICE_LOG(INPUT, "XR: menu %s -> %s", Labels[Gun], value(Gun).c_str());
}

// A value row one step down (dir -1) or up (+1); an On/Off row the other way either way. Each
// change is saved at once; a change of the knock's strength is felt at once in the hand that made it.
void change(int row, int dir, int hand)
{
 if (nativeRow(row)) {
  const auto state = cm::options::snapshot(); const unsigned field = row - NativeGameDifficulty;
  if (state.available && !state.busy)
   cm::options::stage(nativeField(row), std::clamp(int(state.value[field]) + dir, int(state.minimum[field]), int(state.maximum[field])));
  return;
 }
	switch (row)
	{
	case Gun:
 {
  const auto choices = gunChoices(); const int n = (int)choices.size();
  if (n < 2) return;
  int at = gunAt(choices);
  if (at < 0) at = dir > 0 ? -1 : 0;
  for (int i = 0; i < n; ++i) {
   at = (at + dir + n) % n;
   if (choices[at].made) { selectGun(choices, at); return; }
  }
  return;
 }
	case GunSize:
 {
  const float size = std::clamp(gunSize().get() + dir * .05f, .4f, 1.6f);
  gunSize().set(size); config::saveFloat("config", gunSizeKey(), size); gunsTurned = true; break;
 }
	case GunAngle:
	{
		// 1 degree a step; at an end nothing (no save, no line)
		const int was = gunAngle();
		const int angle = std::clamp(was + dir, -GunAngleMax, GunAngleMax);
		if (angle == was)
			return;
		config::VrGunAngle.set(angle);
		config::saveInt("config", "vr.GunAngle", angle);
		gunsTurned = true;
		break;
	}
	case Reload:
	{
		const int was = std::clamp((int)config::VrReload, 0, 1);
		const int reload = std::clamp(was + dir, 0, 1);
		if (reload == was)
			return;
		config::VrReload.set(reload);
		config::saveInt("config", "vr.Reload", reload);
		break;
	}
	case TwoPlayers:
	{
		// with two players in, the guns swap hands once the menu closes (xr_host.cpp updateLightgun); playing
		// alone nothing moves: it's for the next time player 2 joins
		const char *const hand = twoPlayerP1Hand() == Left ? "right" : "left";
		config::VrTwoPlayerP1Hand.set(hand);
		config::saveStr("config", "vr.TwoPlayerP1Hand", hand);
		break;
	}
	case Laser:
 {
  const int choice = vr::laser::nextChoice(config::VrLaser, config::VrLaserWidth, dir);
  config::VrLaser.set(choice != 0);
  if (choice != 0) config::VrLaserWidth.set(vr::laser::savedWidth(choice));
  config::saveBool("config", "vr.Laser", config::VrLaser);
  config::saveInt("config", "vr.LaserWidth", config::VrLaserWidth);
  break;
 }
	case Reticle:
        config::VrReticleMode.set(vr::reticle::nextMode(config::VrReticleMode,dir));
        config::saveInt("config","vr.ReticleMode",config::VrReticleMode);break;
    case ReticleMinSize:
    {
        const float value=std::clamp(std::round(vr::reticle::minimum(config::VrReticleMinSize)*100.f)+dir,5.f,100.f)/100.f;
        config::VrReticleMinSize.set(value);config::saveFloat("config","vr.ReticleMinSize",value);break;
    }
    case ReticleMaxSize:
    {
        const float value=std::clamp(std::round(vr::reticle::maximum(config::VrReticleMaxSize)*100.f)+dir*5.f,25.f,300.f)/100.f;
        config::VrReticleMaxSize.set(value);config::saveFloat("config","vr.ReticleMaxSize",value);break;
    }
    case ReticleShrinkDistance:
    {
        const float value=std::clamp(std::round(vr::reticle::distance(config::VrReticleShrinkDistance)*10.f)+dir,2.f,200.f)/10.f;
        config::VrReticleShrinkDistance.set(value);config::saveFloat("config","vr.ReticleShrinkDistance",value);break;
    }
	case Haptics:
	{
		const int was = hapticsLevel();
		const int strength = std::clamp(was + dir, 0, 3);
		if (strength == was)
			return;
		config::VrHaptics.set(HapticsWords[strength]);
		config::saveStr("config", "vr.Haptics", HapticsWords[strength]);
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
	case HudSize:
    {
        const float size=vr::hud::step(config::VrHudSize,dir);
        config::VrHudSize.set(size);config::saveFloat("config","vr.HudSize",size);break;
    }
	case WiderView:
  config::VrWidenMode.set(vr::view::nextMode(config::VrWidenMode, dir));
  config::VrWidenMode.save();
  break;
	case GameZoom:
		setGameZoom(!gameZoomOn());
		break;
	case HowToPlay:
		// on or off, the menu staying open (closing it turns it off: xr_host.h). The next frame the
		// hands swap gun and controller, and a ray moves by the muzzle's offset: that update learns
		// the rows again, so the selection stays on this row.
		NOTICE_LOG(INPUT, "XR: menu How to play");
		setHowToPlay(!howToPlayOn(), "chosen");
		fresh = true;
		return;
	default:
		return;
	}
	NOTICE_LOG(INPUT, "XR: menu %s -> %s", Labels[row], value(row).c_str());
	if (row == Gun)
		chosenGun();	// (its "XR: gun: ..." line at once)
}

void resetMenuSettings()
{
	setGameZoom(false);
    // Only VR menu settings are reset. Game data and native operator settings remain intact.
    config::setAutoSave(false);
    config::VrGunScale.reset(); config::saveFloat("config", "vr.GunScale", config::VrGunScale.get());
    config::VrHandScale.reset(); config::saveFloat("config", "vr.HandScale", config::VrHandScale.get());
    config::VrGun.reset();
    config::saveInt("config", "vr.Gun", config::VrGun.get());
    config::VrGunFile.reset();
    config::saveStr("config", "vr.GunFile", config::VrGunFile.get());
    config::VrGunAngle.reset();
    config::saveInt("config", "vr.GunAngle", config::VrGunAngle.get());
    config::VrReload.reset();
    config::saveInt("config", "vr.Reload", config::VrReload.get());
    config::VrTwoPlayerP1Hand.reset();
    config::saveStr("config", "vr.TwoPlayerP1Hand", config::VrTwoPlayerP1Hand.get());
    config::VrLaserWidth.reset();
    config::saveInt("config", "vr.LaserWidth", config::VrLaserWidth.get());
    config::VrLaser.reset();
    config::saveBool("config", "vr.Laser", config::VrLaser.get());
    config::VrReticleMode.reset();
    config::saveInt("config", "vr.ReticleMode", config::VrReticleMode.get());
    config::VrReticleMinSize.reset();
    config::saveFloat("config", "vr.ReticleMinSize", config::VrReticleMinSize.get());
    config::VrReticleMaxSize.reset();
    config::saveFloat("config", "vr.ReticleMaxSize", config::VrReticleMaxSize.get());
    config::VrReticleShrinkDistance.reset();
    config::saveFloat("config", "vr.ReticleShrinkDistance", config::VrReticleShrinkDistance.get());
    config::VrHaptics.reset();
    config::saveStr("config", "vr.Haptics", config::VrHaptics.get());
    config::VrWorldScale.reset();
    config::saveFloat("config", "vr.WorldScale", config::VrWorldScale.get());
    config::VrHudDepth.reset();
    config::saveFloat("config", "vr.HudDepth", config::VrHudDepth.get());
    config::VrHudSize.reset();
    config::saveFloat("config", "vr.HudSize", config::VrHudSize.get());
    config::VrWidenMode.reset();
    config::VrWidenMode.save();
    config::setAutoSave(true);
}

// Precise pills choose their literal value; the body of every value row advances.
void exactChoice(int row, int choice, int hand)
{
 if (row == Gun) { selectGun(gunChoices(), choice); return; }
 int was = 0;
 switch (row) {
 case TwoPlayers:
  if (choice < 0 || choice > 1) return;
  was = twoPlayerP1Hand(); break;
 case Laser: was = vr::laser::choice(config::VrLaser, config::VrLaserWidth); break;
 case WiderView: was = vr::view::mode(config::VrWidenMode); break;
 case Reticle: was = vr::reticle::mode(config::VrReticleMode) - 1; break;
 case Haptics: was = hapticsLevel(); break;
 case GameZoom: was = gameZoomOn(); break;
 case HowToPlay: was = howToPlayOn(); break;
 default: return;
 }
 if (choice == was) return;
 if (row == Laser) {
  config::VrLaser.set(choice != 0); if (choice != 0) config::VrLaserWidth.set(vr::laser::savedWidth(choice));
  config::saveBool("config", "vr.Laser", config::VrLaser); config::saveInt("config", "vr.LaserWidth", config::VrLaserWidth); return;
 }
 if (row == WiderView) { config::VrWidenMode.set(choice); config::VrWidenMode.save(); return; }
 if (row == Haptics) {
  config::VrHaptics.set(HapticsWords[choice]); config::saveStr("config", "vr.Haptics", HapticsWords[choice]); sampleKnock(hand); return;
 }
 change(row, choice > was ? +1 : -1, hand);
}

void choose(int row, int hand)
{
 switch (row) {
 case AdvancedGun: setPage(Page::GunAdvanced); break;
 case AdvancedWorld: setPage(Page::WorldAdvanced); break;
 case Cancel: back(); break;
 case ResetDefaults: confirmation = Confirmation::Reset; focus = { SettingsHit::Kind::Row, 0 }; fresh = true; break;
 case Quit: confirmation = Confirmation::Quit; focus = { SettingsHit::Kind::Row, 0 }; fresh = true; break;
 case Confirm:
  if (confirmation == Confirmation::Reset) {
   resetMenuSettings(); setHowToPlay(false, "reset to defaults"); chosenGun();
   confirmation = Confirmation::None; focus = { SettingsHit::Kind::Row, 0 }; gunsTurned = fresh = true;
  } else if (confirmation == Confirmation::Quit) { confirmation = Confirmation::None; quitting = true; quitToStart(); }
  break;
 case ServiceMenu: if (openServiceMenu()) close(); break;
 case ApplyGame: cm::options::apply(); break;
 default: change(row, +1, hand); break;
 }
}

void activate(const SettingsHit& hit, int hand)
{
 if (cm::options::busy()) return;
 if (hit.kind == SettingsHit::Kind::Close) { close(); return; }
 if (hit.kind == SettingsHit::Kind::Back) { back(); return; }
 if (hit.kind == SettingsHit::Kind::Tab) { setPage((Page)hit.index); return; }
 const auto rows = pageRows();
 if (hit.kind != SettingsHit::Kind::Row || hit.index < 0 || hit.index >= (int)rows.size()) return;
 const auto p = panel();
 if (p.rows[hit.index].dim) return;
 const int row = rows[hit.index];
 if (hit.choice >= 0) exactChoice(row, hit.choice, hand);
 else if (hit.direction) change(row, hit.direction, hand);
 else choose(row, hand);
}

bool sameFocus(const SettingsHit& a, const SettingsHit& b) { return a.kind == b.kind && a.index == b.index; }
std::vector<SettingsHit> navigation()
{
 std::vector<SettingsHit> hits;
 const auto p = panel();
 if (!p.dialog) {
  hits.push_back({ SettingsHit::Kind::Close });
  for (int t = 0; t < 4; ++t) hits.push_back({ SettingsHit::Kind::Tab, t });
  if (p.back) hits.push_back({ SettingsHit::Kind::Back });
 }
 for (int i = 0; i < (int)p.rows.size(); ++i) if (!p.rows[i].dim) hits.push_back({ SettingsHit::Kind::Row, i });
 return hits;
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
 setPage(Page::Gun);
 activeHand = -1; hovered = {};
 pointing[Left] = pointing[Right] = false;
 fresh = true; gunsTurned = quitting = activeTracked = false;
 own::scan();
	requestPause(true);
}

void close()
{
 confirmation = Confirmation::None;
 pointing[Left] = pointing[Right] = false; activeTracked = false;
 requestPause(false);
}

void reset()
{
 confirmation = Confirmation::None; quitting = false; activeHand = -1;
}

void update(const Input& input)
{
 if (!isOpen() || quitting) return;
 const bool locked = cm::options::busy();
 if (!locked && (input.toggle || input.back)) { back(); return; }
 const bool perHand = input.perHand || input.hand[0].select || input.hand[0].up || input.hand[0].down || input.hand[0].left || input.hand[0].right || input.hand[0].repeat ||
  input.hand[1].select || input.hand[1].up || input.hand[1].down || input.hand[1].left || input.hand[1].right || input.hand[1].repeat;
 bool transfer = false;
 if (activeHand < 0) {
  activeHand = input.ray[Right].valid ? Right : input.ray[Left].valid ? Left : Right;
 }
 // Only a tracked other-hand trigger may transfer control; the transfer frame does nothing else.
 const int other = 1 - activeHand;
 if (input.ray[other].valid && input.ray[other].click) {
  activeHand = other; hovered = {}; transfer = true;
 }
 const Input::Ray& ray = input.ray[activeHand];
 activeTracked = ray.valid;
 const auto p = panel();
 pointing[Left] = pointing[Right] = false;
 pointing[activeHand] = ray.valid && settingsPanelHit(model, p, ray.origin, ray.dir, &pointAt[activeHand]);
 const auto hit = ray.valid ? settingsHit(model, p, ray.origin, ray.dir) : SettingsHit{};
 if (!fresh && !gunsTurned && !transfer && hit.kind != SettingsHit::Kind::None && !sameFocus(hit, hovered)) focus = hit;
 hovered = hit;
 const bool suppressClick = fresh || gunsTurned || transfer;
 fresh = gunsTurned = false;
 if (transfer || locked) return;
 const Input::Hand controls = perHand ? input.hand[activeHand] : Input::Hand { input.select, input.up, input.down, input.left, input.right, input.repeat };
 if (controls.up || controls.down) {
  const auto hits = navigation();
  if (!hits.empty()) {
   int at = -1; for (int i = 0; i < (int)hits.size(); ++i) if (sameFocus(hits[i], focus)) at = i;
   at = (at + (controls.up ? -1 : 1) + (int)hits.size()) % (int)hits.size(); focus = hits[at];
  }
 }
 if (!suppressClick && ray.click && hit.kind != SettingsHit::Kind::None) { focus = hit; activate(hit, activeHand); return; }
 if (controls.left || controls.right) {
  const int direction = controls.left ? -1 : 1;
  if (focus.kind == SettingsHit::Kind::Tab && !controls.repeat) {
   const int tab = (focus.index + direction + 4) % 4; setPage((Page)tab); focus = { SettingsHit::Kind::Tab, tab };
  } else if (focus.kind == SettingsHit::Kind::Row) {
   const auto rows = pageRows();
   if (focus.index >= 0 && focus.index < (int)rows.size() && !panel().rows[focus.index].dim) {
    const int row = rows[focus.index];
    if ((stepper(row) || onOff(row)) && (!controls.repeat || repeats(row))) change(row, direction, activeHand);
   }
  }
 }
 if (controls.select) { auto chosen = focus; chosen.choice = -1; chosen.direction = 0; activate(chosen, activeHand); }
}

bool pointer(int hand, glm::vec3& at)
{
 if (!isOpen() || hand < 0 || hand > 1 || hand != activeHand || !pointing[hand]) return false;
 at = pointAt[hand]; return true;
}

int pointerHand() { return isOpen() && activeTracked ? activeHand : -1; }

void draw(const glm::mat4& viewProj)
{
 if (isOpen()) drawSettingsPanel(viewProj, model, panel());
}

void drawContact(const glm::mat4& viewProj)
{
 if (!isOpen()) return;
 if (activeHand >= 0 && pointing[activeHand]) {
  glm::mat4 contact = model;
  contact[3] = glm::vec4(pointAt[activeHand] + glm::vec3(model[2]) * .004f, 1.f);
  drawPanelContact(viewProj, contact);
 }
}

}
