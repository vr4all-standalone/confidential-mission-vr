# VR Light Gun Hub: Confidential Mission (v1) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Confidential Mission (NAOMI, MAME `confmiss`) plays in VR like HOTD2 VR and The Maze of the Kings VR, in the user's own release-ready app **VR Light Gun Hub** (`com.vrlightgunhub.app`), with HOTD2 and MOK unbroken.

**Architecture:** No new rendering approach. Confidential Mission (CM) gets one `GameProfile` in `core/rend/vr_reproject.cpp` `makeProfiles()` whose numbers come from reverse engineering on the PC build, and one `ARCADE` row in the launcher. Task 1 builds a committed PC test harness (`hotd2-vr\vr-test.ps1`, `hotd2-vr\re\vrcheck.py`, `hotd2-vr\accept.ps1`) that boots a game with config overrides, drives the light gun over UDP, takes screenshots and rips, and asserts log lines; every RE task proves its values with it and appends them with evidence to `hotd2-vr\notes\confmiss-findings.md`, which later tasks read. App identity, release signing and packaging are Gradle/Java/script changes. One Quest build at the very end goes to the user with a written headset checklist.

**Tech Stack:** C++17 (Flycast fork), Gradle Kotlin DSL (AGP 9.2.1) + Java (Android shell), Windows PowerShell 5.1, Python 3.13 (numpy, pillow, capstone), SH4 machine code (hand-encoded patch words).

**Spec:** `docs/superpowers/specs/2026-10-21-confidential-mission-vr-design.md` (approved; source of truth).
**Research notes (read before any RE task):** `hotd2-vr/notes/adding-a-game.md` ("notes §N" below). They hold the MOK template, every game-specific spot with file:line, the profile fields, the RE procedure (§7) and the risks (§8). This plan does not repeat them.

## Global Constraints

