# Confidential Mission VR 1.2: headset checklist

One session on the Quest 3, in this order (about 75 minutes), for what the PC harness (the
OpenXR simulator, `vr.XrSim`) cannot prove. Each item says what to do and what should happen.

**The build** (made by `package-release.ps1` and signed with the release key; not installed yet):
- Quest: `E:\___flattovrdev\hotd2-vr\dist\confidential-mission-vr-quest.apk`, versionName `1.2.0-g0728039be` (versionCode 9, built 2026-10-24 from commit 0728039be, signed with the release key)
- PC VR: `E:\___flattovrdev\hotd2-vr\dist\confidential-mission-vr-pcvr.zip` (its `flycast.exe` a Release build of the same commit, 0728039be)

**How to report:** play the build, then mark each item `[x]` (works) or `[!]` (fails) and add a
short note after a failing one (what you saw, where in the game, which hand). Leave items you
did not get to as `[ ]`. Tell the next session "headset checklist updated"; it reads this file
and turns every `[!]` into a fix task. Keep the log of a run with a failure (below) and say
where it is.

**Before you start:**
- The headset charged, its USB cable, and on the PC adb (Android platform-tools) or SideQuest.
- Confidential Mission VR 1.1 still installed, played at least once (its guns made), with a Gun row choice of your
  own saved (Auto, Cabinet gun or Arcade gun; or the Namco gun) and World size or Haptics changed: section 1 checks they carry over.
- Your three game files in the headset's Download folder: `confmiss.zip`, `confmiss/gds-0001.chd`, `naomi.zip`.
- OVR Metrics Tool on the headset (free, Meta Horizon Store), for section 12.
- **The log.** With the headset on USB, `adb logcat -s flycast:* > cm-log.txt` in a PC window writes the
  app's log to `cm-log.txt` while it runs (Ctrl+C stops it). Later, `adb logcat -d -s flycast:* > cm-log-<section>.txt`
  right after a section saves its recent lines. Where an item names a log line, look for it there.
- **The app's data folder** on the headset: `/sdcard/Android/data/com.vrlightgunhub.confmiss/files/`
  (`emu.cfg`; `data/` holds the guns made from your game, `gun-cm-*.bin`, and the bundled models, `data/models/`).
  Below, `$D` stands for its `data` folder: `/sdcard/Android/data/com.vrlightgunhub.confmiss/files/data`.

## 1. Install over 1.1

- [ ] **Do:** start the log window, then `adb install -r E:\___flattovrdev\hotd2-vr\dist\confidential-mission-vr-quest.apk`.
  **Expect:** `Success`, with no uninstall needed (the same release key as 1.1).
- [ ] **Do:** open Confidential Mission VR and Play.
  **Expect:** no first-start panel (`VR hands: the guns, made before`: 1.1 made them); your World size, Haptics, Laser
  and Panel distance as 1.1 left them. A saved Auto, Cabinet gun or Arcade gun comes up as the in-game pistol
  (`XR: gun: ...`); a saved Namco gun comes up as the Arcade gun. 1.1's old `gun-cm-cabinet.bin` stays unread.

## 2. The start screen

- [ ] **Do:** look at the start screen.
  **Expect:** VR4All's look (dark panel, violet title, a soft glow), the version `v1.2`, the badge READY in lime, a chip
  per file (`confmiss.zip ✓`, `naomi.zip ✓`, `CHD ✓`) fitting the panel's width, and *Play Confidential Mission* lit.
  The ✓, ▶ and › glyphs render as marks (no boxes, no mojibake such as `âœ“`).
- [ ] **Do:** move `naomi.zip`, then the `confmiss` folder, then `confmiss.zip` out of Download in turn
  (e.g. `adb shell mv /sdcard/Download/naomi.zip /sdcard/` and back), pressing *Look again* after each.
  **Expect:** MISSING FILES (violet), that file's chip says `missing`, and a line names the file and the folder it goes
  in; Play greyed out (dimmed, and a tap does nothing). With all three back: READY, and Play lit again.
- [ ] **Do:** *Where to put your files*.
  **Expect:** the steps open (*Allow*, *Look again*, the Found path of the zip and the disc); tap again to close them.
- [ ] **Do:** take the access away: `adb shell appops set com.vrlightgunhub.confmiss MANAGE_EXTERNAL_STORAGE deny`
  (or Settings > Apps > Special app access > All files access, off), then open the app again.
  **Expect:** ALLOW ACCESS, the hint "Allow it below…" under it, Play greyed out. *Allow* opens the setting; after
  switching it on and coming back: READY.
- [ ] **Do:** Play.
  **Expect:** the game starts in VR. No permission prompt.

## 3. The first start

