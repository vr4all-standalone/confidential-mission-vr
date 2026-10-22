/*
	OpenXR simulator (hotd2-vr): a headset and two controllers inside Flycast, for the PC
	test harness (vr-test.ps1 -XrSim) on a machine with no headset.

	With vr.Xr and vr.XrSim, xr_host takes its OpenXR functions from here instead of the
	loader. The simulator is the runtime "Flycast XR simulator": two 832x720 eyes (a Quest 3's
	field of view, 64 mm apart), 72 Hz, the controllers of /interaction_profiles/ +
	vr.XrSimProfile. xrEndFrame puts the left eye in the window, so window shots show the
	headset's view. The head and hands are where the harness puts them, and the controllers'
	inputs are what it sends, over the light gun's UDP port (vr.UdpPort):

		XR <path> <value>                          e.g. XR /user/hand/right/input/a/click 1
		XRPOSE <head|left|right> x y z yaw pitch [roll]   metres and degrees, in its stage space
		XRSTATE <focused|visible>                  visible: the app loses focus (a system menu)

	PC builds only: on Android active() is false and none of this is built.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#pragma once
#ifdef USE_OPENXR
#include <openxr/openxr.h>
#endif

namespace vr::xr::sim
{

#if defined(USE_OPENXR) && !defined(__ANDROID__)
// vr.XrSim on a PC build (read once)
bool active();
// The simulator's OpenXR function of that name, nullptr if it has none.
PFN_xrVoidFunction proc(const char *name);
// A line from the UDP port that starts with "XR" (see above). Any thread.
void udpLine(const char *line);
#else
inline bool active() { return false; }
#ifdef USE_OPENXR
inline PFN_xrVoidFunction proc(const char *) { return nullptr; }
#endif
inline void udpLine(const char *) {}
#endif

}
