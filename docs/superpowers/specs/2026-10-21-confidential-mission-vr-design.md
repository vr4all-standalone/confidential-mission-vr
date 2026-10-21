# Confidential Mission VR: design

Date: 2026-10-21. Status: draft, waiting for the user's approval.

## Intent

The user wants Sega's arcade light-gun games playable in VR on a Meta Quest, starting with
**Confidential Mission** (NAOMI, 2000), the way *The House of the Dead 2 VR* and *The Maze of
the Kings VR* already play: the original game runs in an emulator, its 3D scenes are rebuilt
around the player in stereo, and the controller is the light gun. This is the first game of the
user's own app, which will become a launcher for all NAOMI light-gun games (a later project), and
after that maybe Model 3 (a separate, bigger project). The user will publish it.

The base is mikermak's [hotd2-vr](https://github.com/mikermak/hotd2-vr), a Flycast fork that
already does all of this for HOTD2 (Dreamcast) and The Maze of the Kings (NAOMI GD-ROM, like
Confidential Mission). Adding a game to it means adding a per-game profile of reverse-engineered
numbers; the Maze of the Kings commit (`eb2e300cd`) is the template.

## What the player experiences (v1)

1. Starts the app from the Quest library. The setup panel finds the game in the headset's Download
   folder (or asks which game, if several are there) and starts it.
2. Boot and attract: logos, title and "How to Play" sit on the floating 2D panel in front of them;
   the 3D attract demo scenes are rebuilt around them like gameplay.
3. Free play: pressing Start (A) begins the game. No coin button.
4. In the game they stand where the game's camera is and ride its path; they can look around freely
   and lean. The view is widened inside the game, as HOTD2 VR (41° to 74° vertical) and Maze of the
   Kings VR (60° to 90°) do, as far as Confidential Mission allows while holding full frame rate on
   a Quest 3. Behind them there is nothing (the game only draws what is in front of its camera).
5. The HUD, menus and subtitles are on the panel. The game's own shot flash and marker are hidden;
   a laser line and dot show the aim.
6. The gun: either controller; shots are traced into the rebuilt scene and land on the dot; recoil,
   muzzle flash and a haptic knock on each shot. Reload by shooting away from the game's view (with
   two guns: point a gun at the floor). The gun shown is the red arcade gun HOTD2 VR uses by default.
   Only the game's original view counts for shots (its light gun only knows that frame): in the
   widened border around it the aim dot turns into a grey ring and the trigger does nothing, as in
   HOTD2 VR.
7. One player by default. The left controller's menu button lets player 2 join with a gun in the
   left hand (one person, two guns), as in HOTD2 VR; it drops back to one gun when player 2 is out.
   No floating "press start" prompt for player 2 in a one-player game.

## Goals

- v1: everything in "What the player experiences", on Quest 3 (primary) and PC VR (same code; built
  and checked on the PC).
- The app is the user's own: its own name and application ID, so it installs next to HOTD2 VR and
  normal Flycast. Working title **NAOMI Gun VR** (the user may rename it before the first release;
  the application ID must be settled before the first public release, since changing it later
  breaks updates).
- The inherited games keep working (HOTD2 Dreamcast, The Maze of the Kings).
- v1.1 (the next update, not v1): the agents' own pistol taken from the user's copy of the game, the
  way HOTD2 VR takes the agent's hands, if a usable scene shows it whole; with it, racking the slide
  to reload.

## Non-goals

- A full 360° scene.
- The Dreamcast release of Confidential Mission (NAOMI set only).
- Other NAOMI games, the launcher for them, Model 2/3: later projects.
- Two players on two headsets (netplay).
- Coin insertion.

## Anti-goals (fails even if it "works")

- Shots that don't land where the dot is, anywhere inside the game's view.
- Discomfort: frame drops below the headset's refresh rate on Quest 3 (72 Hz, the fork's default
  `vr.XrRefreshRate`), streaks or bent 2D where the
  HUD meets the 3D, things popping in or vanishing when the player turns their head (the game
  culling against its own narrower frame), 2D or 3D coming right at the face.
- Breaking HOTD2 or The Maze of the Kings.
- Any game data (code, graphics, sound, ROMs, ripped models) in the repository, the APK or a
  release; ripped data stays on the user's device.

## Constraints

- Repository: `E:\___flattovrdev\hotd2-vr` (git clone of mikermak/hotd2-vr), branch `naomi-gun-vr`
  from `hotd2-vr` at `eeb629dce`. Follow the fork's patterns: a `GameProfile` in
  `core/rend/vr_reproject.cpp` `makeProfiles()`, an entry in `VrGames.java` `ARCADE`.
