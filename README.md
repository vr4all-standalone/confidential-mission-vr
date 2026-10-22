# VR Light Gun Hub

Light-gun games in VR, on a **Meta Quest** (standalone) or a **PC VR** headset:
**The House of the Dead 2** (Sega Dreamcast), **The Maze of the Kings** and **Confidential
Mission** (arcade, Sega NAOMI), with more games to come.
The original game runs in an emulator, its 3D scenes are rebuilt around you in stereo, and
your controller is the light gun.

**VR Light Gun Hub is a fork of mikermak's [The House of the Dead 2 VR](https://github.com/mikermak/hotd2-vr)
(hotd2-vr).** The VR mode, The House of the Dead 2 and The Maze of the Kings in VR, the agent's
hands, the hero's staff, the setup panel and the tools are his work, and most of this readme is
his. This app adds Confidential Mission, its own name and app ID (so it installs next to HOTD2 VR),
and its own releases. hotd2-vr is itself a fork of the [Flycast](https://github.com/flyinghead/flycast)
Dreamcast and NAOMI emulator. There is no remake here: the original game runs, and the emulator
turns what it draws into a VR view.

> **Unofficial fan project, not affiliated with Sega. No game is included.** You need your own
> copies of the games. This project contains no Sega code, data, graphics or sound. Not
> affiliated with or endorsed by Sega, Namco, Meta or the Flycast project.

Status: experimental. The House of the Dead 2 and The Maze of the Kings play as in HOTD2 VR,
where they were played on a Quest 3. Confidential Mission is new: checked on the PC, not yet
tried on a headset (see [its section](#confidential-mission-new-experimental)). Made for the
Quest 3; Quest 2, Quest Pro and PC VR are not tested yet.

## Games

| Game | System | What goes in the headset's Download folder |
|---|---|---|
| The House of the Dead 2 | Dreamcast: the European (`MK-5100250`) or US (`MK-51002`) release | the disc: a `.chd`, a `.gdi`, a `.cue` with its `.bin` files, or a `.cdi` (any file name) |
| The Maze of the Kings | NAOMI, the MAME set `mok` | `mok.zip` with its key inside, `naomi.zip` next to it (or the BIOS inside `mok.zip`), and the folder `mok` with `gds-0022.chd` next to it |
| Confidential Mission | NAOMI, the MAME set `confmiss` | `confmiss.zip` with its key inside, `naomi.zip` next to it (or the BIOS inside `confmiss.zip`), and the folder `confmiss` with `gds-0001.chd` next to it |

Only the games you have, for example:

```
Download/
    House of the Dead 2.chd      (or .gdi / .cue with their .bin files, or .cdi)
    naomi.zip                    (the NAOMI BIOS, unless it is inside the games' zips)
    mok.zip
    mok/gds-0022.chd
    confmiss.zip
    confmiss/gds-0001.chd
```

With more than one game there, the setup panel asks which one to play. No Dreamcast BIOS is
needed: Flycast's built-in replacement (HLE) works.

## Install on the Quest

For Quest 3, 3S, 2 and Pro (Quest 2 and Pro: not tested yet, tell me how it runs).

1. **Developer mode.** Make a (free) developer account at
   [developers.meta.com](https://developers.meta.com/horizon/) with your Meta account, then in
   the Meta Horizon app on your phone: Devices > your headset > Headset settings > Developer
   mode. Restart the headset.
2. **The app.** Download `vr-light-gun-hub-quest.apk` from [Releases](../../releases) and install
   it with [SideQuest](https://sidequestvr.com/setup-howto) (*Install APK file from folder*), or
   `adb install -r vr-light-gun-hub-quest.apk`.
3. **Your games.** Copy your own copies of the games to the headset's **Download** folder, as
   in [Games](#games). With SideQuest (*Manage files*), or connect the headset to a PC with USB,
   allow access in the headset, and open *Quest > Internal shared storage > Download*. A disc
   image packed in a `.zip` or `.7z` needs unpacking first; the MAME sets' zips stay as they are.
4. **Play.** Start *VR Light Gun Hub* from your Library (*Unknown sources*). The first time, a
   panel asks for *All files access* (to find the games in Download) and tells you what's
   still missing, if anything.

The first time The House of the Dead 2 or The Maze of the Kings starts, the app makes the
agent's hands or the hero's staff from your game: it plays itself for a short while, fast,
behind a panel that says so (a minute or two). Once only; B skips it (the red arcade gun then,
and another go next time).

Updates install over the old version; settings, saves and what the app made from your games
stay.

**Had HOTD2 VR on the headset?** The app ID changed from `com.flycast.emulator.vr` (mikermak's
HOTD2 VR, and this app's early test builds) to `com.vrlightgunhub.app`, so VR Light Gun Hub
installs as a separate app next to it. HOTD2 VR's settings, its `hands.bin` and any games copied
into its app storage don't carry over: copy your games to Download again (the hands are made
again at the first start). Uninstalling HOTD2 VR is up to you.

## PC VR

The same, on a PC with a VR headset: SteamVR, Quest Link / Air Link or Virtual Desktop
(anything with an OpenXR runtime that does OpenGL). Not tested yet on every one of them:
tell me how it goes.

1. Download `vr-light-gun-hub-pcvr.zip` from [Releases](../../releases) and unpack it anywhere.
2. Connect the headset (start SteamVR, Quest Link or Virtual Desktop) so it is the active
   OpenXR runtime.
3. Start `flycast.exe`. Its window is Flycast's own: add the folder with your games (the
   files as in [Games](#games)) and start one. It plays in the headset; the window stays as it
   was. For the NAOMI games, put `naomi.zip` next to `flycast.exe` or in the `data` folder next
   to it, or have the BIOS inside the game's zip.

Controllers: Quest Touch as on the Quest. Valve Index: A is Start (the left A lets player 2
join), B the gun's B, the left thumbstick click recenters, the right one turns the laser off
or on. Vive wands and Windows Mixed Reality: the menu button is Start (the left one lets player
2 join), a click on the pad the gun's B; the pad or stick is the D-pad (Windows Mixed Reality:
the thumbstick clicks as on the Index, Vive wands: the runtime's own recenter). With
`vr.DualWield = no` the left Start is player 1's. The first start of The House of the Dead 2 or
The Maze of the Kings makes the hands or the staff, as on the Quest. Settings: `emu.cfg` next to
`flycast.exe` (`vr.Xr = yes` turns VR on; without a headset the game plays in the window).

If the world looks the wrong size, set `vr.WorldScale = 0.11` in `emu.cfg` (the default) and
adjust from there.

## Controls

| | Left controller | Right controller |
|---|---|---|
| Shoot | trigger | trigger |
| Reload | shoot away from the view (past the grey ring), or rack the slide (below); with two guns: point the gun at the floor | the same |
| Skip a story scene, back in menus (the gun's B) | Y | B |
| Start, pause | | A |
| Player 2 joins: a gun in each hand | menu button ≡ | |
| Menus (the gun's D-pad) | thumbstick | thumbstick |
| Recenter | X or thumbstick click | |
| Laser off/on (kept) | | thumbstick click |
| World size (kept) | squeeze grip (thumbstick at rest), then thumbstick up/down | the same |
| Gun size (kept) | squeeze grip (thumbstick at rest), then thumbstick left/right | the same |
| Rack the slide (the agent's pistol) | grip, near the back of the pistol, and pull | the same |

The gun goes to the hand you last fired with; B and Y both work, whichever hand holds it.
In Confidential Mission, B (Y) is the cabinet's second button, which reloads there.

**Two guns.** Press ≡ on the left controller and player 2 joins, as at the cabinet: the
right hand holds player 1's gun (red aim), the left player 2's (blue aim), both lives and
scores on screen. With no hand free for the slide, point a gun at the floor to reload it
(or shoot past the view). When player 2 is out of the game (the game asks for player 2's
Start again), it's one gun again. `vr.DualWield = no` makes ≡ player 1's Start as before.

With the agent's pistol, bring the other hand to the back of the pistol, over the gun
hand, squeeze its grip and pull towards you: the hand takes the slide, and all the way back
it reloads (a knock in both hands). Let go and it springs home.
Holding the Meta button recenters too.
Quit with the Meta button, then *Quit*.

## The House of the Dead 2

Built for and tested with the European release (PAL, product ID `MK-5100250`) on a Quest 3. The
US release (`MK-51002`) has its own profile (checked on the PC: the same view, aim and hands as
PAL). Other versions need their own profile (see [How it works](#how-it-works)).
Shooting lands on the aim dot, the B button skips story scenes and the game's own hit flash
is gone, and the agent's hands and racking the slide work (all tested on the headset).
HOTD2 VR's own plans are in mikermak's [roadmap](ROADMAP.md).

### What you get

- **Immersive 3D.** The Dreamcast's GPU only sees vertices already projected to the
  screen (x, y, 1/w). Knowing the game's lens, each one is lifted back into 3D and drawn
  per eye from your head, so you look around and lean into the scene at the headset's
  refresh rate, while the game runs at its own 60 Hz.
- **A wider view than the original.** The game's own field of view is raised from 41° to
  74° (vertical), inside the game: it draws and culls with it. Its light gun still only
  counts shots inside the original view, so that is where you can hit things; around it
  the aim dot turns into a grey ring and the trigger does nothing.
- **2D on a screen in front of you.** Menus, text and the HUD sit on a plane at a fixed
  distance, framed as in the original. The game's own hit flash (a glow and rays drawn
  just off its 2D plane, which in the headset came right at your face) is left out after
  shots into the 3D scene: the aim dot, blood and the hit show where it went. 2D the game
  draws at other depths keeps its original size too: the pause text on the plane, the menus'
  and the route map's pictures behind their 3D, as on the TV. What lies past the screen's
  edges is cut off, as a TV does (a chapter title zooming in no longer fills the room).
- **The light gun.** Either controller. Shots are traced into the rebuilt scene from the
  barrel and handed to the game at its own screen framing, so they land on the dot; a
  laser line and dot show where (optional). Recoil, muzzle flash, smoke and a haptic
  knock on every shot.
- **The agent's own hands.** Made from your copy of the game, by the app itself: his right hand
  around his pistol in place of the arcade gun, his open left hand on the other controller.
  Reload like with a real pistol: take the slide with the other hand (grip button) and pull
  it back.
- **Comfort.** Things that come right into your face (a zombie grabbing you) are pulled
  back a little, so your eyes don't have to cross. Recentering, adjustable world size.
- Starts straight into the game from the headset's app library. No floating "PRESS START
  BUTTON" for player 2 in a one-player game.

### The agent's hands (from your game)

The hands and pistol in the headset are the game's own: the agent's, as the game draws him
at the game over scene, kneeling with his pistol in his hand. Nothing of them is in this
repository or in the app: the app takes them from your copy of the game the first time
(above), into `hands.bin` in its data folder. Keep that file to yourself.

How: the game is run to that scene (Start through the notices, the title and its menu
while there's little 3D on screen, then nothing, so the agent loses), the frame with his
pistol is taken apart into the pistol, the hand around it and the open hand, lifted back
into 3D, the pistol stood upright and its slide cut free along the line painted on its
sides (`core/rend/vr/hands_rip.cpp`, `hands_build.cpp`).

On the PC the same can be done by hand, from a rip of the game over scene: build the
Windows version, install Python with numpy and pillow (`pip install -r
hotd2-vr\assets\requirements.txt`), and run `powershell -File hotd2-vr\rip-hands.ps1`
(`-NoPush` keeps it on the PC, `-Manual` lets you play to the game over yourself). It
gives the same `hands.bin`, with a preview.

## The Maze of the Kings (experimental)

Sega's NAOMI light-gun game from 2002 (Hitmaker, the HOTD team) works too, with its own
profile: its view is widened in the game itself (the maze from 60 to 90 degrees, the story
from 40 to 60), the gun's calibration is kept neutral so shots land on the dot, the HUD and
subtitles go on the panel, the menus (drawn in layers of depth) flat on it, and the game's own
shot flash and the floating "PRESS 2P START" are gone. Two guns work as in HOTD2. First played
in the headset, then again with the staff and the two fixes below; only `vr.StaffHand` is
new and not tried there yet.

Put your own MAME set in the headset's Download folder: `mok.zip` (with its key
`317-0333-com.pic`, and the NAOMI BIOS in it or as `naomi.zip` next to it) and the folder `mok`
with the disc, `gds-0022.chd` (a `.cue` or `.gdi` of it works too). With more than one game on
the headset, the setup panel asks which one to play.

**The hero's staff.** The heroes carry a staff, not a gun, and so do you: the hero's own, head
forward, with his left glove on your other hand. Like the agent's hands in
HOTD2, the app makes it from your game the first time it starts: the game plays its attract
demo by itself, fast and without input, up to the first frame with the hero and his staff whole
in it (about half a minute of the game's time, under 20 seconds on the PC), behind a panel that
says so. B skips it (the arcade gun then, and another go next time). It goes into `staff-mok.bin`
next to `hands.bin`; each game has its own and never shows the other's. It is shortened to
about 1 m (the game's is 1.4 m: the shaft behind the fist is cut down to a hand's width, its
gold tail put back on there); grip + thumbstick left/right makes it bigger or smaller
(`vr.HandScale`). There's no slide to rack: reload by pointing away from the screen and
pulling the trigger. His right glove around it is left out: the game made that fist for a staff
held upright like a walking stick, so with the staff pointing forward its wrist pointed at the
floor (`vr.StaffHand` brings it back).

Fixed since the first go in the headset:
- Streaks over the floor. The game clips its 3D at W 1.0 (the floor under the camera, the walls
  beside it, the desert's ground and sky), right where the 2D plane was, so the corners it cut
  there were pinned onto the plane. Its 2D plane is now at its HUD's W, 1.082.
- People beside the camera in the story scenes went missing when you turned your head: the
  game culls models against its own frame, with a margin of half a bounding sphere (the `0.5`
  at `0x8C08DC4C`). Made 4, they are drawn too (about a third more polygons in the story; the
  game's own frame stays the same).

How it was worked out (PC: `vr.DebugShots`, rips and RAM dumps): its field of view comes from
camera data, not code, as one angle the projection builder halves; three instructions of the
builder (`0x8C08D8EE`) make that three quarters instead. Its light-gun hit test keeps the stock
focal (`0x0C0E7248`), which the aim follows. The staff is one model with one texture (a head
piece and a shaft, 66 polygons), the gloves part of the hero's body texture; the app takes the
rod his right glove is around, whole (its 66 polygons with all their 514 corners, and nothing of
it or the gloves on the near plane, which keeps the polygons it cuts), stands it along its axis
with the head forward and the ears of the animal head on it up, and fits his left glove onto
the mirror image of the right one (`hands_build.cpp`, `buildStaff`).

## Confidential Mission (new, experimental)

Sega's NAOMI light-gun game from 2000, the first game this app adds, with its own profile in
the style of The Maze of the Kings'.

Put your own MAME set in the headset's Download folder: `confmiss.zip` (with its key inside,
and the NAOMI BIOS in it or as `naomi.zip` next to it) and the folder `confmiss` with the disc,
`gds-0001.chd`. With more than one game on the headset, the setup panel asks which one to play.

- **The arcade gun in your hand.** The red arcade gun, as in the other games before their
  hands or staff are made. The agents' own pistol, made from your game like HOTD2's hands
  (with a slide to rack), is planned for a later version. Until then, reload by shooting away
  from the view, or with B (the cabinet's second button reloads in this game).
- **A wider view, inside the game.** The game draws and culls a wider view itself: stage 1
  goes from about 61 by 48 degrees to 111 by 95, the story scenes from about 70 degrees across
  to 109. Its light gun still counts shots only inside the original view, as in the other
  games: around it the aim dot turns into a grey ring and the trigger does nothing.
- **The aim.** The gun's calibration is kept neutral, so shots land on the dot; the game's
  gun adjustment screen before each game needs nothing from you.
- **On the panel.** The HUD, the subtitles, the briefing and the menus sit on the 2D plane; the
  game's own shot marker is hidden, and so is player 2's start prompt in a one-player game.
- **Two guns.** The left controller's ≡ lets player 2 join with a gun in your left hand, as in
  HOTD2; when player 2 is out, it's one gun again.
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

Settings live in `/sdcard/Android/data/com.vrlightgunhub.app/files/emu.cfg` (PC: `emu.cfg`
next to `flycast.exe`), under `[config]`. The useful ones:

| Option | Default | |
|---|---|---|
| `vr.WorldScale` | 0.11 | metres per game unit, so the game's camera is at eye height (also set with grip + thumbstick) |
| `vr.FovScale` | 2 (profile) | how much wider than the original (tan of the half angle) |
| `vr.WidenFov` | yes | widen through the game's own field of view (the profile knows where) |
| `vr.Laser` | yes | laser line and dot (also with a click on the right thumbstick) |
| `vr.ShowGun` | yes | the gun model |
| `vr.GunScale` | 0.68 | size of the gun, times the 25 cm arcade gun it is modelled on (also set with grip + thumbstick) |
| `vr.GameHands` | yes | the agent's hands and pistol, or the hero's staff (made from the game at the first start) |
| `vr.HandScale` | 1 | their size: 1 is a 20 cm pistol, a 1 m staff (also set with grip + thumbstick) |
| `vr.SlideReload` | yes | rack the slide with the other hand to reload |
| `vr.StaffHand` | no | the hero's glove around his staff (The Maze of the Kings) |
| `vr.DropShotMarker` | yes | hide the game's own 2D shot flash after shots into the 3D scene (with `no` it shows at the stock-framed spot, not where the shot landed) |
| `vr.DualWield` | yes | the left controller's ≡ lets player 2 join with a gun in your left hand |
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
  the per-game profile: matrix addresses and the field-of-view literals to patch.
  HOTD2's four perspective setups load their angle (`0x1D3C`, 41.1°) from two literals
  (`0x8C029C12`, `0x8C02B370`) for the routine at `0x8C0383C0`. Each game's numbers are in
  its profile (`makeProfiles()`); `hotd2-vr/notes/adding-a-game.md` is how to add a game.
- `core/rend/gles/gles.cpp`: the vertex shader does the lifting and reprojection; only
  frames the game actually presents reach the headset.
- `core/hw/pvr/ta_vtx.cpp`: hands each parsed render pass to `vr::dropShotMarker` before
  its index is built, which hides the game's 2D shot marker after shots into 3D.
- `core/rend/vr/xr_host.*`: the OpenXR session (bound to Flycast's GL context: EGL and
  GLES on the Quest, WGL and desktop OpenGL on the PC), headset-paced frames, controllers
  and the light gun ray cast.
- `core/rend/vr/xr_gun.*`, `gun_model.h`: the gun, its effects and shaders.
- `core/rend/vr/xr_hands.*`: the agent's hands and pistol from `hands.bin`, with the
  slide that moves, or the hero's staff from `staff-mok.bin` (only the running game's file).
  The other hand's grip and the rack are in `xr_host.cpp`.
- `core/rend/vr/hands_rip.*`, `hands_build.*`: the run to the game over scene and the
  model made from it (the C++ twin of `hotd2-vr/assets/rip_hands.py`); in The Maze of the
  Kings the run through its attract demo and the staff.
- `core/rend/vr/xr_panel.*`: a text panel in the headset (the hands run).
- `shell/android-studio/.../VrSetupActivity.java`, `VrGames.java`: the setup panel and
  finding the games on the headset.
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
app, *VR Light Gun Hub* (`com.vrlightgunhub.app`), next to a normal Flycast and HOTD2 VR. Its
version is `hubVersion` in `shell/android-studio/flycast/build.gradle.kts` and the commit
(`0.1.0-g<commit>`). Without the release key (below) it is signed with the public debug key,
and Gradle says so: fine for trying things out, not for a release.

`hotd2-vr\build-win.cmd` builds the Windows version (Visual Studio 2022 Build Tools) with
PC VR (`-DUSE_OPENXR=ON`: CMake fetches the Khronos OpenXR loader and builds it in), also used
for the webcam finger-gun mode and for testing.

By hand on the Quest (without the setup panel): the app also boots the first `.cue`,
`.gdi`, `.chd` or `.cdi` in its own `files/games`:
```bat
adb shell run-as com.vrlightgunhub.app mkdir -p files/games
adb exec-in run-as com.vrlightgunhub.app sh -c "cat > files/games/hotd2.chd" < hotd2.chd
```

## Releasing

1. **The release key, once.** `powershell -File hotd2-vr\make-release-key.ps1` makes
   `release.jks` (RSA 4096, alias `vrlightgunhub`, valid 10000 days) and
   `release-signing.properties` (with its random password) in `%USERPROFILE%\.vrlightgunhub`,
   where the build looks for them. **Back that folder up**, somewhere safe and private: every
   update must be signed with the same key, and losing it means no more updates (players would
   have to uninstall, losing their settings and what the app made from their games). Never
   commit it; `.gitignore` keeps out `*.jks` and `release-signing.properties`. Elsewhere:
   `-Dir <folder>`, and `$env:VRLGH_SIGNING` set to that folder's `release-signing.properties`.
2. **The packages.** `powershell -File hotd2-vr\package-release.ps1` builds both versions and
   puts `vr-light-gun-hub-quest.apk` and `vr-light-gun-hub-pcvr.zip` (`flycast.exe`, `emu.cfg`,
   `README.txt`, `LICENSE.txt`) in `dist\`. It refuses an APK signed with the public debug key
   (`-AllowDebugKey` takes one for a test build), and either file if its file list has anything
   of a game in it (disc images, keys, rips, frame grabs, the hands or the staff). `-SkipBuild`
   packages what was built last; `-CheckOnly <archives>` only checks the contents.
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
- `hotd2-vr/rip-hands.ps1`, `hotd2-vr/assets`: the agent's hands from your game on the PC
  (above). The PC build rips a frame on request (`rip.request` in its working folder: every polygon as
  the GPU got it, with the VRAM), `assets/riplib.py` reads that and decodes the textures,
  and `assets/rip_hands.py` lifts the hands and pistol back into 3D, stands the pistol
  upright, cuts its slide free along the line painted on its sides and writes `hands.bin`.
- `hotd2-vr/re/sh4dis.py`: disassembles game code from a Flycast RAM dump (Capstone),
  which is how the field-of-view literals were found.
- `hotd2-vr/run-test.ps1`: PC test runs with screenshots.
- `hotd2-vr/vr-test.ps1`, `hotd2-vr/accept.ps1`: scripted PC runs of any of the three games
  (gun input over UDP, window shots, rips, log checks), and the check that HOTD2 and The Maze
  of the Kings still play as before (`accept.ps1 -Only hotd2,mok`). They find the games through
  `$env:VRLGH_GAMES` (the folder with the MAME sets and `naomi.zip`) and `$env:VRLGH_HOTD2`
  (HOTD2's disc).

The older scripts look for your game in a `game` folder next to the repository, or in
`%HOTD2VR_DATA%\game`.

## Problems? Ideas?

Open an [issue](../../issues) here (not on mikermak's project): the form asks for the build
(Quest app or PC VR), the release, the headset (and the PC VR runtime), the game and its
version, and a log, which is what it takes to find out what's wrong. On the Quest, with the
headset connected to a PC by USB, `adb logcat -d -s flycast:*` (adb comes with Android's
platform-tools) prints the app's log, right after the problem. PC VR writes `flycast.log`
next to `flycast.exe` with `log:LogToFile = yes` in `emu.cfg`. Never attach or link game files.

## Credits and licences

- **[The House of the Dead 2 VR](https://github.com/mikermak/hotd2-vr) by mikermak**: VR Light
  Gun Hub is a fork of it. The VR mode, The House of the Dead 2 and The Maze of the Kings in VR,
  the hands and the staff, the setup panel and the tools are his. GPL-2.0-or-later.
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
- The House of the Dead is a trademark of Sega; The Maze of the Kings and Confidential Mission
  are Sega's games. Namco is a trademark of Bandai Namco.

Unofficial fan project, not affiliated with Sega. No game is included.
