# Confidential Mission VR 1.1: design

## Intent

v1 (shipped to the user's Quest 3 as "VR Light Gun Hub" 0.1.0) plays Confidential Mission (NAOMI set `confmiss`, CM) in VR, and the user loves it. 1.1 makes it the user's own standalone app and adds what they asked for after playing:
- the gun from the game in hand;
- a button layout they chose;
- an in-headset settings menu "like port0r";
- Quest controllers that show how to play before the game starts.

HOTD2 VR and Maze of the Kings VR are another person's mods (mikermak's). We learned from them, and the engine is their GPL Flycast fork, but this app is not their launcher or hub. The user may later build a launcher for several standalone mods; each mod must stand alone.

Research behind this design, in `.superpowers/sdd/2026-10-21-confidential-mission-vr/`:
- `v11-research-code.md`: input, the panel, pausing, render models, haptics, the start screen, renaming.
- `v11-research-game.md`: the pistol and the cabinet gun, the per-player gun state.
- `headset-1-*`: the floor fix that is in 0.1.0-gf34dd76df.

## What the player experiences

1. **The app.** In the Quest library, "Confidential Mission VR" has its own icon. It opens a simple start screen:
   - On first use, it asks for All files access.
   - It checks the headset's Download folder for `confmiss.zip`, `naomi.zip` (the NAOMI BIOS) and `confmiss/gds-0001.chd`, and names whatever is missing and where it goes.
   - One button, **Play Confidential Mission**, starts the game in VR.
   - There's no game list and nothing about other games.
2. **First start only: getting the guns.** Before the first game, the app plays the game fast behind a panel that says it is taking the guns from the user's copy of the game, as HOTD2 VR does with its hands. It takes about a minute or two; B skips it, and the gun falls back to the built-in one. It runs again only if its result is missing.
3. **Before play: Quest controllers in the hands.**
   - From the start, each hand that isn't holding a gun shows a Quest controller model.
   - Until the player's first Start or first shot, short labels float by the buttons that matter:
     - right hand: A Start, B Reload, trigger Shoot, stick click Laser;
     - left hand: X Player 2, Y Reload P2, ≡ Menu, stick click Recenter.
   - The right hand turns into the gun when player 1 starts (A) or first pulls the trigger. The left hand turns into the second gun when player 2 joins (X), and back to a controller when player 2 leaves.
