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