- [ ] **Do (back the guns up first):** `adb pull $D/gun-cm-pistol.bin` and `adb pull $D/gun-cm-rifle.bin` (into the
  PC's current folder), then `adb shell rm $D/gun-cm-pistol.bin $D/gun-cm-rifle.bin`, note the clock, and Play.
  **Expect:** the panel in the start screen's colours (violet title, cream text, a lime bar on slate): "Making your
  guns from your game files (once)", "About a minute or two, only this once", the footer "B: skip (the Arcade gun this
  time)", the bar filling. Then "Got it. The gun from your game is in your hand." and "Reload: B, or point it away
  from the screen and pull." The in-game pistol in the hand after A.
  **Note:** the minutes from Play to the panel going away (the Quest's first start time; about six minutes of game,
  about a minute on a PC). The log: `VR hands: done after N s of game`.
- [ ] **Do:** `adb shell rm $D/gun-cm-pistol.bin $D/gun-cm-rifle.bin` again, Play, and press B at the panel.
  **Expect:** the game starts with the Arcade gun in the hand, and no gun file is made (`adb shell ls $D`).
- [ ] **Do:** `adb push gun-cm-pistol.bin $D/` and `adb push gun-cm-rifle.bin $D/`, then Play.
  **Expect:** no panel; the in-game pistol in the hand.

## 4. The labels (a fresh start, before pressing anything)

- [ ] **Do:** read the labels on both controllers.
  **Expect:** right "A - P1 Start", "B - P1 Reload", "Trigger - P1 Shoot", "Stick - Laser"; left "X - P2 Start",
  "Y - P2 Reload", "Trigger - P2 Shoot", "Stick - Recenter", "Menu". Readable at arm's length.
- [ ] **Do:** tip the left controller up to see its trigger.
  **Expect:** the new "Trigger - P2 Shoot" line ends on the left trigger's front, as the right one's does. Note any
  line that points beside its button (which, how far).

## 5. The guns

- [ ] **Do:** ≡, step the Gun row (+ and -).
  **Expect:** In-game pistol, Arcade gun, Zapper, then any of your own, round and round; no Auto, no Cabinet gun, no
  Namco gun. Each shows in the hand at once, while the game is paused.
- [ ] **Do:** look at the Zapper next to the Arcade gun.
  **Expect:** the Zapper is visibly bigger than in 1.1 (0.30 m long), held by the grip.
- [ ] **Do:** shoot with each gun.
  **Expect:** the laser leaves the muzzle, the flash sits at the muzzle, the shots land on the dot.

## 6. Gun angle

- [ ] **Do:** ≡, the Gun angle row (under Gun): tap right, then left.
  **Expect:** `+1°`, then `0°`; the gun turns about the grip, the muzzle up for +.
- [ ] **Do:** hold the stick right, then left.
  **Expect:** it keeps stepping (after about 0.4 s, then about 10 a second) and stops at `+30°` and at `-30°`.
- [ ] **Do:** set about +15°, close the menu, shoot.
  **Expect:** the laser and the shots follow the barrel: the shots still land on the dot.
- [ ] **Do:** with the angle still set, press X for player 2, and later pick up an assault rifle.
  **Expect:** the left gun and the rifle are tilted the same way, and their shots land on their dots (not covered by
  the PC tests).
- [ ] **Do:** quit, start again.
  **Expect:** the angle as you left it.

## 7. Reload

- [ ] **Do:** Reload row at Shoot off-screen (the default); empty some rounds, point away from the game's view and pull.
  **Expect:** it reloads, with no kick.
- [ ] **Do:** Reload at Point off-screen; empty some rounds, point away from the view.
  **Expect:** it reloads once, with a short buzz and no kick (`XR: player 1 pointed off screen (reload)`); pointing
  away again reloads only after coming back onto the screen.
- [ ] **Do:** in Point off-screen: point at the grey ring around the view; point away with a full magazine; close the
  menu while pointing away; let the arm rest down through a cutscene and raise it after.
  **Expect:** none of them reloads. B and Y still reload.
- [ ] **Do:** in Point off-screen, with player 2 in, point a gun at the floor.
  **Expect:** no floor reload in this mode; pointing away from the view reloads instead. Set Reload back to Shoot
  off-screen.

## 8. No kick when empty

- [ ] **Do:** with each gun (In-game pistol, Arcade gun, Zapper, and one of your own if you have one), empty the
  magazine and pull again, then reload.
  **Expect:** a light click in the hand on the empty pull, no kick and no flash; a reload gives no kick; after it each
  shot kicks again.
- [ ] **Do:** Haptics Off, a few shots.
  **Expect:** no knock, but the gun still kicks and flashes on each round.
- [ ] **Do:** ≡, the Haptics row: step it from Off through Light, Medium and Strong, a few shots at each.
  **Expect:** a fresh install (or 1.2's Normal) starts at Medium; a sample knock at the new level with each step; Light
  feels like 1.2's Normal, Medium like 1.2's Strong, Strong clearly longer than Medium (shots, empty clicks, the rifle's
  bursts, still a row of separate knocks).

## 9. The rifle with each gun

- [ ] **Do:** with the In-game pistol, shoot an assault rifle a fallen enemy drops.
  **Expect:** the game's rifle in the hand (`XR: P1 hand: assault rifle (gun-cm-rifle.bin)`), bursts with a kick, flash
  and knock per round; fired dry, the pistol comes back.
- [ ] **Do:** the same with the Arcade gun, then with the Zapper.
  **Expect:** the chosen gun stays in the hand and fires the rifle's bursts, each round with a small kick and a flash at
  its own muzzle (`XR: P1 hand: rifle rounds with the ...`); fired dry, its own feel comes back (`... rifle rounds over`).
- [ ] **Do:** player 2 picks up a rifle (with the Arcade gun).
  **Expect:** the same in the left hand, its knocks in the left controller only.

## 10. Cutscenes

- [ ] **Do:** Start a game and watch the stage 1 intro.
  **Expect:** about 0.3 s after the letterbox bars come, both hands are empty: no gun, no controller, no laser line or
  dot; the guns come back the moment play resumes (about 0.1 s after the bars go).
- [ ] **Do:** a new game; press A (Start) in the intro.
  **Expect:** the game leaves the scene (about 1.4 s in the stage 1 intro) and the guns come back with it.
- [ ] **Do:** in a cutscene, pull the trigger at the screen.
  **Expect:** either nothing (the game ignores it; the hands stay empty), or, if the game fires a round, the guns come
  back at once and stay until the scene ends (`XR: cutscene ended by a shot (P1)`).
- [ ] **Do:** play through stage 1's hall (its entrance scene) and on into stages 2 and 3.
  **Expect:** no flicker between guns and empty hands in fights. **Note** any letterboxed moment where you need to
  shoot: the guns should come back on your first shot.
- [ ] **Do:** in a cutscene, open ≡, point at a row, turn How to play on, then close the menu.
  **Expect:** the menu's pointer line and dot show while it's open; How to play shows both labelled controllers; once
  the menu closes the hands are empty again, until play resumes.

## 11. Player 2

- [ ] **Do:** before player 2 joins, pull the left trigger.
  **Expect:** it shoots for player 1: the one gun goes to the left hand (expected, as the README says).
- [ ] **Do:** press X while the game takes player 2, then shoot with each hand.
  **Expect:** the left hand holds player 2's gun (blue aim), the left trigger shoots for player 2 only, each hand's
  knocks in its own controller. When player 2 is out, it's one gun again.
- [ ] **Do:** ≡, the Hands row (under Reload): flip it to "P1 left hand - P2 right hand", look at How to play, close the
  menu; Start with X, let player 2 join with A, shoot with each hand, then Y and B with some rounds gone. Flip it back
  to "P1 right hand - P2 left hand" with both players in, and close the menu.
  **Expect:** the labels read X - P1 Start, Y - P1 Reload on the left and A - P2 Start, B - P2 Reload on the right; the
  left hand holds player 1's gun (red aim, P1's lives and score), the right player 2's (blue); Y reloads player 1 and
  B player 2. Flipped back, the guns change hands as the menu closes, both players still in. The row keeps its value
  after a restart.

## 12. 72 Hz

- [ ] **Do:** turn on OVR Metrics Tool's performance overlay (FPS), and watch it:
  1. the stage 1 story scene right after Start (the busiest), with both controllers shown in it;
  2. the first room, then with player 2 in (two guns);
  3. with the Zapper in the hand and the menu's How to play on.

  **Expect:** a steady 72. **Note** the lowest FPS and where.

## 13. The PC VR zip

- [ ] **Do:** unpack `confidential-mission-vr-pcvr.zip`, put the three files in its `roms\` folder, connect the
  headset (SteamVR, Quest Link or Virtual Desktop), double-click `Play Confidential Mission.cmd`.
  **Expect:** the game boots in VR; `flycast.log` (with `log:LogToFile = yes` in `emu.cfg`) says `XR: runtime <name>`,
  never `XRSIM`. The zip has `CREDITS.txt` at its top and `data\models\`.
- [ ] **Do:** the menu's Gun row; the labels before the first Start.
  **Expect:** In-game pistol (once made), Arcade gun, Zapper; the labels name your controllers' own buttons in the same
  "<button> - <meaning>" form ("Trigger - P1 Shoot").
