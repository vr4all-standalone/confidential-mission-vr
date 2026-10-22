# Adding a game to hotd2-vr (recipe, from The Maze of the Kings)

Researched 2026-10-21 on branch `naomi-gun-vr` = `hotd2-vr` @ `eeb629dce` (30 commits over
`origin/master`). Template commit: `eb2e300cd` "The Maze of the Kings (NAOMI), with the hero's staff".
Next target: **Confidential Mission** (NAOMI GD-ROM, MAME `confmiss`).

Marks: **[V]** read in the code / git history; **[I]** my inference, not checked by running anything.
Line numbers are for `eeb629dce`.

---

## 1. What eb2e300cd changed, file by file

| File | Change | Kind |
|---|---|---|
| `core/rend/vr_reproject.cpp/.h` | Profiles went from a flat array to `struct GameProfile` built by `makeProfiles()`; new fields `fovPatch` (code words), `stockFocalAddr`, `focal`, `pokes`, `markerMinW/MaxW`, `p2PromptBox`, `snapMaxW/snapStep`, `letterboxW`, `hudW`, `flatScreens`, `unitScale`, `model`. New `applyProfileWrites` (vblank), `widensByCode`, `profileCodeThere`, `stockFocal`, `worldScale`, `gameModelSource`, `snapFlatScreen`, `keepMixedIn3D`. MOK profile data. | generic capability + MOK data [V] |
| same file, `readFloat/writeFloat` | RAM mask `0x00FFFFFF` -> `0x01FFFFFF` (NAOMI has 32 MB RAM) | fix [V] |
| same file, `hidePlayer2Prompt` | hard-coded bottom-right corner -> profile `p2PromptBox` | generic [V] |
| same file, `dropShotMarker` | marker W band also from profile (`markerMinW/MaxW`); `snapFlatScreen` before `snapNear2D`; `keepMixedIn3D` (3D polygons with some corners at the 2D plane's W stay 3D) | generic, motivated by MOK [V] |
| `core/hw/naomi/naomi_cart.cpp:251` | `naomi_cart_LoadBios(path)` instead of the bare file name: a merged MAME zip's own BIOS files are found | fix [V] |
| `core/hw/naomi/gdcartridge.cpp:514-529` | GD-ROM may also be `<set>/<gds>.cue` or `.gdi`, not just `.chd` | generic capability [V] |
| `core/cfg/option.cpp/.h` | `vr.StaffHand` (MOK only), `vr.DebugShots` (PC test aid) | MOK option + generic tool [V] |
| `core/rend/vr/debug_shots.cpp` (new) + `core/rend/CMakeLists.txt` | `vr.DebugShots = N`: `shot_NNNN.png` every N s of game time into the writable data dir | generic tool [V] |
| `core/rend/vr/hands_rip.cpp/.h` | `hands::Source {kind, file, pistol, staff}`; one model file per game (`modelPath()`); a second run type: attract demo with no input (`StaffRun = 33 s`, give up 150 s) | generic capability + MOK constants [V] |
| `core/rend/vr/hands_build.cpp/.h` | `StaffParts`, `StaffRodPolys = 66`, `buildStaff()` + helpers (`rods`, `uvIn`, `cutByNearPlane`, `staffFrom`, `write`, `clip(axis)`); MOK constants (`NearW = 1.0`, glove/head UV boxes) at `hands_build.cpp:49-68` | MOK-specific on generic helpers [V] |
| `core/rend/vr/xr_hands.cpp/.h` | load the running game's model (again when another game starts); `travel = 0` = no slide; staff gloves hidden unless `vr.StaffHand` | generic + MOK [V] |
| `core/rend/vr/xr_host.cpp` | `worldScale()` instead of `vr.WorldScale`; aim uses `cam.stockTan` (profile's stock focal) instead of `vr.Focal`; no slide rack when `travel == 0`; recoil tips less for long weapons; prep-panel strings chosen by model kind | generic + MOK strings [V] |
| `core/rend/vr/xr_gun.h` | comments only | - |
| `VrGames.java` | `ARCADE` table (MAME set, title, GD image), `arcadeTitle()`, `isArcadeFolder()`, list of all games | generic + MOK row [V current state; diff not read line by line] |
| `VrSetupActivity.java` | "Play <title>" button per game; ends the game process; starts `MainActivity` with `EXTRA_GAME`; MOK in help text | generic + MOK text [V] |
| `BaseGLActivity.java` | `findVrGame()`: picked game, else the only one, else the setup panel | generic [V] |
| `src/vr/AndroidManifest.xml` | setup panel in `android:process=":setup"` | fix [V] |
| `.gitignore` | `hands*.bin`, `staff*.bin`, `HND1*.bin` (ripped Sega data) | hygiene [V] |
| `README.md`, `ROADMAP.md` | MOK section, roadmap | docs |

Later MOK touch-ups: `550b830da` (gloves mirrored, `xr_hands.cpp`), `af4124cb4` (MOK `unitScale = 4.4f`
removed when `vr.WorldScale` default went 0.025 -> 0.11; place2DLayers notes MOK walls), `3dcb03178`
(clipFarOut: clamped near-plane corners; MOK pins W to 1.0, HOTD2 to 0.001), `789dfa7bc` (netplay
for MOK; `netplayDigest` hashes profile writes) [V].

---

## 2. Every game-specific spot (current code)

How games are told apart: `settings.content.gameId` [V]
- Dreamcast: IP.BIN product number (`core/emulator.cpp:857-862`), e.g. `MK-5100250`.
- NAOMI: boot-ID title, `trim_trailing_ws(bootId.gameTitle)` (`core/hw/naomi/naomi_cart.cpp:647-667`,
  logged as `NAOMI GAME ID [...]`); fallback MAME name (`:449`). Only trailing whitespace is trimmed:
  upstream matches Confidential Mission as **`" CONFIDENTIAL MISSION ---------"`** (leading space,
  dashes; `core/hw/maple/maple_jvs.cpp:1527`) [V]. Confirm with the log line before using it [I].

| Where | Keys on | Holds |
|---|---|---|
| `core/rend/vr_reproject.cpp:242-350` `makeProfiles()` | `gameId` | all per-game numbers (section 3) |
| `vr_reproject.cpp:354-360` `gameModelSource()` | `gameId` + `model.kind != None` | which model to rip/draw |
| `vr_reproject.cpp:365-429` `applyGameProfile` (Event::Start) | `gameId` | overrides `vr.ProjAddr/ViewportAddr` (only if 0!), `vr.Focal`, `vr.HudW` (only if 1.0), `vr.FovScale` (headset/netplay only); plugs DC light guns (`isConsole()` only) |
| `core/rend/vr/hands_rip.cpp:44-55` | model kind | run timings: HOTD2 Start presses / `Story3D=600` / 115 s; MOK `StaffRun=33`, give-up 150 s |
| `core/rend/vr/hands_rip.cpp:265-275` `legacyModelPath` | `AgentsPistol` | old `files/hands.bin` |
| `core/rend/vr/hands_build.cpp:49-68` | (one game each) | HOTD2 pistol / MOK staff geometry constants, `NearW = 1.0` |
| `core/rend/vr/hands_rip.h:31-37` | `Kind {None, AgentsPistol, HerosStaff}` | name strings |
| `core/rend/vr/xr_hands.cpp:174-175, 340-343` | `HerosStaff` | gloves mirrored, fist hidden unless `vr.StaffHand` |
| `core/rend/vr/xr_host.cpp:1937-1958` | `HerosStaff` else HOTD2 | prep panel title/text (binary choice) |
| `core/rend/vr/xr_host.cpp:509` | - | OpenXR app name "VR Light Gun Hub" (cosmetic) |
| `core/cfg/option.cpp:105,115` | - | `vr.WorldScale 0.11` (HOTD2 camera 15 units = eye height), `vr.StaffHand` |
| `core/cfg/option.cpp:81` | - | `vr.Focal` default **640** = HOTD2's stock focal; `vr.HudW` default 1.0 = HOTD2's HUD W |
| `VrGames.java:55-57` `ARCADE` | MAME zip name | `{ "mok", "The Maze of the Kings", "gds-0022" }` |
| `VrGames.java:125` `score()` | file name contains "hotd" | sort order |
| `VrSetupActivity.java:57, 79-81` | - | title and help text naming HOTD2 + MOK |
| `.gitignore` | file name | `hands*.bin`, `staff*.bin` |
| `hotd2-vr/run-test.ps1` | - | HOTD2 PAL `vr.ProjAddr=5006816, vr.ViewportAddr=5007112` baked in; picks `game\*.cue/gdi/chd` |
| `hotd2-vr/rip-hands.ps1` | - | HOTD2 texture `5fe380`, Start timings 44-58.5 s |
| `hotd2-vr/netplay-test.ps1:28,52,65` | `-Game pal/us/mok` | MOK at `<data>\naomi\mok.zip`, route timings |
| `core/hw/maple/maple_jvs.cpp:1526-1537` (upstream) | NAOMI gameId | light-gun I/O board (section 4) |
| `README.md` | - | per-game sections |

Generic thresholds tuned on HOTD2 (no profile field): `MarkerMaxSize 96`, `MarkerMaxOffset 120`,
`MarkerMs 500`, `FlashMs 200` (`vr_reproject.cpp:575-580`), `SnapMinW 0.75` (`:631`), the
`0.85..1.15` marker W band (`:1590`), overlay tolerance `0.002` (`onOverlay`) [V].

### Checklist: to add game X you must touch
1. `vr_reproject.cpp` `makeProfiles()`: a new `p = GameProfile(); p.gameId = "<exact id>"; ...; list.push_back(p);`
   At minimum `projAddr`, `viewportAddr`, and a stock focal (`focal` and/or `stockFocalAddr`); without
   them `vr.Focal` stays 640 (HOTD2's) and both the 2D plane framing and the aim are wrong [V `stockFocal()` :520].
2. `VrGames.java` `ARCADE`: `{ "confmiss", "Confidential Mission", "gds-0001" }`. Without it
   `confmiss.zip` is reported as "packed, unpack it" (`:172`) and `confmiss/gds-0001.chd` is listed as a
   Dreamcast disc "gds-0001" that won't boot [V logic; I outcome].
3. `VrSetupActivity.java:79-81` help text, `README.md` section (setup layout, profile story).
4. Optional model: `hands::Source` kind + build function + run logic + `xr_hands`/`xr_host` strings +
   `.gitignore` pattern for its `.bin` (section 5). Skip for a first version (arcade gun).
5. Optional: `run-test.ps1`/`netplay-test.ps1` game switch; `vr.*` option only if the game needs one.
6. Nothing in `maple_jvs.cpp` for confmiss: upstream already sets up its gun board [V].

---

## 3. The profile (`struct GameProfile`, `vr_reproject.cpp:199-261`)

All RAM offsets are `address & 0x01FFFFFF`, accessed at `0x8C000000 + offset` (NAOMI `0x0C0E7A60` ->
`0x0E7A60`) [V :44-58]. Code words are 16-bit little-endian.

| Field | Meaning |
|---|---|
| `gameId` | exact `settings.content.gameId` |
| `projAddr`, `viewportAddr` | 4x4 float matrices; live focal = `abs(vp[0]*proj[0])`, `abs(vp[5]*proj[5])` (`readGameFocal` :64-76; offsets 0 and 20 bytes). Applied only if the config value is 0. |
| `fovScale` | how much wider (tan of half angle) for `fovLiterals` / viewport scaling; for a `fovPatch` game only a "widen" flag (>1) |
| `fovLiterals[4]`, `fovStock` | RAM offsets of 16-bit angle literals (65536 = 360 deg), rewritten each vblank while they still hold `fovStock` (`widenGameView` :112-150). HOTD2 style. |
| `fovPatch[8]` `{offset, stock, patched}` | code words; written all-or-nothing once every word is stock (`applyProfileWrites`), only with `vr.WidenFov` and `vr.FovScale > 1`; the list ends at the first `offset` 0. MOK style (4 words); CM uses all 8, three of them unchanged guard words (`stock == patched`) that make sure the patch lands only on the code it expects. Also gates pokes (`profileCodeThere`). |
| `stockFocalAddr` | float in RAM: the focal the game's light-gun hit test unprojects with (may change per scene); read by `stockFocal()` -> `GameCamera::stockTan` -> aim (`xr_host.cpp:919`) |
| `stockFocalInverse` | the float at `stockFocalAddr` is 1/focal, not the focal (default off). CM: `tan(A/2)/320` at 0x0C154ED0. |
| `focal` | stock focal constant -> overrides `vr.Focal` (fallback for the above, and the 2D plane's stock framing) |
| `pokes[12]` `{offset, u16}` | written every vblank when different, once the code is there |
| `markerTextures[4]`, `markerMinW/MaxW` | the game's shot marker / flash (VRAM tex addresses, W band); hidden after shots into 3D within 120 px / 500 ms (`dropShotMarker`) |
| `markerWidened` | the game draws its marker and impacts where its widened view puts the shot, not at the gun (its hit test keeps the stock view): `shotDrawnAt` maps the gun by live/stock focal (default off: at the gun). CM. |
| `noShotFlash` | the game draws no shot flash: `vr.DropShotFlash` never applies, so a bright full-screen 2D quad after a shot stays (default off). CM, whose cross-fade (tex 404800) the flash rule took for one. |
| `p2PromptTextures[4]`, `p2PromptBox` | player 2's start prompt sprites and their screen box (640x480 px). Hidden (`vr.HideP2Prompt`) and used as "P2 not playing"; **dual wield only works when set** (`player2PromptKnown()`, `xr_host.cpp:1450`) |
| `p2HideTrailing` | while the prompt shows, flat polygons on its line just after it are hidden too (HOTD2's credit count) (default on). CM: off, nothing follows its prompt in free play. |
| `snapMinW`, `snapMaxW`, `snapStep` | 2D layers between W `snapMinW` (default 0.75) and `snapMaxW` on multiples of `snapStep` go onto the 2D plane (`snapNear2D`). CM: 0.005 to 10 in steps of 1/16000. |
| `offScreen2DMaxW` | 2D layers nearer than this W are snapped even partly or wholly off the screen; `cropPlaneToScreen` hides those wholly off it and cuts axis-aligned quads to it (default 0: none). Keep it below the game's near plane. CM: 0.3 (its caption ticker). |
| `farOutBelowW` | `clipFarOut` cuts a 3D polygon reaching more than 2.5 screens out only when such a corner is nearer than this W, a pinned corner (default 0: whatever its W). CM: 0.299, just under its near plane (W 0.300). |
| `letterboxW` | W of black letterbox bars (0: the 2D plane's) (`hideLetterbox`) |
| `letterboxTextures[4]` | textures that make a polygon at `letterboxW` a bar whatever its colour (default none: opaque black ones only). CM: 400680 (white bars drawn black by it). |
| `sightTextures[8]`, `sightMinW/MaxW` | the game's sights over its enemies and their W band, put on the 2D plane before the snap (`placeSights`; default none). CM: 400000 at W 0.95-1.005. |
| `hudW` | W of its 2D plane (0: `vr.HudW` = 1.0). Must never be a W its 3D uses: the vertex shader pins anything at it onto the plane (`gles.cpp:139`) |
| `flatScreens` | a pass with no 3D (menus in layers of depth) goes wholly onto the 2D plane (`snapFlatScreen` :879) |
| `unitScale` | game unit size vs HOTD2's (metres = `vr.WorldScale * unitScale`, `worldScale()` :513). HOTD2 camera ~15 units over the street. |
| `model` | `hands::Source` (kind, file, texture addresses) |

A game that clips its 3D at its own near plane (no W below it in a rip) should set `farOutBelowW` just under that plane: its corners far outside the screen are then real 3D beside the view (CM's floor reaches 88.6 degrees below the camera's axis), and cutting them leaves the floor missing in a headset. To see it on the PC, give the window a Quest 3 eye's view (`config:vr.WindowTanX=1.2799,config:vr.WindowTanY=1.1106`, 104 x 96 degrees) and measure the blank with `vrcheck.py blank` (accept.ps1's `cm-floor`).

### MOK's values (worked example) [V `vr_reproject.cpp:300-349`]

```
gameId         "THE MAZE OF THE KINGS"
projAddr       0x0E7A60   (RAM 0x0C0E7A60: diag f/320, f/240)
viewportAddr   0x0F24D0   (diag 320, -240)
fovScale       2          (flag only; the patch widens by a fixed 1.5x angle)
fovPatch[0]    0x08D8EE  1F41 -> 6D43   mov.l r4,@(4,r15) -> mov r4,r13
fovPatch[1]    0x08D8F2  5DF1 -> 4409   mov.l @(4,r15),r13 -> shlr2 r4
fovPatch[2]    0x08D8F6  4D21 -> 3D48   shar r13 -> sub r4,r13     => half angle 0.75A instead of A/2
fovPatch[3]    0x08DC4E  3F00 -> 4080   high half of float at 0x8C08DC4C: cull margin 0.5 -> 4.0
stockFocalAddr 0x0E7248   (240/tan(A/2), kept stock by setFovAngle 0x8C0746B0)
focal          415.72     (the maze: 60 deg -> 240/tan 30)
pokes          u16 at 0x0E2058 + 12*player + 2*k = {0,0,512,0,0,512} for players 1,2
               (gun calibration "ROD CONTROLLER SETTINGS", factory 64,128,500,0,128,500)
markerTextures 0x50b800 (starburst), 0x4f6800 (red stars); markerMinW/MaxW 1.45 / 1.55
p2Prompt       0x68d800, 0x48c000 in box {250,440,390,462} ("PRESS 2P START", bottom centre)
snapMaxW/Step  1.6 / 0.001;  letterboxW 1.090;  hudW 1.082;  flatScreens true;  unitScale 1
model          HerosStaff, "staff-mok.bin", scenes {rod 0xC1C000, body 0xCB0800},
               {rod 0xD87000, body 0xDA9800}
```
(SH4 decodings above are mine, checked against the encodings `0110nnnnmmmm0011` mov,
`0100nnnn00001001` shlr2, `0011nnnnmmmm1000` sub, `0100nnnn00100001` shar.) MOK facts from the
comments: FOV 60 maze / 40 story / 50 menus / 80 attract; setPerspective `0x8C074630`, setFovAngle
`0x8C0746B0`, builder `0x8C08D8D0`; culling `0x8C084BB0` + node loops `0x8C0912A0/0x8C0923C2/0x8C09916E`,
margin copied to `0x0C0F2550/54`; near plane exactly W 1.0; HUD W 1.082, subtitles 1.040, area names
1.052-1.054, CONTINUE 1.08-1.085, damage flash 1.4, shot effects 1.5, letterbox 1.090 [V comments].

HOTD2 PAL for contrast: proj `0x4C65E0`, viewport `0x4C6708`, `fovScale 3`, `fovLiterals 0x029C12,
0x02B370`, `fovStock 7484` (41.1 deg, routine `0x8C0383C0`), markers `0x59bb80, 0x587380` at W 0.9-1.01,
P2 `0x57b700, 0x56bb00`, `AgentsPistol` `hands.bin`; focal = default 640, HUD W = default 1.0 [V].

---

## 4. Light gun on NAOMI

Path [V]:
1. Headset: `xr_host.cpp` `updateGun` (:1158-1334) ray-casts into the rebuilt scene (`aimAtScene` :779),
   puts the hit on screen with the **stock** framing (`cam.stockTan`, :919; `vr.AimWidened` off), and
   calls `lightgunSet(player, x*10000, y*10000, buttons)` (:1334). Off-screen / reload: `OffScreen = -10000`
   with button bit 2. Trigger outside the stock view but inside the widened one: nothing (grey ring).
2. `core/input/udp_lightgun.cpp:79-112` `lightgunSet`: `mo_x_abs/mo_y_abs` (640x480), and kcode bits:
   1 trigger -> `DC_BTN_A`, 2 -> `DC_BTN_RELOAD`, 4 -> `DC_BTN_START`, 8/16/32/64 d-pad, 128 -> `DC_BTN_B`;
   bit 256 = shot at the 2D plane (marker kept). Calls `vr::noteGunShot` for players 1-2. Same entry for
   UDP (`vr.UdpPort`, text `LG <player> <x> <y> <buttons>`).
3. `mo_x_abs` -> `MapleInputState.absPos` (`core/network/ggpo.cpp:55`).
4. NAOMI JVS (upstream): `naomi_button_mapping` (`maple_jvs.cpp:39-59`): A -> `NAOMI_BTN0` (trigger),
   B -> `NAOMI_BTN1`, Start -> `NAOMI_START`, RELOAD -> `NAOMI_RELOAD`. In `read_digital_in` (:250-261)
   reload also sets BTN0, and with `lightgun_as_analog` a trigger off-screen (or reload) sets **BTN1 =
   "offscreen sensor"**. Analog read 0x22 (:2299-2320): x = `absPos.x*0xFFFF/639`, y likewise; 0,0 when
   off-screen or reloading.
5. The game applies its own calibration to those raw values -> MOK needed the neutral-calibration pokes
   ("kept neutral": 0,0,512 x2) else aim stretched up to 30 px off-centre [V comment :311-314].
6. DC-only: `applyGameProfile` plugs `MDT_LightGun` into ports A/B only for `isConsole()` (:410-424);
   "An arcade game's guns are on its I/O board already" [V].
7. Player 2 / dual wield: left ≡ joins if `vr.DualWield`, both controllers tracked and
   `player2PromptKnown()`; leaves when the P2 prompt is seen again after 4 s (`xr_host.cpp:1435-1455`) [V].
   Player 2 = `lightgunSet(1, ...)` -> JVS player 2 on the same board [I, works for MOK per README].

Upstream setup for **confmiss** [V]:
- `naomi_roms.cpp:5059-5075`: `"confmiss"`, no parent, BIOS `naomi`, GD, ROT0, key
  `317-0298-com.pic` (0x4000, crc 0x15971bf6), disc `gds-0001`, `&lightgun_inputs`, `confmiss_eeprom_dump`.
- `maple_jvs.cpp:1526-1537`: same branch as MOK, Death Crimson OX, Lupin Shooting and arcade HOTD2:
  one `jvs_837_13551` board, `lightgun_as_analog = true`, `settings.input.lightgunGame = true`.
- `naomi_roms_input.h:197-204` `lightgun_inputs`: TRIGGER (BTN0), RELOAD, Start, base buttons. Same as MOK.
- `naomi_roms_eeprom.h:127` default EEPROM (no "gun calibrated" note, unlike lupinsho at :138) [V];
  whether it has free play/calibration values set: unknown [I].
- Other targets: Lupin `"LUPIN THE THIRD  -THE SHOOTING-"` and DCOX `"DEATH CRIMSON OX"` same board
  (DCOX ignores BTN1); arcade HOTD2 `gameId.substr(0,5) == "hotd2"`; **Ninja Assault `"NINJA ASSAULT"`
  uses `jvs_namco_jyu`** (different gun path, :1519-1525) [V].

Things that assumed MOK's/HOTD2's I/O:
- B/Y ("skip story") sends `DC_BTN_B` -> `NAOMI_BTN1` = the off-screen sensor bit; harmless in MOK [I],
  unknown in CM [I].
- "Trigger + B + Start = reset" guard (`xr_host.cpp:1310`) is a Dreamcast HOTD2 thing [V].
- Calibration pokes are MOK addresses; CM needs its own (or none if its default is neutral) [I].
- On NAOMI the `hands_rip` runs send `lightgunSet(0, OffScreen, OffScreen, ...)` every vblank (fine) [V].

---

## 5. Weapon model / hands

[V] `hands::Source` in the profile picks a kind and a file (`hands_rip.h:31-37`). On Event::Start
(`hands_rip.cpp:96-142`): no source or `vr.GameHands` off -> nothing; model file exists -> load; else
(headset, or PC with `vr.HandsPrep`, never online) fast-forward a run behind a panel:
- HOTD2 `AgentsPistol`: press Start every 2 s while <600 3D polys/frame (notices, title, menu), then hands
  off; the agent loses and kneels at ~115 s; give up 330 s. `observePass` takes the first pass with the
  pistol texture (>= `MinGunPolys`) and `build()` cuts hands, cuff, pistol, slide (painted line).
- MOK `HerosStaff`: no input; attract demo; first pass with a whole rod (66 polys of the rod texture),
  ~33 s; give up 150 s; `buildStaff()` (rod 514 corners, gloves from the body texture's UV box, shortened
  to ~1 m, head forward).
- Model file in the writable data dir (`modelPath()` :258); each game its own; `.part` then rename.
- `xr_hands` draws it; `travel == 0` means no slide (reload by shooting off-screen).

New game needs: a scene where the weapon/hands are whole and in front of the near plane, their VRAM
texture addresses (rip), a new `Kind` (+ `name()`), a builder in `hands_build.cpp` (or reuse `build()` if
the parts match HOTD2's: two hands, cuff, pistol with a slide line - unlikely [I]), run logic in
`hands_rip.cpp` (input script, timings, detection), strings in `xr_host.cpp:1937-1958`, a `.gitignore`
pattern. Confidential Mission is first-person with no visible weapon in play [I]; a model would have to
come from attract/cutscenes.

**Fallback** [V]: `model.kind = None` -> `gameModelSource()` returns nullptr -> no run, no panel, and
the Namco-style arcade gun (`xr_gun.*`, `gun_model.h`, CC BY 4.0) is drawn (`xr_hands.h` header).
Laser, recoil, muzzle flash, smoke, haptics, two guns all work without it. HOTD2's first release
(`3bf61260d`) shipped with only the arcade gun; the agent's hands came later (`4257a290c`). **Good
enough for a first CM version** [I]; reload = shoot off-screen (or point at the floor with two guns).

---

## 6. Launcher (Android)

[V] `VrGames.scan()` (`VrGames.java`): looks in `files/games` (internal and external app dirs, depth 1),
then with All files access in `Download` (depth 3) and the shared storage root (depth 2), skipping
`Android`, `DCIM`, etc. Images: `.cue .gdi .chd .cdi` (a `.cue/.gdi` only if its tracks are next to it);
archives `.zip .7z .rar`. An archive named `<set>.zip/.7z` for a set in `ARCADE` counts as a game if
`<dir>/<set>/<gds>.chd|.cue|.gdi` exists (`arcadeTitle` :132); images inside a folder named like a set
are skipped (`isArcadeFolder` :204). Other archives -> "unpack it". Discs first (HOTD by name), arcade
sets after.

Layout for CM on the headset (from MOK's README, by analogy) [I]:
```
Download/confmiss.zip          (317-0298-com.pic; BIOS inside it or as naomi.zip next to it)
Download/naomi.zip
Download/confmiss/gds-0001.chd (or .cue/.gdi)
```
BIOS: `naomi_cart_LoadBios(path)` opens the game zip (+ parent) and loads BIOS `naomi` from them,
falling back to Flycast's normal BIOS lookup (`naomi_cart.cpp:168-212`) [V]; "naomi.zip next to it"
works per README [V README].

Handoff [V]: setup panel button -> `VrSetupActivity.play()` kills the game process, starts
`MainActivity` with extra `hotd2vr.game` = absolute path (`:161-171`); `BaseGLActivity.findVrGame()`
(:209) takes that extra, else the only game found, else shows the panel; `JNIdc.setGameUri(game)`
(:177). Native: Flycast loads the zip as a NAOMI MAME set, sets `gameId` from the boot ID, Event::Start
-> `applyGameProfile`. Test aid: `adb shell am start ... --es hotd2vr.config "config:vr.X=..."`
(`BaseGLActivity`, launch config overrides) [V].

To add CM: one `ARCADE` row, the help text. Nothing else in Java [V].

---

## 7. Reverse-engineering recipe (PC), as done for MOK

Sources: README "How it was worked out", profile comments, commit messages, `hotd2-vr/` tools. Steps
marked [I] are my reconstruction of how to apply it; the tools and config keys are [V].

### 7.0 Setup
- `hotd2-vr\build-win.cmd` -> `build-win\flycast.exe`. Python + `pip install capstone numpy pillow`.
- Data layout like `netplay-test.ps1` uses for MOK: `<data>\naomi\confmiss.zip`,
  `<data>\naomi\confmiss\gds-0001.chd`, `<data>\naomi\naomi.zip`; `<data>` = folder above the repo or
  `%HOTD2VR_DATA%`. Run from `<data>\run` (rip files are relative to the working dir).
- Base command (what the scripts pass; `pvr.rend=0` OpenGL + native depth are required by the hooks):
```
cd <data>\run
..\hotd2-vr\build-win\flycast.exe -config "config:pvr.rend=0,config:rend.NativeDepthInterpolation=yes,config:vr.Reproject=yes,config:aica.Volume=0,log:LogToFile=yes" "<data>\naomi\confmiss.zip"
```
- Do **not** use `run-test.ps1` as is: it forces HOTD2's `vr.ProjAddr/ViewportAddr` (which beat any
  profile, `applyGameProfile` only fills zeros) and picks a `.chd` (the GD image, not the zip) [V].

### 7.1 Game ID and timeline
1. `flycast.log`: `NAOMI GAME ID [ CONFIDENTIAL MISSION ---------] ...` -> `p.gameId` [V log line].
2. Add `,config:vr.DebugShots=5` -> `shot_0000.png, shot_0005.png ...` in Flycast's writable data dir
   every 5 s of game time (window may be minimised) [V]. Map boot, attract, title, first stage, cutscenes.
3. Scripted input: `,config:vr.UdpPort=27016`, then UDP to 127.0.0.1:27016 `LG 0 5000 5000 4` (Start),
   `LG 0 x y 1` (fire at x,y in 0..10000), `LG 0 -10000 -10000 2` (reload) — `rip-hands.ps1` shows the
   PowerShell loop [V]. Coins/free play: check the test menu if Start does nothing [I].

### 7.2 Rips and RAM dumps (`ripRequested`, `vr_reproject.cpp:1159-1290`)
PC only. Write a file `rip.request` in the working dir: `count interval ram [texhex polys]`.
- `1 1 1` -> `rip\pass0000.bin` (every polygon as the TA got it: x,y in fb pixels, z = 1/W, u,v,
  colours, isp/tsp/tcw/pcw), `rip\vram0000.bin` + `pal0000.bin` (when changed), `rip\ram0000.bin`
  (whole main RAM, 32 MB on NAOMI, base 0x0C000000).
- `20 60 1` -> 20 passes 60 apart; `1 1 0 5fe380 20` -> wait for >= 20 polys with texture 0x5fe380.
- Header `RIP1`: fb size, live focal x/y, `vr.HudW`, `vr.Focal`. Read with `hotd2-vr\assets\riplib.py`
  (`Poly.tex`, `.lst` 0 opaque/1 punch-through/2 translucent) [V].
- From rips: W histogram (HUD W, subtitle/menu W, letterbox W, the near-plane W where many corners are
  pinned, e.g. MOK 1.0), texture addresses of HUD sprites, P2 prompt, shot effects, weapon [I procedure].

### 7.3 Projection and viewport matrices
[I] Scan `ram0000.bin` for the viewport diag (MOK: 320.0 at +0 and -240.0 at +20) and a projection whose
[0]/[5] are f/320 and f/240 (ratio 0.75):
```
python -I -c "import struct,sys;d=open(sys.argv[1],'rb').read();[print(hex(0x0C000000+i)) for i in range(0,len(d)-24,4) if struct.unpack_from('<f',d,i)[0]==320.0 and struct.unpack_from('<f',d,i+20)[0]==-240.0]" rip\ram0000.bin
```
Verify on PC: `,config:vr.ProjAddr=<decimal offset>,config:vr.ViewportAddr=<decimal offset>`; the log's
`VR: focal X x Y ...` (`logSceneStats`) must be plausible and change with scene FOV [V log text].

### 7.4 Field of view (what the game builds its projection from)
1. [I] Find code that references the projection address: search the RAM dump for the 32-bit value
   `0x8C000000|off` or `0x0C000000|off` (literal pools), then
   `python hotd2-vr\re\sh4dis.py 8c08d8d0 8c08d960 rip\ram0000.bin` (shows literal values) [V tool].
2. Look for the angle: SH4 `fsca` takes a 16-bit angle (65536 = turn) in FPUL; a halving (`shar`/`shlr`)
   before it is "A/2"; tan = sin/cos [I]. Trace callers to where A comes from.
   - A loaded from code literals (HOTD2: `0x1D3C` = 41.1 deg at `0x8C029C12`, `0x8C02B370`) -> `fovLiterals`
     + `fovStock`; widening by `fovScale` is computed for you.
   - A from camera data (MOK) -> patch the builder so the half angle is bigger (`fovPatch`, <= 4 words,
     each `stock -> patched`, hand-encoded SH4). Check the stock focal the hit test uses is **not** changed
     by the patch (MOK: setFovAngle still stores 240/tan(A/2)).
3. PC test of the widening: add `,config:vr.FovScale=2,config:vr.WidenFov=yes` (the profile's
   `fovScale` is only applied automatically in the headset or online) -> log `VR: the game's field of
   view widened (code at ...)` or `VR: field of view X -> Y degrees at ...` [V]. Check DebugShots: the game
   draws and culls the wider view; menus/HUD unchanged.

### 7.5 Light-gun hit-test focal and calibration
- [V comment] HOTD2 on the headset: a shot sent at x 410 had its hit effect ~8 deg nearer the centre ->
  the game hit-tests with the stock view. Find the float the gun unprojection uses: MOK `0x0C0E7248` =
  240/tan(A/2). [I] Search the dump for 240/tan(A/2) of the current scene (or the focal read in 7.3 with
  widening off); confirm it changes with scene FOV and stays stock with the patch on -> `stockFocalAddr`;
  also set `focal` to the main-play value.
- Calibration (MOK: test menu "ROD CONTROLLER SETTINGS", 6 u16 per player at `0x0C0E2058/64`). [I] Read
  the test-menu numbers, search the dump for that u16 sequence, poke neutral values (MOK: 0,0,512), and
  verify with widening off: UDP shots at corners/centre must land where sent ("within a pixel" for MOK).
  The PC log prints every shot's nearby 2D polygons: `VR: shot +N ms gun X,Y: ... px off, W a..b, tex T`.
- Pokes are written from the first vblank if the profile has no `fovPatch` (`profileCodeThere` returns
  true) [V] - with a NAOMI GD game that is before the code is loaded; gate them [I].

### 7.6 Cull margin (found in the headset: people beside the camera vanish when turning the head)
[V comment] MOK culls models by bounding sphere against its own frame (0x8C084BB0, node loops), margin
from a 0.5 literal at `0x8C08DC4C` copied by both builders to `0x0C0F2550/54`; patched to 4.0 via the high
half word (`0x3F00 -> 0x4080`) as a 4th `fovPatch` entry; ~1/3 more polygons in the story. [I] Find the
frustum planes the builder writes, then the code reading them with a radius multiplier.

### 7.7 2D plane, HUD, menus, letterbox
PC logs (`dropShotMarker` path, every pass) [V]: `VR: 2D layer at W %.3f (... ) put on the 2D plane`,
`VR: 2D layer at W ... tex ... kept at its W, at the stock framing`, `VR: a screen without 3D (...) put
on the 2D plane`, `VR: kept a 3D polygon with n of m corners at the 2D plane's W in 3D`, census from
`logOverlayCensus` (mixed overlay/3D W). Choose `hudW` = the HUD's W, never the near plane W (MOK fix:
floor streaks); `snapMaxW/snapStep` to catch its other 2D layers; `letterboxW`; `flatScreens` if menus
are drawn in depth layers with no 3D.

### 7.8 Shot marker / flash, P2 prompt
- [V] PC logs every shot: polygons near the gun with tex, W range, size, `DROPPED`; `logShotTextures`
  lists textures that appear within 0.5 s of a shot. Put the marker textures in `markerTextures`, its W
  band in `markerMinW/MaxW`. A bright full-screen 2D flash at the 2D plane's W is dropped by default.
- P2 prompt: rip an attract/one-player frame, take its texture(s) and bbox -> `p2PromptTextures`,
  `p2PromptBox`. This also enables dual wield [V].

### 7.9 Scale
[I] `unitScale` = 15 / (camera height in CM units) so the camera rides at eye height with
`vr.WorldScale 0.11` (HOTD2 15 units, MOK ~14.5 -> 1) [V comments option.cpp:105, profile :340].
Lift floor vertices from a rip (W and y with the focal) to measure.

### 7.10 Headset check
`hotd2-vr\build-quest.cmd`, install, put the set in Download, play. The headset logs the first 8 shots'
diagnostics too (`VR diag: shot ...`) [V].

---

## 8. Risks for Confidential Mission

1. **FOV widening may not fit either mechanism** [I]. HOTD2 = 16-bit angle literals, MOK = one angle
   halved in one builder. CM may use a float FOV, several builders, or precomputed matrices. `fovPatch`
   holds 4 words and MOK already used one slot for the cull margin; a bigger patch means growing the
   array (netplay digest follows automatically [V]). Fallback: viewport scaling (`vr.ViewportAddr` +
   `fovScale`, not online) [V :139-149].
2. **Aim/hit test** [I]: unknown where CM keeps its stock focal and whether widening changes it; without
   `focal`/`stockFocalAddr` the aim uses HOTD2's 640 [V]. Gun calibration is game data (MOK needed
   pokes); CM's default EEPROM from upstream may not be neutral.
3. **2D/HUD/near-plane W** [I]: everything 2D depends on picking `hudW` and snap bands; if CM's HUD W
   equals its near plane W (MOK: near 1.0, HUD 1.082) or it draws HUD in depth, expect streaks/bent text
   until tuned. RTT passes are left un-reprojected [V `ctx.isRTT`].
4. **Shot marker, flash and P2 prompt** [I]: generic marker detection assumes W 0.85-1.15, <= 96 px,
   within 120 px; CM's effects may differ. **No dual wield until `p2PromptTextures` are known** [V].
   B/Y map to NAOMI BTN1, the off-screen sensor bit [V]; effect in CM unknown.
5. **Tooling and timing assume HOTD2/MOK** [V]: `run-test.ps1` forces HOTD2 matrix addresses and picks
   the GD image; `rip-hands.ps1` is HOTD2-only; pokes without a `fovPatch` start at the first vblank, during
   NAOMI boot / GD loading; the profile's `fovScale` is not applied on PC unless set by hand.
6. Weapon model: none at first; prep panel text is HOTD2/MOK binary; a third kind needs code [V].
7. Free play/coins: CM's default EEPROM settings unknown; the hands-style runs and scripts press Start
   only [I].
