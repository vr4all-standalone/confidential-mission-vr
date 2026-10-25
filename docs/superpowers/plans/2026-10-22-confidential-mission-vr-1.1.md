# Confidential Mission VR 1.1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn v1 (Confidential Mission in "VR Light Gun Hub" 0.1.0) into the user's standalone app **Confidential Mission VR** 1.1. It adds:
- the user's button layout;
- an in-headset settings menu that pauses the game;
- Quest controllers with labels before play;
- the game's own guns, taken on the first start;
- haptics driven by the game;
- the assault rifle pickup.

**Architecture:** This is the same Flycast fork and per-game profile approach as v1.
- **The simulator (Task 1):** an in-process OpenXR simulator (`vr.XrSim`, PC only). The whole headset path then runs in the PC harness: frame loop, actions, haptics and render models. Every XR feature gets a scripted test, and the real runtime path stays as it is.
- **New XR features** are small new files under `core/rend/vr/` (xr_sim, xr_menu, xr_controllers, xr_labels, glb_mesh), hooked into `xr_host.cpp`.
- **Game facts** (the gun RAM, the rifle, the gun textures) are `GameProfile` fields in `vr_reproject.cpp`.
- **The guns** are taken by the existing hands pipeline (`hands_rip`, `hands_build`, `xr_hands`), extended with a gun-only kind.
- **The start screen** is a new Java activity in the existing `:setup` process.

**Tech Stack:** C++17 (Flycast fork), OpenXR 1.1.43 (`XR_FB_render_model`), cgltf 1.14 (MIT, single header, vendored), Gradle Kotlin DSL + Java (JUnit 4, already a `testImplementation`), Windows PowerShell 5.1, Python 3.13 (numpy, pillow).

**Spec:** `docs/superpowers/specs/2026-10-22-confidential-mission-vr-1.1-design.md` (binding).

**Research:** read the sections each task names; this plan does not repeat them.
- `.superpowers/sdd/2026-10-21-confidential-mission-vr/v11-research-code.md` ("code Qn" below).
- `.superpowers/sdd/2026-10-21-confidential-mission-vr/v11-research-game.md` ("game An/Bn").
- `cm-vr/notes/confmiss-findings.md` ("findings").

## Global Constraints

From the brief, verbatim:
- No game data anywhere (repo, APK, zip, reports).
- Never touch `C:\Users\ege19\.vrlightgunhub` or print the key password.
- Never stage `core/deps/gamesdk`.
- Commit messages start `hotd2-vr:` and end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Every CM accept scenario keeps passing.
- Quest 72 Hz (verts p95 ≤ 13904).
- No headset installs by task agents.
- The user is never asked anything mid-run.

Carried over from v1 and the spec:
- **Repo:** `E:\___flattovrdev\hotd2-vr`, branch `naomi-gun-vr` (HEAD f34dd76df when this plan was written).
- **What counts as game data.** Findings, profile comments and commit text may hold addresses, numeric values and texture IDs, nothing more.
  - Rips, RAM dumps, screenshots and ripped models (`gun-cm-*.bin`) stay under `build-win\` (git-ignored) or the session scratchpad.
  - Ripped models are made on the player's device and never shipped.
  - Game files are read in place through `$env:VRLGH_GAMES` and `$env:VRLGH_HOTD2`, never hard-coded and never copied.
- **App identity:**
  - name **Confidential Mission VR**;
  - application ID **`com.vrlightgunhub.confmiss`**;
  - versionName **`1.1.0-g<hash>`**;
  - dist files **`confidential-mission-vr-quest.apk`** and **`confidential-mission-vr-pcvr.zip`**.
- **No other mod's names.** Nothing a player sees names HOTD2, MOK, mikermak's mods or "VR Light Gun Hub". Keep the line "Unofficial fan project, not affiliated with Sega."
- **CM's game ID** is `" CONFIDENTIAL MISSION ---------"`, and its profile log line is `VR: camera profile for  CONFIDENTIAL MISSION ---------` (two spaces).
- **Per-game numbers live only in CM's `GameProfile`** (`core/rend/vr_reproject.cpp` ~415-484), each with a comment saying what it is and how it was found. Generic code stays generic.
  - Leave the HOTD2/MOK profiles, their rip runs, `buildStaff` and `vr.StaffHand` in place, unreachable from the app. This keeps upstream merges small.
- **hotd2/mok accept scenarios stay as engine regression checks.** `.\cm-vr\accept.ps1 -Only hotd2,mok` must pass after every task that touches C++. No task here is expected to make them impossible. If one does, it says so in its report and its commit message, and names the scenario.
- **CM regression after every task that touches C++:** `.\cm-vr\accept.ps1 -Only cm-boot,cm-widen,cm-aim,cm-hud,cm-p2,cm-floor` passes. So do the 1.1 scenarios added so far; `accept.ps1` with no `-Only` runs everything, so run it in the background.
- **PC build:** `cm-vr\build-win.cmd` → `build-win\flycast.exe` (~2 min).
- **Quest build:** used only where a task says so. Set `JAVA_HOME=E:\UnityEd\6000.0.78f1\Editor\Data\PlaybackEngines\AndroidPlayer\OpenJDK` and `ANDROID_HOME=E:\AndroidSdk`, then run `cm-vr\build-quest.cmd` → `shell\android-studio\flycast\build\outputs\apk\vr\flycast-vr.apk`. Signing goes through the existing Gradle `hub` config; never open or print its properties file.
- **This PC has no headset.** `-Xr` falls back to the flat window (`XR_ERROR_FORM_FACTOR_UNAVAILABLE`, findings `## Acceptance`).
  - Everything XR is tested through the simulator (Task 1, `-XrSim`).
  - What only a headset can show goes into `cm-vr/notes/confmiss-vr-1.1-headset-checklist.md`: Task 1 creates it, each task appends its own `## Task N` items, and Task 15 orders it.
- **PC-only debug input.** `vr.XrSim`, `vr.XrSim*`, `vr.Watch*` and the UDP lines `XR`, `XRPOSE`, `XRSTATE` and `POKE` exist only on PC (`#ifndef __ANDROID__`). They do nothing unless set.
- **Simulator runs prepare guns only on request.** Under the simulator the first-start gun run (hands prep) runs only with `vr.HandsPrep=yes`, as on the flat PC; the headset always runs it.
  - Any CM run may still *watch* for a gun in play and save `build-win\data\gun-cm-*.bin`.
  - So a scenario that compares hand pixels or names a gun sets up those files itself: it removes them, or it requires them.
