# Confidential Mission VR 1.1: headset checklist

One session on the Quest 3, in this order (about 90 minutes), for what the PC harness (the
OpenXR simulator, `vr.XrSim`) cannot prove. Each item says what to do and what should happen.

**The build** (made by `package-release.ps1` and signed with the release key; not installed yet):
- Quest: `E:\___flattovrdev\hotd2-vr\dist\confidential-mission-vr-quest.apk`, versionName `1.1.0-g509071f58` (versionCode 8, built 2026-10-24 from commit 509071f58, signed with the release key)
- PC VR: `E:\___flattovrdev\hotd2-vr\dist\confidential-mission-vr-pcvr.zip` (its `flycast.exe` a Release build of the same commit, 509071f58)

**How to report:** play the build, then mark each item `[x]` (works) or `[!]` (fails) and add a
short note after a failing one (what you saw, where in the game, which hand). Leave items you
did not get to as `[ ]`. Tell the next session "headset checklist updated"; it reads this file
and turns every `[!]` into a fix task. Keep the log of a run with a failure (below) and say
where it is.

**Before you start:**
- The headset charged, its USB cable, and on the PC adb (Android platform-tools) or SideQuest.
- Your three game files in the headset's Download folder: `confmiss.zip`, `confmiss/gds-0001.chd`, `naomi.zip`.
- OVR Metrics Tool on the headset (free, Meta Horizon Store), for section 11.
- VR Light Gun Hub 0.1.0 stays installed until section 12.
- **The log.** With the headset on USB, `adb logcat -s flycast:* > cm-log.txt` in a PC window writes the
  app's log to `cm-log.txt` while it runs (Ctrl+C stops it). Keep it running through sections 2 and 3.
  Later, `adb logcat -d -s flycast:* > cm-log-<section>.txt` right after a section saves its recent lines.
  Where an item names a log line, look for it there.
- **The app's data folder** on the headset: `/sdcard/Android/data/com.vrlightgunhub.confmiss/files/`
  (`emu.cfg`; `data/` holds the guns made from your game, `gun-cm-*.bin`, and the bundled models, `data/models/`).
  Below, `$D` stands for its `data` folder: `/sdcard/Android/data/com.vrlightgunhub.confmiss/files/data`.

## 1. Install the APK

- [ ] **Do:** `adb install -r E:\___flattovrdev\hotd2-vr\dist\confidential-mission-vr-quest.apk` (or SideQuest: Install APK file from folder).
  **Expect:** `Success`. VR Light Gun Hub 0.1.0 is still installed beside it (another app ID).
- [ ] **Do:** open the Library, Unknown sources.
  **Expect:** "Confidential Mission VR" with its own icon (the CM monogram in red corner brackets).

## 2. The start screen and files

- [ ] **Do:** start the log window (Before you start), then open Confidential Mission VR.
  **Expect:** the 2D start screen ("CONFIDENTIAL MISSION VR"), not the game; no game list, nothing about other games.
- [ ] **Do:** follow step 1, All files access.
  **Expect:** on this fresh install step 1 says to allow it, and the file rows say "Not checked yet". Allow opens the
  setting; after switching it on and coming back the screen says "Done."
- [ ] **Do:** move `naomi.zip`, then the `confmiss` folder, then `confmiss.zip` out of Download in turn
  (e.g. `adb shell mv /sdcard/Download/naomi.zip /sdcard/` and back), pressing Look again after each.
  **Expect:** the row in red names the missing file and where it goes. With all three back, every row says "Found"
  and the Play button appears.
- [ ] **Do:** note the clock, then press "Play Confidential Mission".
  **Expect:** the game starts in VR (first the gun panel of section 3). No permission prompt, or an understandable
  one about the controllers' models.
- [ ] **Log** (the first start copies the bundled models from the APK, never run on a device before):
  `Bundled models: 9 files copied to …/files/data/models`, once; then
  `XR: own models from …/data/models/guns, …/data/models/controllers (2 guns, controllers left right)`;
  `XR: runtime extensions:` lists `XR_FB_render_model`; the runtime named is the headset's, never `XRSIM`.

## 3. The first-start gun run

- [ ] **Do:** watch, touching nothing.
  **Expect:** the panel "CONFIDENTIAL MISSION VR", "Taking the guns from your copy of the game" instead of the game,
  up the whole time (the attract demo plays to its train-car scene, then the game is started for the other guns).
  **Note:** the minutes from pressing Play to the panel going away: the Quest's first start time, with the models'
  copy. The log: `VR hands: done after N s of game`.
- [ ] **Log:** `VR hands: the assault rifle from this game`, `VR hands: the cabinet gun from this game`,
  `VR hands: the agents' pistol from this game (complete)`, then `XR: gun: agents' pistol (auto)`
  (or `cabinet gun (auto: the agents' pistol is incomplete)`).
