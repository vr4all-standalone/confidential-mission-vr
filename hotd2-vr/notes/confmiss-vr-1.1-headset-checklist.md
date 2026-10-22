# Confidential Mission VR 1.1: headset checklist

What the PC harness (the OpenXR simulator, `vr.XrSim`) cannot prove, to check in the headset.
Each task of the 1.1 plan adds its items under its own heading.

How to report: play the build, then mark each item `[x]` (works) or `[!]` (fails) and add a
short note after a failing one (what you saw, where in the game, which hand). Leave items you
did not get to as `[ ]`. Tell the next session "headset checklist updated"; it reads this file
and turns every `[!]` into a fix task. Keep `flycast.log` from a run with a failure (the Quest
build: the app's files directory, or the PC: next to `flycast.exe`) and say where it is.

## Task 1

- [ ] PC VR (Link/SteamVR) still uses the real runtime: the log says `XR: runtime <name>`, never `XRSIM`.

## Task 2

- [ ] Each Quest button does what the README table says (A, B, X, Y, the left menu button, both thumbstick clicks, trigger, grip + thumbstick); B's reload sounds like the game's own.

## Task 3

- [ ] The left menu button mid-game freezes the picture under a "PAUSED" panel; the picture still follows your head (look around and lean: it stays put in the room), the guns still follow your hands, and the game's sound stops. The menu button or B goes on exactly where it stopped, and B does not reload.
- [ ] Take the headset off with the menu open, wait 10 s, and put it back on: still paused, with the panel in front of you (recentred, as v1 does after more than 3 s away); no black headset, and the game has not run on.
- [ ] The Quest button mid-game (no menu open) and back: the game resumes as in v1.