- Repo `E:\___flattovrdev\hotd2-vr`, branch `naomi-gun-vr`. Commit messages start `hotd2-vr:` (the fork's style) and end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Don't touch the modified `core/deps/gamesdk` submodule.
- CM's game ID is exactly `" CONFIDENTIAL MISSION ---------"` (leading space, nine dashes; log line `NAOMI GAME ID [ CONFIDENTIAL MISSION ---------] region ff players 2`). The profile log line is therefore `VR: camera profile for  CONFIDENTIAL MISSION ---------` (two spaces).
- App name **VR Light Gun Hub**; application ID **`com.vrlightgunhub.app`**. Neither names an emulator or a system.
- **No game data anywhere in the repo, the APK or the PC zip**: no ROMs, CHDs, rips (`rip\*.bin`), RAM dumps, screenshots of game frames, ripped models, disassembly listings. Findings and profile comments may hold addresses, numeric values, texture IDs and the mnemonic of each patched word (as MOK's profile does), nothing more. All run output goes under `build-win\vrtest\` (git-ignored via `build-win/`).
- Game files are read in place, never copied into the repo: NAOMI sets in `E:\___flattovrdev\confidential mission\Roms\` (`confmiss.zip`, `confmiss\gds-0001.chd`, `mok.zip`, `mok\gds-0022.chd`, `naomi.zip`); HOTD2 US disc `E:\___flattovrdev\confidential mission\House of the Dead 2 The (USA).chd` (profile `MK-51002`). Scripts find them through `$env:VRLGH_GAMES` (the Roms folder) and `$env:VRLGH_HOTD2` (the disc), never hard-coded.
- HOTD2 (`MK-51002`, `MK-5100250`) and MOK (`THE MAZE OF THE KINGS`) must behave exactly as at `9968dd020`: `.\hotd2-vr\accept.ps1 -Only hotd2,mok` passes after every task that touches C++.
- Follow the fork's patterns: per-game numbers only in the `GameProfile` (with comments saying what each value is and how it was found, MOK block style, `vr_reproject.cpp:296-347`); generic code changes stay generic.
- The aim uses the game's **stock** view (`vr.AimWidened` stays off): CM's widening must leave the focal its light-gun hit test uses unchanged.
- CM ships with the arcade gun: `p.model` stays default (`hands::Source::None`). Free play comes from `ForceFreePlay` (default true, `core/cfg/option.cpp:41`); no coin input.
- Quest target 72 Hz (`vr.XrRefreshRate` default, unchanged).
- Builds: PC `hotd2-vr\build-win.cmd` -> `build-win\flycast.exe` (~2 min). Quest (only Tasks 7, 8, 10): set `JAVA_HOME=E:\UnityEd\6000.0.78f1\Editor\Data\PlaybackEngines\AndroidPlayer\OpenJDK`, `ANDROID_HOME=E:\AndroidSdk`, run `hotd2-vr\build-quest.cmd` -> `shell\android-studio\flycast\build\outputs\apk\vr\flycast-vr.apk`. Nothing is installed on the headset before Task 10.
- No googletest: Flycast's suite (`tests/`, `BUILD_TESTING`) needs a second full core build as a console exe, and the C++ added here is profile data plus small branches that the harness's log assertions cover. `vrcheck.py` has its own `selftest`.
- Each task is one fresh subagent, under ~100 tool calls. RE tasks name a stop point: at it, hand back findings so far (values, candidates, what failed, file:line) instead of pushing on.
- Harness runs take 1-3 min; anything expected over ~4 min runs in the background and is waited on with one blocking `until` loop (CLAUDE.md).

## Review Focus

1. **Shots near the edges of the stock view while widened** (x or y at 2-5 % and 95-98 % of the frame): a person expects the hit exactly on the dot up to the edge, then the grey ring. Owner: Task 5 (edge shots with widening on, mapped-position check).
2. **Scenes with other fields of view** (attract demo, cutscenes, a boss, a vehicle section): the hit-test focal must follow each scene and the widening must hold in each, or aim drifts and the view snaps. Owner: Tasks 2-3 (three scene types per check), Task 5 (aim checked in two scene types).
3. **Profile writes before the GD-ROM code is there or after a reset** (BIOS boot, GD loading, game over -> attract -> new game): pokes or patches written into BIOS-time RAM corrupt the boot; missing after a new game breaks calibration. Owner: Task 3/5 (`ready` gate, boot-to-second-game run).
4. **3D right at the camera colliding with the 2D plane's W** (enemies at point blank, close walls, the vehicle/rope sections): streaks and bent HUD. Owner: Task 4 (census over a close-range scene, yawed screenshots).
5. **Player 2 prompt variants** (P2 CONTINUE countdown, P2 GAME OVER, attract prompts): a wrong box or texture keeps dual wield stuck on or drops it mid-game. Owner: Task 6 (join, play, drop-out, rejoin run).

---

## File map

| File | Task | Responsibility |
|---|---|---|
| `hotd2-vr/vr-test.ps1` (new) | 1 | one PC run: boot, scripted UDP gun, screenshots, rips, DebugShots, log assertions |
| `hotd2-vr/re/vrcheck.py` (new) | 1 | offline checks on RAM dumps, rips and `flycast.log` |
| `hotd2-vr/accept.ps1` (new) | 1, 9 | named acceptance scenarios (regression first, CM added in Task 9) |
| `hotd2-vr/notes/confmiss-findings.md` (new) | 1-6, 9 | every found value with its evidence |
| `core/rend/vr_reproject.cpp` | 2-6 | CM profile; `ready` gate, maybe `fovPatch[8]`, P2 prompt log |
| `shell/android-studio/flycast/src/main/java/com/flycast/emulator/VrGames.java`, `VrSetupActivity.java` | 7 | launcher row, setup panel title/help |
| `shell/android-studio/flycast/build.gradle.kts`, `src/vr/AndroidManifest.xml`, `src/vr/res/drawable-*/ic_launcher.png` | 7, 8 | app ID, label, icon, release signing |
| `core/rend/vr/xr_host.cpp:509`, `core/sdl/sdl.cpp:156,876` | 7 | OpenXR app name, PC window title |
| `hotd2-vr/assets/make-icon.py` (new) | 7 | draws the icon (no game art) |
| `hotd2-vr/make-release-key.ps1` (new), `hotd2-vr/package-release.ps1`, `README.md`, `.gitignore` | 8 | private key, packaging, the app's README |
| `hotd2-vr/notes/confmiss-headset-checklist.md` (new) | 10 | the user's Quest 3 test list |

---

### Task 1: PC test harness and baselines

**Files:**
- Create: `hotd2-vr/vr-test.ps1`, `hotd2-vr/re/vrcheck.py`, `hotd2-vr/accept.ps1`, `hotd2-vr/notes/confmiss-findings.md`
- Read for reuse: `E:\Temp\claude\E-----flattovrdev-confidential-mission\d6169e14-cc76-489f-bc38-2f929fa6e762\scratchpad\build\smoke.ps1` (NAOMI boot via `Dreamcast.BiosPath/ContentPath`, PrintWindow capture), `hotd2-vr/run-test.ps1` (key posting, capture), `hotd2-vr/rip-hands.ps1:36-66` (rip request, UDP loop, `input:device1=7` for a disc game's gun in port A), `hotd2-vr/assets/riplib.py` (`Pass(ripdir, n)`, `Poly`), notes §7.0-7.2.

**Interfaces:**
- Produces `vr-test.ps1` (PowerShell 5.1), run from the repo root:
  - `-Game <string>` (mandatory): a MAME set name (`confmiss`, `mok`: `$env:VRLGH_GAMES\<set>.zip`, BIOS/content paths set to `$env:VRLGH_GAMES`) or a disc image path (adds `input:device1=7`).
  - `-Name <string>` = `test`: output and working dir `build-win\vrtest\<Name>\` (emptied first). Flycast runs with this as its working dir, so `rip.request` and `rip\` land there.
  - `-Config <string>`: extra `config:x=y,...` appended last (wins).
  - `-At <string[]>`: timed actions `"<seconds>=<action>"`, seconds from process start: `start [p]` (button 4 held 200 ms), `aim p x y` (resting position, 0..10000), `fire p x y` (aim there, trigger bit 1 held 100 ms), `reload [p]` (`-10000 -10000` with bit 2 for 200 ms), `button p bits ms`, `shot [label]` (window screenshot `t<sec>[-label].png`), `rip <request text>` (writes `rip.request`, notes §7.2), `mark <text>` (line in `actions.log`). From the first gun action on, every touched player's `LG <p> <x> <y> <buttons>` goes to `127.0.0.1:27016` every 50 ms.
  - `-Seconds <double>` = 60, `-DebugShots <int>` = 0 (moves `shot_*.png` from Flycast's writable data dir, see `core/windows/winmain.cpp:70-90`, into the output dir; clears them before the run), `-Expect <string[]>` / `-Reject <string[]>` (regexes over `flycast.log`), `-Xr` switch (PC VR; default flat window `vr.Xr=no`).
  - Base config: `pvr.rend=0, rend.NativeDepthInterpolation=yes, vr.Reproject=yes, vr.Xr=no, aica.Volume=0, vr.UdpPort=27016, log:LogToFile=yes, log:Verbosity=4`. Never sets `vr.ProjAddr/ViewportAddr`.
  - Output lines `PASS expect <re>`, `FAIL expect <re>`, `FAIL reject <re> (line N)`; exit 0 all pass, 1 a check failed, 2 Flycast exited early. Kills a leftover `flycast` process before starting.
- Produces `vrcheck.py` (run with `python -I`), subcommands and output:
  - `matrices <ram.bin>` -> `viewport 0x0F24D0 320.0 -240.0` per 4x4 float matrix with [0]=320 and [5]=-240 (or +240); `proj 0x0E7A60 focal 415.7 415.7` per matrix with [1..4]=0, `abs([0])*320` and `abs([5])*240` within 0.5 % of each other. Offsets printed as RAM offsets (`addr & 0x01FFFFFF`), ready for the profile.
  - `find-float <ram.bin> <value> [--tol 0.01]` -> `0x0E7248 415.72` per float within relative tol.
  - `find-u16 <ram.bin> <v1,v2,...>` -> offset of each occurrence of the sequence.
  - `whist <ripdir> <n> [--max-w 3]` -> per W rounded to 0.001 (polygons whose corners share one W): `W 1.082 polys 57 list tr tex 4f6800,50b800 box 12,20..630,470`, sorted by W.
  - `border <ripdir> <n> <stockFocal> <liveFocal> [--min-ratio 0.5]` -> 3D polygon centres counted in bands of r = |x-320| / (320 * live/stock): `band 0.0-0.8`, `0.8-1.0`, `1.0-1.2`, `1.2-1.5` with count and density per unit r, then `cliff ratio R` = density(1.0-1.2)/density(0.8-1.0); exit 1 if R < min-ratio.
  - `shots <flycast.log> [--tex t1,t2] [--max-off N]` -> one line per shot (`VR: shot +ms gun x,y ...` lines grouped by gun position and time): nearest polygon (restricted to `--tex` if given), its px off, W band, tex; exit 1 if any shot's off > N.
  - `focal <flycast.log> [--expect F --tol 0.02]` -> from `VR: focal X x Y verts N ...` lines: `focal min/median/max`, `verts p50/p95/max`, `wnear` (the distinct `W near` values, rounded to 0.001, with counts), `count`; exit 1 if the median focal is off `--expect`.
  - `selftest`: builds synthetic RAM bytes and a synthetic log in a temp dir and asserts `matrices`, `find-float`, `find-u16`, `shots`, `focal` outputs.
- Produces `accept.ps1`: `-Only <string[]>` (default all), one function per scenario calling `vr-test.ps1`/`vrcheck.py`, prints `PASS <name>` / `FAIL <name>: <reason>`, exits 1 if any fail. This task adds scenarios `hotd2` and `mok`; Task 9 adds `cm-*`.
- Produces the findings file with these sections (later tasks append under them; each entry `- <what>: <value> — evidence: <run name, log line or vrcheck output> (date)`): `## Timeline`, `## Baselines`, `## Camera`, `## Field of view`, `## 2D plane`, `## Gun`, `## Player 2`, `## Acceptance`.

- [ ] **Step 1: Write `vrcheck.py selftest` first, run `python -I hotd2-vr\re\vrcheck.py selftest`, expect it to fail (subcommands missing).**
- [ ] **Step 2: Implement the subcommands; `selftest` prints `selftest OK`, exit 0.**
- [ ] **Step 3: Implement `vr-test.ps1`. Negative check:** `.\hotd2-vr\vr-test.ps1 -Game confmiss -Name neg -Seconds 40 -Expect 'NOT IN ANY LOG'` exits 1 with `FAIL expect`. Positive: `-Expect 'NAOMI GAME ID \[ CONFIDENTIAL MISSION ---------\]'` exits 0.
- [ ] **Step 4: CM timeline.** One ~150 s run with `-DebugShots 5`, `start 0` presses every 2 s from 30 s to 50 s, then `fire 0` at 5000,5000 every 3 s, `shot` every 10 s. View the frames with `python ~/.claude/tools/shot.py sheet`. Record under `## Timeline`, as seconds from process start for this exact press script: `sega`, `attract3d` (first 3D attract demo frame), `howto`, `freeplay` (FREE PLAY shown, yes/no), `startOk` (first press that begins a game), `stage1` (first stage gameplay), `cutscene` (first in-game cutscene). Adjust and rerun once if stage 1 isn't reached. If Start never begins a game, stop and hand back (free play broken).
- [ ] **Step 5: Prove the tools on MOK (ground truth, notes §3).** MOK run with `-Config 'config:vr.FovScale=2,config:vr.WidenFov=yes'`, `start 0` presses around 30-45 s, a `rip 1 1 1` in gameplay and `fire 0` shots at 5000,5000. Acceptance: `vrcheck matrices` lists `viewport 0x0F24D0` and `proj 0x0E7A60`; `vrcheck find-float ... 415.72` lists `0x0E7248` when the maze is on screen; `vrcheck whist` shows W 1.082 (HUD) and 1.090 (letterbox) if they're on screen; log has `VR: the game's field of view widened (code at 8c08d8ee)`. If a tool disagrees with MOK's known values, fix the tool.
- [ ] **Step 6: Baselines on unmodified code.** In `accept.ps1`, scenario `mok` (the Step 5 run: expects `VR: camera profile for THE MAZE OF THE KINGS`, the widened line, `vrcheck shots --max-off 2` on centre shots) and scenario `hotd2` (`$env:VRLGH_HOTD2`, `-Config 'config:vr.FovScale=3,config:vr.WidenFov=yes'`, Start presses as `run-test.ps1`/`rip-hands.ps1` time them; expects `VR: camera profile for MK-51002` and the `VR: field of view 41.1 -> 96.7 degrees at` line, or whatever exact numbers the unmodified build prints). Run `.\hotd2-vr\accept.ps1 -Only hotd2,mok`: both PASS. Record under `## Baselines`: the exact expected lines, and `vrcheck focal` verts p95 of the widened gameplay for each (the frame-rate budget reference for Task 3: both hold 72 Hz on Quest 3).
- [ ] **Step 7: Commit** `hotd2-vr/vr-test.ps1 hotd2-vr/re/vrcheck.py hotd2-vr/accept.ps1 hotd2-vr/notes/confmiss-findings.md` — `hotd2-vr: PC test harness for adding games (UDP gun, rips, log checks), baselines`.

---

### Task 2: CM profile recognised; camera matrices, focal, hit-test focal candidate, scale

**Files:**
- Modify: `core/rend/vr_reproject.cpp` `makeProfiles()`: new block after MOK's `list.push_back(p);` (~line 347), before `return list;`
- Modify: `hotd2-vr/notes/confmiss-findings.md` (`## Camera`)

**Interfaces:**
- Consumes: Task 1 harness, `## Timeline` times.
- Produces: CM profile with `gameId`, `projAddr`, `viewportAddr`, `focal`, `unitScale` (if not 1); under `## Camera`: `stockFocalAddr` candidate (confirmed in Tasks 3/5), live focal per scene type, the three rips' run names.

- [ ] **Step 1: Failing check.** `.\hotd2-vr\vr-test.ps1 -Game confmiss -Name t2-id -Seconds 45 -Expect 'VR: camera profile for  CONFIDENTIAL MISSION ---------'` -> exit 1.
- [ ] **Step 2: Add the skeleton** (comment style as MOK's block):

```cpp
	// Confidential Mission (NAOMI GD-ROM, MAME confmiss). Its boot ID starts with a space.
	p = GameProfile();
	p.gameId = " CONFIDENTIAL MISSION ---------";
	list.push_back(p);
```
  Build, rerun Step 1: exit 0.
- [ ] **Step 3: Rips in three scene types** (notes §7.2): `rip 1 1 1` at the `attract3d`, `stage1` and `cutscene` times (three runs or one run with three requests after each rip lands). Stop point for the task: ~70 calls.
- [ ] **Step 4: Matrices** (notes §7.3): `vrcheck matrices` on each `ram0000.bin`; pick the pair present in all three. Verify: run with `-Config 'config:vr.ProjAddr=<decimal>,config:vr.ViewportAddr=<decimal>'` through attract and stage 1; `vrcheck focal` count > 0, focal 100..2000, constant within a scene, changing between scene types if their FOV differs. Set `p.projAddr`, `p.viewportAddr`.
- [ ] **Step 5: Hit-test focal candidate** (notes §7.5 first bullet): `vrcheck find-float <ram> <that scene's live focal_y> --tol 0.005` on all three dumps; the offset whose value follows each scene's focal is the candidate `stockFocalAddr` (write it under `## Camera` with the three values; don't set it yet). Set `p.focal` = stage 1's focal_y.
- [ ] **Step 6: Scale** (notes §7.9): from the stage 1 rip, camera height h in game units from floor vertices under the camera (`h = (sy - 240) * W / focal_y`, median over floor corners). `unitScale = 15 / h`; set it only if outside 0.8..1.25 (MOK: 14.5 units -> left at 1). Record h and the result.
- [ ] **Step 7: Verify from the profile alone**: rerun the Step 4 runs without the config addresses; `-Expect` the profile line and `vrcheck focal --expect <stage1 focal>` passes on the stage 1 run. `.\hotd2-vr\accept.ps1 -Only hotd2,mok` PASS.
- [ ] **Step 8: Commit** `core/rend/vr_reproject.cpp hotd2-vr/notes/confmiss-findings.md` — `hotd2-vr: Confidential Mission profile: camera matrices, focal, scale`.

---

### Task 3: Widen CM's field of view (and its cull margin if needed)

**Files:**
- Modify: `core/rend/vr_reproject.cpp`: CM profile (`fovScale`, and `fovLiterals`+`fovStock` or `fovPatch`); only if needed, the `GameProfile` struct (~lines 185-240): `CodePatch fovPatch[8] {}` instead of `[4]`, and the `ready` gate (interface below)
- Modify: `hotd2-vr/notes/confmiss-findings.md` (`## Field of view`)

**Interfaces:**
- Consumes: Task 2 profile, `## Camera` (matrix addresses, live focal per scene, `stockFocalAddr` candidate), `## Baselines` verts p95 budget.
- Produces: CM widened with `config:vr.FovScale=<n>,config:vr.WidenFov=yes` on PC (the headset applies the profile's `fovScale` itself); the widen log line and the confirmed `stockFocalAddr` under `## Field of view`, which Tasks 5 and 9 assert.
- If pokes are needed without a `fovPatch` (a data cull margin here, or calibration in Task 5), whichever task comes first adds, and the other reuses:
  - `struct CodeWord { u32 offset; u16 value; };` next to `CodePatch`, and `CodeWord ready {};` in `GameProfile` ("the game's code is loaded once this word holds this value"), picked from CM's own code (not BIOS RAM) and checked stock in all three dumps.
  - `profileCodeThere()` returns false while `ready.offset != 0` and the word differs, before its `fovPatch` loop.
  - `applyProfileWrites` logs once per game start `VR: the game's code is there: profile writes on` (flag reset in `applyGameProfile`).

- [ ] **Step 1: Failing check.** CM run to `stage1` with `-Config 'config:vr.FovScale=2,config:vr.WidenFov=yes'` and `-Expect "VR: (the game's field of view widened|field of view [0-9.]+ -> [0-9.]+ degrees)"` -> exit 1.
- [ ] **Step 2: Find what CM builds its projection from** (notes §7.4 items 1-2, `hotd2-vr\re\sh4dis.py`). Mechanism order: (a) 16-bit angle literals -> `fovLiterals` + `fovStock`, `fovScale` = the factor; (b) code patch -> `fovPatch` (grow to `[8]` if more than 4 words; `applyProfileWrites`, `profileCodeThere` and `netplayDigest` already skip zero entries); (c) neither found by ~60 calls: stop and hand back candidates (the viewport-scaling fallback, notes §8.1, is the user's call, since the game would still cull to its stock frame).
- [ ] **Step 3: How far.** Aim for a main-play vertical FOV of 90-100° (HOTD2 96.7°, MOK 60° -> 90°). Budget: widened `vrcheck focal` verts p95 over a 60 s stage 1 run and an attract run ≤ the larger of HOTD2's and MOK's widened p95 from `## Baselines`; over it, widen less. Record the stock and widened angles and p95s.
- [ ] **Step 4: The hit test stays stock.** In the same scene with widening off and on, the `stockFocalAddr` candidate's value (`find-float`/RAM read on a rip's `ram*.bin`) is equal within 0.5 %, while the live focal shrinks by the widening factor. If the widening changes it, widen elsewhere (MOK patched the builder, not the stored focal) or stop and hand back. Set `p.stockFocalAddr`.
- [ ] **Step 5: Every scene type widens.** Runs through `attract3d`, `stage1`, `cutscene`: widen line present; `vrcheck focal` shows the widened focal in each; screenshots (`shot`) of each, viewed with `shot.py sheet`: the 3D fills the wider frame, menus/HUD unchanged.
- [ ] **Step 6: No culling cliff** (notes §7.6). `vrcheck border <rip> 0 <stock> <live> --min-ratio 0.5` on widened rips of the three scenes. A failure means CM culls to its stock frame: find the cull margin/frustum use and widen it (a `fovPatch` entry, or a gated poke on data), then recheck `border` and the Step 3 budget. If that added a gated poke (`ready`), also run from boot to `attract3d`: logos and attract as in the timeline (screenshots), `VR: the game's code is there: profile writes on` exactly once.
- [ ] **Step 7: Step 1's check passes**; `.\hotd2-vr\accept.ps1 -Only hotd2,mok` PASS (the struct may have changed).
- [ ] **Step 8: Commit** — `hotd2-vr: Confidential Mission: the game's view widened to <N> degrees`.

---

### Task 4: 2D plane: HUD W, snap bands, letterbox, near plane

**Files:**
- Modify: `core/rend/vr_reproject.cpp` CM profile: `hudW`, `snapMaxW`, `snapStep`, `letterboxW`, `flatScreens`
- Modify: `hotd2-vr/notes/confmiss-findings.md` (`## 2D plane`)

**Interfaces:**
- Consumes: Tasks 2-3 profile, timeline; notes §7.7 (log lines), §8.3.
- Produces: the near-plane W, HUD W, every 2D layer's W and texture under `## 2D plane` (Task 5 needs the marker's W, Task 6 the prompt's).

- [ ] **Step 1: Census.** Widened runs (Task 3 config) over attract (logos, title, How to Play, 3D demo), stage 1 with shots and a reload, a cutscene with subtitles, and a close-range scene (enemy at point blank or the closest wall; add a `closeup` time to `## Timeline`). Rips in each; `vrcheck whist`; `logOverlayCensus` lines and `VR: focal ... W near` from the log.
- [ ] **Step 2: Pick the values.** `hudW` = the HUD's W, never a W the 3D uses (check `W near` in all scenes, incl. `closeup`); `snapMaxW/snapStep` covering subtitles, stage titles, CONTINUE, results; `letterboxW` if bars exist; `flatScreens = true` if menus/How to Play are layers of depth with no 3D. Record each with its census evidence.
- [ ] **Step 3: Checks** (rerun Step 1's runs; the `snapNear2D`/`hideLetterbox` log lines show what moved):
  - `-Expect 'VR: 2D layer at W'` (layers snapped), and every census 2D layer either snapped or listed as kept on purpose in the findings.
  - Near plane: `hudW` differs by more than 0.002 (the `onOverlay` tolerance) from every `wnear` value `vrcheck focal` prints for these runs, and the number of `VR: kept a 3D polygon with` lines per scene is no higher than in the Step 1 census (record both counts).
  - Yawed views: the same runs with `-Config '...,config:vr.Yaw=25'` and `vr.Pitch=-15`, `shot` in each scene; `shot.py sheet` + `crop` on the HUD and the floor near the camera: no streaks from screen edge to the 3D, no bent text, HUD flat in front.
- [ ] **Step 4:** `.\hotd2-vr\accept.ps1 -Only hotd2,mok` PASS.
- [ ] **Step 5: Commit** — `hotd2-vr: Confidential Mission: its HUD, menus and subtitles on the 2D plane`.

---

### Task 5: The gun: calibration, gate, aim on the dot, shot marker and flash

**Files:**
- Modify: `core/rend/vr_reproject.cpp` CM profile: `pokes` (if needed), `ready` (Task 3 interface, if pokes and no `fovPatch`), `markerTextures`, `markerMinW/MaxW`
- Modify: `hotd2-vr/notes/confmiss-findings.md` (`## Gun`)

**Interfaces:**
- Consumes: Tasks 2-4 profile and findings; notes §4, §7.5, §7.8.
- Produces: under `## Gun`: the 9-point table (sent, measured, off) widening off and on, the marker textures and W band, the effect of B, the reload check; Task 9 replays them.

The 9 points (UDP 0..10000): centre 5000,5000; corners 1000/9000 pairs; edge midpoints 5000,1000 / 5000,9000 / 1000,5000 / 9000,5000. Edge points for Step 5: 300 and 9700 on each axis at the other's 5000.

- [ ] **Step 1: Failing check, widening off**, in stage 1, `config:vr.DropShotMarker=no,config:vr.DropShotFlash=no` (the game's own marker stays visible and logged): `fire 0` at the 9 points 1.5 s apart (`reload 0` when ammo runs out), then `vrcheck shots <log> --max-off 2` (restricted with `--tex` once the marker texture is known from the `VR new after shot` / `logShotTextures` lines). Expect failure unless CM's default calibration is neutral; if it already passes at all 9, record that and skip Step 2.
- [ ] **Step 2: Calibration** (notes §7.5 second bullet): read CM's gun calibration from its test menu or EEPROM, find it in RAM (`vrcheck find-u16`), poke neutral values (`p.pokes`), gated with `ready` if there is no `fovPatch`. Rerun Step 1: all 9 within 2 px.
- [ ] **Step 3: Gate.** A run from boot through a game over (no input after `startOk` until the player dies and the attract returns) into a second game: boot reaches `sega` and `attract3d` as in the timeline (screenshots), `VR: the game's code is there` (if `ready` was added) appears once per start, and Step 1's 9 shots in the second game still pass.
- [ ] **Step 4: Widening on** (Task 3 config): 9 shots again. The game's 2D marker is still ≤ 2 px from the gun position; the 3D impact (`VR new after shot ... (centre D px from the gun)` lines for 3D polygons, e.g. sparks or hits on walls/enemies) lies at the gun position mapped to the widened frame, `x' = 320 + (x - 320) * live/stock` (likewise y), within 3 px, in stage 1 and in a second scene type with a different FOV. If CM shows no 3D impact, the Task 3 Step 4 check (hit-test focal stays stock) stands in, and it must be rechecked in both scenes here.
- [ ] **Step 5: Edges.** The four edge points with widening on: each registers a shot (marker or impact logged within 200 ms).
- [ ] **Step 6: Reload and B.** `reload 0` refills the ammo (`shot` of the HUD before and after, `crop`). `button 0 128 300` (B, mapped to NAOMI BTN1, notes §4) in gameplay and in a cutscene: record what it does. Harmless -> note it. Fires, reloads, or breaks something -> stop and hand back (the fix is a design decision).
- [ ] **Step 7: Marker and flash hidden.** `markerTextures` and `markerMinW/MaxW` from the shot logs (notes §7.8). With defaults (`DropShotMarker`, `DropShotFlash` on): `-Expect 'VR: hid the game''s shot marker'`, and screenshots 100 ms after shots show no marker and no full-screen flash.
- [ ] **Step 8:** `.\hotd2-vr\accept.ps1 -Only hotd2,mok` PASS.
- [ ] **Step 9: Commit** — `hotd2-vr: Confidential Mission: the gun's aim lands where it points, its shot marker hidden`.

---

### Task 6: Player 2 prompt (and dual wield)

**Files:**
- Modify: `core/rend/vr_reproject.cpp`: CM `p2PromptTextures`, `p2PromptBox`; a log on each change of `player2Prompting()` (`:1017`), next to where `hidePlayer2Prompt` (`:953`) records the prompt
- Modify: `hotd2-vr/notes/confmiss-findings.md` (`## Player 2`)

**Interfaces:**
- Consumes: Task 4 census (prompt textures/W), timeline.
- Produces: log lines `VR: player 2 prompt shown` and `VR: player 2 prompt gone`, once per change of the half-second state `player2Prompting()` returns (so PC runs can see what the headset's join/leave, `xr_host.cpp:1435-1455`, will see). Generic: HOTD2 and MOK print them too.

- [ ] **Step 1: Failing check.** One-player stage 1 run, `-Expect 'VR: player 2 prompt shown'` -> exit 1 (no log yet, no CM prompt data).
- [ ] **Step 2: Add the log;** HOTD2 run in a one-player game: `-Expect 'VR: player 2 prompt shown'` passes (HOTD2's prompt is known), so the log works.
- [ ] **Step 3: Find CM's prompt** (notes §7.8 second bullet): rip a one-player frame; the P2 prompt's texture(s) and bbox in 640x480 -> `p2PromptTextures`, `p2PromptBox` (the box must not also cover player 1's prompt or HUD).
- [ ] **Step 4: Join/leave run:** one-player game; `start 1` with player 2 aimed on screen at ~10 s into stage 1; player 2 idle until out (CONTINUE runs out); `start 1` again. Expect in order: `shown`, `gone`, `shown` (P2 out), `gone`. Add `p2out` to `## Timeline`. Screenshot during one-player play: no P2 prompt visible with `vr.HideP2Prompt` on (default).
- [ ] **Step 5:** `.\hotd2-vr\accept.ps1 -Only hotd2,mok` PASS.
- [ ] **Step 6: Commit** — `hotd2-vr: Confidential Mission: player 2's prompt known (hidden; dual wield)`.

---

### Task 7: Launcher entry and the app's identity

**Files:**
- Modify: `shell/android-studio/flycast/src/main/java/com/flycast/emulator/VrGames.java:55-57` (`ARCADE`)
- Modify: `shell/android-studio/flycast/src/main/java/com/flycast/emulator/VrSetupActivity.java:57` (title), `:79-81` (help text)
- Modify: `shell/android-studio/flycast/build.gradle.kts` (vr build type, lines ~62-85)
- Modify: `shell/android-studio/flycast/src/vr/AndroidManifest.xml:32` (label)
- Create: `shell/android-studio/flycast/src/vr/res/drawable-{ldpi,mdpi,hdpi,xhdpi,xxhdpi}/ic_launcher.png` (overrides main's five), `hotd2-vr/assets/make-icon.py`
- Modify: `core/rend/vr/xr_host.cpp:509`, `core/sdl/sdl.cpp:156,876`, `hotd2-vr/rip-hands.ps1:85`, `hotd2-vr/netplay-quest.ps1:37`, `hotd2-vr/build-quest.cmd:2` (comment)

**Interfaces:**
- Produces: APK package `com.vrlightgunhub.app`, label `VR Light Gun Hub`; PC window title and OpenXR application name `VR Light Gun Hub`; setup panel title `VR LIGHT GUN HUB`. Java namespace and class names stay `com.flycast.emulator`.

- [ ] **Step 1: Launcher row** `{ "confmiss", "Confidential Mission", "gds-0001" }` after MOK's; help text names the three games and CM's layout (`Download/confmiss.zip`, `naomi.zip` next to it or BIOS inside, folder `confmiss` with `gds-0001.chd`), keeping "Nothing of the games comes with this app. Unofficial fan project, not affiliated with Sega."
- [ ] **Step 2: Application ID.** Replace the vr build type's `applicationIdSuffix = ".vr"` with `androidComponents { onVariants(selector().withBuildType("vr")) { it.applicationId.set("com.vrlightgunhub.app") } }`; keep `versionNameSuffix`. If AGP 9.2.1 rejects it, stop and hand back. `${applicationId}.provider` (main manifest :181) follows on its own.
- [ ] **Step 3: Names.** Manifest label, `xr_host.cpp:509`, both `sdl.cpp` titles (one `static const char *` if they're in one file), setup panel title, the two scripts' package name, `build-quest.cmd` comment. `grep -rn "HOTD2 VR\|com.flycast.emulator.vr" core shell/android-studio/flycast/src hotd2-vr/*.ps1 hotd2-vr/*.cmd` afterwards: only HOTD2-specific strings remain (prep panel title `xr_host.cpp:1939`, docs).
- [ ] **Step 4: Icon.** `make-icon.py` draws it with Pillow from shapes only (gun sight / light gun silhouette, the name's initials; no game or Sega art), writes the five densities at main's sizes. View with `shot.py fit`.
- [ ] **Step 5: Verify.** `build-quest.cmd`; `aapt2 dump badging` (from `E:\AndroidSdk\build-tools\*`) prints `package: name='com.vrlightgunhub.app'` and `application-label:'VR Light Gun Hub'`; `aapt2 dump resources` (or unzip listing) shows the five `ic_launcher.png` from `src/vr/res`. `build-win.cmd`; a 20 s `vr-test.ps1` run plus `(Get-Process flycast).MainWindowTitle` -> `VR Light Gun Hub`. `.\hotd2-vr\accept.ps1 -Only hotd2,mok` PASS.
- [ ] **Step 6: Commit** — `hotd2-vr: VR Light Gun Hub (com.vrlightgunhub.app), Confidential Mission in the launcher`.

---

### Task 8: Release signing, packaging, the app's README

**Files:**
- Modify: `shell/android-studio/flycast/build.gradle.kts` (`signingConfigs`, vr build type)
- Create: `hotd2-vr/make-release-key.ps1`
- Modify: `hotd2-vr/package-release.ps1`, `README.md`, `.gitignore`

**Interfaces:**
- Consumes: Task 7 identity.
- Produces:
  - Signing config `hub` read from a properties file (`storeFile`, `storePassword`, `keyAlias`, `keyPassword`; `storeFile` relative to the properties file): path from env `VRLGH_SIGNING`, else `<user.home>/.vrlightgunhub/release-signing.properties`. Present: the vr build type signs with it. Absent: debug key and a Gradle `logger.warn` naming the missing file. The vr build type stays debuggable (run-as for logs and the existing adb tools; mikermak's releases are the same).
  - `make-release-key.ps1 [-Dir <path>]` (default `%USERPROFILE%\.vrlightgunhub`): `keytool -genkeypair` RSA 4096, validity 10000 days, alias `vrlightgunhub`, dname `CN=VR Light Gun Hub`, random 24-character password; writes `release.jks` and `release-signing.properties`; refuses if either exists; ends by telling the user to back the folder up (losing it means no updates).
  - `package-release.ps1 [-SkipBuild] [-AllowDebugKey]` -> `dist\vr-light-gun-hub-quest.apk`, `dist\vr-light-gun-hub-pcvr.zip` (`flycast.exe`, `emu.cfg` as now, `README.txt`, `LICENSE.txt`). Refuses (exit 1) if the APK's signer certificate (`apksigner verify --print-certs`) equals `shell/android-studio/debug.keystore`'s, unless `-AllowDebugKey`. Refuses if either artefact contains `*.chd *.gdi *.cue *.cdi *.pic`, `rip/`, `shot_*.png`, `hands*.bin`, `staff*.bin`, `HND1*.bin` (list entries, check names).

- [ ] **Step 1: Failing check:** `.\hotd2-vr\package-release.ps1 -SkipBuild` currently packages the debug-signed APK under the old names; after the change, with no key present, it must exit 1 with a "signed with the public debug key" message.
- [ ] **Step 2: Implement** signing, `make-release-key.ps1`, packaging checks and names. `.gitignore`: `release-signing.properties`, `*.jks` (check `playstore.jks` isn't tracked first).
- [ ] **Step 3: Verify with a throwaway key only** (never create the user's real key; that is the user's step): `make-release-key.ps1 -Dir <scratchpad>\testkey`; `$env:VRLGH_SIGNING='<scratchpad>\testkey\release-signing.properties'`; `build-quest.cmd`; `apksigner verify --print-certs` shows `CN=VR Light Gun Hub`; `package-release.ps1 -SkipBuild` exits 0 and lists the two files. A second `make-release-key.ps1 -Dir` on the same dir refuses. Unset the env var, rebuild: Gradle warns, `package-release.ps1 -SkipBuild` exits 1, `-AllowDebugKey` exits 0. Plant a dummy `rip/x.bin` into a copy of the zip's stage: the content check fails.
- [ ] **Step 4: README.** `README.md` becomes the hub's: name and what it is (light-gun games in VR, Quest 3 and PC VR; more games later); games: HOTD2 (Dreamcast, EU/US), MOK and CM (NAOMI MAME sets) with each one's Download layout; install (SideQuest/adb, All files access, the setup panel); PC VR; controls; building; releasing (`make-release-key.ps1`, back the key up, `package-release.ps1`); credits and licences: mikermak's hotd2-vr (link), Flycast (GPL-2.0-or-later), the gun model "Namco Arcade Gun" by Martoscar (CC BY 4.0) and therefore GPL-3.0 builds, source published with every release; "Unofficial fan project, not affiliated with Sega. No game is included." Keep mikermak's per-game, options, how-it-works and tools sections (retitled where they say HOTD2 VR; package names updated). `README.txt` in the zip: the same install steps for PC VR in short, credits, licence.
- [ ] **Step 5: Commit** — `hotd2-vr: release signing with a private key, VR Light Gun Hub packages and README`.

---

### Task 9: Acceptance on PC (CM) and regression

**Files:**
- Modify: `hotd2-vr/accept.ps1` (scenarios `cm-boot`, `cm-widen`, `cm-aim`, `cm-hud`, `cm-p2`)
- Modify: `hotd2-vr/notes/confmiss-findings.md` (`## Acceptance`)

**Interfaces:**
- Consumes: everything under `## Timeline` .. `## Player 2`.
- Produces: `.\hotd2-vr\accept.ps1` (all scenarios) as the one regression command for future games.

- [ ] **Step 1: Scenarios** from the findings, each a spec "Done means" PC line:
  - `cm-boot`: game ID line, profile line, `ForceFreePlay` Start begins a game by `stage1`.
  - `cm-widen`: widened line; `vrcheck focal` widened in attract/stage/cutscene; `shot`s of the three kept in the output dir.
  - `cm-aim`: the 9 points widening off and on (`vrcheck shots --max-off 2`, the mapped 3D check or the stock-focal check), edges register.
  - `cm-hud`: 2D layer lines, no increase in `kept a 3D polygon` lines vs `## 2D plane`, `VR: hid the game's shot marker`, yawed `shot`s.
  - `cm-p2`: shown / gone / shown / gone.
- [ ] **Step 2: Run** `.\hotd2-vr\accept.ps1` (background, one blocking wait): all PASS, incl. `hotd2` and `mok` against `## Baselines`. View the screenshots as one `shot.py sheet` (attract, stage 1, cutscene, yawed): no streaks, HUD flat, wider 3D.
- [ ] **Step 3: PC VR smoke** without a headset is impossible here: run `vr-test.ps1 -Xr` once for 20 s and record whether it fails cleanly (no runtime) without a crash.
- [ ] **Step 4: Record** each scenario's result and output dir under `## Acceptance`. Commit — `hotd2-vr: acceptance checks for Confidential Mission, HOTD2 and MOK`.

---

### Task 10: The Quest build and the user's headset checklist

**Files:**
- Create: `hotd2-vr/notes/confmiss-headset-checklist.md`

**Interfaces:**
- Consumes: Tasks 7-9 (`package-release.ps1`, identity, `## Acceptance`).
- Produces: `dist\vr-light-gun-hub-quest.apk`, `dist\vr-light-gun-hub-pcvr.zip`, the checklist; nothing published.

- [ ] **Step 1: Key gate.** Ask the orchestrator whether the user has run `hotd2-vr\make-release-key.ps1` (their own key; the agent never makes it). Yes: build signed with it. No: build with `-AllowDebugKey`, and the checklist's first line says this test install must be uninstalled before the first real release (a different key can't update it; uninstalling loses settings).
- [ ] **Step 2: Build and package:** `package-release.ps1` (with the env from Global Constraints); both content checks pass; `aapt2 dump badging` package/label as in Task 7.
- [ ] **Step 3: Checklist** (each item: what to do, what "good" looks like, what to report: OK / not OK + scene + roughly when):
  1. Install (`adb install -r dist\vr-light-gun-hub-quest.apk` or SideQuest) next to HOTD2 VR (`com.flycast.emulator.vr`) and Flycast: all three in the library; name and icon right.
  2. Files: `Download/confmiss.zip`, `Download/naomi.zip`, `Download/confmiss/gds-0001.chd`; setup panel asks for All files access, lists CM (and HOTD2/MOK if present; picking one starts it).
  3. Boot/attract: logos, title, How to Play on the panel; the 3D demo around you.
  4. A starts a game (free play, no coin).
  5. Stage 1: look around and lean; wider view than the panel; nothing popping in/out when turning the head; nothing at the face.
  6. Aim: the hit lands on the dot at centre, corners and edges of the game's view; grey ring and no shot outside it; reload by shooting away; recoil, muzzle flash, haptics.
  7. HUD/subtitles on the panel, no streaks where HUD meets 3D, no game shot flash or marker.
  8. Player 2: left controller's menu button joins with a gun in the left hand; drops to one gun when P2 is out; no floating P2 prompt in one-player play.
  9. Frame rate: steady at 72 Hz all through stage 1 (judder or the Meta performance overlay / OVR Metrics Tool if installed); name any scene that stutters.
  10. HOTD2 and MOK still play (one stage each).
  11. PC VR over Link: unzip `vr-light-gun-hub-pcvr.zip`, start `flycast.exe`, add the Roms folder, play CM's first stage; same points 3-8.
  12. When something is wrong: the log (`adb shell run-as com.vrlightgunhub.app cat <path>`; take the path from README "Problems? Ideas?" with the new package) and the first 8 shots' `VR diag: shot` lines.
- [ ] **Step 4: Commit** the checklist — `hotd2-vr: headset checklist for Confidential Mission`. Hand back the two `dist` paths, the signing state and the checklist path.