- [ ] **Do:** once the game shows, quit (Meta button, Quit), open the app again and Play.
  **Expect:** no panel (`VR hands: the guns, made before`), no `Bundled models` line, and after A the agents' pistol
  in the hand. Stop the log window now.
- [ ] **Do (skip and missing guns; back the files up first):** `adb pull $D/gun-cm-cabinet.bin`, the same for
  `gun-cm-pistol.bin` and `gun-cm-rifle.bin` (into the PC's current folder). Then `adb shell rm $D/gun-cm-rifle.bin`
  and start.
  **Expect:** no panel; the game starts as usual (the rifle is only watched for in play).
- [ ] **Do:** `adb shell rm $D/gun-cm-cabinet.bin $D/gun-cm-pistol.bin`, start, and press B at the panel.
  **Expect:** the game starts with the built-in arcade gun and keeps it all session, through the reload tutorial too
  (no `VR hands: the cabinet gun from this game`); the log says `VR hands: watching for the assault rifle in play`.
  Quit and start again: the panel shows again (B again to skip).
- [ ] **Do:** `adb push` the three files back to `$D/`, start, choose "Agents' pistol" on the menu's Gun row (≡, not
  Auto), quit; `adb shell rm $D/gun-cm-pistol.bin`, start, and press B if the panel shows.
  **Expect:** the cabinet gun in the hand (`(chosen: the agents' pistol is not made yet)`); the Gun row skips the
  agents' pistol.
- [ ] **Do:** `adb push gun-cm-pistol.bin $D/`, start, set the Gun row back to Auto.
  **Expect:** no panel; the agents' pistol in the hand again.

## 4. Controllers and labels (a fresh start, before pressing anything)

- [ ] **Do:** look at both hands.
  **Expect:** the bundled Quest controllers (`XR: controller model from bundled models/controllers/left.glb (5609
  triangles`, and right), textured, not plain grey; the handle where your hand is, the buttons, stick and trigger over
  the real ones, moving with no lag. If not, note how far off (cm, which way) and how far turned (degrees).
- [ ] **Do:** read the labels.
  **Expect:** right "A Start", "B Reload", "Trigger Shoot", "Stick click Laser"; left "X Player 2", "Y Reload P2",
  "Menu", "Stick click Recenter". Readable at arm's length without leaning in.