- Licences: Flycast is GPL-2.0-or-later; builds including the gun model (CC BY 4.0) are GPL-3.0.
  The source is published with every release. Credit mikermak, Flycast and the gun model's author.
- Game files (the user's MAME sets, never committed or copied into the repo):
  `E:\___flattovrdev\confidential mission\Roms\`: `confmiss.zip` (only `317-0298-com.pic`),
  `confmiss\gds-0001.chd`, `naomi.zip` (BIOS); also `mok` (for the regression check) and other
  NAOMI/Atomiswave sets for later projects. The HOTD2 regression check uses the Dreamcast disc
  `E:\___flattovrdev\confidential mission\House of the Dead 2 The (USA).chd` (US profile `MK-51002`;
  the `hotd2` zips are the arcade sets, which have no profile).
- Builds (both verified unmodified on this machine, about 2 minutes each):
  - PC: `hotd2-vr\build-win.cmd` -> `build-win\flycast.exe`.
  - Quest: `JAVA_HOME=E:\UnityEd\6000.0.78f1\Editor\Data\PlaybackEngines\AndroidPlayer\OpenJDK`,
    `ANDROID_HOME=E:\AndroidSdk`, then `hotd2-vr\build-quest.cmd` ->
    `shell\android-studio\flycast\build\outputs\apk\vr\flycast-vr.apk`.
- The reverse engineering happens on the PC build (rips, RAM dumps, `vr.DebugShots`, scripted UDP gun
  input, `hotd2-vr\re\sh4dis.py`). The procedure, MOK's profile values, the game-specific spots and
  the known traps are in `hotd2-vr\notes\adding-a-game.md`; read it first.
- Headset tests are done by the user on their Quest 3 (sideload); each round needs them, so batch
  what goes into a headset build. PC VR can be checked with the Quest over Link.
- Upstream already gives `confmiss` the same light-gun I/O board as MOK (`maple_jvs.cpp:1527`); its
  boot ID is `CONFIDENTIAL MISSION ---------` (check the exact spacing against the log line).

## Decided

- Confidential Mission is added as a `GameProfile` (game ID from the NAOMI header) plus a launcher
  entry, on the existing engine; no new rendering approach.
- v1 ships with the arcade gun (profile model: none); the pistol is v1.1.
- Free play, no coin button.
- Both Quest and PC VR, from the same code.
- The user's own app identity (name, application ID, icon) instead of contributing back to mikermak.

## Publishing (Claude's calls; the user may override)

- Shared the way mikermak shares HOTD2 VR: an APK installed with SideQuest and a PC VR zip, on the
  user's own GitHub Releases, with the source. Not Meta's store: it would not take an app that reads
  the player's own game files with "All files access", which step 1 relies on.
- Releases are signed with a new private key the user keeps outside the repository (the committed
  `shell/android-studio/debug.keystore` is public, so anyone could sign an "update" with it). The
  key is made before the first release, since changing it later forces a reinstall that loses
  settings and anything ripped from the game. How the build finds the key (a local properties file
  or environment variables, never committed) and whether release builds stay debuggable are the
  builder's call.
- In this project: a release-ready build (the private-key signing, the packaging script making the
  APK and the PC VR zip, a README for this app with install steps for Confidential Mission and the
  credits). Out of this project: actually publishing (creating the GitHub repository, pushing,
  uploading a release). That happens only when the user says so, since it is public.

## Left to the builder

- How to widen Confidential Mission's view (literal patch, builder patch or something else), and
  how far; all addresses and values of the profile; the cull-margin fix if the game needs one.
- Whether the NAOMI EEPROM/test-menu settings need pokes (free play, gun calibration), and how to
  gate pokes until the GD-ROM code is loaded.
- Generalising the PC test scripts (`run-test.ps1` forces HOTD2's matrix addresses).
- The application ID, the icon, and how the build sets the name.
- Which scenes and screen positions the PC checks cover.

## Done means

- PC: the game is recognised by its profile (log), its view is widened (log line and screenshots of
  attract, the first stage and a cutscene), UDP shots at the centre, corners and edges of the game's
  view land on the sent position (shot log), the HUD sits on the plane with no streaks, the shot
  flash is gone, player 2 joins and leaves, HOTD2 and MOK still boot with their profiles.
- Quest 3: the user plays the first stage and confirms the feel, the aim, full frame rate, and that
  the app installs next to HOTD2 VR.
- Release-ready: the packaging script makes a privately signed APK and a PC VR zip with the README;
  nothing of the game is in either.
