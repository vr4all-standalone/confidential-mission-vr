# Confidential Mission on the headset: checklist

**This is the release-signed build** of VR Light Gun Hub (`com.vrlightgunhub.app`), signed with
your own release key. Its version is `0.1.0-g` and a commit code (the build report has the exact
one).

- **First uninstall any earlier VR Light Gun Hub test build** (`com.vrlightgunhub.app`). Those
  were signed with the debug key, and an app signed with a different key can't install over them
  (adb says `INSTALL_FAILED_UPDATE_INCOMPATIBLE`). Its settings go with it.
- **HOTD2 VR (`com.flycast.emulator.vr`) stays installed.** Its settings and the hands it made
  don't carry over to the hub: the hub makes HOTD2's hands again the first time HOTD2 starts
  (README, "Had HOTD2 VR on the headset?").
- **Headset tested on:** ________ (Quest 3 or Quest 3S)

## Install

1. **Remove an old test build**, if there is one: in SideQuest's list of installed apps
   (`com.vrlightgunhub.app`, *Uninstall*), or in a command window:
   `adb uninstall com.vrlightgunhub.app` (an error there just means there was none).
2. **Install the app:** SideQuest, *Install APK file from folder*:
   `E:\___flattovrdev\hotd2-vr\dist\vr-light-gun-hub-quest.apk`. Or:
   `adb install -r E:\___flattovrdev\hotd2-vr\dist\vr-light-gun-hub-quest.apk` (it says `Success`).