- [ ] **Do:** bring each controller up close and turn it (tip it up to see the trigger).
  **Expect:** the how-to-play label lines end on their buttons (X, Y, Menu, stick; A, B, stick, trigger), on the
  model's own buttons, the stick's cap and the trigger's front (the log has `XR: labels: left x at (...) in the aim pose
  (from the file)` for each). Note any that point beside one: which, and how far (mm, which way).
- [ ] **Do:** press A.
  **Expect:** the labels go and the right hand turns into the gun; the left hand shows nothing (controllers show only
  while how to play shows).
- [ ] **Do:** quit, start again, and pull the trigger first instead of A.
  **Expect:** that hand turns into the gun, the labels go, and the pull fires.

## 5. Buttons

- [ ] **Do:** each Quest button from the README table.
  **Expect:** A Start player 1; B reload player 1 (it sounds like the game's own reload); X Start player 2; Y reload
  player 2; ≡ the menu; left stick click recenter; right stick click laser off/on; trigger shoot; grip + stick up/down
  world size; grip + stick left/right gun size; stick the game's D-pad. Holding the Meta button recenters too.
- [ ] **Do:** shoot away from the game's view (past the grey ring).
  **Expect:** it reloads, with no knock.
- [ ] **Do:** the Meta button mid-game (no menu open), then back.
  **Expect:** the game resumes as in v1.
- [ ] **Known:** there is no player 1 Start on the left controller (A is on the right; X is player 2's Start), so
  left-hand-only play can't start player 1. **Do (optional):** with the right controller put down, press X on the
  game's start screen. **Note** what happens.

## 6. The menu and pause

- [ ] **Do:** ≡ mid-game.
  **Expect:** the picture freezes under a "PAUSED" panel of ten rows, about an arm's length ahead and a little below
  your eyes, every row and the footer readable without leaning in, nothing cut off. The picture still follows your
  head (look around and lean: it stays put in the room), the guns still follow your hands, the game's sound stops.
  **Note:** a stutter as the picture freezes (yes or no; the pause takes about 65 ms, 3-4 headset frames).
- [ ] **Do:** point with each hand, the laser on and then off.
  **Expect:** each hand's line and dot land on the row it points at, the selection follows, and either trigger chooses.
- [ ] **Do:** the right and then the left stick up and down, left and right; A.
  **Expect:** the selection moves round (Quit to Resume and back); left/right change a value, and flip an On/Off row
  (Laser, Wider view, How to play); A chooses, and on a value row is +.
- [ ] **Do:** turn your body, then choose Recenter.
  **Expect:** the view goes where you face; the menu stays open, in front of you.
- [ ] **Do:** click the left and right halves of Haptics, World size, Panel distance and Gun; click Laser and Wider view.
  **Expect:** left half steps down, right half up; a click anywhere on an On/Off row flips it; each Haptics change gives
  a sample knock in that hand.
- [ ] **Do:** How to play, On.
  **Expect:** while the menu stays open, both hands hold their Quest controllers with the labels, and the guns leave
  the hands. Off (A again, or the stick left/right) gives the guns back. On again, then close the menu (≡, B or
  Resume): How to play goes off, the guns are back, nothing in the left hand.
- [ ] **Do:** rest the right hand's line on the panel (on any row), step to How to play with the stick, press A three
  times.
  **Expect:** each A flips How to play; the highlighted row stays on How to play although the line's dot jumps a few
  cm each time (it leaves the muzzle with a gun, the controller without). No other setting changes.
- [ ] **Do:** with How to play on, compare where each controller's pointer line leaves it with the Quest's own pointer
  (the Quest's menu after the Meta button: its line from the real controller).
  **Expect:** the line leaves the model where the Quest's own line leaves the real controller, pointing the same way.
  If not, note how far off (cm, which way) and how far turned: only `aim` in `controllers/left.json` and
  `controllers/right.json` would change.
- [ ] **Do:** Quit; choose it again within 3 s.
  **Expect:** the first choose turns its row into "Quit? Choose again" (moving off it, a stick move or 3 s turns it
  back); the second returns to the start screen, and Play starts the game again.
- [ ] **Do:** ≡ or B with the menu open.
  **Expect:** the game goes on exactly where it stopped; the closing B does not reload, and an A that chose Resume does
  not press Start.
- [ ] **Do:** take the headset off with the menu open, wait 10 s, put it back on.
  **Expect:** still paused, the panel in front of you (recentred); no black headset, and the game has not run on.
- [ ] **Do:** the Meta button with the menu open (Quest home), then back.
  **Expect:** the dark room and the menu, then Resume works.
- [ ] **Do:** with the game running, open the app from the Library again.
  **Note:** what you get (the game back, or the start screen; if the start screen, Play starts the game afresh).
- [ ] **Do:** change Laser, Haptics, World size, Panel distance and Wider view; quit, close the app, open it and play.
  **Expect:** all five as you left them (Wider view's change applies now).

## 7. The guns

- [ ] **Do:** step the Gun row (+ and -).
  **Expect:** Auto, Agents' pistol, Cabinet gun, Arcade gun, Namco gun, Zapper (the last two bundled), round and round;
  each shows in the hand at once, while the game is paused.
- [ ] **Do:** step the Gun row onto Zapper and onto Namco gun, a few times each (their files are read when the row
  lands on them); then start a game with one of them chosen.
  **Expect:** stepping onto Zapper/Namco on the Gun row: note any hitch, and how long (the paused view freezing or
  juddering; a guess in ms is fine). The same for the first frame of the game started with one of them.
- [ ] **Do:** click the Gun row's halves with the trigger.
  **Expect:** after each change the dot stays on the Gun row (the new gun's muzzle moves it a few cm; rows are 6.3 cm apart).
- [ ] **Do:** with each gun (agents' pistol, cabinet gun, arcade gun, Zapper, Namco gun) shoot at the screen.
  **Expect:** the gun kicks back on each shot (recoil, as in v1), the laser leaves the muzzle, the flash sits at the
  muzzle, the shots land on the dot. Note any gun whose line or flash leaves elsewhere (which, how far, which way).
- [ ] **Do:** look at each gun from all sides.
  **Expect:** the agents' pistol whole (suppressor, slide, grip, trigger guard; no holes, nothing inside-out); the
  cabinet gun barrel forward, grip down in your hand, about 27 cm, its colours even (not blotchy or faceted); the
  Zapper and the Namco gun real size (25 cm), held by the grip.
- [ ] **Do:** look at the Namco gun's colour.
  **Expect:** the cyan of its Sketchfab page (its vertex colours applied). A lighter, washed-out cyan means they are not.
- [ ] **Do:** a file of yours over a bundled one: from the PC zip's `data\models\guns\`, push the Namco gun under the
  Zapper's name: `adb push namco.glb /sdcard/Download/guns/zapper.glb` and `adb push namco.json
  /sdcard/Download/guns/zapper.json`. Open the menu.
  **Expect:** the Gun row has "Namco gun" twice and no Zapper; the second one (your `zapper.glb`) shows the Namco model;
  the log says `XR: own gun guns/zapper.glb offered: "Namco gun"` (no `bundled models/` before it). Then
  `adb shell rm /sdcard/Download/guns/zapper.glb /sdcard/Download/guns/zapper.json` and open the menu again: the Zapper
  is back.

## 8. Haptics

- [ ] **Do:** fire single shots, then fast.
  **Expect:** the knock comes with the game's own shot (the shot sound, a bullet going from the HUD), not ahead of it or
  late; every shot gets its own knock.
- [ ] **Do:** empty the magazine, pull again, then reload.
  **Expect:** a light click on the empty pull, clearly weaker than a shot; after the reload the knock is back.
- [ ] **Do:** Haptics row Low, Normal, Strong, Off; a few shots each.
  **Expect:** Low, Normal and Strong feel different; Off gives none at all.

## 9. Player 2

- [ ] **Do:** press X while the game takes player 2.
  **Expect:** the left hand holds player 2's gun (blue aim), both lives and scores on screen. When player 2 is out (the
  game asks for player 2's Start again), the left hand's gun goes and nothing is drawn there.
- [ ] **Do:** shoot with each gun.
  **Expect:** player 2's knocks in the left hand, player 1's in the right, each only for its own shots. B reloads the
  right gun, Y the left; pointing a gun at the floor reloads it.

## 10. The rifle

- [ ] **Do:** shoot an assault rifle a fallen enemy drops.
  **Expect:** the game's rifle in the hand (`XR: P1 hand: assault rifle (gun-cm-rifle.bin)`), whole and the right way
  round (muzzle forward, stock back, magazine and grip down). Its laser leaves its muzzle, the flash sits there, shots
  land on the dot.
- [ ] **Note:** the rifle's size in the hand: right, or too big or small (by about how much).
- [ ] **Do:** hold the trigger.
  **Expect:** rapid lighter knocks, one sharp knock per bullet (not a constant buzz); the first, on the pull, is the
  full Shot.
- [ ] **Do:** Haptics Low, then Off, with the rifle.
  **Expect:** Low lighter (it may be faint: note whether each bullet can still be felt); Off none.
- [ ] **Do:** fire it dry, or play on to the next stage.
  **Expect:** the chosen gun comes back (`XR: P1 hand: back to the ...`), and its pull gives the pistol's knock again.
- [ ] **Do:** player 2 picks up a rifle.
  **Expect:** it shows in the left hand, and its knocks are in the left controller only.

## 11. 72 Hz

- [ ] **Do:** turn on OVR Metrics Tool's performance overlay (FPS), and watch it:
  1. with the menu open, looking around, for 10 s;
  2. in stage 1: the story scene right after Start (the busiest scene, about 21,000 vertices, over v1's reference), then
     the first room with the guards;
  3. with player 2 in (two guns);
  4. with the Namco gun in the hand (17,108 triangles, the heaviest model) and the menu's How to play on (two
     controllers of 5,609 triangles).

  **Expect:** a steady 72. **Note** the lowest FPS and where.

## 12. Uninstalling 0.1.0

- [ ] **Do:** `adb uninstall com.vrlightgunhub.app` (or Settings > Apps > VR Light Gun Hub > Uninstall).
  **Expect:** `Success`; Confidential Mission VR still starts, its settings as you left them.

## 13. The PC VR zip

- [ ] **Do:** unpack `confidential-mission-vr-pcvr.zip`, put the three files in its `roms\` folder (`confmiss.zip`, the
  `confmiss` folder with `gds-0001.chd`, `naomi.zip`), add `log:LogToFile = yes` under `[log]` in `emu.cfg` if you
  want the log, connect the headset (SteamVR, Quest Link or Virtual Desktop), double-click
  `Play Confidential Mission.cmd`.
  **Expect:** the game boots in VR; `flycast.log` next to `flycast.exe` says `XR: runtime <name>`, never `XRSIM`. The
  zip has `CREDITS.txt` at its top and `data\models\`.
- [ ] **Do:** look at the hands before the first Start.
  **Expect:** the bundled Quest controllers (from `data\models\controllers`). With that folder renamed aside: the
  runtime's models where it has `XR_FB_render_model` (Quest Link), else the built-in shape (`XR: controller models
  built in (...)`), held the right way: buttons up under the thumb, the trigger under the index finger, the left one's
  small menu button where a Quest 3's is.
- [ ] **Do:** the menu's Gun row; your controllers' buttons from the README's PC VR table.
  **Expect:** the Namco gun and the Zapper are offered; the buttons do what the table says.
