# Confidential Mission 1.3.1 — local candidate

Gun model now has direct In-game pistol, Arcade gun and Zapper buttons. Installed
custom models remain available; unavailable models are individually disabled.
System shows the actual P1 left / P2 right and P1 right / P2 left layouts.

Game zoom temporarily pauses during native cutscenes, then resumes only while
the user's On choice remains enabled. The first visible native cutscene frame
uses neutral zoom; short gaps between native bars do not repeatedly toggle it.
Full-screen gameplay fades are distinguished from cinematic bars. The menu shows
Paused for cutscene while On is temporarily suspended. Explicit Off cancels the
return, and zoom still starts Off for every application/game session.

Native enemy ring/X overlays now follow the scene's zoom factor, preserving
their relationship to the enemies independently of the HUD size setting.
Original Service, fallback and render-to-texture behavior remains guarded.

The accepted 1.3.0 sectioned menu, native Apply settings and restart, laser,
reticle, player/hand controls and native option protocol remain. Package ID,
release signing identity, app label Confidential Mission - VR4ALL and saved
preferences/operator data are preserved. No game media is included.

Software, platform builds, simulator renders, device installation and physical
play are separate checks recorded in the overseer report. This is a local
candidate; GitHub publication requires the user's approval.
