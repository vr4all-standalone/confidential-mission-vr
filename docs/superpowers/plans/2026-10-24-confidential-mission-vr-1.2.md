# Confidential Mission VR 1.2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Take Confidential Mission VR from 1.1.0 (versionCode 8) to **1.2.0, versionCode 9** with the user's eight polish items: three guns, the rifle with file guns, no kick when empty, the Reload and Gun angle rows, a VR4All-style start screen, controllers in cutscenes and new labels.

**Architecture:** This is the same Flycast fork and the same files as 1.1. Most changes are edits to existing code:
- the gun choice: `xr_hands`, `xr_menu`, `xr_own`;
- the first-start rip: `hands_rip`, the CM profile in `vr_reproject.cpp`;
- the per-hand gun loop: `xr_host.cpp` (`updateGun`, `gunKnocks`, `updateMenu`, `holds`);
- the labels: `xr_labels.cpp`;
- the start screen: `CmStartActivity.java`.

Every item gets a PC test through the OpenXR simulator in `hotd2-vr\accept.ps1`. The start screen's logic moves into a pure Java class with JUnit tests.

**Tech Stack:** C++17 (Flycast fork, OpenXR 1.1.43), Java + JUnit 4 (Gradle Kotlin DSL), Windows PowerShell 5.1 harness, Python 3.13 fixtures.

**Spec:** `docs/superpowers/specs/2026-10-24-confidential-mission-vr-1.2-design.md` (binding; sections 1-8 and "Done means"). Read the spec section each task names.

**Repo:** `E:\___flattovrdev\hotd2-vr`, branch `naomi-gun-vr`. Line numbers below are at d8c77c843. Later tasks see earlier tasks' edits, so search for the excerpt when a line has moved.

## Global Constraints

From the spec and the brief, verbatim where quoted:
- "Never print or log the release key's password. Never copy, move or delete `C:\Users\ege19\.vrlightgunhub`. The key never goes in the repo or `dist`."
- "No game data in the repo, the APK, the zip or any report: no ROMs, CHDs, rips or RAM dumps; no game screenshots or disassembly listings; no ripped models; none of the game's on-screen text." Addresses and offsets are fine.
- The user's models are never committed to git. They ship in the APK and the zip from the gitignored `hotd2-vr/models/`, credited in CREDITS. The Zapper (CC BY-ND) gets technical changes only.
- "Never stage or touch the modified `core/deps/gamesdk` submodule. Nothing is pushed or published. The orchestrator installs on the Quest 3 (`2G0YC1ZF840H2Y`) at the end; tasks don't install."
- Downloads are untrusted: each extraction goes in its own new scratchpad folder, and Python runs on them with `-I`.
- Test fixtures come from `hotd2-vr/re/make-own-models.py`. Never assert on the user's own models: a harness run that would draw the bundled `namco.glb`/`zapper.glb` from `build-win\data\models` uses `-Bundled` fixtures or `vr.BuiltInGun=yes` (Task 1).
- Commit messages start with `hotd2-vr:` and end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Stage files by name, never `git add -A`.
- The user is never asked anything mid-run. Decide, note the decision in the report, and keep going.
- Engine work is C++ and PowerShell here; there is no Unity or Godot skill to load.

## Review Focus

These are the five inputs most likely to hurt a player that the spec implies but doesn't spell out. Each has a test in the task that owns it.
1. **A 1.1 player who chose the Namco gun** has `vr.Gun=4`, `vr.GunFile=namco` saved (any case). After the upgrade they get the Arcade gun, not the pistol. Test: Task 1, the `cm-gun` "upgrade" run.
2. **A player's own `guns/zapper.glb`** replaces the bundled one, keeps the Zapper's slot (right after the Arcade gun) and isn't sorted among the player's other guns. Test: Task 1, the `cm-own-gun` "override" run.
3. **With Haptics Off (or Low),** the gun still kicks once per fired round and never on an empty pull. The kick moves next to the haptics code, so this can silently break. Test: Task 3, the `cm-haptics-off` kick count.
4. **Point off-screen** does nothing in these cases:
   - with a full magazine;
   - when aiming at the grey margin;
   - when the menu closes while the gun points away.
   It reloads once per exit, never repeatedly. Test: Task 4, `cm-reload-point`.
5. **Gun angle** out of range: a saved `vr.GunAngle=90` reads as +30, and holding the stick past an end stops there with no extra log lines or saves. Test: Task 4, `cm-gun-angle` "clamp" run.

---

## Order, and why the split differs from the brief

