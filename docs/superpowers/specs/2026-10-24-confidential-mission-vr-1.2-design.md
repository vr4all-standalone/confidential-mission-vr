# Confidential Mission VR 1.2: design

## Intent

1.1 is installed on the user's Quest 3, and the user tried it. This round has seven polish items from the user (2026-10-24), plus one request we're declining (shipping the ripped guns). The app is still one standalone app, "Confidential Mission VR" (`com.vrlightgunhub.confmiss`). It goes to **1.2.0, versionCode 9**.

The user approved the description below in chat. Where this says "my call", the user didn't decide; they can overrule it at the headset round.

## The user's words

> get rid of the cabinet and araced gun. in game gun, namco gun and zapper gun are good eneoguht. the zapper should be 1.2x size
> when its zapper or namco gun it shouldnt siwthc to assould trifle but should have auto shooting animatoni and recoil hapric
> when out of bullet there is no haptic thats amazing but it has recoil animation it shouldnt double check that
> make it so give option it the menu for reloadiong to be either shoot outside the screen or just point the gun outside the screen
> redo the ui for starting of the apk it is replica of the other game. look into vr4all the other app im making make it so its similar feel.
> cal namco gun the arcade gun instead of namco gun
> also in the options give option ro rotate the gun up or down

## 1. Three guns

The Gun row offers three guns, in this order:
1. **In-game pistol**: the pistol ripped from the player's own game.
2. **Arcade gun**: the bundled Namco model (`namco.glb`). The menu calls it "Arcade gun"; the file name stays.
3. **Zapper**: the bundled `zapper.glb`.

Any extra guns a player drops into a guns folder still follow the bundled ones, as in 1.1.

