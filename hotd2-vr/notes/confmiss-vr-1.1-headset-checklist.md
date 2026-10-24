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
- [ ] Quest home with the menu open, then back: the dark room and the menu, then resume works.
- [ ] Opening the menu: do you notice a stutter as the picture freezes? (The pause takes about 65 ms there, 3-4 headset frames.) Note yes or no.

## Task 5

- [ ] The Library shows "Confidential Mission VR" with its own icon (the CM monogram in red corner brackets), under Unknown sources.
- [ ] Opening it shows the 2D start screen ("CONFIDENTIAL MISSION VR"), not the game.
- [ ] All files access: on a fresh install step 1 says to allow it; Allow opens the setting, and after switching it on and coming back the screen says "Done." (Before that, the file rows say "Not checked yet".)
- [ ] Each missing file is named with where it goes: move `naomi.zip`, then the `confmiss` folder, then `confmiss.zip` out of Download in turn (Look again after each); the row in red says which and where. With all three back, every row says "Found" and the Play button appears.
- [ ] "Play Confidential Mission" starts the game in VR.
- [ ] With the game running, open the app from the Library again: note what you get (the game back, or the start screen). If the start screen, Play starts the game afresh.
- [ ] Then uninstall the 0.1.0 hub (`com.vrlightgunhub.app`): `adb uninstall com.vrlightgunhub.app`, or Settings > Apps.
- [ ] PC VR: unpack `confidential-mission-vr-pcvr.zip`, put the three files in its `roms\` folder (`confmiss.zip`, the `confmiss` folder with `gds-0001.chd`, `naomi.zip`), double-click `Play Confidential Mission.cmd`: the game boots in VR.

## Task 6

- [ ] The knock comes with the game's own shot (the shot sound and a bullet going from the HUD), not ahead of it or late; fire fast and every shot still gets its own knock. Shooting off the screen to reload gives no knock.
- [ ] With the magazine empty, a pull gives a light click, clearly weaker than a shot; after a reload the knock is back.
- [ ] Low, Normal and Strong feel different: set `vr.Haptics` to 1, 2 and 3 in emu.cfg (until the settings menu has it) and fire a few shots with each; 0 gives none at all.
- [ ] Two guns (X): player 2's knocks are in the left hand and player 1's in the right, each only for its own shots.

## Task 7

- [ ] The left menu button mid-game opens the menu ("PAUSED", ten rows) about an arm's length ahead, a little below your eyes: every row and the footer are readable at arm's length without leaning in, and the panel is not cut off at the edges of the view.
- [ ] Laser pointing with both hands: each hand's line and dot land on the row it points at (with the laser off too), the selection follows, and the trigger of either hand chooses.
- [ ] Stick plus A: the right and the left thumbstick move the selection up and down (round from Quit to Resume and back) and left/right change a value; A chooses, and on a value row is +.
- [ ] The - and + halves: a click on the left half of Haptics, World size, Panel distance and Gun steps down, on the right half up; a click anywhere on Laser or Wider view flips it. Each Haptics change gives a sample knock in that hand.
- [ ] Choose Quit twice: the first choose turns its row into "Quit? Choose again" (moving off it, a stick move or 3 s turns it back); the second, within 3 s, returns to the start screen ("CONFIDENTIAL MISSION VR"), and Play starts the game again.
- [ ] Settings are kept after restarting the app: change Laser, Haptics, World size and Panel distance, quit, close the app, open it and play: all four are as you left them (and Wider view, changed, applies now).
- [ ] The menu button or B closes the menu and the game goes on; the closing B does not reload, and the A that chose Resume does not press Start.

## Task 8

- [ ] The Quest 3 shows its own controllers in the hands (the log says "XR: controller models from the runtime"), sitting right: the model's handle where your hand is and its buttons, stick and trigger over the real ones, moving with no lag. If not, note how far off (cm, which way) and how far turned (degrees).
- [ ] At the first start there is no permission prompt, or an understandable one (about the controllers' models); "XR: runtime extensions:" in the log lists XR_FB_render_model.
- [ ] On PC VR (SteamVR, Virtual Desktop: a runtime without XR_FB_render_model) the built-in shape is in the hands instead ("XR: controller models built in (...)"): it reads as a controller held the right way, buttons up under the thumb, the trigger under the index finger; the left one has the small menu button where a Quest 3's is.
- [ ] Only free hands show a controller: the gun hand shows the gun, and with the game's hands on, the other hand shows the game's open hand. With the menu open, a free hand's pointer line starts at its controller. `vr.ShowControllers = no` in emu.cfg hides the controllers and nothing else.
- [ ] If the Quest's controllers look plain light grey, the runtime sent KTX2 textures (not decoded yet); the shape should still be Meta's. (The log's "XR: render model" lines then end in "untextured (image/ktx2)".)

## Task 9

- [ ] Before the first Start or shot, both hands show the Quest 3's own controllers with short labels by the buttons: right "A Start", "B Reload", "Trigger Shoot", "Stick click Laser"; left "X Player 2", "Y Reload P2", "Menu", "Stick click Recenter". Each is readable at arm's length without leaning in, and each line ends on its own button on the Quest 3's models (note any that point beside it: which, and how far off).
- [ ] The labels go at the first A (the right hand turns into the gun) or the first trigger pull (that hand turns into the gun, and the pull fires).
- [ ] X: the left hand turns into player 2's gun; when player 2 is out again (the game asks for player 2 in its corner), it is a controller again, without labels.
- [ ] The menu's Show controls: both hands controllers with their labels again; the next trigger pull gives the gun(s) back and fires.

## Task 10

Before these, delete `gun-cm-cabinet.bin` from the app's data folder (or reinstall), so the first start makes it.

- [ ] The first start shows the panel ("CONFIDENTIAL MISSION VR", "Taking the guns from your copy of the game") instead of the game, and it goes within about two minutes (the log: "VR hands: done after N s of game").
- [ ] B skips it: the game starts with the built-in arcade gun, and the next start shows the panel again.
- [ ] The cabinet gun in the hand faces the right way (barrel forward, grip down in your hand), at about the right size (about 27 cm long), with the muzzle flash and the laser starting at its muzzle.
- [ ] The second start has no panel ("VR hands: the guns, made before" in the log), and the cabinet gun is in the hand again.