| # | Task | Spec | Why here |
|---|---|---|---|
| 1 | Three guns: list, order, fallback, old settings | §1 | Tasks 3-6 name guns, and their tests need its fixtures and the `vr.BuiltInGun` switch. |
| 2 | The first-start rip without the cabinet, plus the rip-speed investigation | §1 cabinet, §6 speed | Split from Task 1. The rip is its own subsystem (`hands_rip`, the profile's `guns[]`), and its scenarios take minutes each. The speed question lives in the same code, so it moved here from the start-screen task. |
| 3 | Kick only on a fired round, plus the rifle with file guns | §2, §3 | Both live in `gunKnocks`/`handGun`. Needs Task 1's `chosenGun()`. |
| 4 | The Gun angle and Reload rows, with hold-repeat | §4, §5 | Edits the same `updateGun` block as Task 3, so it runs after it. |
| 5 | Controllers in cutscenes, plus the new labels | §7, §8 | Both are holds/labels work in `xr_host.cpp` `holds()` and `xr_labels.cpp`. The cutscene part is research-heavy, so the labels go first. |
| 6 | VR4All start screen and the first-start wording | §6 | Java plus the headset panel text. Its done line follows Task 4's `vr.Reload`, and its skip line follows Task 1's fallback. |
| 7 | Release 1.2.0 / 9 | Done means | Full regression, both builds, `package-release.ps1`, the 1.2 checklist. |

All tasks run one after another; none can run in parallel safely, because they share `accept.ps1`, `README.md` and `xr_host.cpp`. Each task updates its own README section. Task 7 does a final README pass and writes the checklist from the "Checklist items" line each task ends with.

## File map

| File | Tasks | Responsibility |
|---|---|---|
| `core/rend/vr/xr_hands.{h,cpp}` | 1, 3 | gun kinds, the choice, names, the gun per hand |
| `core/rend/vr/xr_menu.{h,cpp}` | 1, 4 | the settings menu rows |
| `core/rend/vr/xr_own.{h,cpp}` | 1 | the folder guns' order |
| `core/cfg/option.{h,cpp}` | 1, 4 | `vr.Gun` default, `vr.BuiltInGun` (PC), `vr.GunAngle`, `vr.Reload` |
| `core/rend/vr/hands_rip.{h,cpp}`, `hands_build.{h,cpp}` | 2 | the CM first-start run |
| `core/rend/vr_reproject.{h,cpp}` | 2, 5 | the CM profile's guns; the cutscene signal |
| `core/rend/vr/xr_host.cpp` | 3, 4, 5, 6 | kick, tilt, Point reload, menu input, holds, first-start panel |
| `core/rend/vr/xr_gun.h` | 3 | `RifleTint` (removed if unused) |
| `core/rend/vr/xr_labels.cpp` | 5 | the label set |
| `shell/android-studio/flycast/src/main/java/com/flycast/emulator/CmStartActivity.java`, `CmStartStatus.java` (new), `src/test/java/.../CmStartStatusTest.java` (new) | 6 | the start screen |
| `hotd2-vr/re/make-own-models.py` | 1 | `--namco-zapper` fixture |
| `hotd2-vr/accept.ps1` | 1-7 | scenarios |
| `hotd2-vr/notes/confmiss-findings.md` | 2, 5 | rip timing; cutscene signal |
| `README.md`, `hotd2-vr/notes/confmiss-vr-1.2-headset-checklist.md` (new), `shell/android-studio/flycast/build.gradle.kts:16,52`, `hotd2-vr/package-release.ps1:55` | all, 7 | docs, release |
| local, gitignored: `hotd2-vr/models/guns/`, `build-win/data/models/guns/`, `E:\___flattovrdev\confidential mission\my-models\ready\guns\` | 1 | the Zapper's length, the Arcade gun's name |

## Harness conventions (all tasks)

- **Run a scenario:** `.\hotd2-vr\accept.ps1 -Only cm-gun,cm-menu`. Each prints `PASS <name>` or `FAIL <name>: <why>`. Add `-Reuse` to re-check runs already there without playing them again. A scenario is `function Test-X { ...; return $why }` (empty means pass), listed in the `$scenarios` table at ~`accept.ps1:1688`.
- **Build:** `hotd2-vr\build-win.cmd`, and `hotd2-vr\build-quest.cmd` (with the Quest env) where a task says so.
- **Long runs:** run anything over ~4 minutes in the background, then wait with one blocking `until ...; do sleep 20; done` check per call. Don't poll repeatedly.
- **Simulator actions** go in a run's `At` list as `"<sec>=<action>"`:
  - `xr <hand>/<input> <v> [ms]`;
  - `stick <hand> x y [ms]` (y=1 is up);
  - `pose <head|left|right> x y z yaw pitch [roll]`;
  - `poke <addr> <size> <value>`;
  - `shot <name>`;
  - `-On "regex|delay|action"`.
- **Helpers:**
  - Start presses: `Get-CmStartsXr` (171) presses right A every 2 s from 30 to 52 s; the first room runs 53-63 s.
  - Counting: `Get-LogCount`, `Get-LogTimes` (1239).
  - Rifle: `Get-CmRiflePoke` (1231), `Test-RifleBurst` (1251), `Get-CmRifleSimParams` (1286).
  - Folders: `Invoke-OwnFolders { ... } -Fixture <args> -Bundled <args>` (1386) sets aside the player's, the game-dir and the bundled folders, writes fixtures, and restores them.
  - Image checks: `Invoke-VrCheck differ|same|blank|hues|hnd`.
  - Haptics: `Get-CmHapticsActions` (683).
- **Constants:**
  - `$cmWiden` (160).
  - `$cmWidenArcade` (166): the built-in gun, used 19 times.
  - `$cmRifleWatch` (223).
  - Menu geometry: `$cmMenuPitch`/`$cmMenuTop`/`$cmMenuWidth` (600).
  - `$cmGunData` = `build-win\data`, which holds the local ripped `gun-cm-*.bin` used by the pistol/rifle runs.
- **Simulator logs:**
  - haptics: `XRSIM: haptic <hand> <seconds> <amplitude>`. Shot = `0.030 1.000`, Auto = `0.020 0.600`, empty click = `0.012 0.350`, reload = `0.020 0.500`.
- **RAM:**
  - player p's gun block is `0x0C1557B8 + p*0x114`;
  - +0x48 is the mode (2 = rifle); +0x4C ammo (P1 `0c155804`, P2 `0c155918`); +0x50 magazine (P1 `0c155808`); +0x60 shots.
- **Quick regression** (every CM scenario except the slow rip ones), used by the code tasks:
  `cm-boot,cm-widen,cm-aim,cm-hud,cm-p2,cm-floor,cm-sim,cm-buttons,cm-buttons-pc,cm-pause,cm-haptics,cm-haptics-low,cm-haptics-off,cm-haptics-mag,cm-haptics-reload,cm-menu,cm-menu-quit,cm-controllers,cm-controllers-rt,cm-hands,cm-hands-shot,cm-gun,cm-rifle-sim,cm-rifle-p2,cm-merge-unit,cm-own-gun,cm-own-gun-file,cm-own-controllers,own-folders,cm-bundled,spike-keys`, plus each task's new scenarios, and then `-Only hotd2,mok`.
- **Rip set:** `cm-rip-rifle,cm-rip-start,cm-rip-hand,cm-rip-skip,cm-rip-skip-rifle,cm-rip-pistol,cm-rip-pistol-one`. Before Task 2, `cm-rip-start` is still called `cm-rip-cabinet`.

---

### Task 1: Three guns

**Spec:** §1 (all but the cabinet rip, which is Task 2).

**Files:**
- Modify: `core/rend/vr/xr_hands.h` (the `GunKind` enum, a new `ArcadeGunStem`, comments).
- Modify: `core/rend/vr/xr_hands.cpp`:
  - `gunModel` 622-645;
  - `gunName` 659-677;
  - `chosenGun` 679-754;
  - `gunChoices` 783-801.
- Modify: `core/rend/vr/xr_menu.cpp`: `gunAt()`, and the Gun branch of `value()` and `change()`.
- Modify: `core/rend/vr/xr_own.cpp:341-395` (`scan()`; the sort is at 387); the header comment at `xr_own.h:16-17`.
- Modify: `core/cfg/option.cpp:140` and `option.h:505`. Also add `VrBuiltInGun` under the PC-only `#ifndef __ANDROID__` block, next to `VrXrSim`.
- Modify: `hotd2-vr/re/make-own-models.py` (new `--namco-zapper` option).
- Modify: `hotd2-vr/accept.ps1`:
  - `$cmWidenArcade` 166;
  - `Test-CmGun` 1183;
  - `Test-CmOwnGun` 1458;
  - `Test-CmBundled` 1660;
  - every log regex naming `arcade gun` (21 lines; grep them).
- Modify: `README.md`, "The menu" (137) and "Your own models / Guns" (234).
- Local, not git: the three `namco.json`/`zapper.json` copies (Step 4).

**Interfaces:**
- Produces, in `xr_hands.h`:
  - `enum class GunKind { Arcade = 0, Pistol = 2, Rifle = 3 };`
    - Cabinet is removed. The values are kept, so the profile index `guns[(int)kind - 1]` and `OwnGuns = 4` (file gun i = `GunKind(OwnGuns + i)`) stay valid.
    - `Arcade` is now only the hidden built-in model.
  - `constexpr const char *ArcadeGunStem = "namco";`
- Produces `GunKind chosenGun()`. It returns:
  - `Pistol`;
  - a file gun `GunKind(OwnGuns+i)`;
  - or `Arcade`, the last resort.
- Produces `gunName(kind)`:
  - `Pistol` → "in-game pistol";
  - `Rifle` → "assault rifle";
  - `Arcade` → "built-in gun";
  - a file gun → its sidecar name ("Arcade gun", "Zapper", ...).
- Produces `gunChoices()`. The list is `{1, "In-game pistol", made, complete, nullptr}`, then each usable-or-not folder gun `{OwnGuns, name, usable, true, stem}` in `own::guns()` order. Auto and the built-in are not in it.
- Produces `config::VrBuiltInGun` (`Option<bool>("vr.BuiltInGun", false)`, PC only). It forces `GunKind::Arcade` so harness runs never draw the user's bundled models.
- Produces `vr.Gun`, now defaulting to 1. When read, 0, 2, 3 and negative values count as 1. Nothing is rewritten.
- Produces the log `XR: gun: <name> (chosen)` or `XR: gun: <name> (chosen: <why>)`, from `chosenGun()` through the existing `gunLogged`. The whys are:
  - `the in-game pistol is not made yet`;
  - `the game's models are off` (vr.GameHands=no);
  - `<stem> is not there`;
  - `no gun file loads`;
  - `vr.BuiltInGun`.
- Produces `own::guns()` in a new order:
  - first, every stem that exists in the bundled root (`<data>/models/guns`), in file-name order, even when a player's file of the same stem overrides it;
  - then the rest, in file-name order.
- Produces the fixture `make-own-models.py --namco-zapper`. It writes `namco.glb/.json` (name "Arcade gun", a box-gun variant in one distinct colour) and `zapper.glb/.json` (name "Zapper", `"length": 0.30`, another distinct colour).

**What's there now** (`xr_hands.cpp` `chosenGun`): the Auto logic picks the pistol if complete, else the cabinet, else the arcade gun. The `wanted` mapping is 1 → Pistol, 2 → Cabinet, else Arcade. A missing folder gun logs "<name> is not there" and falls to Auto. `gunAt()` in `xr_menu.cpp` uses `std::min(VrGun, OwnGuns)` and `own::sameStem`, and returns 0 if nothing is found.

**The new choice rule:**
```
setting = vr.Gun; if setting is not 1 or 4 -> 1
PC and vr.BuiltInGun        -> Arcade (why "vr.BuiltInGun")
setting 4: the folder gun whose stem sameStem(vr.GunFile) and usable -> it
           else fall through with why "<GunFile> is not there"
pistol: gunModel(Pistol) loads (vr.GameHands on; an incomplete pistol counts) -> Pistol
else the folder gun with stem ArcadeGunStem, usable -> it (why "the in-game pistol is not made yet" / "the game's models are off", unless a why is already set)
else Arcade (why "no gun file loads")
```

- [ ] **Step 1: Write the failing tests** in `accept.ps1`.
  - **`$cmWidenArcade`** becomes `"$cmWiden,config:vr.GameHands=no,config:vr.BuiltInGun=yes"`. Log regexes for `XR: gun: arcade gun` become `built-in gun`. The rifle's literal "arcade gun, recoloured" stays until Task 3.
  - **`Test-CmGun`** is rewritten. Each run uses `Invoke-OwnFolders -Fixture '--guns','--only-box' -Bundled '--namco-zapper'` and `$cmWiden`.
    - **"list"** (pistol present):
      - expects `XR: gun: in-game pistol \(chosen\)`;
      - at 58 s, open the menu (`xr left/menu 1 150`), go down twice (`stick right 0 -1 150`), then right three times (`stick right 1 0 150` at 59.5/60/60.5);
      - expects `XR: menu Gun -> Arcade gun$`, `XR: menu Gun -> Zapper$`, `XR: menu Gun -> Box gun$` in that order;
      - the `emu.cfg.after` has `vr.Gun = 4`, `vr.GunFile = boxgun`.
    - **"migrate-0", "migrate-2", "migrate-3":** `config:vr.Gun=<n>`; each expects `XR: gun: in-game pistol \(chosen\)`.
    - **"upgrade"** (Review Focus 1): `vr.Gun=4,vr.GunFile=NAMCO`; expects `XR: gun: Arcade gun \(chosen\)`.
    - **"nopistol":** hide the pistol file as today's nopistol run does; `vr.Gun=1`; expects `XR: gun: Arcade gun \(chosen: the in-game pistol is not made yet\)`.
    - **"nofiles":** no pistol, and `-Bundled` with no fixture args (an empty bundled folder); expects `XR: gun: built-in gun \(chosen: no gun file loads\)`.
    - **"builtin":** `$cmWidenArcade`; expects `XR: gun: built-in gun \(chosen: vr\.BuiltInGun\)`.
  - **`Test-CmOwnGun`** uses the new menu sequence (In-game pistol, Arcade gun, Zapper, then the player's guns in name order). The nosidecar run expects `XR: gun: (in-game pistol|Arcade gun) \(chosen: boxgun is not there`.
    - **New "override" run** (Review Focus 2): inside the `Invoke-OwnFolders` body, copy the bundled fixture's `zapper.glb/.json` into the player's `build-win\guns` with `"name": "My Zapper"`.
    - It expects `XR: own gun guns/zapper.glb offered`.
    - The menu steps read Arcade gun → My Zapper → (the player's first gun by name).
  - **`Test-CmBundled`:** keep its assertions. Update only an order assertion that assumed one mixed name sort.
- [ ] **Step 2: Run them and see them fail.** Build (`hotd2-vr\build-win.cmd`), then `.\hotd2-vr\accept.ps1 -Only cm-gun,cm-own-gun,cm-bundled`. They should FAIL on the gun lines.
- [ ] **Step 3: Implement** the interfaces above.
  - **Removing the cabinet** in the C++ outside `hands_rip`/the profile: delete every `GunKind::Cabinet` use (grep `Cabinet` in `core/rend/vr/`). `gunModel` keeps `gameModelSource()->guns[(int)kind - 1]`.
  - **The menu:**
    - `gunAt()` maps settings 0/2/3 to the pistol row.
    - `value()` keeps the suffixes `(not there)`, `(not made yet)`, `(game's models off)`, `(incomplete)`.
    - `change()` keeps skipping choices that aren't made, and saves `vr.Gun` and `vr.GunFile` as now.
    - The menu label is `"In-game pistol"`.
  - **The order** in `xr_own.cpp` `scan()`:
    - while merging by stem, set a `bundledSlot` flag on a `Found` when any root that has that stem is the bundled root;
    - change the sort at 387 to sort `bundledSlot` first, then by `lower(name)`;
    - fix the `xr_own.h` comment ("bundled guns first, in file-name order, then the others").
  - **The fixture:** add `--namco-zapper` to `make-own-models.py`. Follow how `--guns` writes a box gun and its sidecar; print the same "muzzle"/"textures" lines the harness parses.
- [ ] **Step 4: Update the local model copies** (gitignored; never `git add` them).
  - **Files:** in each of `E:\___flattovrdev\hotd2-vr\hotd2-vr\models\guns`, `E:\___flattovrdev\hotd2-vr\build-win\data\models\guns` and `E:\___flattovrdev\confidential mission\my-models\ready\guns`:
    - `zapper.json` `"length"`: 0.25 → 0.30;
    - `namco.json` `"name"`: "Namco gun" → "Arcade gun".
  - **How:** use one `python -I` script that loads, edits and dumps each file, keeping its key order and indent. Then check `git check-ignore hotd2-vr/models/guns/zapper.json`.
  - **CREDITS:** `hotd2-vr/models/CREDITS.txt` keeps the real title and author; don't edit it.
- [ ] **Step 5: Verify.**
  - `hotd2-vr\build-win.cmd`.
  - In the background: the quick regression, then `-Only hotd2,mok`. All pass.
  - `hotd2-vr\build-quest.cmd` compiles: the new option is PC-only, and `chosenGun` must guard its use with `#ifndef __ANDROID__`.
  - Don't run the rip set; Task 2 owns it. Until then, `cm-rip-hand` still passes `vr.Gun=2`, which now means the pistol.
- [ ] **Step 6: README.** Update:
  - "The menu": the Gun row lists In-game pistol, Arcade gun, Zapper, then your own guns; no Auto; the fallback chain.
  - "Your own models / Guns": bundled guns first; a file of the same name replaces a bundled one in its place.
- [ ] **Step 7: Commit.**
  ```
  git add core/rend/vr/xr_hands.h core/rend/vr/xr_hands.cpp core/rend/vr/xr_menu.cpp core/rend/vr/xr_own.h core/rend/vr/xr_own.cpp core/cfg/option.h core/cfg/option.cpp hotd2-vr/re/make-own-models.py hotd2-vr/accept.ps1 README.md
  git commit -m "hotd2-vr: CM 1.2 three guns (in-game pistol, Arcade gun, Zapper)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

**Checklist items:**
- The Gun row order and names.
- The Zapper is visibly bigger (0.30 m).
- A 1.1 install with Auto, cabinet or arcade saved comes up with the in-game pistol.
- With the first start skipped, the Arcade gun is in hand.

---

### Task 2: The first-start rip without the cabinet, and how fast it can be

**Spec:** §1 "The cabinet rip"; §6 "check whether that step can run faster without losing quality; report what you find either way".

**What the code says** (and the plan's answer to the spec's "if it is tied in, keep the shared part and say so"): the cabinet **is tied into** the pistol rip.
- In `hands_rip.cpp`, the CM run presses Start at `CmStartAt` (30 s).
- The cabinet is captured in the game's reload tutorial. `CmGiveUpAfter` (120 s) saves its best non-merged capture.
- The merged pistol part starts when the others are done or at `CmMergedBy` (90 s), at line 380:
  ```cpp
  if (cmMergedAt < 0.0 && merging && t >= CmStartAt && (othersDone || t >= CmMergedBy))
  ```
  So the cabinet's capture is what starts the pistol's Start presses: every 2 s for `CmPressFor` (16 s), merging after `CmQuietFor` (20 s) or at `CmLastAt` (150 s).
- **Kept (shared):** the Start at 30 s and the wait for the tutorial.
- **Removed:** the cabinet's target, its capture, its W band (`skipWMin/Max`, used only by the cabinet), and its give-up timer if nothing else uses it.
- **Changed:** the pistol part starts a fixed time after the Start.

**Files:**
- Modify: `core/rend/vr/hands_rip.{h,cpp}`:
  - constants 67-82;
  - `haveModel` 130-140;
  - init 215-231;
  - the run loop 300-420;
  - `Source::modelFile()` (grep it), which returns `guns[0].file` for CM and must return `guns[1].file`.
- Modify: `core/rend/vr_reproject.cpp:507-528`. `guns[0]` (the `"cabinet gun"`, `gun-cm-cabinet.bin`, `{0x400000}`, 279, 200, 0.27f, ... target) becomes `{}`, so `used()` is false. Keep the slot so `guns[(int)kind-1]` still lines up.
- Modify, only if the code is cabinet-only: `core/rend/vr/hands_build.cpp:141`, the plain-texture vertex-colour path whose comment names the cabinet. If the pistol or rifle can reach it, keep it and fix the comment.
- Modify: `hotd2-vr/accept.ps1`:
  - `$cmCabinetFile`, `Remove-CmGuns` 957;
  - `Test-CmRipCabinet` 981 → `Test-CmRipStart`, and the table row `cm-rip-cabinet` → `cm-rip-start`;
  - `Test-CmRipHand` 1012;
  - `Test-CmRipSkip` 1041;
  - `Test-CmRipSkipRifle` 1068;
  - `Invoke-CmRipPistol` 1095;
  - `Test-CmRipPistol` 1113, `Test-CmRipPistolOne` 1119;
  - `Test-CmRipRifle` 1139;
  - the scenario list in the file's header comment.
- Modify: `hotd2-vr/notes/confmiss-findings.md` `## Guns` (268): the 1.2 timing (game seconds per phase; no game text).
- Modify: `README.md` "The game" (166), the first-start paragraph.

**Interfaces:**
- Produces: CM's `gameModelSource()->guns[0]` is empty. `haveModel` (all used targets present) is true with just the pistol and rifle files, so a 1.1 install with both files skips the first start. An old `gun-cm-cabinet.bin` is left on disk and never read.
- Produces: `CmPistolAfter`, a new constant in seconds after `CmStartAt`. The merged pistol part starts at `t >= CmStartAt + CmPistolAfter` (and `othersDone` for the rifle, as now). `CmMergedBy` and the cabinet branch go.
- Consumes: Task 1's `GunKind` (no Cabinet).

- [ ] **Step 1: Baseline.** Before editing, find the newest local run folders of `cm-rip-pistol` and `cm-rip-rifle` (under the harness's run root; `accept.ps1 -Reuse` reads them).
  - If none exist, run `.\hotd2-vr\accept.ps1 -Only cm-rip-pistol,cm-rip-rifle` once on the unchanged code, in the background.
  - From the logs, record the game-second times of: the Start, the cabinet's saved line, the first pistol press, the merge, done, and for the rifle run the rifle capture.
  - `CmPistolAfter` = (cabinet saved − Start), rounded up to the next 2 s Start beat.
- [ ] **Step 2: Write the failing tests.**
  - **`Test-CmRipStart`** replaces `Test-CmRipCabinet`, using the same flat run and HandsPrep. It expects:
    - the pistol's saved line (the one `Invoke-CmRipPistol` expects);
    - no line matching `cabinet`;
    - no `gun-cm-cabinet.bin` in the run's data dir;
    - the first pistol press at `CmStartAt + CmPistolAfter` ±2 s;
    - done at or under the new budget (the measured done + 20 s);
    - the `.part` check moved to `gun-cm-pistol.bin`;
    - `Invoke-VrCheck hnd` on the pistol file with `Invoke-CmRipPistol`'s parameters.
  - **`Test-CmRipHand`:** `vr.Gun=1` (the ripped pistol) against `$cmWidenArcade` (the built-in); `differ` stays.
  - **`Test-CmRipSkip`, `-SkipRifle`, `Invoke-CmRipPistol`, `Test-CmRipRifle`:** drop the cabinet expectations and the cabinet hnd check; reject `the cabinet gun from this game`. Tighten the done budgets (150 → the measured value + 20 s; rifle 460 → the measured value + 20 s) once Step 4's run gives the new numbers.
  - **`Remove-CmGuns`:** stops listing the cabinet file.
- [ ] **Step 3: Run and see them fail:** `-Only cm-rip-start` fails on the cabinet line and the press time.
- [ ] **Step 4: Implement** the interfaces. Then run the rip set in the background, plus `cm-gun,cm-hands,cm-merge-unit` and `-Only hotd2,mok` (HOTD2's hands and MOK's staff share `hands_rip`). All pass.
- [ ] **Step 5: The speed investigation** (report only, unless it is free).
  - From the new runs, write in the findings `## Guns` a short table of each phase's game seconds and wall seconds:
    - the attract wait for the rifle (about 316 s of game time today, the main cost);
    - Start to the first press;
    - presses;
    - quiet wait;
    - merge.
  - Try at most one cheap change if one stands out (e.g. a shorter `CmQuietFor`). Keep it only if `cm-rip-pistol` and `cm-rip-pistol-one` still pass with the same poly counts, completeness and hnd checks; otherwise revert it.
  - Put the result in the report either way.
- [ ] **Step 6: README** "The game": the first start makes the in-game pistol (and the rifle); give the new typical time.
- [ ] **Step 7: Commit.**
  ```
  git add core/rend/vr/hands_rip.h core/rend/vr/hands_rip.cpp core/rend/vr_reproject.cpp hotd2-vr/accept.ps1 hotd2-vr/notes/confmiss-findings.md README.md
  ```
  Also add `core/rend/vr/hands_build.{h,cpp}` if they changed.
  ```
  git commit -m "hotd2-vr: CM 1.2 first start without the cabinet gun" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

**Checklist items:** the first start on a fresh install takes about the new time and ends with the pistol in hand; the skip still works.

---

### Task 3: Kick only on a fired round, and the rifle with file guns

**Spec:** §2, §3.

**The cause** (§3 asks for it): `xr_host.cpp` `updateGun`, ~1586-1593, starts the kick and the flash on every on-screen trigger pull, whether or not the game fires a round:
```cpp
if (trigger && !g.triggerWas) g.burstFlashes = 0;
if (trigger && !g.triggerWas && onScreen) { g.lastShot = time; g.shotCount++; g.autoKick = false; g.burstFlashes = 1; }
```
The haptics already come from the game's own shot counter. In `gunKnocks` (1444-1515), each fired round is a Shot or an Auto knock, and each pull with no round is a click, spaced by `knockEnds`. The Auto branch already sets `g.lastShot=time; g.shotCount++; g.autoKick=true; g.burstFlashes++`.

**Files:**
- Modify: `core/rend/vr/xr_host.cpp`:
  - the pull kick, ~1586-1593;
  - `gunKnocks` 1444-1515 (the Shot branch);
  - the rifle tint draw at 2977.
- Modify: `core/rend/vr/xr_hands.cpp` `handGun` 756-772.
- Modify: `core/rend/vr/xr_gun.h:40`: remove `RifleTint` if nothing else uses it.
- Modify: `hotd2-vr/accept.ps1`:
  - new `Test-CmKick`, `Test-CmRifleFile`;
  - `Test-CmHaptics` 696, `Test-CmHapticsOff` 721;
  - `Test-CmRifleSim` 1295 (the arcade variant);
  - table rows.
- Modify: `README.md` "Controls" (85) and the rifle paragraph in "The game".

**Interfaces:**
- Consumes (Task 1): `chosenGun()`, `gunName(kind)`, `GunKind`, `gunModel(GunKind::Rifle)`; the `--namco-zapper` fixture; `$cmWidenArcade` with `vr.BuiltInGun`.
- Produces: **with a gun-RAM profile (CM), a kick happens only when `gunKnocks` delivers a round.**
  - A Shot sets a full kick: `autoKick=false`, `lastShot`, `shotCount++`, `burstFlashes++`.
  - An Auto works as now.
  - This happens whatever `haptic()` returns (Haptics Off/Low still kick).
  - The pull itself never kicks.
- Produces: profiles without gun RAM (HOTD2, MOK) keep the v1 pull kick (`!g.gunRam`).
- Produces: the off-screen reload shot fires no round, so it gets no kick (it already gets no shot haptic).
- Produces: the log `XR: P%d kick: %s` (gunName of the hand's gun) for each Shot kick. Auto kicks are counted in the existing `XR: P%d rifle burst: %u flashes` line.
- Produces the new `handGun(player)` rule:
  - while `rifleHeld[player]`: return `GunKind::Rifle` only when `chosenGun() == GunKind::Pistol` and the rifle model loads;
  - otherwise return the held gun (`chosenGun()`).
  - Logs:
    - pistol to rifle: keep the 1.1 `XR: P%d hand: assault rifle (<file>)`;
    - other guns: `XR: P%d hand: rifle rounds with the %s` on the pickup and `XR: P%d hand: rifle rounds over (the %s)` when it ends;
    - back from the rifle: `XR: P%d hand: back to the %s`, as now.
  - The tinted built-in rifle ("arcade gun, recoloured") goes: with the pistol and no rifle file, the pistol stays.
- Produces: the per-round set needs no new code. `gunKnocks` picks Shot/Auto from `now.rifle || gunWas.rifle` whatever the model is, and the flash is drawn at the held model's muzzle.

- [ ] **Step 1: Write the failing tests.**
  - **`Test-CmKick`**, new. It does one run per gun: pistol (`vr.Gun=1`, pistol file present), Arcade gun (`4`/`namco`), Zapper (`4`/`zapper`), box gun (`4`/`boxgun`), and built-in (`$cmWidenArcade`). The file guns use `Invoke-OwnFolders -Fixture '--guns','--only-box' -Bundled '--namco-zapper'`.
    - Each run's `At` is `Get-CmStartsXr` plus:
      - `53.5=poke 0c155804 1 0`, `54=xr right/trigger 1 150`;
      - `55=poke 0c155804 1 6`, `55.5=xr right/trigger 1 150`, `56.5=xr right/trigger 1 150`.
    - For 54-55 s it asserts 0 `XR: P1 kick` lines and 1 `XRSIM: haptic right 0.012 0.350`.
    - For 55.5-57.5 s it asserts 2 `XR: P1 kick: <name>` lines (names: `in-game pistol`, `Arcade gun`, `Zapper`, `Box gun`, `built-in gun`) and 2 Shot haptics.
    - The box-gun run also has:
      - an empty rifle: `Get-CmRiflePoke` at 58, then `59=poke 0c155804 1 0`, `59.5=xr right/trigger 1 600`. It asserts no kick line, no `rifle burst` line, and at least one click.
      - an off-screen reload pull at 61 s with the pose `Test-CmHapticsReload` uses. It asserts `shot off screen \(reload\)` and no kick line between 61 and 62.5 s.
    - See `Test-CmHapticsMag` (728) for poking the magazine.
  - **`Test-CmRifleFile`**, new.
    - Zapper fixture, `vr.Gun=4,vr.GunFile=zapper`, `$cmWiden` + `$cmRifleWatch`, the rifle poked and the trigger held as `Get-CmRifleSimParams` does. It expects:
      - `XR: P1 hand: rifle rounds with the Zapper`;
      - no `XR: P1 hand: assault rifle`;
      - `Test-RifleBurst` passes;
      - exactly 1 `XR: P1 kick: Zapper` per burst;
      - after the pickup, the muzzle line names the Zapper (`XR: P1 muzzle: Zapper`);
      - `rifle rounds over` once the mode ends.
    - P2 run, as `Test-CmRifleP2` does it, with `vr.GunFile=namco`: `XR: P2 hand: rifle rounds with the Arcade gun`, and `Test-RifleBurst` on P2.
  - **`Test-CmHaptics`:** the count of `XR: P\d kick:` lines equals the count of Shot haptic lines.
  - **`Test-CmHapticsOff`** (Review Focus 3): the same kick count as `cm-haptics`, with no haptic lines.
  - **`Test-CmRifleSim`:** the arcade variant now expects `XR: P1 hand: rifle rounds with the built-in gun` instead of `arcade gun, recoloured`. The main variant is unchanged (`assault rifle \(gun-cm-rifle\.bin\)`).
- [ ] **Step 2: Run and see them fail:** build, then `-Only cm-kick,cm-rifle-file`. `cm-kick` fails on the empty pull's kick, and `cm-rifle-file` on the hand line.
- [ ] **Step 3: Implement** the interfaces. In `updateGun`, the pull kick line becomes `... && onScreen && !g.gunRam`. `burstFlashes = 0` on a new pull stays.
- [ ] **Step 4: Verify:** build; then in the background the quick regression with `cm-kick,cm-rifle-file`, then `-Only hotd2,mok` (the v1 pull kick). All pass.
- [ ] **Step 5: README:**
  - "Controls": the gun kicks only when the game fires.
  - The rifle paragraph: the in-game pistol becomes the in-game rifle; other guns stay in hand and fire in bursts with recoil and haptics.
- [ ] **Step 6: Commit.**
  ```
  git add core/rend/vr/xr_host.cpp core/rend/vr/xr_hands.cpp core/rend/vr/xr_gun.h hotd2-vr/accept.ps1 README.md
  git commit -m "hotd2-vr: CM 1.2 kick only on a fired round; the rifle with file guns" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

**Checklist items:**
- An empty pull gives no kick and no haptic, for each gun.
- The rifle pickup with the Zapper and the Arcade gun: the model stays, with burst recoil and haptics.
- With the pistol, the in-game rifle appears.

---

### Task 4: The Gun angle and Reload rows

**Spec:** §4, §5.

**Files:**
- Modify: `core/rend/vr/xr_menu.h`: `enum Row` and `struct Input`.
- Modify: `core/rend/vr/xr_menu.cpp`:
  - `Labels[Rows]` (31);
  - `stepper()`, `onOff()`, `value()`, `panel()`, `change()`, `update()`;
  - a new `repeats(row)`.
- Modify: `core/rend/vr/xr_host.cpp`:
  - the `Gun` struct ~150-180 (add `bool offArmed = false;`);
  - `updateOtherHand` 1340-1432 (`restGun = pose*placement`);
  - `updateGun` 1557-1660: the placement, muzzle log, ray, `view.pose`, the pointed-down reload at 1640-1653, and the `if (!live)` return at 1655;
  - `updateMenu` 1825-1922 (the stick edges, `stickWas`).
- Modify: `core/cfg/option.cpp` (next to `VrGun` at 140) and `option.h`.
- Modify: `hotd2-vr/accept.ps1`:
  - `$cmMenuTop` 600;
  - `Test-CmMenu` 619;
  - `Test-CmHapticsReload` 736;
  - new `Test-CmGunAngle`, `Test-CmReloadPoint`;
  - table rows.
- Modify: `README.md`, "The menu" (137) and "Options" (319).

**Interfaces:**
- Consumes: Task 3's `updateGun` (the kick now comes from `gunKnocks`); Task 1's gun names.
- Produces the rows:
  - `enum Row { Resume, Recenter, Gun, GunAngle, Reload, Laser, Haptics, WorldSize, PanelDistance, WiderView, HowToPlay, Quit, Rows };` (12 rows);
  - labels "Gun angle" and "Reload".
- Produces `config::VrGunAngle`: `Option<int>("vr.GunAngle", 0)`, degrees.
  - `clamp(-30, 30)` everywhere it is read.
  - The stepper adds ±1 and saves through `config::saveInt("config","vr.GunAngle",v)`.
  - No log line and no save when a step is clamped and the value doesn't change.
- Produces `config::VrReload`: `Option<int>("vr.Reload", 0)`. 0 is "Shoot off-screen", 1 is "Point off-screen". A stepper with 2 values, saved with `saveInt`.
- Produces `value(GunAngle)`: "+10", "0", "-5" (no degree sign). `panel()` appends the Latin-1 byte `"\xB0"` for this row, because panel text is drawn byte by byte as Latin-1. The log stays ASCII: `XR: menu Gun angle -> +10`, `XR: menu Reload -> Point off-screen`.
- Produces `menu::Input::repeat` (bool). In `updateMenu`, a stick held left or right on the same push for 0.4 s or more emits `left`/`right` again every 0.1 s, with `repeat = true`. `menu::update` acts on a repeat only where `repeats(row)` is true: `GunAngle`, `WorldSize`, `PanelDistance`. The Gun row doesn't repeat (each step loads a model).
- Produces **the tilt**: `glm::mat4 tilt = translate(palm) * rotate(radians(angle), vec3(1,0,0)) * translate(-palm)`. `palm` is the grip point `updateGun` passes to `gunPlacement` (`HandPalm` or the runtime value). A positive angle raises the muzzle. Use `tilt` everywhere `placement` meets the pose:
  - `o = pose*tilt*placement*muzzle`;
  - `d = rot*mat3(tilt)*(0,0,-1)`;
  - `view.pose = pose*tilt*recoilMat*placement`;
  - the muzzle log's point = `tilt*placement*muzzle`, logged again whenever the angle changes;
  - `updateOtherHand`'s `restGun = pose*tilt*placement`.
  
  The menu's rays come from `restMuzzle/restForward`, so they follow. The how-to-play controllers don't use it. The rifle and both hands get it.
- Produces **Point off-screen**: each `Gun` has its own `offArmed`.
  - In `updateGun`, before the `if (!live)` return: `g.offArmed = false` when `!live`. Then the menu, or a menu close while the gun points away, never reloads.
  - After `onScreen`/`aimOutside` are known, when `VrReload == 1 && holds(g.hand) == Holds::Gun`:
    - `onScreen` → `offArmed = true`;
    - otherwise, if `offArmed && !aimOutside` and (no gun RAM, or `gunKnown && ammo < magazine`) → `offArmed = false; g.reloadAt = time; haptic(g.hand, 0.02f, 0.5f);` and log `XR: player %d pointed off screen (reload)`.
  - In Point mode, the pointed-down reload (1640-1653) is skipped, so there is one reload per exit.
  - B/Y and the slide are untouched. There is no kick.

- [ ] **Step 1: Write the failing tests.**
  - **`Test-CmMenu`:**
    - 6 downs to Haptics (was 4);
    - the pointer test aims at the Laser row as Row 5 (was 3);
    - the open line becomes `XR: menu at 1\.20 m, 12 rows, row pitch 0\.063 m, top row \+0\.337 m`;
    - `$cmMenuTop` = 0.337 (the 10-row panel grows by 2 × 0.063, so the top row rises by 0.063; confirm from the log).
    - `cm-menu-quit` wraps up to Quit and needs no change.
  - **`Test-CmGunAngle`**, new. Box fixture (`-Fixture '--guns','--only-box'`), `vr.Gun=4,vr.GunFile=boxgun`, `$cmWiden`.
    - **Runs "0" and "20"** (`config:vr.GunAngle=20`): read the palm from the existing palm log line and the muzzle point from `XR: P1 muzzle: Box gun at x y z`. Assert:
      - x is equal within 0.001;
      - the distance from the palm is equal within 0.001;
      - the angle from (m0 − palm) to (m20 − palm) in the y-z plane is +20° ± 0.5°, with y higher.
      
      The same shot pose at 54 s lands higher on screen in run "20" (compare the `shot at` y).
    - **Run "menu":**
      - at 58 s, open the menu and go down 3 to Gun angle;
      - `stick right 1 0 150` gives exactly one `XR: menu Gun angle -> \+1`;
      - `stick right 1 0 1500` gives 9-14 more lines, each +1;
      - `stick right -1 0 150` gives one line, -1;
      - close the menu; the muzzle line is logged again;
      - `emu.cfg.after` has `vr.GunAngle` equal to the last logged value.
    - **Run "clamp"** (Review Focus 5): `config:vr.GunAngle=90`. The muzzle angle matches +30. Open the menu, go to Gun angle, and hold right for 1000 ms: no `Gun angle ->` line.
  - **`Test-CmReloadPoint`**, new. `$cmWidenArcade,config:vr.Reload=1`, plus `config:vr.WatchRam=0c155804,0c155808`.
    - **The sequence:**
      - shots at 54, 54.5, 55 (ammo 6 → 3);
      - 56 `pose right` away from the screen and its margin (yaw 90; check the shot log calls it off screen) → 1 `XR: player 1 pointed off screen \(reload\)`, 1 `XRSIM: haptic right 0.020 0.500`, ammo back to the magazine in the WatchRam log;
      - stay away until 58 → no second line;
      - 58 back on screen, 58.5 a shot, 59 away → a second line;
      - 60 back, a shot, then 61 open the menu, 61.5 away, 62.5 close it while away → no line;
      - 63.5 back, 64 away with the magazine full → no line;
      - 65 back, a shot, 65.5 a pose into the grey margin (`aimOutside`, as `cm-widen` aims there) → no line;
      - 66.5 back, 67 a shot, 67.5 `xr right/b 1 150` → the ammo reloads (B still works).
    - **P2:** X at 69, a left shot, then left away → `XR: player 2 pointed off screen \(reload\)` and P2 ammo `0c155918` back up.
  - **`Test-CmHapticsReload`** (Shoot mode): reject `pointed off screen`.
- [ ] **Step 2: Run and see them fail:** build, then `-Only cm-menu,cm-gun-angle,cm-reload-point`.
- [ ] **Step 3: Implement** the interfaces.
- [ ] **Step 4: Verify:**
  - build;
  - in the background, the quick regression with `cm-kick,cm-rifle-file,cm-gun-angle,cm-reload-point` (`cm-pause`'s 23 counted lines must not change), then `-Only hotd2,mok`;
  - `hotd2-vr\build-quest.cmd` compiles.
- [ ] **Step 5: README:**
  - "The menu": the two rows, the hold-repeat, ±30° in 1° steps.
  - "Options": `vr.GunAngle`, `vr.Reload`.
- [ ] **Step 6: Commit.**
  ```
  git add core/rend/vr/xr_menu.h core/rend/vr/xr_menu.cpp core/rend/vr/xr_host.cpp core/cfg/option.h core/cfg/option.cpp hotd2-vr/accept.ps1 README.md
  git commit -m "hotd2-vr: CM 1.2 Gun angle and Reload menu rows" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

**Checklist items:**
- Gun angle in 1° steps; holding repeats; ±30; the laser and shots follow the barrel; both hands; the rifle too.
- Reload in Point mode: pointing away reloads once with a buzz; nothing happens in the menu; B/Y still work.

---

### Task 5: Controllers during cutscenes, and the new labels

**Spec:** §7, §8.

**The left trigger, confirmed in code** (§8 asks; the report must say this):
- In `xr_host.cpp` `updateLightgun` (~2095-2170), while there's one gun (`!dualWield`), the gun moves to whichever hand pulls its trigger. **Before player 2 joins, the left trigger fires for player 1.**
- After X (`vr.DualWield`), `guns[1] = Gun{1, Left}`, and the left trigger is player 2's.
- The labels use the user's text anyway.

**Files:**
- Modify: `core/rend/vr/xr_labels.cpp`:
  - the Quest Touch set, 43-46;
  - the PC sets, 48-59;
  - the built-in anchor for a left Trigger, mirroring the right's;
  - a log near 264.
- Modify: `core/rend/vr_reproject.{h,cpp}`: the signal and `bool cutscene()`, next to `player2Prompting()` (1571), which is the pattern for a steady, time-based signal. The letterbox count is at `hideLetterbox` 1466-1482, called at 2043-2056; the CM profile has `letterboxW` 0.2 and `letterboxTextures[0]` 0x400680 at 443-444.
- Modify: `core/rend/vr/xr_host.cpp`:
  - `holds()` 2640-2647;
  - the per-frame hold check near 2701 (`heldWas`) and `noteHolds(why)` 252;
  - the laser and dot draw, if they don't already follow `holds()`.
- Modify: `hotd2-vr/accept.ps1`:
  - `Get-CmStartsXr` (171) gets an `-Until` parameter (default 52);
  - new `Test-CmCutscene`;
  - `Test-CmControllers` 773 (the label-set lines);
  - table row.
- Modify: `hotd2-vr/notes/confmiss-findings.md`: a new `## Cutscenes` section (addresses, method, timings; no game text).
- Modify: `README.md` "Controls" (85): the label list.

**Interfaces:**
- Produces: the Quest Touch labels, exactly as the user wrote them:
  - right: `A - P1 Start`, `B - P1 Reload`, `Trigger - P1 Shoot`, `Stick - Laser`;
  - left: `X - P2 Start`, `Y - P2 Reload`, `Trigger - P2 Shoot` (new), `Stick - Recenter`, `Menu` (unchanged).
  - The PC sets (Index, Vive, WMR) keep their own button names with the same meanings and the same "<button> - <meaning>" form.
- Produces: the log `XR: labels set: <hand>: <label>, <label>, ...` once per hand per session, at the first label draw.
- Produces: `bool vr::cutscene()` in `vr_reproject.h`. It is true while the game shows a non-interactive scene during a game in progress.
  - Debounce: it turns on after the signal holds for N frames (about 0.3 s; tune from the research), and off after at most 3 frames without it.
  - It is false outside a game (attract, title).
- Produces: `holds(hand)` returns `Holds::Controller` for both hands while `p1Armed && vr::cutscene()`; the rest is unchanged. `labelsOn()` is unchanged (`menuHowToPlay || !p1Armed`), so there are no labels in cutscenes.
- Produces: the hold-change log comes through `noteHolds`: `XR: <hand> hand: gun -> controller (cutscene)` and `... controller -> gun (play)`.
- Notes:
  - Shots are not gated by `holds()`. Its callers are 257, 1630 visible, 1633 slide, 1642 pointed-down, 2701 and 3008 draw. So a trigger or Start can still skip a scene.
  - Point reload (Task 4) needs `Holds::Gun`, so it pauses during cutscenes.
  - P1 arms on a pull or right Start (2149-2163). `Get-CmStartsXr`'s first press at 30 s arms P1 before the stage 1 intro (~46 s).

- [ ] **Step 1 (labels): Write the failing test.**
  - `Test-CmControllers` expects exactly:
    - `XR: labels set: right: A - P1 Start, B - P1 Reload, Trigger - P1 Shoot, Stick - Laser`;
    - `XR: labels set: left: X - P2 Start, Y - P2 Reload, Trigger - P2 Shoot, Stick - Recenter, Menu`;
    - plus a `XR: labels: left trigger at` position line.
  - Build and run `-Only cm-controllers`: it fails.
- [ ] **Step 2 (labels): Implement, then pass** `-Only cm-controllers,cm-controllers-rt,cm-own-controllers`.
  - The label panel width follows the longer text; check the `shot` in `cm-controllers` with `python ~/.claude/tools/shot.py fit` so the labels don't overlap.
  - Commit:
    ```
    git add core/rend/vr/xr_labels.cpp hotd2-vr/accept.ps1
    git commit -m "hotd2-vr: CM 1.2 how-to-play labels" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
    ```
- [ ] **Step 3 (cutscenes): Research the signal** (about 25 calls at most).
  - **The run:** `Get-CmStartsXr -Until 44`, so the stage 1 intro plays out, with a temporary PC trace of each candidate per frame (remove it before committing). Collect the windows where each candidate is on, from 40 to 120 s.
  - **Candidates:**
    - the letterbox bar draws per pass, already counted in `hideLetterbox`;
    - the HUD's ammo/reticle draws being absent;
    - a game-state byte near the player blocks (`0x0C15A494` player bits; `vr.WatchRam`).
  - **Pick the one that is:**
    - on for the whole intro and any later scene in the window;
    - off in every room where shots count (53-63 s, and the floor's 52-72 s shot run);
    - off within 3 frames when a Start press skips the scene.
  - **Record it** in the findings `## Cutscenes`: the addresses or texture, the method, and the on/off times.
  - **If no candidate meets all three,** write down what was tried, skip Steps 4-6, still do Step 7's README, and report it. Don't ship a heuristic.
- [ ] **Step 4: Write the failing test `Test-CmCutscene`.**
  - **Run "intro":** `Get-CmStartsXr -Until 44`. Assert:
    - exactly one `XR: right hand: gun -> controller \(cutscene\)` inside the intro window from Step 3;
    - one `controller -> gun \(play\)` before the first room;
    - no hold changes between the first room's start and 63 s;
    - no `XR: labels on` after 30 s.
  - **Run "skip":** the same, plus a right A press 1 s into the intro. Assert `controller -> gun \(play\)` within 1.0 s of the press.
  - **Run "fight":** the floor's shot run (52-72 s, a shot every 0.4 s). Assert no `\(cutscene\)` line in 52-72 s.
  - **Run "p2"**, only if Step 3 found a later scene within ~150 s: X before it. Assert both hands flip to controller and back.
- [ ] **Step 5: Implement** `cutscene()` and the `holds()` rule. Hide the laser and dot while the hand holds a controller, as in the how-to-play phase.
- [ ] **Step 6: Verify:**
  - build;
  - in the background, the quick regression with `cm-kick,cm-rifle-file,cm-gun-angle,cm-reload-point,cm-cutscene` (`cm-pause`, `cm-hands`, `cm-floor` and `cm-p2` especially), then `-Only hotd2,mok`. The signal must be CM-only; other profiles have no cutscene source and `cutscene()` returns false.
- [ ] **Step 7: README** "Controls": the label list (the same text as the table above), and one line on cutscenes. **Commit:**
  ```
  git add core/rend/vr_reproject.h core/rend/vr_reproject.cpp core/rend/vr/xr_host.cpp hotd2-vr/accept.ps1 hotd2-vr/notes/confmiss-findings.md README.md
  git commit -m "hotd2-vr: CM 1.2 controllers during cutscenes" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

**Checklist items:**
- The labels read exactly as the table, and the left trigger label exists.
- Before P2 joins, the left trigger shoots for P1 (expected).
- The stage 1 intro shows controllers, with guns back the moment play starts.
- A Start skip brings the guns back at once.
- No flicker in fights.

---

### Task 6: The VR4All start screen and the first-start wording

**Spec:** §6.

**Read first:**
- VR4All, read-only:
  - `C:\Users\ege19\OneDrive\Documents\ChatGPT\vr mod\public\brand.css`, `styles.css`, `themes.css`;
  - `vr4all-final-preview.png` (view it shrunk: `python ~/.claude/tools/shot.py fit <png>`).
  - Only colour values and layout ideas come over.
- `CmStartActivity.java` (264 lines; read it whole) and `CmFiles.java`.

**Files:**
- Modify: `shell/android-studio/flycast/src/main/java/com/flycast/emulator/CmStartActivity.java`.
- Create: `shell/android-studio/flycast/src/main/java/com/flycast/emulator/CmStartStatus.java`.
- Create: `shell/android-studio/flycast/src/test/java/com/flycast/emulator/CmStartStatusTest.java`.
- Modify: `core/rend/vr/xr_host.cpp`, the CM first-start panel ~2788-2830 (today: the title, "Taking the guns from your copy of the game", "About a minute or two, only this once", footer "B: skip (the built-in gun this time)"; done: "Got it. The gun from your game is in your hand." / "Reload: B, or point it away from the screen and pull.").
- Modify: `hotd2-vr/accept.ps1`: `Test-CmRipSkip` 1041 and `Invoke-CmRipPistol` 1095 (panel lines).
- Modify: `README.md`: the start-screen paragraph.

**Interfaces:**
- Consumes: `CmFiles.find(roots)` → `CmFiles.Result` (zip, disc, bios, and `missing` lines that start with the file name; constants `ZIP="confmiss.zip"`, `DISC="confmiss/gds-0001.chd"`, `BIOS_ZIP="naomi.zip"`); `canReadSharedStorage()`; Task 4's `config::VrReload`; Task 1's fallback (the Arcade gun).
- Produces `final class CmStartStatus` (pure Java, no Android imports):
  - `static CmStartStatus from(CmFiles.Result r, boolean canRead)`, where `r` is null before the first check;
  - `final boolean ready`: true only with all three files found and `canRead`;
  - `final String badge`: `"READY"` when ready, `"ALLOW ACCESS"` when `!canRead`, `"CHECKING"` when `r == null`, else `"MISSING FILES"`;
  - `final List<String> fileMarks`: in order zip, bios, disc, each `"<name> ✓"` or `"<name> missing"`. The disc shows as `CHD`.
  - `final List<String> missing`: one line per missing file, `"<file> missing"`, plus the folder it goes in, taken from `CmFiles`' `missing` line.
- Produces the colour tokens as constants in `CmStartActivity`:
  - deck `#0b0d12`, panel `#151923`, line `#343947`, muted `#9ba5b4`, cream `#f8f5ef`, lime `#d9f36c`, violet `#a99bff`.
  - The old RED/TEXT/SOFT/GOOD/BACK constants go.
- Produces the headset panel text (our own words):
  - while the guns are made: `Making your guns from your game files (once)`. The time line stays, updated to Task 2's measured time if the findings give a new figure.
  - skip footer: `B: skip (the Arcade gun this time)`.
  - done reload line: with `vr.Reload` 0, `Reload: B, or point it away from the screen and pull.`; with 1, `Reload: B, or point it away from the screen.`
  - New log on each panel change: `XR: first start: <text lines under the title, joined with " / "> | <footer>`.

- [ ] **Step 1: Write the failing JUnit tests** in `CmStartStatusTest`:
  - `allFoundWithAccessIsReady` (badge `READY`, no missing, 3 ✓ marks);
  - `missingBiosNamesIt` (badge `MISSING FILES`, `missing` has a line starting `naomi.zip missing`, the mark `naomi.zip missing`);
  - `missingDiscAndZip` (two lines, in order zip then disc);
  - `noAccessIsNotReady` (`ALLOW ACCESS`, `ready` false even with files);
  - `beforeFirstCheck` (`CHECKING`, not ready).
  
  Build `CmFiles.Result` the way `CmFilesTest` does.
  - Run: `cd shell\android-studio && gradlew.bat :flycast:testVrUnitTest --tests com.flycast.emulator.CmStartStatusTest`. Expected: FAIL (class missing).
- [ ] **Step 2: Implement `CmStartStatus`,** then pass `--tests com.flycast.emulator.CmStartStatusTest` and `--tests com.flycast.emulator.CmFilesTest` (18 tests).
- [ ] **Step 3: Restyle `CmStartActivity`** to the spec's layout sketch. Build everything in code, as now.
  - **The page:** deck background, with a soft radial glow via a `GradientDrawable` (RADIAL) if it's cheap.
  - **Top bar:** a slim lime-to-violet accent bar (a `GradientDrawable` LEFT_RIGHT), a small violet rounded tile with "CM", the name in cream, and the version `v<major.minor>` in muted text from `BuildConfig.VERSION_NAME` (it shows 1.1 until Task 7).
  - **Headline:** "Your mission." in cream and "Inside the headset." in violet.
  - **Panel** (panel colour, a 16 dp radius, a 1 dp `line` stroke):
    - a muted "ARCADE · RAIL SHOOTER" label and the status badge (lime `READY`, otherwise violet text);
    - the `fileMarks` row;
    - the `missing` lines;
    - Play: lime with deck-coloured text, disabled and dimmed until `ready`.
  - **Under the panel:** "First start: makes your guns from your own game files (once)."
  - **"Where to put your files ›"** expands today's instructions and the access step (the Allow button) and the "Look again" button.
  - **Text:** system sans-serif, medium/bold.
  - **Keep:** `askForAccess()`, `play()`, `refresh()` (now through `CmStartStatus`), and the footer "Unofficial fan project, not affiliated with Sega."
  - **No game art or game text.**
- [ ] **Step 4: Panel wording** in `xr_host.cpp`, and the log. The tests:
  - `Test-CmRipSkip` expects `XR: first start: Making your guns from your game files \(once\)( / .*)? \| B: skip \(the Arcade gun this time\)`;
  - `Invoke-CmRipPistol` expects `XR: first start: Got it\..* / Reload: B, or point it away from the screen and pull\.` (`vr.Reload` 0, the default).
  
  Build and run `-Only cm-rip-skip,cm-rip-pistol` in the background.
- [ ] **Step 5: Verify.**
  - `hotd2-vr\build-quest.cmd` (Java and native) passes.
  - Both JUnit classes pass.
  - The step-4 scenarios pass.
  - The screen can't be screenshotted on the PC. The checklist item covers it at the headset round.
- [ ] **Step 6: README:** the start screen (the badges, Play enabled when ready, where the files go). **Commit:**
  ```
  git add shell/android-studio/flycast/src/main/java/com/flycast/emulator/CmStartActivity.java shell/android-studio/flycast/src/main/java/com/flycast/emulator/CmStartStatus.java shell/android-studio/flycast/src/test/java/com/flycast/emulator/CmStartStatusTest.java core/rend/vr/xr_host.cpp hotd2-vr/accept.ps1 README.md
  git commit -m "hotd2-vr: CM 1.2 VR4All-style start screen and first-start wording" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

**Checklist items:**
- The start screen's look.
- A badge per missing file, and READY in lime.
- Play disabled until ready.
- "Where to put your files" opens the instructions.
- The access step works.
- The first-start panel wording.

---

### Task 7: Release 1.2.0

**Spec:** "Done means".

**Files:**
- Modify: `shell/android-studio/flycast/build.gradle.kts:16` (`val hubVersion = "1.1.0"` → `"1.2.0"`) and `:52` (`versionCode = 8` → `9`).
- Modify: `hotd2-vr/package-release.ps1:55` (`$appVersion = '1.1.0'` → `'1.2.0'`).
- Create: `hotd2-vr/notes/confmiss-vr-1.2-headset-checklist.md` (start from the 1.1 checklist's form).
- Modify: `README.md` (a final pass; "Releasing" 415 names 1.2.0); `hotd2-vr/accept.ps1` only if a scenario needs fixing.

- [ ] **Step 1: Run everything** in the background, with blocking waits.
  - **The CM list:** 31 quick + 7 rip + 5 new (`cm-kick`, `cm-rifle-file`, `cm-gun-angle`, `cm-reload-point`, `cm-cutscene`) = 43.
  - **Then** `-Only hotd2,mok`.
  - **On failure:** fix it in the owning code with its own commit, or report it with the run folder and the log line.
- [ ] **Step 2: Spec coverage.** Walk spec §1-§8. Each line points to a passing scenario or a checklist item.
- [ ] **Step 3: The checklist,** ordered for one session on the Quest 3. Each item has a checkbox, what to do, and what should happen:
  1. install over 1.1 (settings kept; a saved Auto/cabinet/arcade comes up as the in-game pistol);
  2. the start screen;
  3. the first start (wording, time);
  4. the labels;
  5. the guns (order, names, Zapper size);
  6. Gun angle;
  7. Reload modes;
  8. no kick when empty;
  9. the rifle with each gun;
  10. cutscenes;
  11. player 2 (the left trigger before and after X);
  12. 72 Hz;
  13. the PC VR zip.
  
  Use each task's "Checklist items".
- [ ] **Step 4: Release build.**
  - `hotd2-vr\build-win.cmd`, `hotd2-vr\build-quest.cmd`.
  - Then `hotd2-vr\package-release.ps1` **without** `-SkipBuild`. It signs with the user's key; never print the key or its password, and never touch `C:\Users\ege19\.vrlightgunhub`.
  - Every check passes: versionName `1.2.0-g<HEAD>`, versionCode 9, the CREDITS `File:` lines, the bundled models from `hotd2-vr/models/` (the edited Zapper and Arcade gun sidecars), and no banned files.
  - Record the dist paths and the versionName in the checklist header. Do not install.
- [ ] **Step 5: Commit.**
  ```
  git add shell/android-studio/flycast/build.gradle.kts hotd2-vr/package-release.ps1 hotd2-vr/notes/confmiss-vr-1.2-headset-checklist.md README.md
  ```
  Also add `hotd2-vr/accept.ps1` if it changed.
  ```
  git commit -m "hotd2-vr: CM VR 1.2.0 release and headset checklist" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

## Conflict scan

**Shared files and interfaces, pair by pair.** The tasks run in order, so a "risk" is a later task breaking an earlier one's contract.

| Pair | Shared | Contract that must hold |
|---|---|---|
| T1 / T2 | `GunKind` (`xr_hands.h`); `accept.ps1` rip helpers | T1 drops `Cabinet` but keeps the values (Pistol=2, Rifle=3), so `guns[(int)kind-1]` stays valid after T2 empties `guns[0]`. T1 doesn't run the rip set, and T2 updates `cm-rip-hand`. |
| T1 / T3 | `chosenGun()`, `gunName()`, `handGun()` (`xr_hands.cpp`); the fixture; `$cmWidenArcade` | T3's rifle rule keys on `chosenGun() == GunKind::Pistol`, and its log names come from T1's `gunName`. `cm-kick` relies on the fixture names "Arcade gun"/"Zapper" and on `vr.BuiltInGun`. |
| T1 / T4 | `xr_menu.{h,cpp}`; `option.{h,cpp}` | T4 inserts rows after `Gun`. The Gun row stays index 2, so T1's "down twice" menu steps still reach it. The options sit side by side: `VrBuiltInGun` (PC-only block) and `VrGunAngle`/`VrReload` (all platforms). |
| T1 / T6 | the fallback gun | T6's skip footer says "the Arcade gun", which is true only with T1's fallback. |
| T2 / T6 | first-start time; `cm-rip-skip`, `cm-rip-pistol` | T6 uses T2's measured time for the panel's time line, and adds panel assertions to the rip scenarios that T2 rewrote. |
| T3 / T4 | `updateGun` (`xr_host.cpp` 1557-1660); the `Gun` struct | T4 adds `tilt` to the lines T3 left. The kick stays in `gunKnocks` (T3), and `recoilMat` sits between `tilt` and `placement`. T4's Point reload sets no kick. |
| T3 / T5 | `xr_host.cpp` | Different functions (`gunKnocks`/the pull vs `holds()`). The cutscene state doesn't gate `gunKnocks`, so rounds fired to skip a scene still knock. |
| T4 / T5 | `holds()` consumer; `updateMenu`; row indices | T4's Point reload needs `Holds::Gun`, so it pauses during T5's cutscenes. T5's tests open nothing through the menu; if they do, use T4's 12-row indices. |
| T4 / T6 | `config::VrReload` | T6's done line reads it (0/1) with T4's meaning. |
| T5 / T6 | `xr_host.cpp` (holds ~2640 vs the panel ~2790) | Adjacent regions only; no shared names. |
| T1-T6 / T7 | `README.md`, `accept.ps1`, the checklist | T7 runs the 43-scenario list. Each task's README section stays; T7 only reconciles. |

**Self-consistency, one row per task:**

| Task | Must agree with itself |
|---|---|
| T1 | `gunChoices()`, `gunAt()` and `chosenGun()` read 0/2/3 the same way (the pistol). The menu's suffixes match `chosenGun`'s whys. The sort puts a player's override in the bundled slot both in `own::guns()` and in the menu. |
| T2 | `haveModel`, `modelFile()` (→ `guns[1]`) and the empty `guns[0]` agree. `CmPistolAfter` matches the Step 1 measurement. The scenario budgets match the new runs. |
| T3 | Kick lines = Shot knocks (+ Auto in flashes) with Haptics On, Low or Off. No kick on a click or an off-screen reload. `handGun`'s logs pair up: pickup and over. |
| T4 | `tilt` is used at all five places. `repeats()` covers only GunAngle, WorldSize and PanelDistance. The clamp applies on read and on step. Point mode disarms in the menu and skips the pointed-down reload. |
| T5 | `holds()` drives both the drawing and `noteHolds`. `labelsOn()` is unchanged. The README label list is byte-for-byte the `xr_labels.cpp` Quest set. |
| T6 | The `CmStartStatus` badge strings are the only source the Activity shows. The panel wording matches the start screen's "(once)". The done line follows `vr.Reload`. |
| T7 | `1.2.0` / `9` agree in gradle, `package-release.ps1`, the checklist header and the README. |

## Spec coverage

| Spec | Task |
|---|---|
| §1 Three guns (list, order, Auto gone, old choices, fallback, Zapper 0.30, "Arcade gun") | 1 |
| §1 The cabinet rip (tied in: shared Start and tutorial kept) | 2 |
| §2 The rifle with a file gun | 3 |
| §3 No kick when empty (cause, per gun, off-screen reload) | 3 |
| §4 Reload | 4 |
| §5 Gun angle (1° steps, repeat, ±30, tilt at the grip, ray follows) | 4 |
| §6 Start screen, wording | 6 |
| §6 Rip speed | 2 |
| §7 Cutscenes | 5 |
| §8 Labels, the left-trigger check | 5 |
| Done means | 7 |