4. **The gun.** The default is the agents' own pistol (the suppressed automatic from the stage 1 intro), taken from the game. If that capture comes out incomplete, the default is CM's red cabinet gun (from the game's reload tutorial). The menu switches between the agents' pistol, the cabinet gun and the built-in arcade gun.
   - Recoil, muzzle flash and the aim dot work as in v1.
   - No slide racking: the pistol's slide is not a separate part.
5. **Buttons (Quest Touch):**

   | Button | Does |
   |---|---|
   | A | Start, player 1 |
   | B | reload, player 1 |
   | X | Start, player 2 (joins player 2) |
   | Y | reload, player 2 |
   | ≡ (left menu) | opens or closes the settings menu |
   | left stick click | recenter |
   | right stick click | laser on/off |
   | trigger | shoot |
   | grip + stick up/down | world size (as in v1) |
   | stick | the game's D-pad (as in v1) |

   Reload by shooting away from the game's view, or pointing at the floor, still works as in v1.
6. **The settings menu (≡).**
   - The game pauses: the picture freezes, but the headset keeps drawing it with head tracking, plus a menu panel in front of the player.
   - Point with either hand's laser and pull the trigger, or use the stick and A. ≡ or B closes it and resumes.
   - Items:
     - Resume;
     - Recenter;
     - Gun (agents' pistol / cabinet gun / arcade gun);
     - Laser (on/off);
     - Haptics (off / low / normal / strong);
     - World size (− / + with its value);
     - Panel distance (− / +);
     - Wider view (on/off; marked "next start" if it can't change live);
     - Show controls (the controllers and labels again, until the next shot);
     - Quit to the start screen.
   - Settings are kept between runs.
7. **Haptics match the game.** The knock comes when the game actually fires: its per-player shot counter goes up. A trigger pull on an empty gun gives a light click instead. The strength follows the menu setting. If the game ever fires repeatedly (an automatic weapon), each shot gets its own knock.
8. **PC VR.** The zip has `Play Confidential Mission.cmd`, which starts `flycast.exe` on `roms\confmiss.zip` next to it. The README says which three files go into `roms\`. Everything else is the same as on the Quest.

## Weapon pickups

**The user confirmed (2026-10-22):** "some enemies drop assault rifle on death that i can shoot and equip".

**More from the user:** it is a sub-weapon, an AK-style assault rifle with 45 rounds, that can be picked up during specific sequences of the game. So the ammo or magazine size reading 45 (at +0x4C / +0x50 of the per-player block) is the first thing to look for.

**What the player gets:** when a player shoots a dropped rifle and the game equips it, that player's gun in hand becomes the rifle, and the haptics take the rifle's pattern:
- a knock per shot the game fires (item 7);
- if the rifle auto-fires with the trigger held, a rapid series of lighter knocks at its fire rate.

When the game takes the rifle away (its ammo runs out, a reload or a stage change gives the pistol back), the hand goes back to the chosen gun.

**What's known from the research:**
- After a guard is shot, his pistol lies on the floor (tex b14000 + b13800, 24 polys; t4y-stage1 rip 3 at 63.12 s, box 334,287..363,312, about gun x 5440, y 6250). A seated guard's rifle is in the first room at ~54 s (v11-spray).
- The per-player gun state starts at 0x0C1557B8 + player*276:
  - ammo at +0x4C;
  - magazine size at +0x50 (always 6 so far: the likely place a weapon change shows);
  - the shot counter at +0x60.

**To be found in a spike task, before the implementation:**
- the weapon ID or state that marks the rifle;
- its magazine size and fire mode (auto with the trigger held?) and fire rate;
- how it ends;
- the rifle's model.

The rifle's model is taken like the pistol, on the first start, from whichever scene shows it whole: an enemy holding it, or the dropped item.

**If no whole rifle can be taken:** the hand shows the built-in arcade gun, changed in colour while the rifle is equipped, and the haptics still change.

## Goals, non-goals, anti-goals

### Goals
Everything above, on the Quest 3 (primary) and PC VR.

### Non-goals
- HOTD2 or MOK in this app.
- A multi-mod launcher (later, separately).
- Slide racking.
- A full 360° scene.

### Anti-goals
- **Any game data in the repo, APK or zip.** The guns are captured on the user's device and stay there.
- **Showing another mod's launcher or names.**
- **A menu that blanks the headset or runs the game on underneath it.**
- **Losing frame rate:** 72 Hz on Quest 3 holds.
- **Breaking v1's aim, HUD panel, floor or player 2.** All of `hotd2-vr/accept.ps1`'s CM scenarios (cm-boot, cm-widen, cm-aim, cm-hud, cm-p2, cm-floor) keep passing.

## Decided

### User's decisions
- The button layout (item 5).
- The menu, like port0r's.
- The game's gun.
- The controllers before play.
- CM standalone, not a launcher or hub.

### Claude's calls
- **Name and ID.** The app is "Confidential Mission VR", application ID `com.vrlightgunhub.confmiss`.
  - It's new: fresh settings, and it installs next to the 0.1.0 hub, which we then uninstall from the user's Quest 3. "VR Light Gun Hub" is kept for the later launcher.
  - versionName `1.1.0-g<hash>`.
  - dist names: `confidential-mission-vr-quest.apk` and `confidential-mission-vr-pcvr.zip`.
- **A new start activity, not VrSetupActivity.**
  - The new activity is the LAUNCHER, in the existing `:setup` process. It launches by the existing intent extra.
  - VrSetupActivity leaves the vr manifest. Its source may stay, unused, to keep upstream merges simple.
  - The HOTD2/MOK profiles stay in the engine, unreachable.
- **The menu pauses the emulator,** and the VR host keeps drawing the last frame's scene with fresh head poses (the research found that today a paused emulator blanks the headset: `emulator.cpp:1078`).
- **Controller models come from `XR_FB_render_model`,** with a small glTF loader such as cgltf (MIT, single header) and the Quest's render-model permission. The fallback is a simple built-in controller shape where the runtime gives no model (likely PC VR).
- **The gun rips** follow HOTD2's hands pipeline (`hands_rip.cpp`, `hands_build.cpp`).
  - The cabinet gun is taken from the reload tutorial: tex 400000, 279 polys, model matrix via 0x0C18AB20.
  - The pistol is taken from the stage 1 intro, 50-66 s: tex 7e2800, 6cc400, 82f800 and 76a800, 91 polys. It is merged across frames in its own model space, since a single frame clips parts off-screen. Howard's hand (6ca000) is left out.
  - A completeness check decides the default.
- **Haptics come from the shot counters** (0x0C155818 / 0x0C15592C), not the trigger.

## Left to the builder
- The menu's look.
- Exact label wording.
- The bindings for the other controller profiles (Index, Vive, WMR): analogous, and documented in the README.
- The icon's look.
- The prep panel's wording.
- How the pistol's grip and muzzle points are measured.
- How the paused frame is redrawn.