3. **Copy the games** into the headset's **Download** folder (SideQuest *Manage files*, or USB:
   *Quest > Internal shared storage > Download*). The zips stay zipped.

   | On the headset | From the PC (`E:\___flattovrdev\confidential mission\`) |
   |---|---|
   | `Download/confmiss.zip` | `Roms\confmiss.zip` |
   | `Download/naomi.zip` | `Roms\naomi.zip` |
   | `Download/confmiss/gds-0001.chd` (a folder `confmiss`, the file in it) | `Roms\confmiss\gds-0001.chd` |

   For check 12, also these, if they aren't there yet: `Download/mok.zip`,
   `Download/mok/gds-0022.chd` (from `Roms\`), and HOTD2's disc
   `Download/House of the Dead 2 The (USA).chd`.

## How to report

For each check: **OK** or **not OK**. For not OK, add the scene and roughly when, for example:
`7 not OK: stage 1, the hall, about 2 min in: shots at the top left land a hand's width low`.
Anything else you notice is welcome too.

The PC already passed boot, the wider view, the aim at the centre, corners and edges, the HUD on
the panel and player 2 joining and leaving (`hotd2-vr/notes/confmiss-findings.md`,
`## Acceptance`). These checks are what only a headset can show.

## Checks

1. **The app in the library.**
   Do: open the Library, *Unknown sources*.
   Good: VR Light Gun Hub, HOTD2 VR and Flycast (if you have it) are three separate apps; the hub
   is called "VR Light Gun Hub" and has its own icon.
   Report: OK, or what's missing or wrong.

2. **The setup panel.**
   Do: start VR Light Gun Hub.
   Good: the first time, a panel asks for *All files access*: tap *Allow*, switch it on, come
   back. Then it says "Found, pick one below:" with Confidential Mission (and The House of the
   Dead 2 and The Maze of the Kings if their files are there). Picking Confidential Mission
   starts it.
   Report: OK, or what the panel says instead.

3. **Boot and attract.**
   Do: let it run a few minutes without pressing anything.
   Good: the logos, the title and the How to Play screen sit flat on the panel; the attract's 3D
   demo is all around you, not only on the panel.
   Report: OK, or the scene that looks wrong.

4. **Start a game.**
   Do: press A on the right controller.
   Good: a game starts with no coins (free play). The gun adjustment screen before it needs
   nothing from you: it counts down and moves on.
   Report: OK, or where it stops.

5. **Stage 1: look around.**
   Do: in stage 1, turn your head left, right, up and down, and lean in.
   Good: the 3D world goes on past the edges of the panel (a wider view than the panel). Nothing
   pops in or out as you turn your head. Nothing sits right in front of your face.
   Report: OK, or what pops or sits at your face, the scene and when.

6. **The size of the world.**
   Do: stand up, look at the enemies and at the floor.
   Good: the enemies look human-sized, and the floor is where your feet are.
   If not: squeeze the grip and push the thumbstick up or down until it looks right (it's kept).
   Report: OK, or too big / too small / floor too high / too low, and whether you changed it.

7. **The aim.**
   Do: shoot at things at the centre, the corners and the edges of the game's view (the panel's
   area).
   Good: each hit lands on the aim dot. Outside the game's view the dot turns into a grey ring and
   the trigger doesn't shoot. Shooting away past the grey ring reloads, and so does B. Every shot
   has the gun's recoil, a muzzle flash and a buzz in the controller.
   Report: OK, or where shots land off and by about how much.
   - **An enemy from the wider margin:** when an enemy comes in from the extra view outside the
     panel, wait until it is inside the panel's area and shoot it once. Good: the hit counts where
     you aimed. Report: OK, or what happened.

8. **The panel: HUD, fades, results.**
   Good: the HUD (lives, ammo, score) and the subtitles are flat on the panel. No streaks or
   stretched lines where the HUD meets the 3D. When you shoot, no flash or shot marker from the
   game itself, and the game's own white fades don't flicker or strobe while you shoot.
   - **Results screen and stage titles:** at the end of stage 1 and the start of stage 2, the
     results screen and the stage's title are flat on the panel. (The PC never got that far.)
   Report: OK, or what looks wrong, in words.

9. **Player 2 (two guns).**
   Do: during a game, press the menu button (≡) on the left controller.
   Good: player 2 joins with a gun in your left hand (blue aim), and both guns shoot. In
   one-player play there is no "PRESS START BUTTON" for player 2 at the bottom right. When player
   2 is out of the game, it's one gun again.
   - **Player 2 reloads:** with two guns, empty the left gun, reload it (point it at the floor,
     or shoot past the view) and keep playing with both for a while. Good: you keep both guns; it
     doesn't drop back to one.
   Report: OK, or what happened and when.

10. **Frame rate.**
    Good: smooth at 72 Hz all through stage 1, with no judder (the picture stuttering or doubling
    when you turn your head). If the OVR Metrics Tool is installed, its overlay shows the frame
    rate (72 is good). Watch the story scene that plays right after you press Start, before the
    shooting begins: it is the busiest scene so far (on the PC about 46-52 s after the app
    started, with Start pressed early).
    Report: OK, or which scene stutters and for how long.

11. **After stage 1.**
    Do: play on into stage 2 (continues are free).
    Good: in the shooting scenes there, the hits still land on the dot. (The PC checked stage 1
    only.)
    Report: OK, or the scene and where shots land instead.

12. **The other two games.**
    Do: quit (Meta button, *Quit*), start the hub again and play one stage of The House of the
    Dead 2, then one of The Maze of the Kings. The first start of each makes the hands or the staff
    from your game: it plays itself fast for a minute or two behind a panel that says so (B skips).
    Good: both play as they did in HOTD2 VR.
    Report: OK, or what's different.

13. **PC VR over Link.**
    Do:
    - Connect the Quest with Link (cable or Air Link). In the Meta Quest Link app on the PC, make
      it the active OpenXR runtime (*Settings > General > OpenXR Runtime*); the PC currently uses
      Virtual Desktop's.
    - Unzip `E:\___flattovrdev\hotd2-vr\dist\vr-light-gun-hub-pcvr.zip` into a new folder and
      start `flycast.exe`. In its window add the folder `E:\___flattovrdev\confidential mission\Roms`
      (naomi.zip is in it), start Confidential Mission and play stage 1.
    Good: checks 3 to 9 as on the Quest.
    Report: the check numbers with "PC" in front, e.g. `PC 7 OK`.
    For a log on the PC, add these two lines at the end of `emu.cfg` (next to `flycast.exe`); it
    then writes `flycast.log` next to `flycast.exe`:
    ```
    [log]
    LogToFile = yes
    ```

14. **When something is wrong: the log.**
    - Best: before starting the app, with the headset on USB, run in a command window
      `adb logcat -c` and then `adb logcat -s flycast:* > cm-quest-log.txt`; play; press Ctrl+C
      when done.
    - Afterwards: `adb logcat -d -s flycast:* > cm-quest-log.txt`, straight away (it only holds
      the last few minutes).
    - As a file instead: put the two `[log]` lines above at the end of
      `/sdcard/Android/data/com.vrlightgunhub.app/files/emu.cfg`; the app then writes
      `/sdcard/Android/data/com.vrlightgunhub.app/files/data/flycast.log`
      (`adb pull /sdcard/Android/data/com.vrlightgunhub.app/files/data/flycast.log`).

    Send the log with your notes. It has a `VR diag: shot` line for each of the first 8 shots
    after the app starts, which shows where those shots went. On GitHub later, the issue form
    (`.github/ISSUE_TEMPLATE/bug_report.yml`) asks for the same: the build, the headset, the game
    and the log. Never attach game files.
