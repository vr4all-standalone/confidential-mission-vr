# Confidential Mission VR

Sega's light-gun game **Confidential Mission** (arcade, Sega NAOMI, 2000) in VR, on a **Meta
Quest** (standalone) or a **PC VR** headset. The original game runs in an emulator, its 3D
scenes are rebuilt around you in stereo, and your controller is the light gun.

**Confidential Mission VR is built on mikermak's [hotd2-vr](https://github.com/mikermak/hotd2-vr)**
(The House of the Dead 2 VR): the VR mode, the panel the start screen grew from and the tools
are his work, and much of this readme is too. hotd2-vr is itself a fork of the
[Flycast](https://github.com/flyinghead/flycast) Dreamcast and NAOMI emulator. There is no remake
here: the original game runs, and the emulator turns what it draws into a VR view.

> **Unofficial fan project, not affiliated with Sega. No game is included.** You need your own
> copy of the game. This project contains no Sega code, data, graphics or sound. Not
> affiliated with or endorsed by Sega, Namco, Meta or the Flycast project.

Status: experimental. Checked on the PC (see [The game](#the-game) for what is not tried on a
headset yet). Made for the Quest 3; Quest 2, Quest Pro and PC VR are not tested yet.

## What you need

Your own copy of the game: the MAME set `confmiss`, three files.

| File | What it is |
|---|---|
| `confmiss.zip` | the game, with its key inside (it stays packed) |
| `confmiss/gds-0001.chd` | the disc, in a folder named `confmiss` next to `confmiss.zip` (a `.cue` or `.gdi` of it works too) |
| `naomi.zip` | the NAOMI BIOS, next to `confmiss.zip` (or the BIOS inside `confmiss.zip`) |

On the Quest they go in the headset's Download folder:

```
Download/
    confmiss.zip
    confmiss/gds-0001.chd
    naomi.zip
```

## Install on the Quest

For Quest 3, 3S, 2 and Pro (Quest 2 and Pro: not tested yet, tell me how it runs).

1. **Developer mode.** Make a (free) developer account at
   [developers.meta.com](https://developers.meta.com/horizon/) with your Meta account, then in
   the Meta Horizon app on your phone: Devices > your headset > Headset settings > Developer
   mode. Restart the headset.
2. **The app.** Download `confidential-mission-vr-quest.apk` from [Releases](../../releases) and
   install it with [SideQuest](https://sidequestvr.com/setup-howto) (*Install APK file from
   folder*), or `adb install -r confidential-mission-vr-quest.apk`.
3. **The game.** Copy your three files to the headset's **Download** folder, as in
   [What you need](#what-you-need). With SideQuest (*Manage files*), or connect the headset to a
   PC with USB, allow access in the headset, and open *Quest > Internal shared storage > Download*.
4. **Play.** Start *Confidential Mission VR* from your Library (*Unknown sources*). Its start
   screen asks for *All files access* the first time (to find the game in Download), checks
   the three files and names any that is missing and where it goes. *Play Confidential
   Mission* starts the game in VR.

Updates install over the old version; settings and saves stay.

**Had VR Light Gun Hub 0.1.0 on the headset?** Confidential Mission VR is a separate app
(`com.vrlightgunhub.confmiss`, with fresh settings) and installs next to it: uninstall the hub
(`com.vrlightgunhub.app`).

## PC VR

The same, on a PC with a VR headset: SteamVR, Quest Link / Air Link or Virtual Desktop
(anything with an OpenXR runtime that does OpenGL). Not tested yet on every one of them:
tell me how it goes.

1. Download `confidential-mission-vr-pcvr.zip` from [Releases](../../releases) and unpack it
   anywhere.
2. Make a folder `roms` next to `flycast.exe` and put the three files in it:
   `roms\confmiss.zip`, `roms\confmiss\gds-0001.chd` and `roms\naomi.zip`.
3. Connect the headset (start SteamVR, Quest Link or Virtual Desktop) so it is the active
   OpenXR runtime.
4. Double-click `Play Confidential Mission.cmd`. It starts `flycast.exe` on
   `roms\confmiss.zip`: the game plays in the headset, and the window stays as it was.

Controllers: see [Controls](#controls). Settings: `emu.cfg` next to `flycast.exe` (`vr.Xr = yes`
turns VR on; without a headset the game plays in the window).

If the world looks the wrong size, set `vr.WorldScale = 0.11` in `emu.cfg` (the default) and
adjust from there.

## Controls

Quest Touch:

| Button | Does |
|---|---|
| A | Start, player 1 |
| B | reload, player 1 |
| X | Start, player 2 (player 2 joins: a gun in each hand) |
| Y | reload, player 2 |
| ≡ (left menu button) | opens or closes the settings menu |
| left thumbstick click | recenter |
| right thumbstick click | laser off/on (kept) |
| trigger | shoot |
| grip + thumbstick up/down | world size (kept) |
| thumbstick | the game's D-pad (menus) |

Reload by shooting away from the game's view (past the grey ring), as before; with two guns,
pointing a gun at the floor reloads it too. B and Y are the cabinet's second button, the game's
own reload: with one gun either reloads it, with two guns B reloads the right one and Y the
left one. Grip + thumbstick left/right sizes the gun (kept). The gun goes to the hand you last
fired with.

**Two guns.** Press X and player 2 joins, as at the cabinet: the right hand holds player 1's
gun (red aim), the left player 2's (blue aim), both lives and scores on screen. When player 2
is out of the game (the game asks for player 2's Start again), it's one gun again. X only
joins while the game takes player 2; at other times it does nothing. `vr.DualWield = no` turns
joining off.

Holding the Meta button recenters too.
Quit with the Meta button, then *Quit*.

**PC VR controllers.** Quest Touch (Link, Virtual Desktop) as above. The others:

| | Valve Index | Vive wands | Windows Mixed Reality |
|---|---|---|---|
| Start (right: player 1, left: player 2) | A | menu button | menu button |
| Reload (right: player 1, left: player 2) | B | trackpad click | trackpad click |
| Settings menu | squeeze the left trackpad hard | hold the left menu button (0.8 s) | hold the left menu button (0.8 s) |
| Recenter | left thumbstick click | the runtime's own | left thumbstick click |
| Laser off/on | right thumbstick click | none | right thumbstick click |
| The game's D-pad (menus) | thumbstick | trackpad | thumbstick |
| World size | grip + thumbstick up/down | grip + trackpad up/down | grip + thumbstick up/down |

Where the settings menu is a hold, a shorter press of the left menu button is player 2's Start
when you let go.

## The game

Sega's NAOMI light-gun game from 2000, with its own profile:

- **The arcade gun in your hand.** The red arcade gun. The agents' own pistol, taken from your
  copy of the game, is planned. Until then, reload by shooting away from the view, or with B
  (the cabinet's second button reloads in this game).
- **A wider view, inside the game.** The game draws and culls a wider view itself: stage 1
  goes from about 61 by 48 degrees to 111 by 95, the story scenes from about 70 degrees across
  to 109. Its light gun still counts shots only inside the original view: around it the aim
  dot turns into a grey ring and the trigger does nothing.
- **The aim.** The gun's calibration is kept neutral, so shots land on the dot; the game's
  gun adjustment screen before each game needs nothing from you.
- **On the panel.** The HUD, the subtitles, the briefing and the menus sit on the 2D plane; the
  game's own shot marker is hidden, and so is player 2's start prompt in a one-player game.
- **Two guns.** X (the left controller's Start) lets player 2 join with a gun in your left
  hand; when player 2 is out, it's one gun again.
- **Free play.** No coins: Start (A) begins a game.

Checked on the PC: the profile, the widened view in the attract scenes, the story and stage 1,
shots at the centre, corners and edges of the view landing on the dot, the panel, the hidden
shot marker, player 2 joining and leaving. **Not tried on a headset yet:** the frame rate in
the busiest scene of the stage 1 story, the scale of the world, player 2 joining in the
headset, and PC VR over Quest Link.

How it was worked out (PC: `vr.DebugShots`, rips and RAM dumps): its camera keeps the field of
view as a float (`0x0C154EC8`) that its camera code turns into an angle for the projection
builder (`0x8C09C2F0`), and its story cameras build theirs from a lens angle. The constants
they scale by (65536.0, three copies, and 32.0, two) are patched, so the game draws and culls
the wider view, while the stock focal its light-gun hit test and on-screen checks use
(`0x0C154ED0`) stays as it was. The gun calibration (6 bytes per player at `0x0C15A9C0`) is
held where the game's aim is where the gun points. All of it, with the evidence:
`hotd2-vr/notes/confmiss-findings.md`.

## Options

Settings live in `/sdcard/Android/data/com.vrlightgunhub.confmiss/files/emu.cfg` (PC: `emu.cfg`
next to `flycast.exe`), under `[config]`. The useful ones:

| Option | Default | |
|---|---|---|
| `vr.WorldScale` | 0.11 | metres per game unit, so the game's camera is at eye height (also set with grip + thumbstick) |
| `vr.FovScale` | 2 (profile) | how much wider than the original (tan of the half angle) |
| `vr.WidenFov` | yes | widen through the game's own field of view (the profile knows where) |
| `vr.Laser` | yes | laser line and dot (also with a click on the right thumbstick) |
| `vr.ShowGun` | yes | the gun model |
| `vr.GunScale` | 0.68 | size of the gun, times the 25 cm arcade gun it is modelled on (also set with grip + thumbstick) |
| `vr.DropShotMarker` | yes | hide the game's own 2D shot flash after shots into the 3D scene (with `no` it shows at the stock-framed spot, not where the shot landed) |
| `vr.DualWield` | yes | X (the left controller's Start) lets player 2 join with a gun in your left hand |
| `vr.HideP2Prompt` | yes | no "PRESS START BUTTON / CREDIT(S)" for player 2 in a one-player game |
| `vr.DropShotFlash` | yes | hide a bright full-screen flash right after a shot, should the game draw one |
| `vr.ComfortStart` / `vr.ComfortMin` | 0.8 / 0.4 | comfort zone, metres (0: off) |
| `vr.HudDepth` | 40 | distance of the 2D plane, game units |
| `vr.XrRefreshRate` | 72 | Hz |
| `vr.XrResolution` | 1 | eye buffer size, times the recommended size |
| `vr.Xr` | yes (PC: no) | VR; the PC VR zip's `emu.cfg` turns it on |

## How it works

- `core/rend/vr_reproject.*`: rebuilds eye space from the projected vertices with the
  game's live focal length (read from its projection and viewport matrices in RAM), and
  the per-game profile: matrix addresses and the field-of-view literals to patch. Each
  game's numbers are in its profile (`makeProfiles()`): Confidential Mission's is the one this
  app starts (hotd2-vr's others stay in the engine, unused).
  `hotd2-vr/notes/adding-a-game.md` is how a profile is made.
- `core/rend/gles/gles.cpp`: the vertex shader does the lifting and reprojection; only
  frames the game actually presents reach the headset.
- `core/hw/pvr/ta_vtx.cpp`: hands each parsed render pass to `vr::dropShotMarker` before
  its index is built, which hides the game's 2D shot marker after shots into 3D.
- `core/rend/vr/xr_host.*`: the OpenXR session (bound to Flycast's GL context: EGL and
  GLES on the Quest, WGL and desktop OpenGL on the PC), headset-paced frames, controllers
  and the light gun ray cast.
- `core/rend/vr/xr_gun.*`, `gun_model.h`: the gun, its effects and shaders.
- `core/rend/vr/xr_hands.*`, `hands_rip.*`, `hands_build.*`: models made from a game on the
  player's own device, and drawn in the hands (hotd2-vr's; the agents' pistol is to use them).
- `core/rend/vr/xr_panel.*`: a text panel in the headset.
- `shell/android-studio/.../CmStartActivity.java`, `CmFiles.java`: the start screen and
  finding the game's three files on the headset.
- `core/input/udp_lightgun.*`: the light gun can also be driven over UDP on 127.0.0.1
  (used by the finger-gun tracker and the PC test runs).

## Build

On Windows, with:

- JDK 17 (a Microsoft JDK 17 in Program Files is picked up, otherwise set `JAVA_HOME`)
- the Android SDK with NDK 29.0.14206865 and CMake 3.22.1 (`ANDROID_HOME`, default
  `%LOCALAPPDATA%\Android\Sdk`)
- the git submodules: `git submodule update --init --recursive`

On Windows, two paths in the `core/deps/gamesdk` submodule are symlinks
(`games-frame-pacing/include/common` and `include/swappy`). Without symlink support
(Developer Mode plus `git config --global core.symlinks true` before cloning) they
check out as small text files and the build fails: replace them with copies of
`core/deps/gamesdk/include/common` and `core/deps/gamesdk/include/swappy`.

```bat
hotd2-vr\build-quest.cmd
```

This builds the `vr` build type (arm64, optimised, OpenXR) to
`shell\android-studio\flycast\build\outputs\apk\vr\flycast-vr.apk`. It installs as its own
app, *Confidential Mission VR* (`com.vrlightgunhub.confmiss`), next to a normal Flycast. Its
version is `hubVersion` in `shell/android-studio/flycast/build.gradle.kts` and the commit
(`1.1.0-g<commit>`). Without the release key (below) it is signed with the public debug key,
and Gradle says so: fine for trying things out, not for a release.

`hotd2-vr\build-win.cmd` builds the Windows version (Visual Studio 2022 Build Tools) with
PC VR (`-DUSE_OPENXR=ON`: CMake fetches the Khronos OpenXR loader and builds it in), also used
for the webcam finger-gun mode and for testing.

By hand on the Quest (without the start screen): the app also starts the game from its own
`files/games` when the three files are there:
```bat
adb shell run-as com.vrlightgunhub.confmiss mkdir -p files/games/confmiss
adb exec-in run-as com.vrlightgunhub.confmiss sh -c "cat > files/games/confmiss.zip" < confmiss.zip
adb exec-in run-as com.vrlightgunhub.confmiss sh -c "cat > files/games/naomi.zip" < naomi.zip
adb exec-in run-as com.vrlightgunhub.confmiss sh -c "cat > files/games/confmiss/gds-0001.chd" < confmiss\gds-0001.chd
adb shell am start -n com.vrlightgunhub.confmiss/com.flycast.emulator.MainActivity
```

The start screen's file check has a JVM test: `gradlew.bat :flycast:testDebugUnitTest` in
`shell\android-studio` (`CmFilesTest`).

## Releasing

1. **The release key, once.** `powershell -File hotd2-vr\make-release-key.ps1` makes
   `release.jks` (RSA 4096, alias `vrlightgunhub`, valid 10000 days) and
   `release-signing.properties` (with its random password) in `%USERPROFILE%\.vrlightgunhub`,
   where the build looks for them. **Back that folder up**, somewhere safe and private: every
   update must be signed with the same key, and losing it means no more updates (players would
   have to uninstall, losing their settings and what the app made from their game). Never
   commit it; `.gitignore` keeps out `*.jks` and `release-signing.properties`. Elsewhere:
   `-Dir <folder>`, and `$env:VRLGH_SIGNING` set to that folder's `release-signing.properties`.
2. **The packages.** `powershell -File hotd2-vr\package-release.ps1` builds both versions and
   puts `confidential-mission-vr-quest.apk` and `confidential-mission-vr-pcvr.zip` (`flycast.exe`,
   `emu.cfg`, `README.txt`, `LICENSE.txt`, `Play Confidential Mission.cmd`) in `dist\`. It
   refuses an APK signed with the public debug key (`-AllowDebugKey` takes one for a test
   build), an APK that isn't Confidential Mission VR (its package, version and label, and the
   start screen as its only library entry), a zip with other files or a `README.txt` naming
   another game, and either file if its file list has anything of a game in it (disc images,
   keys, rips, frame grabs, models made from a game). `-SkipBuild` packages what was built
   last; `-CheckOnly <archives>` only checks the contents.
3. **The source.** Publish the source of the release's commit with it (the GPL asks for that):
   tag the commit and push it, so the release page carries its source archives.

## Tools

- `hotd2-vr/fingergun`: play on the PC with **finger guns** in front of a webcam
  (MediaPipe hand tracking). Set up once:
  ```bat
  py -m venv hotd2-vr\fingergun\.venv
  hotd2-vr\fingergun\.venv\Scripts\pip install -r hotd2-vr\fingergun\requirements.txt
  ```
  and put MediaPipe's [hand landmarker model](https://ai.google.dev/edge/mediapipe/solutions/vision/hand_landmarker)
  (`hand_landmarker.task`) in `hotd2-vr\fingergun\`. Then `hotd2-vr\play-fingergun.ps1`
  (after `build-win.cmd`). It starts with a guided calibration; its on-screen text is in Dutch.
  Aim with your index finger, fire by dropping your thumb, reload by opening your hand for
  a moment (`--reload down` brings back the old point-the-gun-down reload). The
  calibration (`K`) moves the game's crosshair to nine targets in turn; aim at each. Aiming
  then follows where your hand is: a webcam in front of you can't tell which way a finger
  points straight at it, so the finger's direction is left out. Press `C` while aiming at
  the centre whenever you sit differently.
  Two players: `play-fingergun.ps1 -Players 2`, two people side by side in front of one
  webcam, one gun hand each (left in the preview is player 1, red crosshair; right is
  player 2, light blue). `K` calibrates both in turn, each with their own settings. A
  thumbs-up (a fist, thumb up) held for half a second presses the game's Start: that is how
  player 2 joins in (and it pauses for a player already in the game).
- `hotd2-vr/gun_model`: bakes a `.glb` gun model into `gun_model.h`.
- `hotd2-vr/assets`: the PC build rips a frame on request (`rip.request` in its working folder:
  every polygon as the GPU got it, with the VRAM), and `riplib.py` reads that and decodes the
  textures. `rip_hands.py` and `hotd2-vr/rip-hands.ps1` are hotd2-vr's hands rip on the PC;
  `make-icon.py` draws the app's icon.
- `hotd2-vr/re/sh4dis.py`: disassembles game code from a Flycast RAM dump (Capstone),
  which is how the field-of-view literals were found.
- `hotd2-vr/run-test.ps1`: PC test runs with screenshots.
- `hotd2-vr/vr-test.ps1`, `hotd2-vr/accept.ps1`: scripted PC runs (gun input over UDP, window
  shots, rips, log checks), and the check that the engine's other profiles from hotd2-vr still
  play as before (`accept.ps1 -Only hotd2,mok`). They find the games through
  `$env:VRLGH_GAMES` (the folder with the MAME sets and `naomi.zip`) and `$env:VRLGH_HOTD2`
  (hotd2-vr's Dreamcast disc).

The older scripts look for your game in a `game` folder next to the repository, or in
`%HOTD2VR_DATA%\game`.

## Problems? Ideas?

Open an [issue](../../issues) here (not on mikermak's project): the form asks for the build
(Quest app or PC VR), the app version, the headset (and the PC VR runtime), the game's files
and a log, which is what it takes to find out what's wrong. On the Quest, with the headset
connected to a PC by USB, `adb logcat -d -s flycast:*` (adb comes with Android's
platform-tools) prints the app's log, right after the problem. PC VR writes `flycast.log`
next to `flycast.exe` with `log:LogToFile = yes` in `emu.cfg`. Never attach or link game files.

## Credits and licences

- **[hotd2-vr](https://github.com/mikermak/hotd2-vr) (The House of the Dead 2 VR) by
  mikermak**: Confidential Mission VR is built on it, a fork of it. The VR mode, the panel the
  start screen grew from and the tools are his. GPL-2.0-or-later.
- [Flycast](https://github.com/flyinghead/flycast) by flyinghead and contributors, GPL-2.0
  or later. This fork's code is too (see `LICENSE`); changed Flycast files carry a note,
  and `git log master..` shows every change. Flycast's own readme:
  [README.flycast.md](README.flycast.md).
- The gun: ["Namco Arcade Gun"](https://sketchfab.com/3d-models/namco-arcade-gun-15fbd5b9add94a34b5c21746e3dd32be)
  by [Martoscar](https://sketchfab.com/Martoscar), made for Age of Joy, licensed
  [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Changed: re-oriented and
  moved into the controller's aim space, quantised, textures and materials dropped,
  recoloured, converted to a C++ header (`core/rend/vr/gun_model.h`, CC BY 4.0). Since CC
  BY 4.0 goes together with GPL-3.0 but not GPL-2.0, the builds, which include it (the Quest
  app and the PC version), are distributed under **GPL-3.0**, which Flycast's "or later"
  allows. The source is published with every release.
- [OpenXR loader](https://github.com/KhronosGroup/OpenXR-SDK) by Khronos, Apache-2.0.
- Roboto (Flycast's own copy, on the headset's panel) by Google, Apache-2.0.
- [MediaPipe](https://github.com/google-ai-edge/mediapipe) by Google, Apache-2.0
  (finger-gun tracker); [Capstone](https://www.capstone-engine.org/), BSD.
- PC VR rendering in an OpenGL Core Profile: [nn10n10](https://github.com/nn10n10) (pull request
  #1 to hotd2-vr).
- The app's icon is drawn for it from plain shapes (`hotd2-vr/assets/make-icon.py`).
- Inspired by [DR-89/time-crisis-vr](https://github.com/DR-89/time-crisis-vr).
- Confidential Mission is Sega's game, and The House of the Dead a trademark of Sega. Namco
  is a trademark of Bandai Namco.

Unofficial fan project, not affiliated with Sega. No game is included.
