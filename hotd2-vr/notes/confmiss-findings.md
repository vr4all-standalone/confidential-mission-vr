# Confidential Mission: findings

What the Confidential Mission port learns, task by task: times, addresses, values, texture IDs
and exact log lines, never game data. Each entry is
`- <what>: <value> — evidence: <run name, log line or vrcheck output> (date)`.
Runs are `hotd2-vr\vr-test.ps1` and `hotd2-vr\accept.ps1` on the Windows build, output in
`build-win\vrtest\<run>\`. Times are seconds from Flycast's start; the log's `mm:ss:ms` runs
about 0.1 s behind the script's clock, and DebugShots frames (game time) lag it by up to 5 s.

## Timeline

Run `cm-timeline`: `-Game confmiss -Seconds 150 -DebugShots 5`, `start 0` every 2 s from 30 to
50 s, `fire 0 5000 5000` every 3 s from 53 to 149 s, `shot` every 10 s; no camera profile yet
(focal 640.0 throughout).

- boot: the NAOMI logo ~10 s, a loading screen ~20 s — evidence: cm-timeline `00:00:421 ... N[NAOMI]: NAOMI GAME ID [ CONFIDENTIAL MISSION ---------] region ff players 2 vertical 1`, frames (2026-10-21)
- sega: 28 s (its light-streak intro from 24 s, 3492 verts at median W 79.9) — evidence: cm-timeline `00:27:962 ... VR: focal 640.0 x 640.0  verts 482 overlay 12  W near 0.998 p10 80.000 median 80.000`, window shot t030 (2026-10-21)
- freeplay: yes, the free-play notice and the start prompt on every attract screen from the first one on — evidence: cm-timeline frames shot_0025 onwards (2026-10-21)
- startOk: 30 s, the first press; it leaves the SEGA logo for the reload tutorial (29.96 s, 2674 verts; 2D layers tex 452800 at W 0.5 and tex 53a800 at W 2.0), then the gun adjustment screen (a target and a countdown) from ~32 s, all 2D — evidence: cm-timeline `00:29:877 input\udp_lightgun.cpp:110 N[INPUT]: Light gun P1 at 320,240: trigger 0 reload 0 start 1 d-pad 0 B 0 2D 0`, `00:31:978 ... verts 384 overlay 380  W near 0.998 p10 0.998 median 1.000` (2026-10-21)
- briefing: the mission briefing with its skip prompt at ~38 s (2D layer tex a47800 at 39.25 s) — evidence: cm-timeline `00:38:001 ... verts 24 overlay 16  W near 0.100` (2026-10-21)
- cutscene: 40 s, the stage 1 intro (museum exterior, subtitles); the presses at ~40, 42 and 44 s may have skipped parts of it — evidence: cm-timeline `00:39:995 ... verts 5873 overlay 108  W near 0.100 p10 9.753 median 74.406 p90 132.785`, shot_0040 (2026-10-21)
- stage1: 44 s — evidence: cm-timeline `00:43:610 rend\vr_reproject.cpp:669 N[RENDERER]: VR: 2D layer at W 1.000 (17 verts at -1,30) put on the 2D plane`, `00:44:012 ... verts 5270 overlay 300  W near 0.300 p10 0.998 median 25.922 p90 38.488`, window shot t050 (2026-10-21)
- first centre shot (unprofiled): OP polys tex 5e8000 at W 1.000, all 2D, 6 px off — evidence: cm-timeline `VR: shot` lines at 52.9 s (2026-10-21)
- after the game: the mission-failed screen ~70 s, the game-over screen ~75 s, an advisory screen ~80 s, light streaks 82-84 s, the SEGA logo 86-90 s, the developer logo 92-94 s — evidence: cm-timeline frames (2026-10-21)
- attract demo (a surveillance-camera picture), 98-122 s: flat, 232-240 verts all at W 0.100 (a framed picture, no 3D in the main pass) — evidence: cm-timeline `VR: focal` lines 98-122 s (2026-10-21)
- title 124-128 s, the advisory screen 130-132 s — evidence: cm-timeline frames (2026-10-21)
- attract3d: 136 s, the how-to-play scene, the first attract scene drawn in 3D — evidence: cm-timeline `02:16:398 ... verts 3658 overlay 124  W near 0.100 p10 0.875 median 63.250 p90 107.000`, window shot t140 (2026-10-21)
- howto: 136 s (the same scene as attract3d) — evidence: as attract3d (2026-10-21)
- accept-hotd2 timeline: the title's start prompt 54.5 s, menu after the 56 s press, Arcade mode from the 60 s press (the 62 s press falls in the fade), intro ~63-85 s, gameplay ~88-100 s, the continue countdown ~100 s; presses after the game starts pause it. The first boot without a save shows a no-save notice until a press and puts the title near 58 s — evidence: hotd2-explore3, accept-hotd2 frames (2026-10-21)
- accept-mok timeline: the rod calibration screen ~35 s, item select ~40-56 s, story ~57-86 s, then the maze from ~86-91 s (it varies by run): its first seconds take no shot (no effect drawn for trigger pulls at 91.9 s and 93.8 s in two runs), the camera walks to ~100 s, then the first fight (hits from ~103 s). Without centre shots on the item screens their timers run out and the maze comes ~20 s later (~110 s) — evidence: accept-mok runs, window shots t035/t060/t090/t110, `vrcheck shots` without `--from` (2026-10-21)

## Baselines

The unmodified build, each game with its profile's own fovScale (HOTD2 3, MOK 2). Verts p95 of
the widened gameplay is the frame-rate budget reference: both hold 72 Hz on Quest 3.

- hotd2 expected lines: `VR: camera profile for MK-51002`, `VR: field of view 41.1 -> 96.7 degrees at 8c02998e`, `VR: field of view 41.1 -> 96.7 degrees at 8c02b0ec` — evidence: accept-hotd2 `00:00:392 rend\vr_reproject.cpp:427 N[RENDERER]: VR: camera profile for MK-51002`, `00:00:450 rend\vr_reproject.cpp:134 N[RENDERER]: VR: field of view 41.1 -> 96.7 degrees at 8c02998e` (and `8c02b0ec`) (2026-10-21)
- hotd2 live focal: 213.4 (240 / tan(96.7 / 2)), stock 640 — evidence: `vrcheck focal accept-hotd2\flycast.log --expect 213.4`: `count 42`, `focal min 213.4 median 213.4 max 320.0` (2026-10-21)
- hotd2 widened gameplay verts p95: 13904 (13888 in an earlier run; 86-100 s) — evidence: `vrcheck focal accept-hotd2\flycast.log --from 86 --to 100`: `count 7`, `verts p50 13521 p95 13904 max 13904`, `wnear 0.001 x7` (earlier run `verts p50 13534 p95 13888 max 13888`); whole run `verts p50 24 p95 13572 max 14892` (2026-10-21)
- mok expected lines: `VR: camera profile for THE MAZE OF THE KINGS`, `VR: the game's field of view widened (code at 8c08d8ee)` — evidence: accept-mok `00:00:429 rend\vr_reproject.cpp:427 N[RENDERER]: VR: camera profile for THE MAZE OF THE KINGS`, `00:11:469 rend\vr_reproject.cpp:475 N[RENDERER]: VR: the game's field of view widened (code at 8c08d8ee)` (2026-10-21)
- mok widened maze verts p95: 9381 (8152 and 9563 in other runs, with fewer or more enemies; focal 240.0) — evidence: `vrcheck focal accept-mok\flycast.log --from 88 --to 112 --expect 240`: `count 12`, `verts p50 6870 p95 9381 max 9381`, `wnear 1.000 x12` (earlier run `verts p50 7956 p95 8152 max 8152`); mok-explore `--from 90 --to 118`: `verts p50 7047 p95 9563 max 9563` (2026-10-21)
- mok shot effects at the gun: a miss draws starburst tex 50b800 then red stars tex 4f6800, a hit draws a spark tex 4e9000, all at W 1.083 in the log (after the 2D snap), first in the frame 41-70 ms after the trigger. They stay where they hit, on a wall or an enemy, so they drift off the gun while the camera walks (2-3 px in their first frame, up to 8 px within 150 ms) or the enemy moves (a spark 5 px off in its first frame at 112.9 s); with the camera standing and no enemy near (101.5-111 s) every centre shot is 0-1 px — evidence: `vrcheck shots accept-mok\flycast.log --tex 50b800,4f6800,4e9000 --max-off 2 --from 101.5 --to 111`: `shots 4 matched 4 worst 1 px off` (three runs: worst 1 px each); walking shots at 96.4 s and 98.9 s: starburst 3 -> 5 px and 2 -> 3 px, red stars 6-8 px; a run checked to 116 s: `shots 6 matched 6 worst 5 px off` (2026-10-21)
- mok matrices (tool proof): `viewport 0x0F24D0 320.0 -240.0` and `proj 0x0E7A60 focal 415.7 415.7` in the story, `240.0 240.0` in the maze (the 0.75A patch: story 40 -> 60 degrees, maze 60 -> 90); others found: viewports 0x0C2FBC (+240), 0x291B28; projections 0x0C60B4 9.6, 0x0C6668 96.0, 0x0C66A4 192.0, 0x0F25F8 (a copy), 0x291C40 and 0x291F60 641.9 — evidence: `vrcheck matrices` on mok-explore rip ram0000 (story), ram0001/ram0002 (maze) and accept-mok rip ram0000 (maze) (2026-10-21)
- mok stock focal (tool proof): `0x0E7248 415.723` in the maze, absent in the story — evidence: `vrcheck find-float mok-explore\rip\ram0001.bin 415.72` (and ram0002) (2026-10-21)
- mok rip headers: focal 415.72 (story) / 240.0 (maze), hudw 1.082, stock 415.72 — evidence: mok-explore rip pass0000-0002 (2026-10-21)
- mok W layers (tool proof): story `W 1.040 polys 1 list tr tex d99000 box 159,364..486,403` (subtitles), `W 1.082 polys 18 list tr tex 5e6000,606000 box 5,7..130,461` (HUD), `W 1.090 polys 2 list tr tex none box 0,0..640,479` (letterbox); maze adds `W 1.053 polys 4 ... tex 606000` (area name) and shot effects at W 1.501-1.504 (tex 4e9000, 4ed800, 523000) — evidence: `vrcheck whist` on mok-explore pass0000 and pass0002 (2026-10-21)
- mok border (tool proof): no cliff with fovScale 2 (the 4.0 cull margin) — evidence: `vrcheck border mok-explore\rip\pass0002.bin`: densities 1630.0 / 235.0 / 195.0 / 100.0, `cliff ratio 0.83`; pass0001 (an empty corridor) `cliff ratio unknown (no polygons at 0.8-1.0)` (2026-10-21)

## Camera

## Field of view

## 2D plane

## Gun

## Player 2

## Acceptance

- baselines on the unmodified build: `PASS hotd2`, `PASS mok`, exit 0 — evidence: `powershell -File hotd2-vr\accept.ps1 -Only hotd2,mok` (accept-hotd2 `focal min 213.4 median 213.4 max 320.0`; accept-mok `shots 4 matched 4 worst 1 px off`, `viewport 0x0F24D0 320.0 -240.0`, `proj 0x0E7A60 focal 240.0 240.0`) (2026-10-21)