- **Removed:** the cabinet gun (ripped) and the built-in arcade gun (v1's drawn model) leave the menu.
- **Auto is gone (my call).** The default is the in-game pistol. If that gun doesn't exist (the first-start step was skipped or failed), the game uses the Arcade gun.
- **Old saved choices (my call):** a saved cabinet, arcade or Auto choice becomes the in-game pistol, falling back as above.
- **Built-in fallback (my call):** v1's built-in arcade model stays in the code only as a hidden last resort, used when no gun file can load at all. It never appears on the menu.
- **The cabinet rip (my call):** if the cabinet gun is ripped in its own step, that step goes. This makes the first start shorter, and the parts that only fed the cabinet gun go too. If the cabinet rip is tied into the pistol rip, keep the shared part and say so.
- **Zapper size:** 1.2x its 1.1 size, from 0.25 m to 0.30 m long. Scaling is a technical change, so the Zapper's CC BY-ND licence allows it. Make the change in the Zapper's sidecar (or wherever its size is set). Update the user's local copies too: `my-models\ready`, `cm-vr/models` and `build-win`.
- **Arcade gun name:** set the Namco sidecar's `"name"` to "Arcade gun" in the same three local copies. CREDITS keeps the model's real title and author.

## 2. The rifle pickup with a file gun

When player 1 or 2 holds the Zapper, the Arcade gun or any other file gun and picks up the rifle, the held model stays. Today the hand switches to the ripped assault rifle.

While the rifle mode lasts, each round the game fires plays the same per-round set the rifle model plays:
- a small kick;
- the muzzle flash at that gun's own muzzle;
- the haptic knock. The first round is Shot (0.030 s, 1.0) and each further round is Auto (0.020 s, 0.6).

With the in-game pistol, the pickup still switches to the in-game rifle, as in 1.1.

## 3. No kick when empty

An empty gun gives no haptic today (correct), but its model still kicks. Find out why, then make sure no gun kicks on a pull that fires no round: the in-game pistol, the Arcade gun, the Zapper, a folder gun, and the rifle. Covered tests must show this per gun.

The off-screen reload shot is a special case. Today it fires a shot at the screen edge so the game reloads; whether it kicks is the builder's call, but keep it consistent with the haptics. If reloading gives a reload haptic and no shot haptic, give no kick.

## 4. The Reload option

A new menu row, **Reload**, with two values:
- **Shoot off-screen**: as now, and the default.
- **Point off-screen**: when a gun's aim leaves the screen, it reloads by itself once, as if the trigger had been pulled off-screen. It re-arms when the aim comes back onto the screen.

Details:
- It works per hand and per player.
- It does nothing while the menu is open or the game is paused.
- The B/Y reload buttons and the slide keep working in both modes.
- The setting is saved like the other options.

## 5. Gun angle

A new menu row, **Gun angle**, tilts the gun up or down in the hand.
- **Range:** −30° to +30° in **1° steps** (the user: "make it tilt at 1 degree steps not 5"). 0° (as in 1.1) is the default. The range is my call.
- **Repeat on hold (my call):** holding the stick on this row repeats the step, about 10 steps a second after a short delay, so a big change doesn't take 30 flicks. Whether other stepper rows repeat too is up to the builder.
- **What it moves (my call):** it rotates the drawn gun around the hand's side-to-side axis, at the grip. The aim ray, the laser and the muzzle flash rotate with it, so the shot always leaves along the barrel.
- **Scope:** it applies to every gun, including the rifle, and to both hands. It is saved.
- **Labels:** the how-to-play controllers are not affected.
- **Menu text:** the row shows the value with a sign, e.g. "+10°".

## 6. A start screen in the VR4All style

`CmStartActivity` (the APK's first screen, built in code) is restyled to feel like **VR4All**, the user's own launcher app. Today it copies mikermak's look.

**Source of the look:** VR4All's source is at `C:\Users\ege19\OneDrive\Documents\ChatGPT\vr mod\` (an Electron app). See `public/brand.css`, `public/styles.css`, `public/themes.css` and `vr4all-final-preview.png`. Read it; nothing from it is copied into our repo except colour values and layout ideas.

**Tokens (from brand.css):**

| Token | Value |
|---|---|
| deck (page) | `#0b0d12` |
| panel | `#151923` |
| line | `#343947` |
| muted text | `#9ba5b4` |
| cream text | `#f8f5ef` |
| signal (lime) | `#d9f36c` |
| violet | `#a99bff` |

**Look:**
- Dark page, with a soft radial glow if it's cheap.
- Rounded panels (about 14–18 dp) with thin `line` borders.
- A slim lime-to-violet accent bar, and a small violet tile for the brand mark.
- The system sans-serif in medium/bold weights, since Android has no Segoe.

**Layout (my call; the user saw this sketch):**
```
┌────────────────────────────────────────────┐
│ ▌[CM] Confidential Mission VR       v1.2   │
│                                            │
│  Your mission.                             │
│  Inside the headset.      ← violet         │
│                                            │
│  ┌──────────────────────────────────────┐  │
│  │ ARCADE · RAIL SHOOTER     ● READY    │  │
│  │ confmiss.zip ✓  naomi.zip ✓  CHD ✓   │  │
│  │ [ ▶  Play ]                          │  │
│  └──────────────────────────────────────┘  │
│  First start: makes your guns from your    │
│  own game files (once).                    │
│  Where to put your files  ›                │
└────────────────────────────────────────────┘
```

**Behaviour:** everything today's screen does keeps working:
- the file checks and the folder path;
- any permission steps;
- Play, and whatever else it offers now.

**States:**
- Each missing file gets its own clear badge (e.g. "naomi.zip missing") and the folder it goes in.
- The status badge reads **READY** in lime when all files are there; otherwise it says what's missing.
- Play is disabled until the files are there, unless today's screen allows otherwise.

**No game content:** no game artwork, cover or screenshot, and none of the game's on-screen text. Only our own words and badges.

**First-start gun step:** the start screen says the first start makes the guns from the player's own files, once. If the in-headset first-start step shows text, word it to match: "Making your guns from your game files (once)". Also check whether that step can run faster without losing quality; report what you find either way.

## 7. Controllers during cutscenes

The user: "for the in game cutscenes if you can during them go back to oculus controller and when game start come back to gun".

During the game's story cutscenes, each hand shows its Quest controller model instead of the gun. When play resumes, the guns come back.
- **Which controller model:** the same one the how-to-play phase draws (the bundled or player-folder file controllers, the runtime model, or the built-in shape).
- **Labels (my call):** no how-to-play labels during cutscenes, just the controllers.
- **What counts as a cutscene:** a span where the game shows a non-interactive scene during a game in progress. That covers the stage intros and story scenes between and inside stages.
  - Attract mode and the title screen already have their own behaviour; leave them as they are.
  - Detection is the builder's research. Possible sources: a game-state value in RAM (see `cm-vr/notes/confmiss-findings.md`), or the game's own "gun input ignored" state, e.g. the HUD/ammo display or the reticle being absent.
  - The detection must not flicker. If a scene can be skipped with a shot or Start, the guns must come back the moment play resumes.
  - Record what was found in `confmiss-findings.md`, with addresses and the method, and no game text.
- **Player 2:** a hand that is player 2's follows the same rule.
- **If it can't be done:** if no reliable signal can be found, say so in the report with what was tried, rather than shipping a heuristic that flickers mid-fight.

## 8. New how-to-play labels

The how-to-play labels use the user's exact text:

| Controller | Labels |
|---|---|
| Right | A - P1 Start · B - P1 Reload · Trigger - P1 Shoot · Stick - Laser |
| Left | X - P2 Start · Y - P2 Reload · Trigger - P2 Shoot · Stick - Recenter |

- **Stick:** "Stick" means the stick click: the right stick toggles the laser, the left stick recenters.
- **Menu label (my call):** the left controller's Menu label stays as it is, so players can find the settings menu. The user can drop it at the headset round.
- **Check the mapping:** confirm in code that the left trigger really is player 2's shot. If the left trigger fires for player 1 before player 2 joins, the labels still use the user's text, and the report must say so.
- **Docs:** README and the checklist use the same list.

## 9. The black screen at the stage 2 boss (investigate, fix if we can)

The user (2026-10-24): "at the end of the 2nd level boss with the helicopter and the tank. when the tank shoots rockets and hits player at the end screen goes black for a few seconds. just fyi im not sure if you can fix it but that is another minor issue".

Find the cause and fix it if it's ours.

**Reference:** the flat game (`vr.Xr=no`). If the flat picture also goes dark there, it's the game's own effect. In that case, the VR view should show what the flat game shows (e.g. the same dim or flash on the panel or the world), not more black than the flat game.

**Likely suspects:** the hit effect is a full-screen or near-camera polygon. Possible causes:
- the re-projection placing it in front of the eyes;
- the HUD-by-W split sending it to the wrong layer;
- the clip/far-out rules;
- a fade value we apply world-wide.

**Fix and test:**
- A fix comes with a sim scenario that reaches the moment and shows the frames are no longer black.
- Reach it with RAM pokes, a stage start, or a local save state. A save state is game data: it stays local and is never committed.

**If it's the game's own effect**, or can't be fixed safely: the report says so with evidence, and it goes on the known-issues list.

## Declined: shipping the in-game guns

The user suggested shipping the ripped pistol and rifle in the APK ("idk up to you"). We don't.
- They are Sega's models. An APK can be unzipped, and a takedown would hit the whole project.
- The rip from the player's own ROM stays the only source.
- The no-game-data rule below stays binding.

## Non-goals

- MOK VR.
- Online co-op.
- GitHub push and release.
- VR4All integration.
- Any change to how the game plays beyond the items above.

## Global constraints (binding, from 1.1)

**The release key:**
- Never print or log the release key's password.
- Never copy, move or delete `C:\Users\ege19\.vrlightgunhub`.
- The key never goes in the repo or `dist`.

**No game data** in the repo, the APK, the zip or any report:
- no ROMs, CHDs, rips or RAM dumps;
- no game screenshots or disassembly listings;
- no ripped models;
- none of the game's on-screen text.

**The user's models:**
- They are never committed to git.
- They ship in the APK and the zip, credited in CREDITS, from the gitignored `cm-vr/models/`.
- The Zapper (CC BY-ND) gets technical changes only.

**Git and devices:**
- Never stage or touch the modified `core/deps/gamesdk` submodule.
- Nothing is pushed or published.
- The orchestrator installs on the Quest 3 (`2G0YC1ZF840H2Y`) at the end; tasks don't install.

**Downloads** are untrusted: put each extraction in its own new folder in the scratchpad, and run Python on them with `-I`.

## Done means

- Every item has a test with fixtures or the sim where it can (`cm-vr\accept.ps1` scenarios).
- The full regression passes: 38 CM scenarios plus `hotd2,mok`, with existing scenarios updated where the gun list or the menu rows changed.
- Both builds pass.
- `package-release.ps1` passes all checks, for 1.2.0, versionCode 9.
- The headset checklist is updated for 1.2.