- **Run hygiene.**
  - Harness runs write under `build-win\vrtest\<Name>\`.
  - From Task 1 on, `vr-test.ps1` restores `build-win\emu.cfg` after every run and keeps the run's own copy as `emu.cfg.after`.
  - Runs over ~4 min go in the background, with one blocking `until` wait (CLAUDE.md).
- **Settings** are saved through `config::saveBool`, `saveInt` and `saveFloat` (`core/cfg/cfg.h`) to `emu.cfg` section `config`. That section is global, which is fine because the app is CM only.
- **No googletest** (as in v1). C++ is checked by harness log and screenshot assertions, Python by `vrcheck selftest`, and Java's `CmFiles` by a JUnit 4 test.
- **One task = one fresh Opus subagent, under ~100 tool calls.** Spike and reverse-engineering steps name a stop point; at it, the agent commits what passes and hands back what it found so far.

## Review Focus

These are the five things a player of this app is most likely to hit that the spec doesn't spell out:

1. **≡ pressed at an awkward moment:** on a 2D screen, in the continue countdown, right after player 2 joins, twice quickly, or while the guns are being made. The player expects the pause to hold, the headset never to go blank, and closing to resume exactly where it was. While the guns are being made, ≡ does nothing. Owners: Task 3 (cm-pause: 20 quick cycles plus an open on the 2D gun adjustment screen) and Task 10 (≡ during the gun run).
2. **The headset comes off, or the Quest menu opens, while the settings menu is open or the guns are being made.** Back in the headset, the game is still paused with the menu, and the gun run carries on. Owners: Task 3 (the simulator's focus loss and return keep the pause; the `JNIdc_resume` guard) and the headset checklist.
3. **Game files in the wrong place or form:**
   - `confmiss.zip` without its CHD folder;
   - the CHD loose in Download;
   - no `naomi.zip`, but the BIOS inside `confmiss.zip`;
   - files in a subfolder of Download;
   - an upper-case `Confmiss.zip`.
   
   The start screen names exactly what's missing and where it goes, or plays. Owner: Task 5 (the CmFilesTest cases).
4. **The gun run interrupted or poor:** B pressed, the app killed and a `.part` file left behind, or a pistol capture that fails the check. The player gets the cabinet gun or the arcade gun, never a broken or inside-out model, and the run repeats only when its result is missing. Owners: Task 10 (skip, `.part`, `vrcheck hnd`) and Task 11 (complete or incomplete flag, cabinet fallback).
5. **Haptics on the edges:** off-screen reload pulls, an empty gun, player 2, and strength "off". The expected result:
   - no shot knock on a reload pull;
   - a light click on an empty gun;
   - P2's knocks in the left hand;
   - nothing at all on "off", menu clicks included.
   
   Owner: Task 6 (cm-haptics runs).

## Order

The order puts risk first, and deviates from the brief's suggested order for these reasons:
- **The simulator (Task 1) and the button layout (Task 2) come first** because every XR task's PC test needs them.
- **Pause (Task 3)** is the riskiest code change.
- **The rifle spike (Task 4)** is the biggest unknown, so it comes before the low-risk app work (Task 5).
- **Task 4 could run alongside Tasks 1-3.** It changes only harness and debug code and notes, so it can run in its own worktree. Its merge conflicts would be limited to `vr-test.ps1`'s action switch, `accept.ps1`, the parser in `vrcheck.py`, and `option.{h,cpp}`.
- **The gun tasks (10-14) come late:** they build on the simulator, the menu (the Gun row) and the haptics.

## File map

| File | Tasks | Responsibility |
|---|---|---|
| `core/rend/vr/xr_sim.{h,cpp}` (new) | 1, 3, 8 | simulated OpenXR runtime (PC): frames to the window, actions from UDP, haptics and render models logged |
| `core/rend/vr/xr_host.{h,cpp}` | 1-3, 6-10, 13, 14 | dispatch table, bindings, pause, menu, haptics, controllers, hand states, gun per hand |
| `core/emulator.cpp:1078-1104`, `shell/.../jni/src/Android.cpp:321-330` | 3 | paused render path; resume guard |
| `core/rend/vr/xr_panel.{h,cpp}` | 7, 9 | row panels with a pose and hit test; small text textures |
| `core/rend/vr/xr_menu.{h,cpp}` (new) | 7 | the settings menu |
| `core/deps/cgltf/` (new), `core/rend/vr/glb_mesh.{h,cpp}` (new) | 8 | GLB loading |
| `core/rend/vr/xr_controllers.{h,cpp}`, `controller_model.h` (new), `cm-vr/controller_model/make_controller.py` (new) | 8 | runtime or built-in controller models |
| `core/rend/vr/xr_labels.{h,cpp}` (new) | 9 | button labels |
| `core/rend/vr/hands_rip.{h,cpp}`, `hands_build.{h,cpp}`, `xr_hands.{h,cpp}`, `xr_gun.{h,cpp}` | 10-14 | CM gun runs, gun-only build, HND1 v2, several models, tint |
| `core/rend/vr_reproject.{h,cpp}` | 4, 6, 10-14 | RAM and texture watches, pokes, gun RAM, CM profile guns |
| `core/cfg/option.{h,cpp}`, `core/input/udp_lightgun.cpp` | 1, 4, 6-8 | new options; UDP debug lines |
| `shell/android-studio/flycast/...` (Java, manifest, gradle, icons) | 5, 7, 8 | standalone app, quit to start, render-model permission |
| `cm-vr/vr-test.ps1`, `accept.ps1`, `re/vrcheck.py`, `re/make-test-glb.py` (new) | 1-15 | harness |
| `cm-vr/package-release.ps1`, `assets/make-icon.py`, `assets/Play Confidential Mission.cmd` (new), `README.md`, `.github/ISSUE_TEMPLATE/bug_report.yml`, `.gitignore` | 5, 8, 15 | release |
| `cm-vr/notes/confmiss-findings.md`, `notes/confmiss-vr-1.1-headset-checklist.md` (new) | 1-15 | evidence; the user's Quest checklist |

## Harness conventions (all tasks)

- **Simulator input** (vr-test actions from Task 1):
  - `xr <hand>/<input> <value> [ms]`. Inputs `a b x y menu thumbstick` map to `/user/hand/<hand>/input/<input>/click`, and `trigger squeeze` to `.../value`. An input that contains `/` is used verbatim under `/user/hand/<hand>/input/`, e.g. `xr left/trackpad/force 1 300`.
  - `stick <hand> <x> <y> [ms]` and `pose <head|left|right> <x> <y> <z> <yawDeg> <pitchDeg> [rollDeg]`, in metres in the simulator's stage space.
  - `xrstate <focused|visible>`.
- **Accept helpers:**
  - `Get-CmStartsXr` (Task 1) is `xr right/a 1 200` every 2 s from 30 to 52 s: v1's `Get-CmStarts` through the controller.
  - `Get-LogCount <run> <regex>` (Task 2).
- **Scene keys** (Task 4): `-On "<regex>|<delay>|<action>[|<times>]"`, `poke <addr> <size> <value>`, `vr.WatchRam`, `vr.WatchTex`.
- **Stage 1 timing** with `Get-CmStartsXr` is the same as with v1's starts: the first room runs 53-63 s (findings Timeline, "timing behind the gun adjustment screen").

---

### Task 1: OpenXR simulator for the PC harness

**Read:**
- code Q2, "Pointing, clicking and pausing";
- in `xr_host.cpp`: 453-591 (init), 593-647 (pollEvents), 1821-1825 (`enabled`), 1874-2127 (`frame`);
- `mainui.cpp` 105-125;
- `udp_lightgun.cpp` 60-80.

**Files:**
- Create: `core/rend/vr/xr_sim.h`, `core/rend/vr/xr_sim.cpp`, `cm-vr/notes/confmiss-vr-1.1-headset-checklist.md`
- Modify:
  - `core/rend/vr/xr_host.cpp`: every `xr*(` call goes through the table;
  - `core/cfg/option.{h,cpp}`: the `vr.*` block, 80-129;
  - `core/ui/mainui.cpp:113`;
  - `core/input/udp_lightgun.cpp:69`, `handle()`;
  - `CMakeLists.txt:1281-1284` and `:1475-1478`, the xr source lists;
  - `cm-vr/vr-test.ps1`, `cm-vr/re/vrcheck.py`, `cm-vr/accept.ps1`.

**Interfaces:**
- Produces, in options:
  - `Option<bool> VrXrSim("vr.XrSim", false)`: on PC with `vr.Xr`, run the headset path against the built-in simulator.
  - `Option<std::string> VrXrSimProfile("vr.XrSimProfile", "oculus/touch_controller")`: the simulated controllers, one of `valve/index_controller`, `htc/vive_controller` or `microsoft/motion_controller`.
- Produces, in `xr_sim.h`:
  ```cpp
  namespace vr::xr::sim {
  bool active();                               // vr.XrSim on a PC build (always false on Android)
  PFN_xrVoidFunction proc(const char *name);   // the simulator's function, nullptr if it has none
  void udpLine(const char *line);              // "XR <path> <value>", "XRPOSE <who> x y z yaw pitch [roll]", "XRSTATE <focused|visible>"
  }
  ```
- Produces, in `xr_host.cpp`: an internal table of `PFN_xr*` pointers, filled by name at init. With `sim::active()` it comes from `sim::proc`; otherwise from the loader's `xrGetInstanceProcAddr`, with `XR_NULL_HANDLE` for the functions called before an instance exists. Calls go through a macro `XR(fn)`.
- **Simulator facts that later tests rely on:**
  - **Runtime name:** "Flycast XR simulator".
  - **Interaction profile:** `/interaction_profiles/` + `vr.XrSimProfile`.
  - **Eyes:** 832x720 each; field of view ±52° across and ±48° up and down, the size of a Quest 3 eye (findings: `vr.WindowTanX=1.2799,vr.WindowTanY=1.1106`); 64 mm apart.
  - **Default poses:** the head at (0, 1.6, 0) looking −Z. The right hand at (0.18, 1.3, −0.35) and the left at (−0.18, 1.3, −0.35), with aim and grip poses both pointing −Z.
  - **Pacing:** `xrWaitFrame` holds the loop to 72 Hz.
  - **Display:** `xrEndFrame` blits the left eye's projection-layer image to the window, so window shots show the headset view.
  - **Extensions listed:** `XR_KHR_opengl_enable` and `XR_FB_display_refresh_rate`.
  - **Session states:** READY, then SYNCHRONIZED, VISIBLE and FOCUSED after `xrBeginSession`. `XRSTATE visible` drops to VISIBLE (focus lost); `XRSTATE focused` returns.
  - **Actions:** the simulator answers from the bindings suggested for its current profile, with path values fed over UDP. A boolean is on at value ≥ 0.5, and every action is active.
- **Log lines:**
  - `XRSIM: runtime up for "<applicationName>" (832x720 per eye, 72 Hz, /interaction_profiles/<profile>)` (NOTICE).
  - `XRSIM: haptic <left|right> <seconds %.3f> <amplitude %.3f>` (NOTICE), once per `xrApplyHapticFeedback`.
  - `XRSIM: input <path> <value>` (INFO).
- **vr-test.ps1:**
  - a `-XrSim` switch, which adds `config:vr.Xr=yes,config:vr.XrSim=yes`;
  - the actions `xr`, `stick`, `pose` and `xrstate` (see Harness conventions), sent as UDP lines to `vr.UdpPort`;
  - before the start, `build-win\emu.cfg` is copied to `<out>\emu.cfg.before`. In `finally`, the run's file is copied to `<out>\emu.cfg.after` and `.before` is put back.
- **vrcheck.py:**
  - `same <a> <b> [--box x0,y0,x1,y1] [--max-diff 2.0]`;
  - `differ <a> <b> [--box ...] [--min-diff 6.0]`.
  
  The box is given in fractions of the image. The measure is the mean absolute difference per channel (0-255) over the box. Exit 0 means pass.
- **accept.ps1:** `Get-CmStartsXr`, and the scenario `cm-sim`.

**Steps:**

- [ ] **Step 1: Write the failing vrcheck tests.** Add `selftest` cases:
  - two identical 64x64 images: `same` passes and `differ` fails;
  - the same pair with a 10%-wide box brightened by 40: `differ --box` over that box passes, and `same --box` over the rest passes.
  
  Run `python cm-vr\re\vrcheck.py selftest`. Expected: FAIL (no `same` subcommand).
- [ ] **Step 2: Implement `same` and `differ`.** `selftest` passes.
- [ ] **Step 3: Write the failing scenario `cm-sim`.** It is one vr-test run:
  - `Game confmiss`, `-XrSim`, `Config $cmWiden`, `Seconds 64`;
  - `At = Get-CmStartsXr + '55=shot a','58=shot b','58.5=pose head 0 1.6 0 30 0','60=shot c','61=pose head 0 1.6 0 0 0','62=shot d'`.
  
  Checks:
  - Expect `XRSIM: runtime up for ".*" \(832x720 per eye, 72 Hz, /interaction_profiles/oculus/touch_controller\)`, `XR: session created`, `XR: eye buffers 832x720` and `VR: camera profile for  CONFIDENTIAL MISSION ---------$`.
  - Reject `XR: initialisation failed`.
  - `vrcheck blank a b c d --region full --max 0.5`: no shot is mostly black.
  - `vrcheck differ b c`: the head turned, so the view changed.
  - `vrcheck differ a b`: the game runs.
  
  Run `.\cm-vr\accept.ps1 -Only cm-sim`. Expected: FAIL (`vr.XrSim` is unknown, so XR init fails and the flat window shows).
- [ ] **Step 4: Implement the simulator and the pieces around it.**
  - **The dispatch table and the simulator.** Handles are small integers. Swapchain images are GL_RGBA8 textures the simulator makes on the current context. Action spaces locate to their hand's pose, and `xrLocateViews` places the eyes at the head pose ± 32 mm.
  - **The option.**
  - **The window.** In `mainui.cpp:113`, the window is presented when `vr::xr::sim::active()`.
  - **UDP routing.** Lines starting `XR` go to `sim::udpLine`.
  - **vr-test:** the new actions and the `emu.cfg` restore.
- [ ] **Step 5: Verify.**
  - Run `cm-vr\build-win.cmd`.
  - `accept.ps1 -Only cm-sim` passes.
  - The CM regression and `-Only hotd2,mok` pass, since the simulator is off by default.
- [ ] **Step 6: Create the headset checklist** with a header saying how the user reports results, plus `## Task 1`: "PC VR (Link/SteamVR) still uses the real runtime: the log says `XR: runtime <name>`, never `XRSIM`."
- [ ] **Step 7: Commit.**
  ```bash
  git add core/rend/vr/xr_sim.h core/rend/vr/xr_sim.cpp core/rend/vr/xr_host.cpp core/cfg/option.h core/cfg/option.cpp core/ui/mainui.cpp core/input/udp_lightgun.cpp CMakeLists.txt cm-vr/vr-test.ps1 cm-vr/re/vrcheck.py cm-vr/accept.ps1 cm-vr/notes/confmiss-vr-1.1-headset-checklist.md
  git commit -m "hotd2-vr: OpenXR simulator for the PC harness (vr.XrSim), vrcheck same/differ" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

**Stop point:** at ~85 calls. If frames render but input doesn't, commit what passes `cm-sim` and hand back the list of functions still missing.

### Task 2: Button layout

**Read:** code Q1 (the whole section, including "The new layout"); `xr_host.cpp` 207-352 (initActions) and 1337-1489 (updateLightgun).

**Files:**
- Modify:
  - `core/rend/vr/xr_host.cpp`: 207-352 for the actions and bindings; 1370-1489 for recenter, the ring dump, the join and the routing; and a new `XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED` case in pollEvents (593-647);
  - `README.md`, the controls section at :108-137;
  - `cm-vr/accept.ps1`;
  - the checklist.

**Interfaces:**
- **Consumes:** Task 1's `xr`/`stick` actions and `vr.XrSimProfile`.
- **Produces:**
  - A new boolean action `menu`. Each frame, `updateLightgun` computes a `menuEdge`; for now it logs `XR: menu button`, and Task 3 replaces that log with the pause.
  - The interaction profile in use, read with `xrGetCurrentInteractionProfile` on the change event and logged as `XR: controllers: <profile path>`. Tasks 8 and 9 use it through `const char *controllerProfile()` in `xr_host.h`.
  - The log line `XR: recenter (button)` on the recenter edge.
  - The `skip` action keeps its name, because the gun run's B-skip reads it (xr_host:1372-1384); only its bindings change, and it means "reload".
- **Bindings:**

  | Profile | start | skip (reload) | menu | recenter | laser |
  |---|---|---|---|---|---|
  | `oculus/touch_controller` | right `a/click`, left `x/click` | right `b/click`, left `y/click` | left `menu/click` | left `thumbstick/click` | right `thumbstick/click` |
  | `valve/index_controller` | `a/click` both | `b/click` both | left `trackpad/force` | left `thumbstick/click` | right `thumbstick/click` |
  | `htc/vive_controller` | `menu/click` both | `trackpad/click` both | hold left start ≥ 0.8 s | none | none |
  | `microsoft/motion_controller` | `menu/click` both | `trackpad/click` both | hold left start ≥ 0.8 s | left `thumbstick/click` | right `thumbstick/click` |

  Trigger, squeeze, the poses, the stick and the haptics stay as in v1. Where the menu is a hold, the left Start acts on release, and only if it was held less than 0.8 s.
- **Routing** (replaces xr_host 1477-1484):
  - **Dual:** P1 = right start·4 | right skip·128, and P2 = left start·4 | left skip·128.
  - **Single:** P1 = right start·4 | (right or left skip)·128. The left start joins P2 when `canJoin`, and otherwise does nothing (v1's fallback of left Start as P1 Start goes).
  - Recenter is the left stick click only. The ring dump moves to holding the left stick click for 1.5 s.
  - B stays bit 128, CM's own reload (R12).

**Steps:**

- [ ] **Step 1: Write the failing scenarios.**
  - **Helper** `Get-LogCount([string]$run, [string]$regex)`: the number of `flycast.log` lines that match.
  - **`cm-buttons`** (`-XrSim`, `$cmWiden`, `Seconds 72`). At:
    - `Get-CmStartsXr`;
    - `'56=xr right/b 1 200'`, `'60=xr left/x 1 200'`, `'62=xr left/y 1 200'`, `'64=xr left/menu 1 200'`, `'66=xr left/thumbstick 1 200'`, `'68=xr right/thumbstick 1 200'`.
    
    Expect:
    - `Light gun P1 at .*start 1`;
    - `Light gun P1 at .* B 1`;
    - `XR: player 2 joins: a gun in each hand`;
    - `Light gun P2 at .* B 1`;
    - `XR: menu button`;
    - `XR: recenter \(button\)`;
    - `XR: laser off`;
    - `XR: controllers: /interaction_profiles/oculus/touch_controller`.
  - **`cm-buttons-pc`:** two short runs (`Seconds 40`, no starts needed).
    - With `config:vr.XrSimProfile=valve/index_controller` and `'35=xr left/trackpad/force 1 300'`: Expect `XR: controllers: /interaction_profiles/valve/index_controller`, and `Get-LogCount` of `XR: menu button` is 1.
    - With `htc/vive_controller`, `'33=xr left/menu 1 200'` (a short press) and `'36=xr left/menu 1 1200'` (a hold): `Get-LogCount 'XR: menu button'` is exactly 1.
  
  Run `accept.ps1 -Only cm-buttons,cm-buttons-pc`. Expected: FAIL (no `menu` action; X still recenters).
- [ ] **Step 2: Implement** the bindings, the hold-for-menu rule, the routing, the profile event, and the log lines.
- [ ] **Step 3: Verify.** Build. Both scenarios pass, as do `cm-sim`, the CM regression and `-Only hotd2,mok`.
- [ ] **Step 4: Update the README controls section.**
  - The Quest table from spec item 5.
  - The other profiles from the bindings table.
  - Reload by pointing away from the game's view or at the floor, as in v1.
  
  Add a checklist item: "each Quest button does what the README table says; B's reload sounds like the game's own".
- [ ] **Step 5: Commit** `xr_host.cpp`, `README.md`, `accept.ps1` and the checklist, with the message "hotd2-vr: the 1.1 button layout (X joins, Y reloads P2, menu button), PC profiles".

### Task 3: Pause, with the paused frame redrawn in VR

**Read:**
- code Q2, "Pointing, clicking and pausing";
- in `emulator.cpp`: 806-845 (`stop`), 997 (`start`), 1052 (`checkStatus`), 1078-1104 (`render`);
- `mainui.cpp` 40-125;
- `core/ui/gui.cpp` 408-430 (`gui_open_settings` stops the emulator from the render thread: copy that pattern);
- `Android.cpp` 298-330;
- in `xr_host.cpp`: 1874-2127, where `frame()` calls `rend_vr_drain` and then draws each eye;
- `gles.cpp` 1457-1491 and 1579 (the kept `vrFrame`).

**Files:**
- Modify:
  - `core/emulator.cpp:1078-1104`;
  - `core/rend/vr/xr_host.{h,cpp}`;
  - `shell/android-studio/flycast/src/main/jni/src/Android.cpp:321-330`;
  - `core/rend/vr/xr_sim.cpp` (focus events are already there from Task 1; check that they reach pollEvents);
  - `cm-vr/accept.ps1`;
  - the checklist.

**Interfaces:**
- **Produces,** in `xr_host.h`:
  ```cpp
  bool menuPaused();           // the VR menu holds the emulator stopped
  void requestPause(bool on);  // render thread; acted on at the top of the next Emulator::render
  ```
- **The paused render path.** In `Emulator::render`, after `checkStatus()`: `if (state != Running) return vr::xr::enabled() && vr::xr::menuPaused() && vr::xr::frame();`
  - A paused `frame()` skips `rend_vr_drain`.
  - It still waits for and begins the frame, then draws the kept frame for each eye with the new views, plus the guns and the panel.
  - It sends no game input: `updateLightgun` returns early, as on the gun-run path.
- **The placeholder panel** until Task 7: `drawPanel` with "PAUSED" and the footer "≡ or B: resume". `menuEdge` or B toggles the pause.
- **Log lines:**
  - `XR: menu open (paused)`;
  - `XR: menu closed (resumed)`;
  - `XR: paused, redrawing the last frame` (once per pause).
- **Android:** `JNIdc_resume` starts the emulator only if `!vr::xr::menuPaused()`. When the app pauses with the menu open, the pause stays.
- **Focus:** losing and regaining focus while paused changes nothing. `menuPaused()` holds, and v1's automatic recenter after more than 3 s (xr_host:628-632) still runs.

**Steps:**

- [ ] **Step 1: Spike** the stop and start from `requestPause` at the top of `Emulator::render`, and the paused `frame()`. Run the cycle part of Step 2's scenario.
  - **Pass:** no hang (the run reaches its end), 20 opens and 20 closes in the log, and the paused shots are not blank.
  - **If stopping from the render thread hangs:** issue `emu.stop()` from a short-lived helper thread, keep drawing paused frames while `state` turns to Loaded, and record the stack that hung in the report.
  - **Not allowed:** giving up the real pause (spec anti-goal).
- [ ] **Step 2: Write the failing scenario `cm-pause`** (`-XrSim`, `$cmWiden`, `Seconds 125`). At:
  - `Get-CmStartsXr`;
  - `'40=xr left/menu 1 150'`, `'41=shot g'`, `'42=xr left/menu 1 150'`: an open on the 2D gun adjustment screen;
  - `'58=xr left/menu 1 150'`, `'59=shot p1'`, `'60.5=shot p2'`, `'61=pose head 0 1.6 0 25 0'`, `'62=shot p3'`, `'62.5=pose head 0 1.6 0 0 0'`, `'63=xr right/b 1 150'`, `'64=shot r1'`, `'67=shot r2'`;
  - 40 toggles `xr left/menu 1 150`, one per second from 70 to 109 s;
  - `'113=xr left/menu 1 150'`, `'114=xrstate visible'`, `'116=xrstate focused'`, `'117=shot f1'`, `'118.5=shot f2'`, `'119=xr left/menu 1 150'`, `'121=shot q1'`, `'123=shot q2'`.
  
  Checks:
  - `Get-LogCount 'XR: menu open \(paused\)'` = 23, and the same for `menu closed`.
  - `same p1 p2 --max-diff 1.5` and `same f1 f2 --max-diff 1.5`: frozen, including across the focus loss.
  - `differ p2 p3`: head tracking.
  - `blank g p1 p2 p3 f1 --region full --max 0.5`.
  - `differ r1 r2` and `differ q1 q2`: resumed.
  - Reject `XR: \d+ black frames`.
  - vr-test exits 0.
  
  Run it; it should FAIL (no pause).
- [ ] **Step 3: Implement** the API, the render path, the placeholder, the logs and the Android guard.
- [ ] **Step 4: Verify.** Build. `cm-pause` passes, as do `cm-sim`, `cm-buttons`, the CM regression and hotd2/mok.
- [ ] **Step 5: Checklist items:**
  - the menu button freezes the picture, which still follows the head, and the sound stops;
  - take the headset off with the menu open and put it back on: still paused, with the menu;
  - the Quest button mid-game and back: the game resumes as in v1.
- [ ] **Step 6: Commit** `emulator.cpp`, `xr_host.{h,cpp}`, `Android.cpp`, `accept.ps1` and the checklist, with the message "hotd2-vr: the VR menu pauses the emulator; the paused frame is redrawn with head tracking".

### Task 4: Rifle spike (game side) and the harness's scene keys

**Read:**
- the spec's `## Weapon pickups`;
- game B1-B4 and A3;
- findings Timeline and Gun;
- `vr-test.ps1` (all of it);
- `vr_reproject.cpp` 1412-1460 (the rip request's texture wait), 594 (`applyProfileWrites`, VBlank) and 1707 (`dropShotMarker`, every pass);
- the old action lists `b-spray.txt` and `a-pick.txt` in the session scratchpad that game research names.

**Files:**
- Modify:
  - `core/rend/vr_reproject.cpp`: the watches and the poke queue (debug, PC);
  - `core/input/udp_lightgun.cpp`: the `POKE` line;
  - `core/cfg/option.{h,cpp}`;
  - `cm-vr/vr-test.ps1`, `cm-vr/re/vrcheck.py`, `cm-vr/accept.ps1`;
  - `cm-vr/notes/confmiss-findings.md`: a new `## Rifle` section.

**Interfaces (produced):**
- **`vr.WatchRam`** (string, PC): `"addr[:size],..."`, with hex SH4 addresses, size 1, 2 or 4 (default 1), at most 16 entries. Checked each VBlank; a change logs `VR: ram 0c155808 06 -> 2d`.
- **`vr.WatchTex`** (string, PC): `"tex,..."` in hex. Logs `VR: tex 7e2800 shown (78 polys)` on the first pass that has it, and `VR: tex 7e2800 gone` after 10 passes without it.
- **UDP `POKE <addr hex> <size> <value hex>`** (PC): applied at the next VBlank, and logged as `VR: poke 0c155808 = 2d`. The vr-test action is `poke <addr> <size> <value>`.
- **vr-test `-On "<regex>|<delay s>|<action>[|<times>]"`:**
  - It reads new `flycast.log` lines while running; the log flushes per line (`LogManager.cpp:53`).
  - The fields split on `|`, so the regex itself must not contain `|` (use character classes instead).
  - Each matching line schedules the action at now + delay, up to `times` (default 1).
  - `actions.log` gets `on <regex> -> <action>`.
- **vrcheck `find-u8 <dump>... --values v1,v2,... [--range lo-hi]`:** the offsets whose byte takes those values in those dumps, in order, printed as SH4 addresses.
- **The findings contract**, read by Tasks 6, 12 and 14. `## Rifle` in `confmiss-findings.md` has these keys, each with evidence lines:
  - `found:` yes or no.
  - `scene:` where a dying enemy drops the rifle and the player can shoot it; the route that gets there, committed as `Get-CmRifleRoute` in `accept.ps1` and keyed with `-On`; and its success rate (N of M).
  - `state:` the RAM that marks the rifle (offset in the per-player block and value), plus ammo and magazine size at pickup. If 45 at +0x50 is the marker, say so.
  - `fire:` whether it fires automatically with the trigger held; its fire rate (+0x60 rises per second while held); whether +0x60 rises on each automatic shot; and **whether +0x60 rises on an empty pull, for the pistol too**.
  - `ends:` what ends it (ammo out, reload, stage change, continue) and the state afterwards.
  - `model:` textures (hex), polys when whole, list type, and the scene and time where it's drawn whole and large (held or dropped), with W and screen size; also whether it is the commandos' rifle in the stage 1 intro (t4c-intro ~53 and ~68 s).
  - `profile:` proposed values. For `GunRam` (Task 6): `weapon` offset and `rifleValue`. For the rifle's `GunTarget` (Task 10): the textures, `wholePolys`, `minPolys`, and `lengthM` 0.88 unless the model says otherwise.

**Steps:**

- [ ] **Step 1: Write the failing tests.**
  - A vrcheck `selftest` case for `find-u8`: three synthetic dumps, with one byte going 6, 45, 44.
  - Scenario `spike-keys` (flat, `Seconds 60`):
    - `Config 'config:vr.WatchRam=0c15581c,0c155804'`;
    - `At = Get-CmStarts + '56=poke 0c155804 1 03'`;
    - `-On 'VR: ram 0c15581c .* -> 04|0.5|shot keyed'`.
    
    Expect `VR: ram 0c15581c`, `VR: poke 0c155804 = 03` and `VR: ram 0c155804 .. -> 03`, and a `t*-keyed.png` in the run folder.
  
  Expected: FAIL.
- [ ] **Step 2: Implement** the watches, the poke queue, the UDP line, `-On`, `poke` and `find-u8`. Build. `spike-keys` and `selftest` pass. The CM regression is unchanged, since the options are off by default.
- [ ] **Step 3: Commit the harness part** (`vr_reproject.cpp`, `udp_lightgun.cpp`, `option.{h,cpp}`, `vr-test.ps1`, `vrcheck.py`, `accept.ps1`), with the message "hotd2-vr: scene-keyed actions, RAM/texture watches and pokes for the harness".
- [ ] **Step 4: Hunt for the rifle** (flat runs, in the background, 3-4 at a time at ~7 min each).
  - **Watches:** `vr.WatchRam=0c155804,0c155808,0c155818:2,0c155800:4,0c155810:4,0c155814:4`. These cover P1's ammo, magazine, shots and the zero bytes +72..+95. A byte going to 45 (0x2d), or to 44 after a shot, marks the rifle.
  - **Fire:** v11-spray's stage 1 pattern (a 5x4 grid every 0.3 s, a reload every 6th, start every 3 s), continued to 400 s, with the grid varied per run.
  - **Key the captures to the pickup:** `-On 'VR: ram 0c1558[01][0-9a-f] .* -> 2d|0|rip 3 0.5 1'` and the same key with `shot pickup`.
  - **Check the stage 1 intro:** use `vr.WatchTex` on any new OP texture group seen in the rip census near dying enemies and the commandos.
  - **Answer the empty-pull question first:** pistol, fire 7 times without a reload, and read +0x60 against ammo +0x4C.
- [ ] **Step 5: When a rifle is seen dropped but never picked up,** key aimed fire at it: `-On 'VR: tex <tex> shown|0.3|fire 0 X Y|5'`, with X and Y from the rip box in gun units (game B1's conversion).
  - **After a pickup:** hold the trigger, `-On '<pickup line>|1|button 0 1 3000'`, and count +0x60 rises per second. Then run the magazine out and reload, and log what follows.
  - **Model:** take rips keyed on the rifle's texture (`rip 4 0.3 0 <tex> <polys>`), and use riplib's census for the largest whole capture, as game research did.
- [ ] **Step 6: Write `## Rifle`** with the contract's keys and the route.
- [ ] **Step 7: Commit** `confmiss-findings.md` and `accept.ps1`, with the message "hotd2-vr: CM rifle pickup findings and route".

**Stop point:** if there is no pickup by ~70 calls, write `found: no` with every route tried, how far each got, and the answer to the empty-pull question, then commit. Tasks 12 and 14 then use the spec's fallback, the recoloured arcade gun; Task 14's state polling stays possible if a later find fills `state:`.

### Task 5: The standalone app

**Read:** code Q7 (all of it); spec items 1 and 8, and "Claude's calls", the name and ID and the start activity.

**Files:**
- Create:
  - `shell/android-studio/flycast/src/main/java/com/flycast/emulator/CmStartActivity.java`;
  - `.../emulator/CmFiles.java`;
  - `shell/android-studio/flycast/src/test/java/com/flycast/emulator/CmFilesTest.java`;
  - `cm-vr/assets/Play Confidential Mission.cmd`.
- Modify:
  - `src/vr/AndroidManifest.xml`: the label at :34; the alias at 41-57 loses LAUNCHER; VrSetupActivity at 62-74 leaves; CmStartActivity is added in `:setup` with MAIN, LAUNCHER and `com.oculus.intent.category.2D`. Keep the alias exported with MAIN, `com.oculus.intent.category.VR` and `IMMERSIVE_HMD`.
  - `build.gradle.kts`: :16 `hubVersion = "1.1.0"` and :160 `com.vrlightgunhub.confmiss`.
  - `BaseGLActivity.java`: 166-185 (`VR_APPLICATION_ID`) and 210-241 (`findVrGame` takes `EXTRA_GAME` or `CmFiles`; `showVrSetup` opens CmStartActivity).
  - `cm-vr/assets/make-icon.py` and `src/vr/res/drawable-*/ic_launcher.png`: a new look, no game art.
  - `core/rend/vr/xr_host.cpp:509` and `core/sdl/sdl.cpp:113`: "Confidential Mission VR".
  - `cm-vr/package-release.ps1`: names at :2-5 and :136-138; checks at :146-172; the banned list at :36 adds `gun-*.bin`; the zip adds the `.cmd`; README.txt and LICENSE.txt texts.
  - `README.md`: a CM-only app. The HOTD2 and MOK sections at :139-244 go, as does the migration paragraph at :75-80.
  - `.github/ISSUE_TEMPLATE/bug_report.yml`: the title, a Game field that lists CM only, and an app version field.
  - `.gitignore`: `gun-*.bin`.

**Interfaces:**
- **`CmFiles`** is pure Java (`java.io`, `java.util.zip`), so a JVM test can run it:
  ```java
  final class CmFiles {
      static final class Result { File zip; boolean chd, bios; List<String> missing; }
      static Result find(List<File> roots);  // roots: app files/games (internal, external), Download
  }
  ```
  - **Finding the zip:** `confmiss.zip`, case-insensitive, searched to depth 3 under each root.
  - **The CHD:** `<zip folder>/confmiss/gds-0001.chd`; `.cue` and `.gdi` are also accepted, as in `VrGames.arcadeTitle`.
  - **The BIOS:** `naomi.zip` in the same folder, or the NAOMI BIOS's file names inside `confmiss.zip` (the `"naomi"` BIOS entry in `core/hw/naomi/naomi_roms.cpp`).
  - **`missing`:** one line per missing file, saying where it goes. For example: "naomi.zip (the NAOMI BIOS): in Download, next to confmiss.zip", and "confmiss/gds-0001.chd: in a folder named confmiss next to confmiss.zip".
- **CmStartActivity:**
  - Strings: the title "CONFIDENTIAL MISSION VR"; step 1 "Allow" (VrSetupActivity's `askForAccess`); a checklist of the three files; the button "Play Confidential Mission", shown when nothing is missing; the footer "Unofficial fan project, not affiliated with Sega."
  - Play works like `VrSetupActivity.play()`: `endGameProcess()`, then the alias `com.flycast.emulator.MainActivity` with `EXTRA_GAME` (`"hotd2vr.game"`) set to the zip and `NEW_TASK | CLEAR_TASK`, then `finish()`.
  - VrSetupActivity's source stays in the tree, unused.
- **`public static void BaseGLActivity.quitToStart(Activity)`:** opens CmStartActivity in the home, as `showVrSetup` does, then finishes the game activity. Task 7 calls it through JNI.
- **PC:** `Play Confidential Mission.cmd` is `@echo off` then `start "" "%~dp0flycast.exe" "%~dp0roms\confmiss.zip"`. README.txt says `roms\` holds `confmiss.zip`, `confmiss\gds-0001.chd` and `naomi.zip`.

**Steps:**

- [ ] **Step 1: Write the failing JUnit test `CmFilesTest`,** using a `TemporaryFolder`:
  - all three files present → `missing` empty;
  - no CHD folder → `missing` names `confmiss/gds-0001.chd`;
  - the CHD as `Download/confmiss.chd` → still missing;
  - no `naomi.zip`, but a BIOS entry name inside `confmiss.zip` → `bios` true;
  - `Download/Confmiss.ZIP` → found;
  - `Download/a/b/confmiss.zip` → found (depth 3); `Download/a/b/c/d/confmiss.zip` → not found.
  
  Run, from `shell\android-studio` with `JAVA_HOME` and `ANDROID_HOME` set: `.\gradlew.bat :flycast:testVrUnitTest --tests com.flycast.emulator.CmFilesTest`. Expected: FAIL (no class).
- [ ] **Step 2: Write the failing release checks in `package-release.ps1`.**
  - From `aapt2 dump badging`: `package: name='com.vrlightgunhub.confmiss'`, `versionName='1.1.0-g<HEAD short hash>'` and `application-label:'Confidential Mission VR'`.
  - From `aapt2 dump xmltree --file AndroidManifest.xml`: `android.intent.category.LAUNCHER` appears only under CmStartActivity, and there is no `VrSetupActivity`.
  - The zip holds exactly `flycast.exe`, `emu.cfg`, `README.txt`, `LICENSE.txt` and `Play Confidential Mission.cmd`, and nothing banned.
  - README.txt contains none of `Light Gun Hub`, `House of the Dead` or `Maze of the Kings`.
- [ ] **Step 3: Implement** everything in Files.
- [ ] **Step 4: Verify.**
  - `CmFilesTest` passes.
  - `cm-vr\build-quest.cmd`, then `cm-vr\package-release.ps1`: every check passes, producing `dist\confidential-mission-vr-quest.apk` and `dist\confidential-mission-vr-pcvr.zip`. Do not install anything.
  - PC build; then `cm-sim`'s Expect is tightened to `XRSIM: runtime up for "Confidential Mission VR"` and passes; the CM regression passes.
- [ ] **Step 5: Checklist items:**
  - the Library shows "Confidential Mission VR" with its icon;
  - opening it shows the 2D start screen, not the game;
  - the All files access flow;
  - each missing file named;
  - Play starts the game in VR;
  - then uninstall the 0.1.0 hub (`com.vrlightgunhub.app`).
- [ ] **Step 6: Commit** the Java, manifest, gradle and icons, `make-icon.py`, the `.cmd`, `package-release.ps1`, `README.md`, the issue template, `.gitignore`, `xr_host.cpp`, `sdl.cpp`, `accept.ps1` and the checklist. Message: "hotd2-vr: Confidential Mission VR as its own app (com.vrlightgunhub.confmiss 1.1.0), CM start screen".

### Task 6: Haptics from the shot counters

**Read:**
- spec item 7;
- game B2 and B3;
- findings `## Rifle`, `fire:` (Task 4: does +0x60 rise on an empty pull?);
- `xr_host.cpp` 960-975 (`haptic`, `recoil`) and 1158-1335 (`updateGun`; the pull edge and `recoil()` at 1314-1316);
- `vr_reproject.h:99` (where to declare exports).

**Files:**
- Modify:
  - `core/rend/vr_reproject.{h,cpp}`: the `GunRam` field, CM's values, `gunState`, and the VBlank log;
  - `core/rend/vr/xr_host.cpp`;
  - `core/cfg/option.{h,cpp}`;
  - `README.md`, `cm-vr/accept.ps1`, the checklist.

**Interfaces (produced):**
- **The gun state:**
  ```cpp
  // vr_reproject.h
  struct GunState { bool valid; u8 ammo, magazine; u16 shots; };
  bool gunState(int player, GunState& out);   // read now from the running profile's gun RAM; false without one
  // GameProfile
  struct GunRam { u32 base = 0, stride = 0; u8 ammo = 0, magazine = 0, shots = 0; } gunRam;
  ```
  CM's values are `{0x0C1557B8, 276, 0x4C, 0x50, 0x60}`, with comments citing game B2.
- **The magazine log.** At VBlank, a magazine change logs `VR: P1 magazine 6 -> 45`.
- **`Option<int> VrHaptics("vr.Haptics", 2)`:** 0 off, 1 low, 2 normal, 3 strong. Every `haptic()` call is scaled: amplitude × {0, 0.45, 1, 1} and duration × {0, 1, 1, 1.6}.
- **Knocks:** a Shot is `haptic(hand, 0.030, 1.0)`, v1's `recoil()`; an Empty is `haptic(hand, 0.012, 0.35)`.
- **The rule,** per player, each frame:
  - Poll `gunState`.
  - Each rise of `shots` gives one Shot if `ammo` is lower than at the last poll, otherwise an Empty. Several rises in one poll give several knocks.
  - An on-screen pull edge with no rise within 0.1 s gives an Empty.
  - Off-screen (reload) pulls give no Shot; the `recoil()` call at 1314-1316 goes.
  - Without a `GunRam` (other games), v1's behaviour: a Shot on the pull edge.
- **Which hand:** P1's knocks go to the hand holding P1's gun, P2's to the left hand in dual play.
- Recoil animation and the flash stay on the pull edge, as in v1.

**Steps:**

- [ ] **Step 1: Write the failing scenarios** (`-XrSim`, `$cmWiden`, `Seconds 75`, `Get-CmStartsXr`).
  - **`cm-haptics`.** Pulls `xr right/trigger 1 100` every 0.6 s from 54.0 to 58.2 s (8 pulls), then `'59.5=xr right/b 1 200'`, then 3 pulls from 61.0 s. Then `'64=xr left/x 1 200'` and 2 left pulls from 66 s.
    - `Get-LogCount 'XRSIM: haptic right 0\.030 1\.000'` = 9 and `'XRSIM: haptic right 0\.012 0\.350'` = 2.
    - `Get-LogCount 'XRSIM: haptic left 0\.030 1\.000'` ≥ 1.
    - Reject `VR: P1 magazine`.
  - **`cm-haptics-low`:** the same with `config:vr.Haptics=1`. Expect `XRSIM: haptic right 0\.030 0\.450`, and Reject `1\.000$`.
  - **`cm-haptics-off`:** `config:vr.Haptics=0`. Reject `XRSIM: haptic`.
  - **`cm-haptics-mag`:** `'60=poke 0c155808 1 2d'`. Expect `VR: P1 magazine 6 -> 45`.
  - **`cm-haptics-reload`:** off-screen pulls with `pose right 0.18 1.3 -0.35 0 -80` (pointing at the floor). Reject `XRSIM: haptic right 0\.030`.
  
  Expected: FAIL.
- [ ] **Step 2: Implement.** If Task 4's `fire:` says +0x60 does not rise on an empty pull, the 0.1 s rule gives the click; otherwise the ammo rule does. Both stay in the code.
- [ ] **Step 3: Verify.** Build. The five scenarios pass, as do the earlier 1.1 scenarios, the CM regression and hotd2/mok.
- [ ] **Step 4: README and checklist.** Add a README line on haptics. Checklist items: the knock comes with the game's shot; an empty gun clicks; Low, Normal and Strong feel different; P2's knocks are in the left hand.
- [ ] **Step 5: Commit** with the message "hotd2-vr: haptics from the game's shot counters, vr.Haptics strength".

### Task 7: The settings menu

**Read:** spec item 6; code Q2 ("What UI exists", "A proposed menu"); `xr_panel.{h,cpp}` (all); Task 3's pause API; `Android.cpp` for `g_jvm` and `g_activity`.

**Files:**
- Create: `core/rend/vr/xr_menu.h`, `core/rend/vr/xr_menu.cpp`
- Modify:
  - `core/rend/vr/xr_panel.{h,cpp}`;
  - `core/rend/vr/xr_host.{h,cpp}`: Task 3's placeholder goes; `recenter`, `quitToStart` and `showControls` are added;
  - `core/cfg/option.{h,cpp}`: `vr.Gun`;
  - `CMakeLists.txt` (both lists);
  - `README.md`, `accept.ps1`, the checklist.

**Interfaces:**
- **Produces:**
  ```cpp
  // xr_panel.h
  struct PanelRow { std::string label, value; bool selected = false, dim = false; };
  struct RowsPanel { std::string title; std::vector<PanelRow> rows; std::string footer; };
  void drawRowsPanel(const glm::mat4& viewProj, const glm::mat4& model, const RowsPanel&); // model: centre, +y up, faces +z
  int rowAt(const glm::mat4& model, const RowsPanel&, const glm::vec3& origin, const glm::vec3& dir, float *u); // row or -1; u 0..1 across
  // xr_menu.h
  namespace vr::xr::menu {
  enum Row { Resume, Recenter, Gun, Laser, Haptics, WorldSize, PanelDistance, WiderView, ShowControls, Quit, Rows };
  struct Input { bool toggle, back, select, up, down, left, right; struct Ray { bool valid, click; glm::vec3 origin, dir; } ray[2]; };
  bool isOpen();
  void open(const glm::mat4& head);  // 1.2 m ahead of the head, yaw only, centre 0.1 m below the eyes; requestPause(true)
  void close();                      // requestPause(false)
  void update(const Input&);
  void draw(const glm::mat4& viewProj);
  }
  // xr_host.h
  void recenter();       // what the stick click does
  void quitToStart();    // Android: BaseGLActivity.quitToStart via JNI; PC: Flycast exits (dc_exit)
  void showControls();   // Task 9 fills it; until then logs only
  ```
- **`Option<int> VrGun("vr.Gun", 0)`:** 0 auto, 1 agents' pistol, 2 cabinet gun, 3 arcade gun. Task 13 resolves it; until then every value draws the arcade gun.
- **Rows,** in spec order:

  | Row | Shows | Changes |
  |---|---|---|
  | Resume | | closes |
  | Recenter | | `recenter()` |
  | Gun | Agents' pistol / Cabinet gun / Arcade gun | `vr.Gun` (cycle; auto shows what it resolves to) |
  | Laser | On / Off | `vr.Laser` |
  | Haptics | Off / Low / Normal / Strong | `vr.Haptics`, with a sample knock |
  | World size | − 0.110 + | `vr.WorldScale` ×/÷ 1.1, clamped 0.005-0.2 |
  | Panel distance | − 40 + | `vr.HudDepth` ± 5, clamped 15-120 |
  | Wider view | On / Off, plus "(next start)" once changed | `vr.Widen` |
  | Show controls | | `showControls()`, then closes |
  | Quit | | `quitToStart()` |

  Each change is saved at once.
- **Input:**
  - Either hand's laser, plus a trigger click, picks a row. On a value row, a click on the left half (u < 0.5) is −, the right half is +.
  - Stick up and down move the selection, wrapping from Resume up to Quit. Stick left and right are − and +.
  - A selects. ≡ or B closes.
  - The game gets no input while the menu is open.
  - While `hands::preparing()`, ≡ is ignored and logs `XR: menu: not while the guns are being made`.
- **Log lines:**
  - `XR: menu open (paused)` and `XR: menu closed (resumed)`, as in Task 3;
  - `XR: menu at 1.20 m, 10 rows, row pitch <m> m, top row <+m> m` on open;
  - `XR: menu <Row label> -> <value>`, for example `XR: menu Haptics -> Strong`;
  - `XR: menu quit`.

**Steps:**

- [ ] **Step 1: Write the failing scenarios** (`-XrSim`, `$cmWiden`, `Get-CmStartsXr`).
  - **`cm-menu`** (`Seconds 72`):
    - `'58=xr left/menu 1 150'`, then `stick right 0 -1 150` at 58.5, 59, 59.5 and 60, then `'60.5=xr right/a 1 150'` → Expect `XR: menu Haptics -> Strong`.
    - `stick right 0 -1 150` once more, at 61, to World size, then `stick right 1 0 150` at 62 → `XR: menu World size -> 0\.121`.
    - At 63, `pose right` aimed at the Laser row. The scenario computes the yaw and pitch from the open line's geometry (the panel centre is 1.2 m ahead at 1.5 m height), with the hand at (0.18, 1.3, −0.35). Then `'63.5=xr right/trigger 1 100'` → `XR: menu Laser -> Off`.
    - `'65=xr right/b 1 150'` → `XR: menu closed \(resumed\)`.
    - Afterwards, `emu.cfg.after` contains `vr.Haptics = 3`, `vr.Laser = no` and `vr.WorldScale = 0.121`, and `build-win\emu.cfg` is unchanged.
  - **`cm-menu-quit`** (`Seconds 70`): open at 58, `stick right 0 1 150` at 58.5 (wraps to Quit), `xr right/a 1 150` at 59 → Expect `XR: menu quit`, and vr-test's exit code is 2 (Flycast left early).
  
  Expected: FAIL.
- [ ] **Step 2: Implement.** Generalise `xr_panel` without changing how the gun-run panel looks: `drawPanel` keeps its signature. Add the menu, the actions and the JNI call.
- [ ] **Step 3: Verify.** Build. Both scenarios pass; so do `cm-pause` (its placeholder is replaced by the menu, and the counts stay the same), the earlier 1.1 scenarios, the CM regression and hotd2/mok.
- [ ] **Step 4: README section "The menu"** and checklist items: readable at arm's length; laser pointing with both hands; stick plus A; −/+ halves; Quit returns to the start screen; settings are kept after restarting the app.
- [ ] **Step 5: Commit** with the message "hotd2-vr: in-headset settings menu (pauses; rows from the 1.1 spec)".

### Task 8: Controller models

**Read:**
- code Q3;
- `xr_gun.cpp` 56-61 (materials), 99-195 (the model program) and 372-414 (`drawGunModel`);
- the head of `core/rend/vr/gun_model.h` and `cm-vr/gun_model/convert_gun.py` (the bake pattern);
- `build-win/_deps/openxr_sdk-src/include/openxr/openxr.h` 4451-4530 (`XR_FB_render_model`).

**Files:**
- Create:
  - `core/deps/cgltf/cgltf.h` and `core/deps/cgltf/LICENSE`: cgltf v1.14 from `https://raw.githubusercontent.com/jkuhlmann/cgltf/v1.14/cgltf.h`;
  - `core/rend/vr/glb_mesh.{h,cpp}`, `core/rend/vr/xr_controllers.{h,cpp}`, `core/rend/vr/controller_model.h` (generated);
  - `cm-vr/controller_model/make_controller.py`, `cm-vr/re/make-test-glb.py`.
- Modify:
  - `xr_host.cpp`: at 493-498, enable `XR_FB_render_model` when listed, and log the extension list once; in `frame`, draw the controllers;
  - `xr_sim.cpp`: render-model functions when `vr.XrSimRenderModel` is set;
  - `option.{h,cpp}`;
  - `shell/android-studio/flycast/src/vr/AndroidManifest.xml`;
  - `package-release.ps1`: the manifest check;
  - `CMakeLists.txt`, `accept.ps1`, the checklist.

**Interfaces:**
- **Produces:**
  ```cpp
  // glb_mesh.h
  struct GlbMesh { std::vector<float> positions, normals, uvs; std::vector<u32> indices; int texWidth = 0, texHeight = 0; std::vector<u8> rgba; };
  bool loadGlb(const u8 *data, size_t size, GlbMesh& out, std::string& error); // all primitives, node transforms applied; base colour PNG/JPEG via stb_image; other images: untextured
  // xr_controllers.h
  namespace vr::xr::controllers {
  enum Button { A, B, X, Y, Menu, Stick, Trigger, Grip, Buttons };
  void init(XrInstance, XrSession);      // once, at the first FOCUSED state
  void draw(const glm::mat4& viewProj, const glm::vec3& eyePos, int hand, const glm::mat4& grip);
  glm::vec3 anchor(int hand, Button);    // grip space, metres: where that button's label points
  bool fromRuntime(int hand);
  void term();
  }
  ```
- **Runtime models:**
  - Paths: `/model_fb/controller/left` and `/model_fb/controller/right`, through `xrEnumerateRenderModelPathsFB`, `xrGetRenderModelPropertiesFB` (with `XrRenderModelCapabilitiesRequestFB`, flag `XR_RENDER_MODEL_SUPPORTS_GLTF_2_0_SUBSET_1_BIT_FB`) and `xrLoadRenderModelFB`.
  - Drawn in grip space with one correction matrix, `kRuntimeModelOffset` (identity to start), which the headset checklist confirms.
  - Lit by the gun's model program.
- **Built-in shape:**
  - Made by `make_controller.py`, self-made with no Meta data: a Quest-3-like handle and face plate, a stick, two face buttons and a trigger.
  - At most 1500 triangles a hand, with parts per button (their centres are the anchors). It is built for the right hand and mirrored for the left.
  - Used when the runtime has no models or a load fails.
- **Options:**
  - `Option<std::string> VrXrSimRenderModel("vr.XrSimRenderModel", "")` (PC): a GLB file that the simulator serves as both controllers' render model, with `XR_FB_render_model` then listed.
  - `vr.ShowControllers` (bool, true) for the hand drawing.
- **Log lines:**
  - `XR: runtime extensions: <space-separated list>`, once;
  - `XR: controller models from the runtime (left <n> triangles, right <n>)`;
  - `XR: controller models built in (<why>)`.
- **Drawing for now:** each tracked hand not holding a gun draws its controller. In v1's single-gun play that is the free hand. Task 9 adds the state before play.
- **Manifest:** `<uses-permission android:name="com.oculus.permission.RENDER_MODEL"/>` and `<uses-feature android:name="com.oculus.feature.RENDER_MODEL" android:required="false"/>`. These names come from memory, so the checklist verifies them.

**Steps:**

- [ ] **Step 1: Write the test GLB maker.** `python cm-vr\re\make-test-glb.py <out.glb>` writes a box (12 triangles) and a 64-segment cylinder (128 triangles) with a 2x2 PNG texture: 140 triangles in all.
- [ ] **Step 2: Write the failing scenarios** (`-XrSim`, `Seconds 62`, `Get-CmStartsXr`). The hand-box shots happen after 56 s, with the left hand moved in front of the left eye: `pose left -0.1 1.45 -0.3 0 0`.
  - **`cm-controllers`:**
    - Expect `XR: controller models built in \(the runtime has no XR_FB_render_model\)`.
    - Shots: `in` with the left hand there, and `out` after `pose left -0.1 0.2 0.5 0 0`. `differ in out --box 0.1,0.3,0.6,0.95`.
  - **`cm-controllers-rt`:**
    - `config:vr.XrSimRenderModel=<abs path to build-win\vrtest\test-controller.glb>` → Expect `XR: controller models from the runtime \(left 140 triangles, right 140\)`.
    - The shot `in` differs from `cm-controllers`' `in` in the same box.
  
  Expected: FAIL.
- [ ] **Step 3: Implement.** Vendor cgltf; then write `glb_mesh`, `make_controller.py` (and run it), `xr_controllers`, the simulator's render-model functions, the extension, and the manifest. In `package-release.ps1`, check that `aapt2 dump xmltree` lists the permission.
- [ ] **Step 4: Verify.** Build. Both scenarios pass; so do the earlier 1.1 scenarios, the CM regression and hotd2/mok.
- [ ] **Step 5: Checklist items:**
  - the Quest 3 shows its own controllers in the hands, sitting right (not shifted or turned); if not, note how far off;
  - there is no permission prompt, or a prompt that is understandable;
  - on PC VR, the built-in shape.
- [ ] **Step 6: Commit** with the message "hotd2-vr: controller models (XR_FB_render_model, cgltf) with a built-in fallback". Add `core/deps/cgltf` with its LICENSE.

### Task 9: Labels and per-hand switching

**Read:** spec item 3; code Q4; Task 8's controllers API; `xr_host.cpp` 1435-1471 (the join and the single-gun hand move).

**Files:**
- Create: `core/rend/vr/xr_labels.{h,cpp}`
- Modify:
  - `core/rend/vr/xr_panel.{h,cpp}`: small text textures;
  - `core/rend/vr/xr_host.{h,cpp}`;
  - `CMakeLists.txt`, `accept.ps1`, the checklist.

**Interfaces:**
- **Produces:**
  ```cpp
  // xr_panel.h
  unsigned textTexture(const std::string& text, float px, const glm::vec3& colour, int& width, int& height); // cached by its arguments
  // xr_labels.h
  namespace vr::xr::labels {
  void draw(const glm::mat4& viewProj, int hand, const glm::mat4& grip, const char *profile); // next to controllers::anchor()
  }
  // xr_host.h
  enum class Holds { Controller, Gun };
  Holds holds(int hand);   // 0 left, 1 right
  ```
- **The state rules:**
  - When a game run starts, both hands are Controllers and the labels are on.
  - The right hand becomes the Gun at the first P1 Start (A), or at the first trigger pull of either hand. In that second case the pulling hand becomes P1's gun hand, as in v1's single-gun move. The labels go off at the same moment.
  - The left hand becomes the Gun when P2 joins, and a Controller again when P2 leaves.
  - In single play, the hand not holding the gun is a Controller.
  - `showControls()` makes both hands Controllers with labels until the next on-screen pull. That pull restores the previous state and fires as usual.
- **Label text:**
  - Quest Touch: right hand "A Start", "B Reload", "Trigger Shoot", "Stick click Laser"; left hand "X Player 2", "Y Reload P2", "≡ Menu" ("Menu" if the font has no ≡), "Stick click Recenter".
  - Other profiles use their own button names from Task 2's table.
- **Log lines:**
  - `XR: right hand: controller -> gun (start)` or `(shot)`;
  - `XR: left hand: controller -> gun (player 2 joins)`;
  - `XR: left hand: gun -> controller (player 2 out)`;
  - `XR: labels off (start)` or `(shot)`;
  - `XR: show controls`.

**Steps:**

- [ ] **Step 1: Write the failing scenarios** (`-XrSim`, `$cmWiden`).
  - **`cm-hands`** (`Seconds 100`). At:
    - `'20=pose right 0.1 1.45 -0.3 0 0'`, `'25=shot labels'`;
    - `Get-CmStartsXr`;
    - `'35=shot gun'`, `'60=xr left/x 1 200'`, `'90=xr left/menu 1 150'`, then `stick right 0 1 150` at 90.5 and 91 (up from Resume wraps to Quit, then Show controls), `'91.5=xr right/a 1 150'`, then `'94=xr right/trigger 1 100'`.
    
    Expect:
    - `XR: labels off \(start\)`;
    - `XR: right hand: controller -> gun \(start\)`;
    - `XR: left hand: controller -> gun \(player 2 joins\)`;
    - `XR: left hand: gun -> controller \(player 2 out\)` (P2 idles out at ~85 s; findings p2out);
    - `XR: show controls`;
    - `XR: right hand: controller -> gun \(shot\)`.
    
    `differ labels gun --box 0.3,0.3,0.9,0.95`.
  - **`cm-hands-shot`** (`Seconds 34`): only `'28=xr right/trigger 1 100'` → Expect `XR: right hand: controller -> gun \(shot\)` and `XR: labels off \(shot\)`.
  
  Expected: FAIL.
- [ ] **Step 2: Implement.**
- [ ] **Step 3: Verify.** Build. Both scenarios pass; so do the earlier 1.1 scenarios, the CM regression and hotd2/mok.
- [ ] **Step 4: Checklist items:** the labels are readable and sit by the right buttons on the Quest 3's own models; they go at the first Start or shot; Show controls brings them back; the left hand switches with P2.
- [ ] **Step 5: Commit** with the message "hotd2-vr: controller labels before play, per-hand controller/gun switching".

### Task 10: The cabinet gun rip (and the CM gun run)

**Read:**
- spec item 2 and "The gun rips";
- game A1 candidate 2, A2 and A3;
- `hands_rip.cpp` (all 389 lines);
- `hands_build.h`;
- `hands_build.cpp` 107-121 (`eye`), 331-392 (`gunAxes`), 738 (`write`) and 1121-1238 (`build`);
- `xr_hands.cpp` 1-179;
- `xr_host.cpp` 1935-1960 (the gun-run panel) and 1372-1384 (the B skip).

**Files:**
- Modify:
  - `core/rend/vr/hands_build.{h,cpp}`: `GunTarget`, `buildGun`, writing HND1 version 2;
  - `core/rend/vr/hands_rip.{h,cpp}`: the `CmGuns` kind and the CM run;
  - `core/rend/vr/xr_hands.cpp`: reading version 2; gun-only models with empty hand meshes;
  - `core/rend/vr_reproject.cpp`: CM's profile `p.model`;
  - `core/rend/vr/xr_host.cpp`: the panel's CM wording;
  - `cm-vr/re/vrcheck.py`: `hnd`;
  - `accept.ps1`, the checklist.

**Interfaces:**
- **Produces:**
  ```cpp
  // hands_build.h
  struct GunTarget {
      const char *name;          // "cabinet gun"
      const char *file;          // "gun-cm-cabinet.bin"
      uint32_t textures[4];      // 0: unused
      uint16_t wholePolys;       // polys when whole
      uint16_t minPolys;         // a capture needs at least this many
      float lengthM;             // the built model's length along the barrel, metres
      float skipWMin, skipWMax;  // polys with W in this band are not the gun (HUD sights)
      bool merge;                // merge captures across frames (Task 11)
  };
  bool buildGun(const std::vector<Rip>& captures, const GunTarget&, const TextureSource&,
                std::vector<uint8_t>& out, std::string& error, bool *complete);
  // hands_rip.h: Source::Kind gains CmGuns; Source gains GunTarget guns[3] (name()/file from guns[0] for modelPath())
  ```
- **HND1 version 2** is version 1 plus `u32 flags` after `onSlide[16]`: 1 = gun only (no hand meshes, travel 0), 2 = complete. The loader reads both versions.
- **CM profile:** `p.model.kind = hands::Source::CmGuns`, and `guns[0] = {"cabinet gun", "gun-cm-cabinet.bin", {0x400000}, 279, 200, 0.27f, 0.95f, 1.005f, false}`. The comment cites game A1 candidate 2.
- **The gun model's space** (matches what `xr_gun` expects of HND1):
  - the origin at the top centre of the grip;
  - the barrel along −z, found by `gunAxes`;
  - +y away from the grip;
  - scaled so the extent along z is `lengthM`;
  - the muzzle at the middle of the front face, as `build()` finds it.
  
  0x0C18AB20 is not used for the frame, because it holds the matrix only while the gun is drawn (game A3). Its rotation may be logged as a cross-check.
- **The CM run** (`onVBlank` for `CmGuns`, in game seconds):
  - Press P1 Start once at 30 s: the SEGA logo goes to the reload tutorial (findings startOk).
  - Capture the first pass that has at least `minPolys` of tex 400000 outside the sight band.
  - Task 11 continues the run from here. Give up at 120 s.
  - Prepare when any target's file is missing; Task 12 notes the exception for a rifle that is caught in play.
- **The panel:**
  - title "CONFIDENTIAL MISSION VR";
  - lines "Taking the guns from your copy of the game" and "About a minute or two, only this once";
  - footer "B: skip (the built-in gun this time)".
- **Log lines:**
  - `VR hands: no model yet, taking the guns from the game`;
  - `VR hands: the cabinet gun from this game, <n> bytes to <path>`;
  - on a failed build, `VR hands: the cabinet gun, but <error>` (WARN, the same form as the existing `VR hands: the hero's staff, but %s` at hands_rip.cpp:352);
  - the existing `VR hands: %s after %.0f s of game`, with why = `done` or `skipped`.
- **vrcheck `hnd <file> [--polys N] [--length L] [--gun-only] [--complete]`:**
  - It prints the version, flags, triangles and extents.
  - Checks: at least N triangles; the z extent within ±5% of L; muzzle z < −0.4·L and |muzzle x| < 0.01; the mesh centroid's y below the muzzle's y (the grip hangs down); the flags match.

**Steps:**

- [ ] **Step 1: Write the failing tests.**
  - A vrcheck `selftest` case: write a synthetic HND1 v2 file, then `hnd` passes; a mirrored copy (muzzle at +z) fails.
  - **`cm-rip-cabinet`:**
    - Remove `build-win\data\gun-cm-*.bin*`, then write junk to `build-win\data\gun-cm-cabinet.bin.part`.
    - Run flat with `config:vr.HandsPrep=yes`, `Seconds 150`.
    - Expect `VR hands: no model yet, taking the guns from the game` and `VR hands: the cabinet gun from this game, \d+ bytes to .*gun-cm-cabinet\.bin`. Reject `VR hands: .*, but `.
    - `vrcheck hnd build-win\data\gun-cm-cabinet.bin --polys 279 --length 0.27 --gun-only`.
  - **`cm-rip-hand`** (`-XrSim`, file present): Expect `XR: the game's own model from .*gun-cm-cabinet\.bin`. Take a shot `held` with `pose right 0.1 1.45 -0.3 0 0` after the start, and `differ` it against the same shot from a `vr.GameHands=no` run, in the hand box.
  - **`cm-rip-skip`:**
    - Remove the files. Run `-XrSim` with `config:vr.HandsPrep=yes`, with `'10=xr left/menu 1 150'` and then `'12=xr right/b 1 200'`.
    - Expect `VR hands: skipped after`, `XR: menu: not while the guns are being made` and `XR: no .*gun-cm-cabinet\.bin, the arcade gun it is` (or the existing arcade-gun line); the file is absent.
    - A second run with the same config: Expect `VR hands: no model yet` again.
  
  Expected: FAIL.
- [ ] **Step 2: Implement.**
- [ ] **Step 3: Verify.** Build.
  - The three scenarios pass, and `cm-rip-cabinet`'s `VR hands: done after N s of game` is at most 120.
  - The earlier 1.1 scenarios, the CM regression, and `-Only hotd2,mok` pass. HOTD2's `AgentsPistol` and MOK's `HerosStaff` paths are untouched.
- [ ] **Step 4: Checklist items:**
  - the first start shows the panel, which goes within about two minutes;
  - B skips it;
  - the cabinet gun in the hand faces the right way, at about the right size, with the flash at its muzzle;
  - the second start has no panel.
- [ ] **Step 5: Commit** with the message "hotd2-vr: CM gun run and the cabinet gun from the player's game (HND1 v2, gun only)".

### Task 11: The agents' pistol rip, merged across frames, with the completeness check

**Read:** game A1 candidate 1 and A2 (agents' pistol, including the "Open issue"); findings Timeline (the stage 1 intro); Task 10's code.

**Files:**
- Modify:
  - `core/rend/vr/hands_rip.cpp`: the run's second phase;
  - `core/rend/vr/hands_build.cpp`: the merge and completeness check;
  - `core/rend/vr_reproject.cpp`: CM `guns[1]`;
  - `core/cfg/option.{h,cpp}`: `vr.HandsCaptureLimit`, PC;
  - `accept.ps1`, the checklist;
  - `confmiss-findings.md`: `## Guns`, values only.

**Interfaces:**
- **Consumes:** `GunTarget`, `buildGun` and the HND1 v2 flags (Task 10).
- **Produces:**
  - `gun-cm-pistol.bin`, with flag 2 set when complete.
  - The log line `VR hands: the agents' pistol from this game (complete|incomplete: <first failed check>), <n> bytes to <path>`.
- **CM `guns[1]`:** `{"agents' pistol", "gun-cm-pistol.bin", {0x7e2800, 0x6cc400, 0x82f800, 0x76a800}, 91, 10, 0.30f, 0, 0, true}`. Howard's hand (tex 0x6ca000) is not in the list.
- **Phase 2 of the run:**
  - After the cabinet capture, or at 90 s, press Start every 2 s for 16 s (as v1's presses from 30 to 46 s did), then stop pressing.
  - Capture every pass with at least `minPolys` of the pistol's textures.
  - The phase ends 20 game seconds after the last such pass, or at 150 s.
  - Merge targets are captured only in this run, never while watching in play.
- **The merge** (this design decides it):
  - Lift each capture to eye space with `eye()`.
  - Key each vertex by (texture, u, v), quantised to 1/1024.
  - The reference is the capture with the most body (7e2800) polys.
  - Fit every other capture to it by Kabsch on the shared keys. A fit needs at least 8 keys and rms ≤ 1% of the body's length; a capture that fails is dropped and counted.
  - The merged set is the union of polys keyed by (texture, sorted model-space vertex keys); the first one seen wins.
  - Then build the frame, scale (0.30 m along the barrel plus suppressor) and muzzle as in Task 10.
- **Complete** means all of these hold; otherwise the log names the first one that failed:
  - the merged polys number 91, split 78 / 10 / 2 / 1 by texture;
  - the body has at most 30 open edges;
  - the body's extents are within 10% of the ratio 1.92 : 1.46 : 0.26.
- **`vr.HandsCaptureLimit`** (int, 0 = no limit, PC): limits how many captures are merged, so a run can exercise the incomplete path.

**Steps:**

- [ ] **Step 1: Write the failing scenarios.**
  - **`cm-rip-pistol`:** remove `gun-cm-*.bin`; flat, `config:vr.HandsPrep=yes`, `Seconds 240` (in the background).
    - Expect the cabinet line and `VR hands: the agents' pistol from this game \(complete\)`.
    - `VR hands: done after N s of game`, with N ≤ 150.
    - `vrcheck hnd ...gun-cm-pistol.bin --polys 91 --length 0.30 --gun-only --complete`.
  - **`cm-rip-pistol-one`:** the same with `config:vr.HandsCaptureLimit=1`. The log's `(complete|incomplete: ...)` agrees with `vrcheck hnd`'s flag: `--complete` is passed only if the log says complete.
  
  Expected: FAIL.
- [ ] **Step 2: Implement.**
- [ ] **Step 3: Verify.** Build. Both scenarios pass; so do `cm-rip-cabinet`, the earlier 1.1 scenarios, the CM regression and hotd2/mok. Append `## Guns` to the findings: the textures, poly counts, how many captures were merged, the rms, and the run's game seconds.
- [ ] **Step 4: Checklist items:** after the first start, the menu's Gun row offers the agents' pistol (Task 13); in the hand it looks whole, with no holes and nothing inside-out.
- [ ] **Step 5: Commit** with the message "hotd2-vr: the agents' pistol from the stage 1 intro, merged across frames, completeness check".

**Stop point:** if three rounds of tuning can't reach "complete", commit with the incomplete result (the default then falls to the cabinet gun in Task 13), and report the check that fails, with numbers.

### Task 12: The rifle rip

**Read:** findings `## Rifle` (Task 4); Task 10's and Task 11's interfaces.

**Files:**
- Modify: `core/rend/vr_reproject.cpp` (CM `guns[2]`), `core/rend/vr/hands_rip.cpp`, `accept.ps1`, the checklist, and the findings.

**Interfaces:**
- **Consumes:** findings `model:` and `profile:`; `GunTarget` and `buildGun`.
- **Produces:** `gun-cm-rifle.bin` (HND1 v2), and the log line `VR hands: the assault rifle from this game, <n> bytes to <path>`.
- **Where it is caught.** Decided by findings `model:`:
  - **Drawn whole in a scene the gun run passes** (for example, the commandos in the stage 1 intro): `guns[2]` with `merge` as needed, captured in phase 2.
  - **Otherwise:** caught in play by the watcher. `observePass` already runs in play, and the first pass where the rifle's textures reach `wholePolys` saves the file. For this target only, a missing file does not start a gun run, so the run isn't repeated at every start.
- **`found: no`, or no whole model:** no code. Write "rifle model: none; Task 14 uses the recoloured arcade gun" in the findings, and commit only that.

**Steps:**

- [ ] **Step 1: Write the failing scenario `cm-rip-rifle`.**
  - If the rifle is in the run's scenes: the `cm-rip-pistol` setup.
  - Otherwise: flat, with Task 4's `Get-CmRifleRoute`, retried up to 3 times if its success rate is below 1.
  
  Expect the rifle line, and `vrcheck hnd ...gun-cm-rifle.bin --polys <wholePolys> --length <lengthM> --gun-only`.
- [ ] **Step 2: Implement. Step 3: Verify,** with the same regression set as Task 11.
- [ ] **Step 4: Commit** with the message "hotd2-vr: the assault rifle model from the player's game".

### Task 13: The gun choice (several models)

**Read:** spec item 4; `xr_hands.{h,cpp}`; `xr_gun.{h,cpp}` (`gameGun()` at 342-345, `gunMuzzle()` at 347-350); Task 7's Gun row.

**Files:**
- Modify:
  - `core/rend/vr/xr_hands.{h,cpp}`: models per kind;
  - `core/rend/vr/xr_gun.{h,cpp}`: the hand's model, muzzle and flash;
  - `core/rend/vr/xr_host.cpp`;
  - `core/rend/vr/xr_menu.cpp`: the Gun row's availability;
  - `README.md`, `accept.ps1`, the checklist.

**Interfaces:**
- **Produces:**
  ```cpp
  enum class GunKind { Arcade, Cabinet, Pistol, Rifle };
  const HandsModel *gunModel(GunKind);   // loaded on first use from its file; nullptr if missing or unreadable
  bool gunComplete(GunKind);             // HND1 v2 flag 2
  GunKind chosenGun();                   // vr.Gun: 0 auto = pistol if complete, else cabinet if present, else arcade;
                                         // 1-3 as chosen, a missing one falling back the same way
  GunKind handGun(int player);           // chosenGun() now; Task 14 adds the rifle
  ```
- **Drawing and muzzle:** `gameGun()` and `gunMuzzle()` take the hand's `GunKind`. HOTD2 and MOK keep `handsModel()`, the single model.
- **The Gun row:**
  - It cycles agents' pistol → cabinet gun → arcade gun.
  - A missing gun shows dimmed with "(not made yet)" and is skipped.
  - An incomplete pistol can be chosen and shows "(incomplete)".
- **Log line,** at start and at each change: `XR: gun: <agents' pistol|cabinet gun|arcade gun> (<auto|chosen>[: <why the fallback>])`. For example: `XR: gun: cabinet gun (auto: the agents' pistol is incomplete)`.

**Steps:**

- [ ] **Step 1: Write the failing scenario `cm-gun`** (`-XrSim`). It needs `gun-cm-cabinet.bin` and `gun-cm-pistol.bin`; if they are absent it first runs `cm-rip-pistol`.
  - Expect `XR: gun: agents' pistol \(auto\)`, or `XR: gun: cabinet gun \(auto: the agents' pistol is incomplete\)` when the pistol file is flagged incomplete.
  - The menu's Gun row: + gives `XR: gun: cabinet gun \(chosen\)`, and + again gives `XR: gun: arcade gun \(chosen\)`.
  - Shots `g1`, `g2`, `g3` with the hand in view; they `differ` pairwise in the hand box.
  - A run with `config:vr.Gun=1` while `gun-cm-pistol.bin` is renamed to `.hold`, and restored in `finally`: Expect `XR: gun: cabinet gun \(chosen: the agents' pistol is not made yet\)`.
  
  Expected: FAIL.
- [ ] **Step 2: Implement. Step 3: Verify,** with the same regression set as Task 11, plus `cm-menu`.
- [ ] **Step 4: README** (the guns, the Gun row, and that they are made from the player's own game). Checklist: switching guns in the menu while paused shows the new gun at once.
- [ ] **Step 5: Commit** with the message "hotd2-vr: gun choice (agents' pistol, cabinet gun, arcade gun) with fallbacks".

### Task 14: The rifle in hand

**Read:** the spec's `## Weapon pickups`; findings `## Rifle`; Task 6's and Task 13's interfaces; `xr_gun.cpp` 56-61 (materials).

**Files:**
- Modify:
  - `core/rend/vr_reproject.{h,cpp}`: `GunRam` gains the weapon fields; `GunState` gains `rifle`; the log;
  - `core/rend/vr/xr_host.cpp`: `handGun`, the haptic pattern;
  - `core/rend/vr/xr_gun.{h,cpp}`: a tint;
  - `accept.ps1`, the checklist.

**Interfaces:**
- **Consumes:** findings `state:`, `fire:` and `ends:`; `gunModel(GunKind::Rifle)`.
- **Produces:**
  - `GunRam` gains `s16 weapon = -1; u8 rifleValue = 0;` (CM's values from `profile:`). If the marker is the magazine size, `weapon = 0x50` and `rifleValue = 45`.
  - `GunState` gains `bool rifle`.
  - At VBlank, the log `VR: P1 weapon pistol -> rifle` and its reverse.
  - `handGun(player)` returns `Rifle` while `rifle`.
  - The hand draws `gunModel(Rifle)` if present; otherwise the arcade gun with `tint = {0.42f, 0.50f, 0.30f}` (olive). `drawGunModel` gets a `const glm::vec3& tint` parameter that multiplies the material colours; it is `{1,1,1}` elsewhere.
  - **Haptics while the rifle is held:** the first rise after a pull edge is a Shot (0.030, 1.0). Each further rise while the trigger stays held is an Auto knock, `haptic(hand, 0.020, 0.6)`, scaled by `vr.Haptics`.
  - **Log lines:** `XR: P1 hand: assault rifle (gun-cm-rifle.bin)` or `XR: P1 hand: assault rifle (arcade gun, recoloured)`, and `XR: P1 hand: back to the <gun>`.
- **`found: no`:** the `GunRam` weapon stays −1, so the rifle never shows. The code still lands, and only the poke test runs, through a test-only `vr.RifleTest` override (PC): `"<offset>:<value>"`.

**Steps:**

- [ ] **Step 1: Write the failing scenarios.**
  - **`cm-rifle`** (flat; only if `found: yes`): `Get-CmRifleRoute`, retried up to 3 times. Expect `VR: P1 weapon pistol -> rifle`, then the route's end, `VR: P1 weapon rifle -> pistol`.
  - **`cm-rifle-sim`** (`-XrSim`, `Seconds 80`, `Get-CmStartsXr`):
    - At 56 s, poke the rifle state from `state:` (the weapon value, ammo and magazine) → Expect `XR: P1 hand: assault rifle \((gun-cm-rifle\.bin|arcade gun, recoloured)\)`.
    - `'58=xr right/trigger 1 2000'` → `Get-LogCount 'XRSIM: haptic right 0\.020 0\.600'` ≥ ⌊1.5 × the fire rate⌋.
      - If the pokes don't make the game fire automatically, the scenario also pokes +0x60 up by 1 every 0.1 s from 58.2 to 59.9 s and expects at least 15.
    - At 62 s, poke the pistol state back → Expect `XR: P1 hand: back to the`.
    - Shots `rifle` (57 s) and `pistol` (63 s) with the hand in view `differ` in the hand box.
  - **`cm-rifle-p2`:** P2 joins with X at 60 s; poke P2's block (base + 276) → Expect `XR: P2 hand: assault rifle` and left-hand Auto knocks.
  
  Expected: FAIL.
- [ ] **Step 2: Implement. Step 3: Verify.** Build. The scenarios pass; so do `cm-haptics*`, `cm-gun`, the CM regression and hotd2/mok.
- [ ] **Step 4: Checklist items:**
  - shooting a dropped rifle puts it in the hand (the model or the olive gun);
  - holding the trigger gives rapid lighter knocks;
  - when it runs out or the stage changes, the chosen gun comes back.
- [ ] **Step 5: Commit** with the message "hotd2-vr: the assault rifle pickup in hand, its haptics".

### Task 15: Acceptance, the 1.1 headset checklist, the release build

**Read:** the spec (all of it); `accept.ps1`; the checklist file as Tasks 1-14 left it; findings `## Baselines` (how v1 measured verts p95).

**Files:**
- Modify: `cm-vr/notes/confmiss-vr-1.1-headset-checklist.md`, `cm-vr/accept.ps1` (only if a scenario needs fixing), and `README.md` (a final pass).

**Steps:**

- [ ] **Step 1: Run everything.** In the background: `.\cm-vr\accept.ps1`, every scenario, with one blocking wait. Then `.\cm-vr\accept.ps1 -Only hotd2,mok`. Everything passes. A failure is fixed in the owning task's code, with its own commit, or reported with the run folder and the log line.
- [ ] **Step 2: Frame budget.** CM's verts p95 over `accept-cm-tour`, `accept-cm-hall` and a `cm-rifle` run, measured by the `## Baselines` method, is ≤ 13904. Note the added draws: the controllers at most 2 × (runtime model or ≤ 1500 triangles), the guns at most a few hundred polys each.
- [ ] **Step 3: Spec coverage.** Walk the spec's "What the player experiences" (items 1-8) and `## Weapon pickups`. Each line points to a passing scenario or a checklist item; add any item that is missing.
- [ ] **Step 4: Order the checklist for one session on the user's Quest 3:**
  1. install the APK;
  2. the start screen and files;
  3. the first-start gun run;
  4. controllers and labels;
  5. buttons;
  6. the menu and pause;
  7. the guns;
  8. haptics;
  9. player 2;
  10. the rifle;
  11. 72 Hz (the OVR Metrics overlay, with the menu open and in stage 1);
  12. uninstalling 0.1.0;
  13. the PC VR zip.
  
  Each item has a checkbox, what to do, and what should happen.
- [ ] **Step 5: Release build.**
  - `cm-vr\build-win.cmd`, `cm-vr\build-quest.cmd` (with the Quest env), then `cm-vr\package-release.ps1`. Every check passes: names, ID, label, versionName `1.1.0-g<HEAD>`, the LAUNCHER, the render-model permission, the zip contents, and no banned files.
  - Record the dist paths and the versionName in the checklist's header. Do not install.
- [ ] **Step 6: Commit** the checklist (and any README or accept fixes), with the message "hotd2-vr: CM VR 1.1 acceptance and headset checklist".
