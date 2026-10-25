<#
  Acceptance runs on the Windows build (build-win.cmd): each scenario plays a game with
  vr-test.ps1 and checks the run with re\vrcheck.py. HOTD2 and MOK are the baselines when
  adding a game: they must keep passing after every change to the C++.

  .\hotd2-vr\accept.ps1                    all scenarios
  .\hotd2-vr\accept.ps1 -Only hotd2,mok
  .\hotd2-vr\accept.ps1 -Only cm-aim -Reuse   check the runs already there, without playing them

  Needs $env:VRLGH_GAMES (the MAME sets: mok.zip, mok\gds-0022.chd, confmiss.zip,
  confmiss\gds-0001.chd, naomi.zip), $env:VRLGH_HOTD2 (the US disc of The House of the Dead 2,
  MK-51002) and Python with numpy. Prints PASS <name> / FAIL <name>: <reason>; exit 1 when any
  fails. Runs land in build-win\vrtest\accept-<run>\; Confidential Mission's scenarios share
  six runs (accept-cm-tour, -off, -edges, -hall, -floor, -floor0), each played once per call;
  cm-sim and cm-buttons play their own (accept-cm-sim, accept-cm-buttons) through the OpenXR
  simulator, cm-buttons-pc two more (accept-cm-buttons-index, -vive) with the PC controllers.
  cm-haptics, -low, -off, -mag and -reload play one each (accept-cm-haptics...) the same way,
  and so do cm-menu and cm-menu-quit (accept-cm-menu, accept-cm-menu-quit), and cm-controllers
  and cm-controllers-rt (accept-cm-controllers, accept-cm-controllers-rt; the latter's GLB is
  build-win\vrtest\test-controller.glb, made by re\make-test-glb.py when it is missing), and cm-hands
  and cm-hands-shot (accept-cm-hands, accept-cm-hands-shot). The gun run (CM VR 1.2: no cabinet gun):
  cm-rip-start plays accept-cm-rip-start (it deletes build-win\data\gun-cm-*.bin first; the run goes from
  Start to the agents' pistol in the stage 1 intro and leaves its file there) and accept-cm-rip-start-next
  (both files there and a 1.1 gun-cm-cabinet.bin: no run, the old file left as it was), cm-rip-hand
  accept-cm-rip-hand and -arcade (it needs the pistol's file), cm-rip-skip accept-cm-rip-skip and -skip2
  (it deletes the files again; after the skip the hand holds the harness's bundled Arcade gun). The XR
  scenarios from before the game's own guns pin the built-in gun ($cmWidenArcade).
  cm-rip-pistol plays accept-cm-rip-pistol (the same run, checked as Task 11 did), cm-rip-pistol-one
  accept-cm-rip-pistol-one (the same with one capture merged; each pass draws all of it, so it's whole too).
  cm-rip-rifle plays accept-cm-rip-rifle (it deletes the files; the whole first-start run: the assault
  rifle from the attract demo at ~316 s of game first, then Start and the pistol) and
  accept-cm-rip-rifle-next (the rifle's file alone missing: no run, watched for in play); it leaves both
  files and a copy of the rifle's, build-win\vrtest\cm-rifle-seed.bin, which cm-rip-start,
  cm-rip-pistol and cm-rip-pistol-one put in place after deleting the files (so their runs start at
  Start, as before); cm-rip-skip doesn't (B skips during the rifle's part). cm-rip-rifle comes
  before them in the list; run without the seed, they fail and say so. cm-rip-skip-rifle plays
  accept-cm-rip-skip-rifle: the skip with the rifle's seed in place, which goes Off (nothing watched for).
  cm-merge-unit plays nothing: it compiles re\hands_build_test.cpp with core\rend\vr\hands_build.cpp
  (the VS 2022 Build Tools, as build-win.cmd; into build-win\vrtest\cm-merge-unit, not the app),
  runs it (the pistol's merge on synthetic captures) and then vrcheck.py's selftest.
  cm-gun plays accept-cm-gun, -migrate-0, -2, -3, -upgrade, -builtin, -nopistol and -nofiles (the three guns,
  CM VR 1.2 Task 1): it needs the pistol's file the rip scenarios leave (it comes after them in the list;
  without it it fails and says so), and moves it away (.hold) for the last two runs, back when it's done.
  cm-rifle-sim plays accept-cm-rifle-sim and accept-cm-rifle-sim-arcade (the assault rifle, Task 14: poked
  into player 1's block, its knocks counted against the rounds the game fires): it needs gun-cm-rifle.bin
  (cm-rip-rifle's), moved away (.hold) for the second run and back when it's done; cm-rifle-p2 plays
  accept-cm-rifle-p2 (player 2's rifle, in the left hand).
  spike-keys plays accept-spike-keys: the harness's scene keys (vr-test.ps1 -On, poke, vr.WatchRam).
  cm-own-gun and cm-own-controllers (Task 14b, the player's own models) play accept-cm-own-gun, -nosidecar, -override,
  accept-cm-own-controllers and -right with re\make-own-models.py's fixtures (self-made boxes) written into
  build-win\guns\ and build-win\controllers\ meanwhile; they, cm-controllers, cm-controllers-rt and cm-bundled first move
  any guns\ and controllers\ folders aside (.accept-hold), next to build-win\flycast.exe and in $env:VRLGH_GAMES, and the
  bundled models' folder build-win\data\models too, and back after (a player's own models and the bundled ones are left
  as they were). cm-own-gun needs cm-gun's rip files. cm-own-gun-file plays accept-cm-own-gun-file: the gun
  accept-cm-own-gun's menu saved, played again with a file sorting before it (it plays cm-own-gun first without its
  emu.cfg.after). own-folders: the folders' restore when a fixture can't be removed (no game run; a player's
  build-win\guns\ goes to guns.own-folders-hold meanwhile). cm-bundled (S37) plays accept-cm-bundled: fixtures in the
  player's folders and in a bundled folder of its own (build-win\data\models), merged file by file; then
  accept-cm-bundled-case (S45): the player's BoxGun.glb over the bundled boxgun.glb, vr.GunFile boxgun kept.
#>
param([string[]]$Only = @(), [switch]$Reuse)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$vrTest = Join-Path $PSScriptRoot 'vr-test.ps1'
$vrCheck = Join-Path $PSScriptRoot 're\vrcheck.py'
$inv = [Globalization.CultureInfo]::InvariantCulture

function Get-RunLog($name) { Join-Path $repo "build-win\vrtest\$name\flycast.log" }

# Expect / Reject regexes (case-sensitive) on a run's flycast.log: $null when all hold, else why not.
function Test-LogLines([string]$name, [string[]]$Expect = @(), [string[]]$Reject = @()) {
	$log = Get-RunLog $name
	if (-not (Test-Path -LiteralPath $log)) { return "no flycast.log in accept run $name" }
	$why = @()
	foreach ($re in @($Expect | Where-Object { $_ })) {
		if (Select-String -LiteralPath $log -Pattern $re -CaseSensitive -Quiet) { Write-Host "  PASS expect $re" }
		else { $why += "FAIL expect $re" }
	}
	foreach ($re in @($Reject | Where-Object { $_ })) {
		$hit = Select-String -LiteralPath $log -Pattern $re -CaseSensitive | Select-Object -First 1
		if ($hit) { $why += "FAIL reject $re (line $($hit.LineNumber))" } else { Write-Host "  PASS reject $re" }
	}
	$why | ForEach-Object { Write-Host "  $_" }
	return $(if ($why) { $why -join '; ' } else { $null })
}

# vr-test.ps1 with these parameters: $null when it passed, else why not. -Reuse: the run's
# Expect/Reject on the flycast.log already there, when there is one.
function Invoke-VrTest([hashtable]$params) {
	if ($Reuse -and (Test-Path -LiteralPath (Get-RunLog $params.Name))) {
		Write-Host "  reusing build-win\vrtest\$($params.Name)"
		return Test-LogLines $params.Name $params.Expect $params.Reject
	}
	$out = @(& $vrTest @params)
	$code = $LASTEXITCODE
	$out | ForEach-Object { Write-Host "  $_" }
	if ($code -eq 0) { return $null }
	$why = ($out | Where-Object { $_ -match '^FAIL|exited early' }) -join '; '
	return $(if ($why) { $why } else { "vr-test.ps1 exit $code" })
}

# vrcheck.py with these arguments: $null when it passed (exit 0 and every -Want regex on a line), else why not.
function Invoke-VrCheck([string[]]$arguments, [string[]]$Want = @()) {
	$out = @(& python -I $vrCheck @arguments)
	$code = $LASTEXITCODE
	$out | ForEach-Object { Write-Host "  $_" }
	$missing = @($Want | Where-Object { $re = $_; -not ($out | Where-Object { $_ -cmatch $re }) })
	if ($code -eq 0 -and -not $missing) { return $null }
	return "vrcheck $($arguments[0]) " + $(if ($code -ne 0) { "exit $code" } else { "no line matching $($missing -join ', ')" })
}

# The House of the Dead 2 (US): its field of view literals widened 3x, from boot on. Start as
# the title needs it (the menu at 56 s, Arcade mode at 60 s; later presses would pause the
# intro), the intro to ~85 s, then the agent loses (the continue countdown at ~100 s).
function Test-Hotd2 {
	if (-not $env:VRLGH_HOTD2) { return 'set $env:VRLGH_HOTD2 to the US disc image of The House of the Dead 2' }
	$at = @(for ($t = 44; $t -le 62; $t += 2) { "$t=start 0" }) + '60=shot', '75=shot', '92=shot'
	$why = Invoke-VrTest @{ Game = $env:VRLGH_HOTD2; Name = 'accept-hotd2'; Seconds = 100; At = $at
		Config = 'config:vr.FovScale=3,config:vr.WidenFov=yes'
		Expect = 'VR: camera profile for MK-51002$',
			'VR: field of view 41\.1 -> 96\.7 degrees at 8c02998e', 'VR: field of view 41\.1 -> 96\.7 degrees at 8c02b0ec' }
	if ($why) { return $why }
	# the live focal of the widened view: 240 / tan(96.7 / 2) (stock 640)
	return Invoke-VrCheck 'focal', (Get-RunLog 'accept-hotd2'), '--expect', '213.4'
}

# The Maze of the Kings: its widening patch, and what it draws where the gun points (a miss:
# starburst 0x50b800 and red stars 0x4f6800; a hit: spark 0x4e9000). Start to the rod
# calibration (~35 s); shots at the centre decide both item screens (else their timers put
# the story ~20 s later); the story runs to ~86-88 s, then the maze: its first seconds take no
# shot, then the camera walks to the first fight (~100 s). The effects stay where they hit,
# on a wall or an enemy, so they drift off the gun while the camera walks (2-3 px in their
# first frame) or the enemy moves (up to 5 px seen at ~113 s); from 101.5 to 111 s the camera
# stands and the enemies are still far, and every centre shot must draw at the gun. The rip
# in the maze must hold the profile's matrices, widened (focal 240).
function Test-Mok {
	$at = @(for ($t = 30; $t -le 46; $t += 2) { "$t=start 0" }) + '50=fire 0 5000 5000', '53=fire 0 5000 5000', '56=fire 0 5000 5000'
	$at += @(foreach ($t in 102.5, 105, 107.5, 110.5) { "$t=fire 0 5000 5000" })
	$at += '100=reload 0', '109=reload 0', '35=shot', '60=shot', '90=shot', '100=rip 1 1 1', '101=shot', '111=shot'
	$why = Invoke-VrTest @{ Game = 'mok'; Name = 'accept-mok'; Seconds = 112; At = $at
		Config = 'config:vr.FovScale=2,config:vr.WidenFov=yes'
		Expect = 'VR: camera profile for THE MAZE OF THE KINGS$', 'VR: the game''s field of view widened \(code at 8c08d8ee\)' }
	if ($why) { return $why }
	$why = Invoke-VrCheck 'shots', (Get-RunLog 'accept-mok'), '--tex', '50b800,4f6800,4e9000', '--max-off', '2', '--from', '101.5', '--to', '111'
	if ($why) { return $why }
	$ram = Join-Path $repo 'build-win\vrtest\accept-mok\rip\ram0000.bin'
	if (-not (Test-Path $ram)) { return 'no rip of the maze (rip\ram0000.bin)' }
	return Invoke-VrCheck 'matrices', $ram -Want '^viewport 0x0F24D0 320\.0 -240\.0$', '^proj 0x0E7A60 focal 240\.0 240\.0$'
}

#
# Confidential Mission (notes\confmiss-findings.md). Its routes are the findings' runs, replayed:
# Start every 2 s from 30 to 52 s starts a game in free play (the gun adjustment screen holds to
# ~43 s, the stage 1 intro from 46 s, stage 1's first room from ~50 s); left alone there, the
# mission fails (~62-70 s), then game over, the logos and the attract (its how-to-play scene in
# 3D from ~140 s). Gun points are 0..10000; the 9 points are the centre, the corners and the
# edges' middles at 10 % and 90 %. Widened runs use fovScale 2.
#
function T([double]$t) { $t.ToString('0.0##', $inv) }
$cmWiden = 'config:vr.FovScale=2,config:vr.WidenFov=yes'
# The XR scenarios from before the game's own guns (Task 10) pin the built-in gun (xr_gun.h's, v1's
# arcade gun): their aim, muzzle (vr.GunScale 0.68: cm-menu's Get-CmMenuAim) and shots assume it, and
# with vr.GameHands off a run neither draws the game's guns nor makes them, whatever ran before.
# vr.BuiltInGun (PC, CM VR 1.2) holds it whatever vr.Gun says, so these runs never draw a bundled gun
# (the user's own models in build-win\data\models): "XR: gun: built-in gun (chosen: vr.BuiltInGun)".
$cmWidenArcade = "$cmWiden,config:vr.GameHands=no,config:vr.BuiltInGun=yes"
$cmPoints = @(@(5000, 5000), @(1000, 1000), @(9000, 1000), @(1000, 9000), @(9000, 9000), @(5000, 1000), @(5000, 9000), @(1000, 5000), @(9000, 5000))

function Get-CmStarts { for ($t = 30; $t -le 52; $t += 2) { "$t=start 0" } }
# the same through the simulator's right controller (vr-test.ps1 -XrSim): A, held 200 ms
function Get-CmStartsXr { for ($t = 30; $t -le 52; $t += 2) { "$t=xr right/a 1 200" } }

# The window as wide as a Quest 3 eye (104 x 96 degrees: the tan of its half angles), for the
# floor's runs; their shot every 0.4 s from 52 to 72 s.
$cmQuestEye = 'config:vr.WindowTanX=1.2799,config:vr.WindowTanY=1.1106'
$cmFloorTimes = @(for ($i = 0; $i -le 50; $i++) { [Math]::Round(52 + 0.4 * $i, 1) })

# The 9 points from $t0, $gap apart, a reload after the 3rd and 6th; -Starts 1: a reload 0.9 s
# before and Start 0.35 s after each (a death continued at once), 2: a second Start 0.85 s
# after (1.1 s after a reloading one); -Label: a window shot 0.1 s after each.
function Get-CmTable([double]$t0, [double]$gap, [int]$count = 9, [int]$starts = 0, [string]$label = '') {
	if ($starts) { (T ($t0 - 0.9)) + '=reload 0' }
	for ($i = 0; $i -lt $count; $i++) {
		$t = $t0 + $i * $gap
		(T $t) + "=fire 0 $($cmPoints[$i][0]) $($cmPoints[$i][1])"
		if ($label) { (T ($t + 0.1)) + "=shot $label-$i" }
		if ($starts -ge 1) { (T ($t + 0.35)) + '=start 0' }
		if ($starts -ge 2) { (T ($t + $(if ($i -eq 2 -or $i -eq 5) { 1.1 } else { 0.85 }))) + '=start 0' }
		if ($i -eq 2 -or $i -eq 5) { (T ($t + $gap / 2)) + '=reload 0' }
	}
}

# t6-join's route: t5f-hall2's for player 1 (t5-w1's 9 points from 52.5 s, a spray over 15
# points every 0.4 s to 126 s with a reload every 6th and Start every 4 s, which clears the
# first rooms, the corridor and reaches the museum hall ~122 s; one pass from 127.5 s and the
# table pass A from 133.5 s, the hall standing still with stock focal 879.2), player 2 joining
# at 60 s (out ~85 s, the agent idle) and again at 140 s.
function Get-CmHallRoute {
	Get-CmStarts
	Get-CmTable 52.5 0.75
	$spray = foreach ($y in 2500, 4500, 6500) { foreach ($x in 1200, 3000, 5000, 7000, 8800) { , @($x, $y) } }
	$n = 0
	for ($t = 60.0; $t -lt 126; $t += 0.4) {
		$p = $spray[$n % $spray.Count]
		(T $t) + "=fire 0 $($p[0]) $($p[1])"
		$n++
		if ($n % 6 -eq 0) { (T ($t + 0.2)) + '=reload 0' }
	}
	for ($t = 61.1; $t -lt 126; $t += 4) { (T $t) + '=start 0' }
	Get-CmTable 127.5 1.5 -count 4 -starts 1
	Get-CmTable 133.5 1.5 -starts 2 -label 'hall'
	'59.6=aim 1 5000 5000', '60=start 1', '140=start 1'
}

# Task 4's rifle hunt route (t4-h3, the furthest of its runs; no rifle picked up on it, see
# confmiss-findings.md ## Rifle): widened ($cmWiden), the 9 points from 52.5 s, then a sweep
# every 0.3 s to $seconds - 1 with a reload every 6th and Start every 4 s from 60.7 s: 17
# columns 500 apart (x 800-9200; a far guard is ~470 units wide widened) on rows cycling over
# y 4000, 5500, 7000, 2500, 8500, a row running right to left when its index mod 4 is 1 or 2.
# It clears the first rooms, the corridor and the gallery and reaches a timed target sequence
# (312-356 s, 2 of 2 runs: t4-h3, t4-r1), then a green-lit corridor. Key it with -On
# (Get-CmRifleKeys) and $cmRifleWatch, the hunt's: player 1's block +0x48..+0x6F.
$cmRifleWatch = 'config:vr.WatchRam=0c155800:4,0c155804,0c155805,0c155806:2,0c155808,0c155809,0c15580a:2,0c15580c:4,0c155810:4,0c155814:4,0c155818:2,0c15581a:2,0c15581c,0c155820:4,0c155824:4'
$cmRiflePickup = 'VR: ram 0c15580[4-9] .* -> 2d'
function Get-CmRifleRoute([double]$seconds = 400) {
	Get-CmStarts
	Get-CmTable 52.5 0.75
	$cols = @(for ($x = 800; $x -le 9200; $x += 500) { $x })
	$sweep = @(for ($k = 0; $k -lt 10; $k++) {
		$y = @(4000, 5500, 7000, 2500, 8500)[$k % 5]
		$row = $(if ($k % 4 -eq 1 -or $k % 4 -eq 2) { $cols[($cols.Count - 1)..0] } else { $cols })
		foreach ($x in $row) { , @($x, $y) }
	})
	$n = 0
	for ($t = 60.0; $t -lt $seconds - 1; $t += 0.3) {
		$p = $sweep[$n % $sweep.Count]
		(T $t) + "=fire 0 $($p[0]) $($p[1])"
		$n++
		if ($n % 6 -eq 0) { (T ($t + 0.15)) + '=reload 0' }
	}
	for ($t = 60.7; $t -lt $seconds - 1; $t += 4) { (T $t) + '=start 0' }
}
# the hunt's keys: on the pickup (ammo or magazine to 45) a window shot and a rip, then the
# trigger held 3 s (the fire rate); a window shot when the magazine leaves 06 (any weapon change)
function Get-CmRifleKeys {
	"$cmRiflePickup|0|shot pickup|3", "$cmRiflePickup|0|rip 3 30 1", 'VR: ram 0c155808 06 -> |0|shot mag|3',
	"$cmRiflePickup|0.2|mute 5", "$cmRiflePickup|0.3|aim 0 5000 5000", "$cmRiflePickup|0.4|button 0 1 3000"
}

# The shared runs (vr-test.ps1 parameters, no checks: each scenario checks its own lines).
$cmRunDefs = [ordered]@{
	# boot to the attract's 3D: Start to 52 s, then nothing (t3-stage1d, t5-gate2's timeline),
	# viewed yawed and pitched (t4y-*), the game's own frames every 4 s
	'accept-cm-tour' = @{ Game = 'confmiss'; Name = 'accept-cm-tour'; Seconds = 156; DebugShots = 4
		Config = "$cmWiden,config:vr.Yaw=25,config:vr.Pitch=-15"
		At = @(Get-CmStarts) + '46.3=shot cutscene', '49=shot cutscene', '55=shot stage1', '60=shot stage1', '65=shot continue',
			'79=shot', '95=shot', '108=shot attract', '118=shot attract', '130=shot attract', '142=shot attract', '148=shot attract', '154=shot attract' }
	# widening off, the marker at the gun: t5-g2-off (shots on the adjustment screen included),
	# with the player 2 prompt hidden (the default; CM's profile keeps the flat 2D beside it,
	# p2HideTrailing off, so the marker at 576,432 stays whole)
	'accept-cm-off' = @{ Game = 'confmiss'; Name = 'accept-cm-off'; Seconds = 60
		Config = 'config:vr.DropShotMarker=no,config:vr.DropShotFlash=no'
		At = @(Get-CmStarts) + '33=fire 0 5313 5313', '36=fire 0 5313 5313', '39=fire 0 5313 5313' + @(Get-CmTable 52.5 0.75) }
	# widened, the screen's edges in stage 1's first room: t5-w2 (B, reload, B, centre after)
	'accept-cm-edges' = @{ Game = 'confmiss'; Name = 'accept-cm-edges'; Seconds = 62
		Config = "$cmWiden,config:vr.DropShotMarker=no,config:vr.DropShotFlash=no"
		At = @(Get-CmStarts) + '46.6=shot preB', '47=button 0 128 300', '47.6=shot postB', '53=fire 0 300 5000', '53.6=fire 0 9700 5000',
			'54.2=fire 0 5000 300', '54.8=fire 0 5000 9700', '55.6=shot hudA', '56=reload 0', '56.8=shot hudB', '57.4=shot preB2',
			'57.6=button 0 128 300', '58.2=shot postB2', '58.6=fire 0 5000 5000', '59.4=shot after' }
	# widened, defaults otherwise (marker hidden): the hall route with player 2 (t6-join)
	'accept-cm-hall' = @{ Game = 'confmiss'; Name = 'accept-cm-hall'; Seconds = 147; Config = $cmWiden
		At = @(Get-CmHallRoute) + '55=shot stage1', '62=shot', '67.5=shot cutscene', '75=shot', '85.5=shot p2out',
			'119.3=shot fade', '125=shot hall' }
	# widened, the window a Quest 3 eye looking 30 degrees down: the hall route to the hall's
	# still camera, shots in stage 1's first room and in the hall
	'accept-cm-floor' = @{ Game = 'confmiss'; Name = 'accept-cm-floor'; Seconds = 131; Config = "$cmWiden,$cmQuestEye,config:vr.Pitch=-30"
		At = @(Get-CmHallRoute | Where-Object { [double]($_ -split '=')[0] -lt 131 }) + '54=shot room', '56.5=shot room', '127.5=shot hall', '129=shot hall' }
	# the same eye straight ahead, the hall route through stage 1: the floor in front at 53.5 s,
	# and a shot every 0.4 s from 52 to 72 s while the camera moves
	'accept-cm-floor0' = @{ Game = 'confmiss'; Name = 'accept-cm-floor0'; Seconds = 73; Config = "$cmWiden,$cmQuestEye"
		At = @(Get-CmHallRoute | Where-Object { [double]($_ -split '=')[0] -lt 73 }) + '53.5=shot wedge' + @($cmFloorTimes | ForEach-Object { (T $_) + '=shot floor' }) }
}
$cmRuns = @{}
# Play a shared run once per call: $null when it ran to the end, else why not.
function Invoke-CmRun([string]$name) {
	if (-not $cmRuns.Contains($name)) { $cmRuns[$name] = Invoke-VrTest $cmRunDefs[$name] }
	if ($cmRuns[$name]) { return "run ${name}: $($cmRuns[$name])" }
	return $null
}

# A run's window shots (vr-test names them t<seconds>[-label].png): $null when all are there.
function Test-RunShots([string]$name, [string[]]$files) {
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$missing = @($files | Where-Object { -not (Test-Path -LiteralPath (Join-Path $dir $_)) })
	Write-Host "  window shots $($files.Count - $missing.Count) of $($files.Count) in build-win\vrtest\$name"
	return $(if ($missing) { "no window shot $($missing -join ', ') in $name" } else { $null })
}

# How many lines of a run's log match a regex (case-sensitive). -From / -To: only the lines
# logged in that window, in seconds of the log's clock (the mm:ss:mmm a line starts with, from
# Flycast's start: about 0.2 s behind vr-test's -At times).
function Get-LogCount([string]$run, [string]$regex, [double]$From = -1, [double]$To = 1e9) {
	$hits = @(Select-String -LiteralPath (Get-RunLog $run) -Pattern $regex -CaseSensitive)
	if ($From -lt 0 -and $To -ge 1e9) { return $hits.Count }
	return @($hits | Where-Object { $_.Line -match '^(\d+):(\d\d):(\d{3}) ' -and
		($t = [int]$Matches[1] * 60 + [int]$Matches[2] + [int]$Matches[3] / 1000) -ge $From -and $t -lt $To }).Count
}

# The log's clock (seconds from Flycast's start) at each line of a run's log matching a regex (case-sensitive), in order.
function Get-LogClocks([string]$run, [string]$regex) {
	return @(Select-String -LiteralPath (Get-RunLog $run) -Pattern $regex -CaseSensitive | ForEach-Object {
		if ($_.Line -match '^(\d+):(\d\d):(\d{3}) ') { [int]$Matches[1] * 60 + [int]$Matches[2] + [int]$Matches[3] / 1000 } })
}

# Lines matching a regex in a run's log, at most $max: $null when so.
function Test-LogCount([string]$name, [string]$re, [int]$max) {
	$n = Get-LogCount $name $re
	Write-Host "  $n lines '$re' in $name (at most $max)"
	return $(if ($n -gt $max) { "$n lines '$re' in $name, over $max" } else { $null })
}

# Boot: the game's ID and profile, and Start alone (free play, no coin) begins a game: stage
# 1's first room, its widened focal (stock 540.8 x 0.406) from 52 s while the guards play on.
function Test-CmBoot {
	$why = Invoke-CmRun 'accept-cm-tour'
	if ($why) { return $why }
	$why = Test-LogLines 'accept-cm-tour' -Expect 'NAOMI GAME ID \[ CONFIDENTIAL MISSION ---------\] region ff players 2',
		'VR: camera profile for  CONFIDENTIAL MISSION ---------$'
	if ($why) { return $why }
	return Invoke-VrCheck 'focal', (Get-RunLog 'accept-cm-tour'), '--from', '52', '--to', '72', '--expect', '219.7'
}

# The view widened: the patch's line, and the live focal against the stock one in a cutscene
# (the stage 1 intro's first shot at ~46 s, stock 449.6), stage 1 (stock 540.8) and the
# attract's how-to-play scene (stock 879.2); window shots of the three (yawed: the 3D past the
# stock frame's edge) and the game's own frames (DebugShots) stay in the run's folder.
function Test-CmWiden {
	$why = Invoke-CmRun 'accept-cm-tour'
	if ($why) { return $why }
	$log = Get-RunLog 'accept-cm-tour'
	$why = Test-LogLines 'accept-cm-tour' -Expect 'VR: the game''s field of view widened \(code at 8c0274b6\)'
	if ($why) { return $why }
	# the stage 1 intro's sample at 46 s (the museum exterior in the 46.3 s shot) catches one of
	# two cameras, by where the 44 s press falls against the gun adjustment screen's countdown
	# (ends ~44.0 s): ~20.8K verts, stock 449.6 (t3-intro-off), widened 224.7 (t3-intro4, Tasks
	# 3-5's runs, the first and third full runs), or 5641 verts at 436.4, the widened 879.2 as in
	# the attract (the second full run). Either, widened. Moving the press to 44.6 s skips both
	# (t9probe1-2: the guard's zooming close-up at 46 s, 657.1 and 662.1).
	# (--tol 0.01: the default 2 % takes 436.4 up to 445, within reach of the swaying stock 449.6)
	$why = Invoke-VrCheck 'focal', $log, '--from', '45.5', '--to', '46.5', '--expect', '224.7', '--tol', '0.01'
	if ($why) {
		$why = Invoke-VrCheck 'focal', $log, '--from', '45.5', '--to', '46.5', '--expect', '436.4', '--tol', '0.01'
		if (-not $why) { Write-Host '  (the 224.7 check above failed: this run caught the other intro camera, 5641 verts at 436.4, the widened 879.2: OK)' }
	}
	if ($why) { return "$why (from 45.5 to 46.5 s: neither 224.7 nor 436.4)" }
	foreach ($check in @(@('52', '72', '219.7'), @('142', '156', '436.4'))) {
		$why = Invoke-VrCheck 'focal', $log, '--from', $check[0], '--to', $check[1], '--expect', $check[2]
		if ($why) { return "$why (from $($check[0]) to $($check[1]) s)" }
	}
	return Test-RunShots 'accept-cm-tour' 't046.3-cutscene.png', 't055-stage1.png', 't142-attract.png'
}

# The gun: widening off, the 9 points draw the game's marker at the gun (<= 2 px); widened, at
# 320 + (x - 320) x live / stock (<= 3 px, R11) in the hall standing still (live/stock 0.496),
# its 3D impacts too (shots into enemies draw none), and the screen's edges register; no shot
# with a marker in a scene whose live focal is near the stock one (a cutscene camera).
function Test-CmAim {
	foreach ($run in 'accept-cm-off', 'accept-cm-edges', 'accept-cm-hall') {
		$why = Invoke-CmRun $run
		if ($why) { return $why }
	}
	$off, $edges, $hall = (Get-RunLog 'accept-cm-off'), (Get-RunLog 'accept-cm-edges'), (Get-RunLog 'accept-cm-hall')
	$why = Invoke-VrCheck 'shots', $off, '--tex', '5e8000', '--union', '--max-off', '2', '--from', '52' -Want '^shots 9 matched 9 '
	if ($why) { return "widening off: $why" }
	$why = Invoke-VrCheck 'shots', $hall, '--tex', '5e8000', '--union', '--max-off', '3', '--from', '133', '--to', '146' -Want '^shots 9 matched 9 '
	if ($why) { return "hall: $why" }
	$why = Invoke-VrCheck 'stock', $hall, '--from', '133', '--to', '146' -Want ' stock 879\.\d live/stock 0\.49\d\.\.0\.49\d$'
	if ($why) { return "hall: $why" }
	# exit 1 by design: shots into an enemy or the floor draw no wall impact (7 of 9 in each run seen)
	$out = @(& python -I $vrCheck 'shots' $hall '--tex' '614000' '--union' '--from' '133' '--to' '146')
	$out | ForEach-Object { Write-Host "  $_" }
	$sum = $out | Where-Object { $_ -match '^shots \d+ matched' } | Select-Object -Last 1
	if (-not ($sum -match '^shots \d+ matched (\d+) worst (\d+) px off$') -or [int]$Matches[1] -lt 6 -or [int]$Matches[2] -gt 3) { return "hall: 3D impacts (tex 614000) want 6 or more matched, worst <= 3 px: $sum" }
	$why = Invoke-VrCheck 'shots', $edges, '--tex', '5e8000', '--union', '--max-off', '3', '--from', '52.5', '--to', '55', '--max-ratio', '0.9' -Want '^shots 4 matched 4 '
	if ($why) { return "edges: $why" }
	return Invoke-VrCheck 'shots', $hall, '--tex', '5e8000', '--union', '--max-ratio', '0.9'
}

# The 2D plane: layers, sights, the letterbox and the flat screens go onto it; no more 3D
# polygons kept with a corner on its W than Task 4 saw (1 a run; the hall route 2 in Task 5);
# each frame's nearest W after it the 3D's near plane or the plane's own; the shot marker
# hidden but not the game's own cross-fade (tex 404800: the previous view drawn with white
# vertex colour, ~119 s on the hall route); yawed window shots.
function Test-CmHud {
	foreach ($run in 'accept-cm-tour', 'accept-cm-hall') {
		$why = Invoke-CmRun $run
		if ($why) { return $why }
	}
	$why = Test-LogLines 'accept-cm-tour' -Expect 'VR: 2D layer at W [0-9.]+ \(.*\) put on the 2D plane',
		'VR: sight \(tex 400000, .* put on the 2D plane at', 'VR: letterbox bars hidden', 'VR: a screen without 3D'
	if ($why) { return $why }
	$why = Test-LogCount 'accept-cm-tour' 'VR: kept a 3D polygon with' 1
	if ($why) { return $why }
	$why = Invoke-VrCheck 'focal', (Get-RunLog 'accept-cm-tour'), '--wnear-allow', '0.300,0.998-1.002,111111.250'
	if ($why) { return $why }
	$why = Test-LogLines 'accept-cm-hall' -Expect 'VR: hid the game''s shot marker' -Reject 'tex 404800 tsp [0-9a-f]+ col [0-9a-f]+ DROPPED'
	if ($why) { return $why }
	$why = Test-LogCount 'accept-cm-hall' 'VR: kept a 3D polygon with' 2
	if ($why) { return $why }
	return Test-RunShots 'accept-cm-tour' 't055-stage1.png', 't060-stage1.png', 't065-continue.png', 't148-attract.png'
}

# Player 2: its prompt known and hidden, it joins (prompt gone), is out (~85 s: shown), joins
# again (140 s: gone) - nothing else.
function Test-CmP2 {
	$why = Invoke-CmRun 'accept-cm-hall'
	if ($why) { return $why }
	$seen = @(Select-String -LiteralPath (Get-RunLog 'accept-cm-hall') -Pattern 'VR: player 2 prompt (shown|gone)' -CaseSensitive |
		ForEach-Object { Write-Host "  $($_.Line)"; $_.Matches[0].Groups[1].Value }) -join ', '
	return $(if ($seen -ne 'shown, gone, shown, gone') { "player 2 prompt: $seen (want shown, gone, shown, gone)" } else { $null })
}

# The floor (headset round 1, notes\confmiss-findings.md), widened, through a Quest 3 eye: what
# is left undrawn shows near-black (vrcheck blank). Looking 30 degrees down, the lower half in
# stage 1's first room and in the museum hall (clipFarOut cut CM's real floor 47.7 degrees
# below the game's axis: about 3/4 and 1/2 of it missing); straight ahead at 53.5 s, the floor
# in front, where the culled pieces of the mirrored floor strips left a wedge; a shot every
# 0.4 s while the camera moves through stage 1: the floor in front (the lowest quarter) never
# blank, nor jumping from one shot to the next (a wedge coming and going: the pop-in); there blank
# is the clear colour itself in 9 x 9 squares, as the soldiers' shadows crossing it are near-black
# and black in places.
function Test-CmFloor {
	foreach ($run in 'accept-cm-floor', 'accept-cm-floor0') {
		$why = Invoke-CmRun $run
		if ($why) { return $why }
	}
	$down = 't054-room.png', 't056.5-room.png', 't127.5-hall.png', 't129-hall.png'
	$series = @($cmFloorTimes | ForEach-Object { 't{0:000.#}-floor.png' -f $_ })
	$why = Test-RunShots 'accept-cm-floor' $down
	if (-not $why) { $why = Test-RunShots 'accept-cm-floor0' (@('t053.5-wedge.png') + $series) }
	if ($why) { return $why }
	$dir, $dir0 = (Join-Path $repo 'build-win\vrtest\accept-cm-floor'), (Join-Path $repo 'build-win\vrtest\accept-cm-floor0')
	$why = Invoke-VrCheck (@('blank') + @($down | ForEach-Object { Join-Path $dir $_ }) + '--region', 'lower', '--max', '0.05')
	if ($why) { return "looking down: $why" }
	$why = Invoke-VrCheck 'blank', (Join-Path $dir0 't053.5-wedge.png'), '--region', 'lower', '--max', '0.02'
	if ($why) { return "straight ahead: $why" }
	$why = Invoke-VrCheck (@('blank') + @($series | ForEach-Object { Join-Path $dir0 $_ }) + '--region', '0,0.75,1,1', '--black', '1', '--patch', '9', '--max', '0.03', '--max-jump', '0.03')
	if ($why) { return "stage 1, the camera moving: $why" }
	return $null
}

# The OpenXR simulator (vr.XrSim, vr-test.ps1 -XrSim): the headset path on the PC, widened. A
# game started with the right controller's A; the left eye in the window: no shot mostly black,
# the game moving (a -> b: stage 1's opening view at 49 s, on screen from 48.5 s or before to
# ~49.7 s, against the camera turned to the first guard by 58 s; measured 23-26), the view
# turning with the head (yaw 30 degrees at 58.5 s, back at 61 s): b -> c (the turn, measured
# ~21) well beyond b -> d (the same pose 4 s on: the scene by itself, measured ~5). From ~50.3 s
# the camera stands in the first room while the guard shoots the player, so a pair there only
# differs when a shot lands on a hit's red flash: a at 55 s did that by chance (15 when it did,
# 2-4 when it didn't).
function Test-CmSim {
	$name = 'accept-cm-sim'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 64; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmStartsXr) + '49=shot a', '58=shot b', '58.5=pose head 0 1.6 0 30 0', '60=shot c', '61=pose head 0 1.6 0 0 0', '62=shot d'
		Expect = 'XRSIM: runtime up for "Confidential Mission VR" \(832x720 per eye, 72 Hz, /interaction_profiles/oculus/touch_controller\)',
			'XR: session created', 'XR: eye buffers 832x720', 'VR: camera profile for  CONFIDENTIAL MISSION ---------$'
		Reject = 'XR: initialisation failed' }
	if ($why) { return $why }
	$shots = 't049-a.png', 't058-b.png', 't060-c.png', 't062-d.png'
	$why = Test-RunShots $name $shots
	if ($why) { return $why }
	$a, $b, $c, $d = @($shots | ForEach-Object { Join-Path $repo "build-win\vrtest\$name\$_" })
	$why = Invoke-VrCheck 'blank', $a, $b, $c, $d, '--region', 'full', '--max', '0.5'
	if ($why) { return "a shot mostly black: $why" }
	$why = Invoke-VrCheck 'same', $b, $d, '--max-diff', '8'
	if ($why) { return "the scene moved too much for the head-turn check: $why" }
	$why = Invoke-VrCheck 'differ', $b, $c, '--min-diff', '12'
	if ($why) { return "the head turned, the view did not: $why" }
	$why = Invoke-VrCheck 'differ', $a, $b
	if ($why) { return "the game stood still: $why" }
	return $null
}

# The 1.1 button layout (Quest Touch) through the simulator, widened: A is player 1's Start
# (the starts), B player 1's B (the cabinet's second button: CM's own reload) at 56 s, X lets
# player 2 join at 60 s (stage 1's first room), Y is then player 2's B, the left menu button
# (the settings menu: last, at 70 s, as it pauses the game), the left stick click recenters
# (66 s), the right one turns the laser off (68 s; on by default). The controllers' profile is logged when the runtime
# names it. X is player 2's Start, never player 1's: A's starts (30-52 s) show as player 1
# Start lines before 53 s on the log's clock (so the count sees them), and none come after.
function Test-CmButtons {
	$name = 'accept-cm-buttons'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 74; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmStartsXr) + '56=xr right/b 1 200', '60=xr left/x 1 200', '62=xr left/y 1 200', '66=xr left/thumbstick 1 200',
			'68=xr right/thumbstick 1 200', '70=xr left/menu 1 200'
		Expect = 'Light gun P1 at .*start 1', 'Light gun P1 at .* B 1', 'XR: player 2 joins: a gun in each hand',
			'Light gun P2 at .* B 1', 'XR: menu button', 'XR: recenter \(button\)', 'XR: laser off',
			'XR: controllers: /interaction_profiles/oculus/touch_controller'
		Reject = 'XR: initialisation failed' }
	if ($why) { return $why }
	$a = Get-LogCount $name 'Light gun P1 at .*start 1' -To 53
	$x = Get-LogCount $name 'Light gun P1 at .*start 1' -From 53
	Write-Host "  player 1 Start lines: $a before 53 s (A, want some), $x from 53 s on (want 0: X is player 2's)"
	if ($a -lt 1) { return 'no player 1 Start line before 53 s: the timed count sees nothing' }
	if ($x -ne 0) { return "$x player 1 Start lines from 53 s on: X (60 s) gave player 1 a Start" }
	return $null
}

# The PC controllers' settings menu, in attract (no starts). Index: the left pad squeezed hard
# (trackpad/force) is the menu button, once. Vive wands (and Windows Mixed Reality): the left
# menu button is Start; held 0.8 s it is the menu instead. A short press (33 s) acts as the left
# Start when let go (player 2 joins, Start for player 2 by 35 s on the log's clock), a 1.2 s
# hold (36 s) opens the menu: one menu line, no second join and no player 2 Start from 35 s on.
function Test-CmButtonsPc {
	foreach ($pc in @(
			@{ Name = 'accept-cm-buttons-index'; Profile = 'valve/index_controller'; At = @('35=xr left/trackpad/force 1 300'); Expect = @() },
			@{ Name = 'accept-cm-buttons-vive'; Profile = 'htc/vive_controller'; At = @('33=xr left/menu 1 200', '36=xr left/menu 1 1200')
				Expect = @('XR: player 2 joins: a gun in each hand') })) {
		$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $pc.Name; Seconds = 40; XrSim = $true
			Config = "$cmWidenArcade,config:vr.XrSimProfile=$($pc.Profile)"; At = $pc.At
			Expect = @("XR: controllers: /interaction_profiles/$($pc.Profile)") + $pc.Expect; Reject = 'XR: initialisation failed' }
		if ($why) { return "$($pc.Profile): $why" }
		$n = Get-LogCount $pc.Name 'XR: menu button'
		Write-Host "  $n lines 'XR: menu button' in $($pc.Name) (want 1)"
		if ($n -ne 1) { return "$($pc.Profile): $n lines 'XR: menu button', want 1" }
		if ($pc.Profile -eq 'htc/vive_controller') {
			$tap = Get-LogCount $pc.Name 'Light gun P2 at .*start 1' -From 33 -To 35
			$hold = Get-LogCount $pc.Name 'Light gun P2 at .*start 1' -From 35
			$joins = Get-LogCount $pc.Name 'XR: player 2 joins'
			Write-Host "  player 2 Start lines: $tap at 33-35 s (the short press, want some), $hold from 35 s on (the hold, want 0); $joins joins (want 1)"
			if ($tap -lt 1) { return "$($pc.Profile): no player 2 Start from the short press (33-35 s)" }
			if ($hold -ne 0) { return "$($pc.Profile): $hold player 2 Start lines from 35 s on: the menu hold was a Start too" }
			if ($joins -ne 1) { return "$($pc.Profile): $joins lines 'XR: player 2 joins', want 1" }
		}
	}
	return $null
}

# The VR menu pauses the emulator (the left menu button opens and closes it, B closes it too),
# through the simulator, widened. Opened on the 2D gun adjustment screen (40.5-41.5 s, between
# the A starts) and in stage 1's first room (58 s): the picture holds still (p1 -> p2) yet turns
# with the head (yaw 25 degrees at 61 s: p3), B resumes it at 63 s and the game moves on (r1 ->
# r2). Then 20 quick open/close cycles (70-109 s), and one open across a focus loss and return
# (114-116 s, the headset's own menu): still paused and still (f1 -> f2), closed at 119 s, moving
# again (q1 -> q2). No paused shot black, no black frames, 23 opens and 23 closes, one redraw
# line each.
function Test-CmPause {
	$name = 'accept-cm-pause'
	$cycles = @(for ($t = 70; $t -le 109; $t++) { "$t=xr left/menu 1 150" })
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 125; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmStartsXr) + @('40.5=xr left/menu 1 150', '41=shot g', '41.5=xr left/menu 1 150',
			'58=xr left/menu 1 150', '59=shot p1', '60.5=shot p2', '61=pose head 0 1.6 0 25 0', '62=shot p3', '62.5=pose head 0 1.6 0 0 0',
			'63=xr right/b 1 150', '64=shot r1', '67=shot r2') + $cycles + @('113=xr left/menu 1 150', '114=xrstate visible',
			'116=xrstate focused', '117=shot f1', '118.5=shot f2', '119=xr left/menu 1 150', '121=shot q1', '123=shot q2')
		Expect = 'XR: menu open \(paused\)', 'XR: menu closed \(resumed\)', 'XR: paused, redrawing the last frame'
		Reject = 'XR: initialisation failed', 'XR: \d+ black frames' }
	if ($why) { return $why }
	foreach ($re in 'XR: menu open \(paused\)', 'XR: menu closed \(resumed\)', 'XR: paused, redrawing the last frame') {
		$n = Get-LogCount $name $re
		Write-Host "  $n lines '$re' (want 23)"
		if ($n -ne 23) { return "$n lines '$re', want 23" }
	}
	# B that closes the menu (63 s, the only B here) is no reload for the game
	$n = Get-LogCount $name 'Light gun P1 at .* B 1'
	Write-Host "  $n lines 'Light gun P1 at .* B 1' (want 0: B closed the menu)"
	if ($n -ne 0) { return "$n player 1 B lines: B that closed the menu went to the game too" }
	$shots = 't041-g.png', 't059-p1.png', 't060.5-p2.png', 't062-p3.png', 't064-r1.png', 't067-r2.png',
		't117-f1.png', 't118.5-f2.png', 't121-q1.png', 't123-q2.png'
	$why = Test-RunShots $name $shots
	if ($why) { return $why }
	$g, $p1, $p2, $p3, $r1, $r2, $f1, $f2, $q1, $q2 = @($shots | ForEach-Object { Join-Path $repo "build-win\vrtest\$name\$_" })
	$why = Invoke-VrCheck 'blank', $p1, $p2, $p3, '--region', 'full', '--max', '0.5'
	if ($why) { return "a paused shot mostly black: $why" }
	# (g and f1 are 2D screens: a picture in the middle of the view, black around it by design.
	# The strip between the picture's left edge, x ~0.24 in g, and the panel, from x ~0.37:
	# measured 0.15 and 0.00)
	$why = Invoke-VrCheck 'blank', $g, $f1, '--region', '0.28,0.36,0.35,0.64', '--max', '0.5'
	if ($why) { return "a paused 2D screen black: $why" }
	$why = Invoke-VrCheck 'same', $p1, $p2, '--max-diff', '1.5'
	if ($why) { return "the paused picture moved: $why" }
	$why = Invoke-VrCheck 'same', $f1, $f2, '--max-diff', '1.5'
	if ($why) { return "the paused picture moved after the focus came back: $why" }
	$why = Invoke-VrCheck 'differ', $p2, $p3
	if ($why) { return "paused, the head turned, the view did not: $why" }
	# (paused, the picture is the same to the bit: diff 0.000. Going on, r1 -> r2 is the continue
	# countdown, mostly still: measured 3.4)
	$why = Invoke-VrCheck 'differ', $r1, $r2, '--min-diff', '2'
	if ($why) { return "the game stood still after B closed the menu: $why" }
	$why = Invoke-VrCheck 'differ', $q1, $q2, '--min-diff', '2'
	if ($why) { return "the game stood still after the menu closed: $why" }
	return $null
}

# The settings menu's place, as its open line says it (xr_menu.cpp): 1.2 m ahead of the head
# (0, 1.6, 0) at 1.5 m (0.1 m below the eyes), 0.9 m wide, its 10 rows 0.063 m apart, the top
# row's middle 0.274 m over the panel's. -Row and -U (0..1 across): where the simulator's right
# hand at (0.18, 1.3, -0.35) points ("yaw pitch", degrees) to put its pointer there. The pointer
# leaves the arcade gun's muzzle (vr.GunScale 0.68), which in the simulator, where the grip pose
# is the aim pose, is 4.4 cm over the hand and 13.7 cm ahead of it: a few rounds of aiming from it.
$cmMenuPitch = 0.063; $cmMenuTop = 0.274; $cmMenuWidth = 0.9
function Get-CmMenuAim([int]$Row, [double]$U) {
	$tx = ($U - 0.5) * $cmMenuWidth; $ty = 1.5 + $cmMenuTop - $Row * $cmMenuPitch; $tz = -1.2
	$yaw = 0.0; $pitch = 0.0
	for ($i = 0; $i -lt 5; $i++) {
		$my = 0.0443 * [Math]::Cos($pitch) + 0.1374 * [Math]::Sin($pitch)
		$mz = 0.0443 * [Math]::Sin($pitch) - 0.1374 * [Math]::Cos($pitch)
		$dx = $tx - (0.18 + $mz * [Math]::Sin($yaw)); $dy = $ty - (1.3 + $my); $dz = $tz - (-0.35 + $mz * [Math]::Cos($yaw))
		$yaw = [Math]::Atan2(-$dx, -$dz); $pitch = [Math]::Atan2($dy, [Math]::Sqrt($dx * $dx + $dz * $dz))
	}
	return '{0} {1}' -f (T ($yaw * 180 / [Math]::PI)), (T ($pitch * 180 / [Math]::PI))
}

# The settings menu through the simulator, widened, in stage 1's first room: the left menu button
# opens it at 58 s (at Resume), the right stick down 4 times to Haptics, A (+ on a value row) makes
# it Strong with a sample knock at that strength (0.03 s x 1.6), down once more to World size,
# right (+) makes it 0.121 (0.110 x 1.1), the right hand points at the Laser row's right half at
# 63 s and its trigger turns the laser off, B closes it at 65 s. That knock is the only one. Each
# change is in emu.cfg at once (the run's, emu.cfg.after); build-win\emu.cfg is as it was.
function Test-CmMenu {
	$name = 'accept-cm-menu'
	$downs = @(foreach ($t in 58.5, 59, 59.5, 60) { "$(T $t)=stick right 0 -1 150" })
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 72; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmStartsXr) + '58=xr left/menu 1 150' + $downs + @('60.5=xr right/a 1 150', '61=stick right 0 -1 150',
			'62=stick right 1 0 150', "63=pose right 0.18 1.3 -0.35 $(Get-CmMenuAim -Row 3 -U 0.75)", '63.5=xr right/trigger 1 100',
			'65=xr right/b 1 150')
		Expect = 'XR: menu button', 'XR: menu open \(paused\)',
			"XR: menu at 1\.20 m, 10 rows, row pitch $($cmMenuPitch.ToString('0.000', $inv) -replace '\.', '\.') m, top row \+$($cmMenuTop.ToString('0.000', $inv) -replace '\.', '\.') m",
			'XR: menu Haptics -> Strong', 'XRSIM: haptic right 0\.048 1\.000', 'XR: menu World size -> 0\.121', 'XR: menu Laser -> Off',
			'XR: menu closed \(resumed\)'
		Reject = 'XR: initialisation failed', 'XR: \d+ black frames' }
	if ($why) { return $why }
	$n = Get-LogCount $name 'XRSIM: haptic'
	Write-Host "  $n lines 'XRSIM: haptic' (want 1: the Haptics row's sample)"
	if ($n -ne 1) { return "$n haptic lines, want 1 (the sample knock only)" }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$after = Join-Path $dir 'emu.cfg.after'
	if (-not (Test-Path -LiteralPath $after)) { return "no emu.cfg.after in $dir" }
	foreach ($re in '^vr\.Haptics = 3$', '^vr\.Laser = no$', '^vr\.WorldScale = 0\.121') {
		$hit = Select-String -LiteralPath $after -Pattern $re -CaseSensitive -Quiet
		Write-Host "  emu.cfg.after $(if ($hit) { 'has' } else { 'lacks' }) $re"
		if (-not $hit) { return "the run's emu.cfg has no line $re" }
	}
	$cfg = Join-Path $repo 'build-win\emu.cfg'
	$before = Join-Path $dir 'emu.cfg.before'
	$same = if (Test-Path -LiteralPath $before) { (Get-FileHash -LiteralPath $before).Hash -eq (Get-FileHash -LiteralPath $cfg).Hash } else { -not (Test-Path -LiteralPath $cfg) }
	Write-Host "  build-win\emu.cfg $(if ($same) { 'as it was' } else { 'changed' })"
	if (-not $same) { return 'build-win\emu.cfg changed by the run' }
	return $null
}

# Quit to the start screen from the settings menu: opened at 58 s, the stick up from Resume wraps
# to Quit, A: Flycast leaves (dc_exit on the PC; the headset goes back to the start screen), so
# vr-test.ps1 ends early with exit code 2.
function Test-CmMenuQuit {
	$name = 'accept-cm-menu-quit'
	$params = @{ Game = 'confmiss'; Name = $name; Seconds = 70; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmStartsXr) + '58=xr left/menu 1 150', '58.5=stick right 0 1 150', '59=xr right/a 1 150', '59.5=xr right/a 1 150'
		Expect = 'XR: menu open \(paused\)', 'XR: menu quit armed', 'XR: menu quit$', 'XR: quitting'; Reject = 'XR: initialisation failed' }
	if ($Reuse -and (Test-Path -LiteralPath (Get-RunLog $name))) { return Invoke-VrTest $params }
	$out = @(& $vrTest @params)
	$code = $LASTEXITCODE
	$out | ForEach-Object { Write-Host "  $_" }
	$fails = @($out | Where-Object { $_ -match '^FAIL' })
	if ($fails) { return $fails -join '; ' }
	if ($code -ne 2) { return "vr-test.ps1 exit $code, want 2 (Flycast left early)" }
	# left on its own: Flycast's exit code 0 (a crash leaves early too, with another code)
	if (-not ($out -match '^Flycast exited early, at .* \(code 0\)$')) { return "Flycast did not exit with code 0: $(($out -match 'exited early') -join '; ')" }
	# one choose only arms Quit: "quit armed" at the first A (59 s), "quit" at the second (59.5 s)
	$log = Get-RunLog $name
	$armed = Select-String -LiteralPath $log -Pattern 'XR: menu quit armed' -CaseSensitive | Select-Object -First 1
	$quit = Select-String -LiteralPath $log -Pattern 'XR: menu quit$' -CaseSensitive | Select-Object -First 1
	$at = { param($hit) $m = [regex]::Match($hit.Line, '^(\d+):(\d+):(\d+)'); [int]$m.Groups[1].Value * 60 + [int]$m.Groups[2].Value + [int]$m.Groups[3].Value / 1000 }
	$gap = (& $at $quit) - (& $at $armed)
	if ($quit.LineNumber -lt $armed.LineNumber -or $gap -lt 0.3) { return ("quit at the first choose: armed at line {0}, quit at line {1}, {2:0.00} s later" -f $armed.LineNumber, $quit.LineNumber, $gap) }
	Write-Host ("  'XR: menu quit armed' then 'XR: menu quit' {0:0.00} s later (the second A)" -f $gap)
	return $null
}

# The haptics (the knock from the game's own shot counter, vr.Haptics) through the simulator,
# widened, in stage 1's first room: 8 right pulls 0.6 s apart from 54 s (6 shots, then 2 on an
# empty gun), B (the game's reload) at 59.5 s, 3 more shots from 61 s, X (player 2 joins) at 64 s
# and 2 left pulls from 66 s. The simulator logs each knock: "XRSIM: haptic right 0.030 1.000".
function Get-CmHapticsActions {
	@(Get-CmStartsXr)
	for ($i = 0; $i -lt 8; $i++) { "$(T (54 + 0.6 * $i))=xr right/trigger 1 100" }
	'59.5=xr right/b 1 200'
	for ($i = 0; $i -lt 3; $i++) { "$(T (61 + 0.6 * $i))=xr right/trigger 1 100" }
	'64=xr left/x 1 200'
	for ($i = 0; $i -lt 2; $i++) { "$(T (66 + 0.6 * $i))=xr left/trigger 1 100" }
}

# Normal strength: a Shot knock (0.030 s, 1.0) for each of the 9 shots, a light click (0.012 s,
# 0.35) for each of the 2 empty pulls, player 2's shots in the left hand; the magazine stays 6
# (its earlier changes: the game clearing its RAM at ~11 s, 16 -> 255 -> 0, and the reload
# lesson's 0 -> 6 at ~30 s).
function Test-CmHaptics {
	$name = 'accept-cm-haptics'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 75; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmHapticsActions); Expect = 'XR: player 2 joins: a gun in each hand'
		Reject = 'XR: initialisation failed', 'VR: P1 magazine 6 -> ' }
	if ($why) { return $why }
	$shot = Get-LogCount $name 'XRSIM: haptic right 0\.030 1\.000'
	$empty = Get-LogCount $name 'XRSIM: haptic right 0\.012 0\.350'
	$left = Get-LogCount $name 'XRSIM: haptic left 0\.030 1\.000'
	Write-Host "  right: $shot shot knocks (want 9), $empty empty clicks (want 2); left: $left shot knocks (want 1 or more)"
	if ($shot -ne 9) { return "$shot right shot knocks, want 9" }
	if ($empty -ne 2) { return "$empty right empty clicks, want 2" }
	if ($left -lt 1) { return 'no left shot knock (player 2)' }
	return $null
}

# vr.Haptics=1 (low): the same pulls, every knock at 0.45 of its amplitude, none at 1.0.
function Test-CmHapticsLow {
	return Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-haptics-low'; Seconds = 75; XrSim = $true
		Config = "$cmWidenArcade,config:vr.Haptics=1"; At = @(Get-CmHapticsActions)
		Expect = 'XR: player 1 shot at', 'XRSIM: haptic right 0\.030 0\.450'
		Reject = 'XR: initialisation failed', 'XRSIM: haptic .* 1\.000$' }
}

# vr.Haptics=0 (off): the same pulls, no haptics at all.
function Test-CmHapticsOff {
	return Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-haptics-off'; Seconds = 75; XrSim = $true
		Config = "$cmWidenArcade,config:vr.Haptics=0"; At = @(Get-CmHapticsActions)
		Expect = 'XR: player 1 shot at'; Reject = 'XR: initialisation failed', 'XRSIM: haptic' }
}

# Player 1's magazine (+0x50) poked to the rifle's 45 at 60 s: the change is logged.
function Test-CmHapticsMag {
	return Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-haptics-mag'; Seconds = 75; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmStartsXr) + '60=poke 0c155808 1 2d'
		Expect = 'VR: poke 0c155808 = 2d', 'VR: P1 magazine 6 -> 45'; Reject = 'XR: initialisation failed' }
}

# Reloads: the right controller points at the floor from 53.5 s and pulls 4 times from 54 s
# (off the screen: the game's reload). No knock at all, neither a Shot nor a click.
function Test-CmHapticsReload {
	return Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-haptics-reload'; Seconds = 75; XrSim = $true; Config = $cmWidenArcade
		At = @(Get-CmStartsXr) + '53.5=pose right 0.18 1.3 -0.35 0 -80' + @(for ($i = 0; $i -lt 4; $i++) { "$(T (54 + 0.6 * $i))=xr right/trigger 1 100" })
		Expect = 'XR: player 1 shot off screen \(reload\)'; Reject = 'XR: initialisation failed', 'XRSIM: haptic right' }
}

# Task 4's scene keys (vr-test.ps1 -On, the poke action, vr.WatchRam): player 1's ammo
# (+0x4C) and +0x64 watched (the game research's "state", which only changes at boot: 01 -> ff
# -> 00 at ~11 s), the ammo poked to 3 at 56 s (stage 1's first room), a window shot keyed to the
# ammo filling (00 -> 06: first at ~30 s, the reload tutorial; the key acts once).
function Test-SpikeKeys {
	$name = 'accept-spike-keys'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 60
		Config = 'config:vr.WatchRam=0c15581c,0c155804'
		At = @(Get-CmStarts) + '56=poke 0c155804 1 03'
		On = 'VR: ram 0c155804 00 -> 06|0.5|shot keyed'
		Expect = 'VR: ram 0c15581c', 'VR: poke 0c155804 = 03', 'VR: ram 0c155804 .. -> 03' }
	if ($why) { return $why }
	$keyed = @(Get-ChildItem -LiteralPath (Join-Path $repo "build-win\vrtest\$name") -Filter 't*-keyed.png')
	Write-Host "  $($keyed.Count) keyed window shots ($($keyed.Name -join ', '))"
	if ($keyed.Count -ne 1) { return "want 1 keyed window shot t*-keyed.png in $name, got $($keyed.Count)" }
	return $null
}

# The controller models (xr_controllers.h) through the simulator, before play starts (S36: the controllers are drawn only
# while how to play shows): the free (left) hand moved in front of the left eye at 24 s, a window shot with it there at
# 25 s, the hand moved away (below and behind the head) at 25.5 s and another shot at 26 s. The left eye's view differs
# where the controller was ($cmControllerBox: around it; with the built-in shape by ~17). The same again in stage 1's
# first room at 56-58 s, in play, where the two shots differ there by ~0.3 without a controller: the left hand holds no
# gun, so nothing is drawn there now (cm-controllers checks it). The simulator lists no XR_FB_render_model: the built-in shape.
$cmControllerBox = '0.32,0.62,0.5,0.88'
function Get-CmControllersParams([string]$name, [string]$config) {
	@{ Game = 'confmiss'; Name = $name; Seconds = 62; XrSim = $true; Config = $config
		At = @('24=pose left -0.1 1.45 -0.3 0 0', '25=shot in', '25.5=pose left -0.1 0.2 0.5 0 0', '26=shot out') + @(Get-CmStartsXr) +
			'56=pose left -0.1 1.45 -0.3 0 0', '57=shot playin', '57.5=pose left -0.1 0.2 0.5 0 0', '58=shot playout'
		Reject = @('XR: initialisation failed') }
}
function Test-CmControllers { Invoke-OwnFolders { Test-CmControllersRun } }
function Test-CmControllersRun {
	$name = 'accept-cm-controllers'
	$params = Get-CmControllersParams $name $cmWidenArcade
	$params.Expect = 'XR: runtime extensions: ', 'XR: controller models built in \(the runtime has no XR_FB_render_model\)',
		'XR: controllers shown: left and right$', 'XR: controllers shown: none$', 'XR: labels: left x at .* in the grip pose \(built in\)$'
	$params.Reject += 'XR: runtime extensions: .*XR_FB_render_model', 'XR: controller models from the runtime', 'XR: labels: .*\(from the file\)'
	$why = Invoke-VrTest $params
	if ($why) { return $why }
	$why = Test-RunShots $name 't025-in.png', 't026-out.png', 't057-playin.png', 't058-playout.png'
	if ($why) { return $why }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-in.png'), (Join-Path $dir 't026-out.png'), '--box', $cmControllerBox
	if ($why) { return "no controller in the left hand: $why" }
	$why = Invoke-VrCheck 'same', (Join-Path $dir 't057-playin.png'), (Join-Path $dir 't058-playout.png'), '--box', $cmControllerBox
	if ($why) { return "a controller drawn in the left hand in play: $why" }
	# the controllers go with the first Start (how to play ends) and stay gone
	$n = Get-LogCount $name 'XR: controllers shown: ' -From 29 -To 62
	$none = Get-LogCount $name 'XR: controllers shown: none$' -From 29 -To 62
	Write-Host "  $n lines 'XR: controllers shown' at 29-62 s, $none of them none (want 1 and 1: the first Start)"
	if ($n -ne 1 -or $none -ne 1) { return "$n lines 'XR: controllers shown' after the first Start ($none none), want one none" }
	return $null
}

# The same with vr.XrSimRenderModel: the simulator lists XR_FB_render_model and serves the test
# GLB (re\make-test-glb.py: an 8 cm box and a 12 cm cylinder, 140 triangles, a 2x2 PNG) as both
# controllers' model; the hand in front of the eye then looks unlike the built-in shape. As a
# Quest, the simulator has no model until the interaction profile is bound (at the first
# xrSyncActions while focused, with its event): the models are unavailable at the first FOCUSED
# state and come with the retry at that event.
function Test-CmControllersRt { Invoke-OwnFolders { Test-CmControllersRtRun } }
function Test-CmControllersRtRun {
	$name = 'accept-cm-controllers-rt'
	$glb = Join-Path $repo 'build-win\vrtest\test-controller.glb'
	if (-not (Test-Path -LiteralPath $glb)) {
		& python -I (Join-Path $PSScriptRoot 're\make-test-glb.py') $glb | ForEach-Object { Write-Host "  $_" }
		if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $glb)) { return "make-test-glb.py could not write $glb" }
	}
	$params = Get-CmControllersParams $name "$cmWidenArcade,config:vr.XrSimRenderModel=$glb"
	$params.Expect = 'XR: runtime extensions: .*XR_FB_render_model', 'XR: controller models built in \(left and right models unavailable\)',
		'XR: controller models from the runtime \(left 140 triangles, right 140\)'
	$params.Reject += 'XR: controller models built in \(the runtime has no XR_FB_render_model'
	$why = Invoke-VrTest $params
	if ($why) { return $why }
	# unavailable first, then from the runtime: the retry
	$log = Get-RunLog $name
	$first = Select-String -LiteralPath $log -Pattern 'XR: controller models built in \(.*unavailable' -CaseSensitive | Select-Object -First 1
	$loaded = Select-String -LiteralPath $log -Pattern 'XR: controller models from the runtime' -CaseSensitive | Select-Object -First 1
	if (-not $first -or -not $loaded -or $first.LineNumber -ge $loaded.LineNumber) {
		return 'want the models unavailable first and then from the runtime (the retry)'
	}
	Write-Host "  PASS retry: unavailable (line $($first.LineNumber)), then from the runtime (line $($loaded.LineNumber))"
	$why = Test-RunShots $name 't025-in.png', 't026-out.png'
	if ($why) { return $why }
	$builtIn = Join-Path $repo 'build-win\vrtest\accept-cm-controllers\t025-in.png'
	if (-not (Test-Path -LiteralPath $builtIn) -and -not $Reuse) {
		Write-Host '  (no cm-controllers shot to compare with: playing accept-cm-controllers)'
		$why = Test-CmControllers
		if ($why) { return "cm-controllers: $why" }
	}
	if (-not (Test-Path -LiteralPath $builtIn)) { return "no $builtIn to compare with" }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-in.png'), (Join-Path $dir 't026-out.png'), '--box', $cmControllerBox
	if ($why) { return "no controller in the left hand: $why" }
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-in.png'), $builtIn, '--box', $cmControllerBox
	if ($why) { return "the runtime's model looks like the built-in one: $why" }
	return $null
}

# What each hand holds and the how-to-play labels (Task 9, S36, S36b) through the simulator, widened: the right hand
# in front of the left eye from 20 s, a controller with its labels (shots at 25, 27 s); the first A (30 s) gives it the
# gun, and the labels and the controllers go (shot at 35 s): once play starts a hand without a gun draws nothing. X lets
# player 2 join at 60 s (the left hand takes its gun), player 2 idles out at ~70 s (findings p2out: the left hand
# without a gun again). The menu at 90 s, the right stick up twice (from Resume it wraps to Quit, then How to play) and
# A at 91.5 s: how to play on (both hands controllers with labels), the menu still open; A again at 92.5 s turns it
# off, the stick's right at 93.5 s on (it's an On/Off row). The right hand's ray stays on the panel: from the arcade
# gun's muzzle on Haptics, from the controller 4.4 cm lower on World size. So each swap moves it to another row, and
# the selection must stay on How to play: every press lands there and no other row changes. The menu button closes
# the menu at 95 s, which ends it: the gun back, nothing drawn in the left hand. The trigger's pull at 96 s is the
# game's: it fires. The menu again at 97 s, How to play chosen with the right trigger at 98.5 s (held 1 s) and the
# menu closed by B at 99 s while it is held: that pull is the menu's, nothing fires; the next pull (101 s, the hand
# back on the game) does. The checks count lines between the menu's own open and close lines in the log.
function Test-CmHands {
	$name = 'accept-cm-hands'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 104; XrSim = $true; Config = $cmWidenArcade
		At = @('20=pose right 0.1 1.45 -0.3 0 0', '25=shot labels', '27=shot labels2') + @(Get-CmStartsXr) + @('35=shot gun', '60=xr left/x 1 200',
			'90=xr left/menu 1 150', '90.5=stick right 0 1 150', '91=stick right 0 1 150', '91.5=xr right/a 1 150', '92.5=xr right/a 1 150',
			'93.5=stick right 1 0 150', '95=xr left/menu 1 150', '96=xr right/trigger 1 100', '97=xr left/menu 1 150',
			"97.5=pose right 0.18 1.3 -0.35 $(Get-CmMenuAim -Row 8 -U 0.5)", '98.5=xr right/trigger 1 1000', '99=xr right/b 1 150',
			'100=pose right 0.1 1.45 -0.3 0 0', '101=xr right/trigger 1 100')
		Expect = 'XR: labels off \(start\)', 'XR: right hand: controller -> gun \(start\)',
			'XR: left hand: controller -> gun \(player 2 joins\)', 'XR: left hand: gun -> controller \(player 2 out\)',
			'XR: menu How to play$', 'XR: how to play on$', 'XR: right hand: gun -> controller \(how to play\)', 'XR: labels on \(how to play\)',
			'XR: how to play off \(chosen\)$', 'XR: right hand: controller -> gun \(chosen\)', 'XR: how to play off \(menu closed\)$',
			'XR: right hand: controller -> gun \(menu closed\)', 'XR: labels off \(menu closed\)', 'XR: menu pointer click on How to play'
		Reject = @('XR: initialisation failed', 'XR: show controls', 'XR: labels off \(shot\)') }
	if ($why) { return $why }
	# The windows: the menu's first open to its close (o1-c1), its second (o2-c2), from the log's lines
	$opens = @(Get-LogClocks $name 'XR: menu at ')
	$closes = @(Get-LogClocks $name 'XR: menu closed')
	$o1 = $opens | Select-Object -First 1
	$c1 = $closes | Where-Object { $_ -gt $o1 } | Select-Object -First 1
	$o2 = $opens | Where-Object { $_ -gt $c1 } | Select-Object -First 1
	$c2 = $closes | Where-Object { $_ -gt $o2 } | Select-Object -First 1
	if ($opens.Count -lt 2 -or $null -eq $c1 -or $null -eq $o2 -or $null -eq $c2) {
		return "the menu didn't open and close twice (opened at $($opens -join ', ') s, closed at $($closes -join ', ') s)"
	}
	Write-Host "  the menu open at $o1-$c1 s and $o2-$c2 s (the log's clock)"
	$rows = 'XR: menu (Resume|Recenter|Gun|Laser|Haptics|World size|Panel distance|Wider view|quit)'
	# from, to (s, the log's clock), the regex, the count wanted ('+': at least one), what it shows
	$checks = @(
		@($o1, $c1, 'XR: menu How to play$', 3, 'each press in the first menu lands on How to play, the menu staying open'),
		@($o1, $c1, 'XR: how to play on$', 2, 'A on the row turns it on, and the stick''s right'),
		@($o1, $c1, 'XR: how to play off \(chosen\)$', 1, 'A again turns it off'),
		@($o1, $c1, $rows, 0, 'the swaps of gun and controller move no selection: no other row changes'),
		@($o1, $c1, 'XR: controllers shown: left and right$', '+', 'how to play draws both controllers'),
		@($c1, ($c1 + 0.5), 'XR: how to play off \(menu closed\)$', 1, 'closing the menu ends it'),
		@($c1, ($c1 + 0.5), 'XR: controllers shown: none$', 1, 'nothing drawn once the menu closed'),
		@($c1, $o2, 'XR: player 1 shot', '+', 'the pull after the close is the game''s'),
		@($c1, $o2, 'XR: controllers shown: (left|right)', 0, 'no controller drawn with the menu closed'),
		@($o2, $c2, 'XR: how to play on$', 1, 'the trigger on the row turns it on'),
		@($o2, $c2, 'XR: controllers shown: left and right$', '+', 'how to play draws both controllers'),
		@($o2, $c2, $rows, 0, 'no other row changes'),
		@($c2, ($c2 + 0.5), 'XR: how to play off \(menu closed\)$', 1, 'B ends it'),
		@($o2, ($c2 + 1.0), 'XR: player 1 shot', 0, 'the pull held through the close is the menu''s'),
		@($c2, 1e9, 'XR: controllers shown: (left|right)', 0, 'no controller drawn with the menu closed'),
		@(($c2 + 1.0), 1e9, 'XR: player 1 shot', '+', 'the next pull is the game''s'))
	foreach ($c in $checks) {
		$n = Get-LogCount $name $c[2] -From $c[0] -To $c[1]
		$good = if ("$($c[3])" -eq '+') { $n -ge 1 } else { $n -eq $c[3] }
		Write-Host "  $(if ($good) { 'PASS' } else { 'FAIL' }) $n lines '$($c[2])' at $($c[0])-$($c[1]) s (want $($c[3]): $($c[4]))"
		if (-not $good) { return "$($c[4]): $n lines '$($c[2])' at $($c[0])-$($c[1]) s, want $($c[3])" }
	}
	$why = Test-RunShots $name 't025-labels.png', 't027-labels2.png', 't035-gun.png'
	if ($why) { return $why }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-labels.png'), (Join-Path $dir 't035-gun.png'), '--box', '0.3,0.3,0.9,0.95'
	if ($why) { return "the labels and the gun look the same: $why" }
	# The game's screen changes between those two as well. The left hand's part of the view: its controller and labels at
	# 25 s, nothing there once play starts (S36); and with nothing done (25, 27 s) the same.
	$left = '0.25,0.74,0.43,0.9'
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-labels.png'), (Join-Path $dir 't035-gun.png'), '--box', $left
	if ($why) { return "the left hand's controller didn't go: $why" }
	$why = Invoke-VrCheck 'same', (Join-Path $dir 't025-labels.png'), (Join-Path $dir 't027-labels2.png'), '--box', $left
	if ($why) { return "the left hand's labels changed with nothing done: $why" }
	return $null
}

# The pull that gives back the gun ('labels off (shot)') fires too: player 1's shot logged within
# 0.2 s of each such line.
function Test-ArmingShot([string]$name) {
	$offs = @(Select-String -LiteralPath (Get-RunLog $name) -Pattern 'XR: labels off \(shot\)' -CaseSensitive)
	if (-not $offs) { return "no 'XR: labels off (shot)' in $name" }
	foreach ($off in $offs) {
		if ($off.Line -notmatch '^(\d+):(\d\d):(\d{3}) ') { return "no time on '$($off.Line)'" }
		$t = [int]$Matches[1] * 60 + [int]$Matches[2] + [int]$Matches[3] / 1000
		$n = Get-LogCount $name 'XR: player 1 shot' -From $t -To ($t + 0.2)
		Write-Host "  $n lines 'XR: player 1 shot' within 0.2 s of 'labels off (shot)' at $t s (want 1 or more)"
		if ($n -lt 1) { return "the pull that gave back the gun at $t s didn't fire" }
	}
	return $null
}

# The first trigger pull (28 s, no Start before it) gives the right hand the gun and fires; the
# labels go with it.
function Test-CmHandsShot {
	$name = 'accept-cm-hands-shot'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 34; XrSim = $true; Config = $cmWidenArcade
		At = @('28=xr right/trigger 1 100')
		Expect = 'XR: right hand: controller -> gun \(shot\)', 'XR: labels off \(shot\)'
		Reject = @('XR: initialisation failed') }
	if ($why) { return $why }
	return Test-ArmingShot $name
}

#
# The gun run (Task 10, core/rend/vr/hands_rip.h; CM VR 1.2 Task 2): with no gun-cm-pistol.bin in build-win\data
# the first start runs the game by itself (on the PC with vr.HandsPrep): Start at 30 s of game, the reload tutorial,
# and from $cmPistolAfter s after Start (hands_rip.cpp CmPistolAfter) Start every 2 s on to the stage 1 intro, where
# the agents' pistol becomes gun-cm-pistol.bin (HND1 v2, gun only). Up to 1.1 the tutorial's cabinet gun came first
# (gun-cm-cabinet.bin) and the presses waited for it: no 1.2 run makes or reads that file. Remove-CmGuns deletes
# every gun-cm-*.bin, an old cabinet gun's too. The done budgets are the measured runs' game seconds + 20 (Task 2).
#
$cmGunData = Join-Path $repo 'build-win\data'
$cmCabinetFile = Join-Path $cmGunData 'gun-cm-cabinet.bin'	# 1.1's: checked never made
$cmPistolAfter = 2
$cmRipDone = 94		# a run from Start (the rifle's seed in place)
$cmRipRifleDone = 379	# the whole first start, the rifle's part first
function Remove-CmGuns {
	Get-ChildItem -LiteralPath $cmGunData -Filter 'gun-cm-*.bin*' -ErrorAction SilentlyContinue | Remove-Item -Force
}
# Task 12: a run without the rifle's file takes it first, from the attract demo at ~316 s of game (about
# a minute more on the PC). The scenarios about the other guns start with cm-rip-rifle's copy of it in
# place, so their runs start at Start as before (no knob in the app for it).
$cmRifleFile = Join-Path $cmGunData 'gun-cm-rifle.bin'
$cmRifleSeed = Join-Path $repo 'build-win\vrtest\cm-rifle-seed.bin'
function Copy-CmRifleSeed {
	if (-not (Test-Path -LiteralPath $cmRifleSeed)) { return "no ${cmRifleSeed}: run cm-rip-rifle first (it makes it; the full list does)" }
	Copy-Item -LiteralPath $cmRifleSeed -Destination $cmRifleFile -Force
	return $null
}
# The log's clock (mm:ss:fff at the start of each line) at the first line matching $pattern, in seconds;
# it throws (the scenario fails) when there's no such line or it has no clock.
function Get-LogClock([string]$log, [string]$pattern) {
	$hit = Select-String -LiteralPath $log -Pattern $pattern -CaseSensitive | Select-Object -First 1
	if (-not $hit) { throw "no line matching $pattern in $log" }
	if ($hit.Line -notmatch '^(\d+):(\d+):(\d+)') { throw "no clock at the start of line $($hit.LineNumber) of $log" }
	return [int]$Matches[1] * 60 + [int]$Matches[2] + [int]$Matches[3] / 1000.0
}
# The number in group 1 of the first line matching $pattern; it throws (the scenario fails) when there's no such line.
function Get-LogNumber([string]$log, [string]$pattern) {
	$hit = Select-String -LiteralPath $log -Pattern $pattern -CaseSensitive | Select-Object -First 1
	if (-not $hit) { throw "no line matching $pattern in $log" }
	return [double]::Parse($hit.Matches[0].Groups[1].Value, $inv)
}

# The run, flat (CM VR 1.2): no files but the rifle's seed, a .part left by a run that died (written over); Start at
# 30 s of game, the first press for the pistol $cmPistolAfter s after it (+-2 s), the agents' pistol saved complete,
# no line about a cabinet gun and no gun-cm-cabinet.bin, the run done by $cmRipDone s of game; the pistol's checks
# (vrcheck hnd, as cm-rip-pistol's). Then "-next", a 1.1 install: both guns' files and an old gun-cm-cabinet.bin, no
# run ("made before": the pistol's file), and the old file never read or changed.
function Test-CmRipStart {
	$name = 'accept-cm-rip-start'
	if (-not $Reuse) {
		Remove-CmGuns
		$why = Copy-CmRifleSeed
		if ($why) { return $why }
		[IO.File]::WriteAllText("$cmPistolFile.part", 'junk from a run that died')
	}
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 150; Config = 'config:vr.HandsPrep=yes'
		Expect = 'VR hands: no model yet, taking the guns from the game', 'VR hands: Start at [\d.]+ s',
			'VR hands: on to the merged guns at [\d.]+ s', $cmPistolSaved, 'VR hands: done after \d+ s of game'
		Reject = @('VR hands: .*, but ', 'cabinet') }
	if ($why) { return $why }
	$log = Get-RunLog $name
	$start = Get-LogNumber $log 'VR hands: Start at ([\d.]+) s'
	$press = Get-LogNumber $log 'VR hands: on to the merged guns at ([\d.]+) s'
	$n = Get-LogNumber $log 'VR hands: done after (\d+) s of game'
	$want = 30 + $cmPistolAfter
	Write-Host "  Start at $start s, the first press for the pistol at $press s (want $want +-2), done after $n s of game (want $cmRipDone or less)"
	if ([math]::Abs($press - $want) -gt 2) { return "the first press for the pistol at $press s of game, want $want +-2" }
	if ($n -gt $cmRipDone) { return "the gun run took $n s of game, over $cmRipDone" }
	if (Test-Path -LiteralPath "$cmPistolFile.part") { return "$cmPistolFile.part is still there" }
	if (Test-Path -LiteralPath $cmCabinetFile) { return "$cmCabinetFile made" }
	if (-not (Test-Path -LiteralPath $cmPistolFile)) { return "no $cmPistolFile" }
	$why = Invoke-VrCheck 'hnd', $cmPistolFile, '--polys', '91', '--length', '0.30', '--gun-only', '--complete'
	if ($why) { return $why }
	# a 1.1 install: no run, an old cabinet gun's file left alone
	$old = [byte[]](1..64)
	try {
		if (-not $Reuse) { [IO.File]::WriteAllBytes($cmCabinetFile, $old) }
		$why = Invoke-VrTest @{ Game = 'confmiss'; Name = "$name-next"; Seconds = 12; Config = 'config:vr.HandsPrep=yes'
			Expect = @('VR hands: the guns, made before: .*gun-cm-pistol\.bin')
			Reject = @('VR hands: no model yet', 'VR hands: watching for', 'VR hands: .*, but ', 'gun-cm-cabinet') }
		if ($why) { return "a 1.1 install: $why" }
		if (-not $Reuse) {
			$same = (Test-Path -LiteralPath $cmCabinetFile) -and
				[Convert]::ToBase64String([IO.File]::ReadAllBytes($cmCabinetFile)) -ceq [Convert]::ToBase64String($old)
			Write-Host "  a 1.1 install: the old $(Split-Path -Leaf $cmCabinetFile) $(if ($same) { 'as it was' } else { 'changed or gone' })"
			if (-not $same) { return "a 1.1 install: $cmCabinetFile changed or gone" }
		}
	}
	finally { Remove-Item -LiteralPath $cmCabinetFile -Force -ErrorAction SilentlyContinue }
	return $null
}

# The in-game pistol (cm-rip-start's file) in the right hand through the simulator, widened (vr.Gun 1): the hand in
# front of the left eye from 20 s, the first A (30 s) gives it the gun, a window shot at 35 s; against the same shot
# with the built-in gun ($cmWidenArcade) in the right hand's part of the view.
function Get-CmRipHandParams([string]$name, [string]$config) {
	@{ Game = 'confmiss'; Name = $name; Seconds = 37; XrSim = $true; Config = $config
		At = @('20=pose right 0.1 1.45 -0.3 0 0') + @(Get-CmStartsXr | Where-Object { [double]($_ -split '=')[0] -lt 36 }) + '35=shot held'
		Reject = @('XR: initialisation failed') }
}
function Test-CmRipHand {
	if (-not (Test-Path -LiteralPath $cmPistolFile)) { return "no $cmPistolFile (cm-rip-start makes it)" }
	$name = 'accept-cm-rip-hand'
	$params = Get-CmRipHandParams $name "$cmWiden,config:vr.Gun=1"
	$params.Expect = @('XR: the game''s own model from .*gun-cm-pistol\.bin', 'XR: gun: in-game pistol \(chosen\)$')
	$params.Reject += 'XR: no .*gun-cm-pistol\.bin'
	$why = Invoke-VrTest $params
	if ($why) { return $why }
	$arcade = 'accept-cm-rip-hand-arcade'
	$params = Get-CmRipHandParams $arcade $cmWidenArcade
	$params.Reject += 'XR: the game''s own model from'
	$why = Invoke-VrTest $params
	if ($why) { return "arcade gun: $why" }
	foreach ($run in $name, $arcade) {
		$why = Test-RunShots $run 't035-held.png'
		if ($why) { return $why }
	}
	$why = Invoke-VrCheck 'differ', (Join-Path $repo "build-win\vrtest\$name\t035-held.png"),
		(Join-Path $repo "build-win\vrtest\$arcade\t035-held.png"), '--box', '0.5,0.5,0.8,0.95', '--min-diff', '2'
	if ($why) { return "the in-game pistol looks like the built-in gun: $why" }
	return $null
}

# B skips the run and the menu button does nothing during it, both keyed on the run's first line
# (the gun comes up ~6 s in, at 30 s of game). Skipped holds for the session (S23): Start from
# 22 s after the skip (30 s of game or more) and on through the reload tutorial, where watching
# would make the pistol (1.1: the cabinet gun): no file, and the hand holds the Arcade gun (the harness's bundled
# namco.glb, -Bundled --namco-zapper: never the user's models). The next start runs it again. With no rifle file the
# skip comes in the rifle's part of the run (the attract demo), and the rifle, which comes in play too, is still
# watched for (Task 12, S23).
function Test-CmRipSkip {
	if (-not $Reuse) { Remove-CmGuns }
	return Invoke-OwnFolders -Bundled '--namco-zapper' {
		$first = 'VR hands: no model yet, taking the guns from the game'
		$keys = @("$first|1|xr left/menu 1 150", "$first|2|xr right/b 1 200")
		$starts = @(22, 25, 28 | ForEach-Object { "VR hands: skipped after|$_|xr right/a 1 200" })
		$params = @{ Game = 'confmiss'; Name = 'accept-cm-rip-skip'; Seconds = 48; XrSim = $true; Config = "$cmWiden,config:vr.HandsPrep=yes,config:vr.Gun=1"
			On = @($keys + $starts)
			Expect = $first, 'VR hands: skipped after', 'XR: menu: not while the guns are being made',
				'VR hands: watching for the assault rifle in play', $cmSkippedGun
			Reject = @('XR: initialisation failed', 'XR: menu open', 'XR: the game''s own model from',
				'the cabinet gun from this game', 'VR hands: the agents'' pistol from this game') }
		$why = Invoke-VrTest $params
		if ($why) { return $why }
		foreach ($file in $cmPistolFile, $cmCabinetFile) { if (Test-Path -LiteralPath $file) { return "$file made though the run was skipped" } }
		$params.Name = 'accept-cm-rip-skip2'
		$params.Seconds = 10
		$params.On = $keys
		$params.Expect = @($first, $cmSkippedGun)
		$params.Reject += 'VR hands: the assault rifle from this game'
		$why = Invoke-VrTest $params
		if ($why) { return "the next start: $why" }
		return $null
	}
}
$cmSkippedGun = 'XR: gun: Arcade gun \(chosen: the in-game pistol is not made yet\)$'

# The skip with the rifle's file there (cm-rip-rifle's seed): no gun from the attract demo is left to
# watch for in play, so the run goes Off, with no watching line (Task 12, S23).
function Test-CmRipSkipRifle {
	if (-not $Reuse) {
		Remove-CmGuns
		$why = Copy-CmRifleSeed
		if ($why) { return $why }
	}
	return Invoke-OwnFolders -Bundled '--namco-zapper' {
		$first = 'VR hands: no model yet, taking the guns from the game'
		$why = Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-rip-skip-rifle'; Seconds = 12; XrSim = $true
			Config = "$cmWiden,config:vr.HandsPrep=yes,config:vr.Gun=1"; On = @("$first|2|xr right/b 1 200")
			Expect = $first, 'VR hands: skipped after', $cmSkippedGun
			Reject = @('XR: initialisation failed', 'VR hands: watching for', 'the cabinet gun from this game',
				'VR hands: the agents'' pistol from this game') }
		if ($why) { return $why }
		foreach ($file in $cmPistolFile, $cmCabinetFile) { if (Test-Path -LiteralPath $file) { return "$file made though the run was skipped" } }
		return $null
	}
}

# Task 11: the same run goes on to the agents' pistol (body tex 7e2800, its suppressor 6cc400, 82f800,
# 76a800; 91 polygons when whole, 78/10/2/1): Start every 2 s for 14 s from $cmPistolAfter s after Start
# (CM VR 1.2; 1.1 waited for the cabinet gun), the stage 1 intro, every pass with 10 or more of its polygons
# captured and merged (hands_build.h buildGun) into gun-cm-pistol.bin, flagged complete when all of it came
# together; no cabinet gun; the run done by $cmRipDone s of game. -one merges the first capture only
# (vr.HandsCaptureLimit=1). Every pass of the intro draws all 91 (Task 11's runs), so one capture
# is whole as well and it comes out complete; it shows the limit holds (1 capture merged) and that
# the log's verdict and the file's flag agree (PF12: the flag 3 only with "complete", else 1 and
# the first check that failed).
$cmPistolFile = Join-Path $cmGunData 'gun-cm-pistol.bin'
$cmPistolLine = 'VR hands: the agents'' pistol from this game \((complete|incomplete: [^)]*)\), \d+ bytes to .*gun-cm-pistol\.bin'
$cmPistolSaved = 'VR hands: the agents'' pistol from this game \(complete\), \d+ bytes to .*gun-cm-pistol\.bin'
function Invoke-CmRipPistol([string]$name, [string]$config, [string]$want) {
	if (-not $Reuse) {
		Remove-CmGuns
		$why = Copy-CmRifleSeed
		if ($why) { return $why }
	}
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 240; Config = $config
		Expect = 'VR hands: no model yet, taking the guns from the game', $want, 'VR hands: done after \d+ s of game'
		Reject = @('VR hands: .*, but ', 'the cabinet gun from this game') }
	if ($why) { return $why }
	$n = Get-LogNumber (Get-RunLog $name) 'VR hands: done after (\d+) s of game'
	Write-Host "  done after $n s of game (want $cmRipDone or less)"
	if ($n -gt $cmRipDone) { return "the gun run took $n s of game, over $cmRipDone" }
	if (-not (Test-Path -LiteralPath $cmPistolFile)) { return "no $cmPistolFile" }
	if (Test-Path -LiteralPath $cmCabinetFile) { return "$cmCabinetFile made" }
	return $null
}
function Test-CmRipPistol {
	$why = Invoke-CmRipPistol 'accept-cm-rip-pistol' 'config:vr.HandsPrep=yes' $cmPistolSaved
	if ($why) { return $why }
	return Invoke-VrCheck 'hnd', $cmPistolFile, '--polys', '91', '--length', '0.30', '--gun-only', '--complete'
}
function Test-CmRipPistolOne {
	$name = 'accept-cm-rip-pistol-one'
	$why = Invoke-CmRipPistol $name 'config:vr.HandsPrep=yes,config:vr.HandsCaptureLimit=1' `
		'VR hands: the agents'' pistol: 1 captures, 1 merged '
	if ($why) { return $why }
	# the log's verdict and the file's flag agree: --complete (and flags 3) only when the log says complete
	$line = Select-String -LiteralPath (Get-RunLog $name) -Pattern $cmPistolLine -CaseSensitive | Select-Object -First 1
	if (-not $line) { return "no line matching $cmPistolLine" }
	$complete = $line.Matches[0].Groups[1].Value -eq 'complete'
	$check = @('hnd', $cmPistolFile, '--polys', '10', '--length', '0.30', '--gun-only') + @(if ($complete) { '--complete' })
	return Invoke-VrCheck $check -Want @(if ($complete) { 'flags 3 ' } else { 'flags 1 \(gun only\)' })
}

# Task 12: the whole first-start run from no files. The assault rifle (tex 662000, 55 polygons, the
# findings' Rifle) comes first: the attract demo plays with no presses (fast-forwarded) until a pass
# draws all of it (the train-car scene's drop, ~316 s of game) or 340 s, and it becomes
# gun-cm-rifle.bin (gun only, flagged complete); then Start and the pistol as before (no cabinet gun, CM VR 1.2),
# the run done by $cmRipRifleDone s of game. The clock time of the run is printed. Then (PF20) the rifle's file alone missing starts
# no run: it's watched for in play. The rifle's file is kept as cm-rifle-seed.bin for the others.
$cmRifleLine = 'VR hands: the assault rifle from this game, \d+ bytes to .*gun-cm-rifle\.bin'
function Test-CmRipRifle {
	$name = 'accept-cm-rip-rifle'
	$first = 'VR hands: no model yet, taking the guns from the game'
	if (-not $Reuse) { Remove-CmGuns }
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 200; Config = 'config:vr.HandsPrep=yes'
		Expect = $first, $cmRifleLine, $cmPistolSaved, 'VR hands: done after \d+ s of game'
		Reject = @('VR hands: .*, but ', 'the cabinet gun from this game') }
	if ($why) { return $why }
	$log = Get-RunLog $name
	$hit = Select-String -LiteralPath $log -Pattern 'VR hands: done after (\d+) s of game' -CaseSensitive | Select-Object -First 1
	$n = [int]$hit.Matches[0].Groups[1].Value
	$clock = (Get-LogClock $log 'VR hands: done after') - (Get-LogClock $log $first)
	$rifleAt = (Get-LogClock $log $cmRifleLine) - (Get-LogClock $log $first)
	Write-Host ('  done after {0} s of game (want {3} or less), {1:0.0} s on the clock (the rifle at {2:0.0} s)' -f $n, $clock, $rifleAt, $cmRipRifleDone)
	if ($n -gt $cmRipRifleDone) { return "the gun run took $n s of game, over $cmRipRifleDone" }
	if (Test-Path -LiteralPath $cmCabinetFile) { return "$cmCabinetFile made" }
	$why = Invoke-VrCheck 'hnd', $cmRifleFile, '--polys', '55', '--length', '0.88', '--gun-only', '--complete'
	if ($why) { return $why }
	# the pistol as its own scenarios check it
	$why = Invoke-VrCheck 'hnd', $cmPistolFile, '--polys', '91', '--length', '0.30', '--gun-only', '--complete'
	if ($why) { return "pistol: $why" }
	Copy-Item -LiteralPath $cmRifleFile -Destination $cmRifleSeed -Force
	# the next start without the rifle's file alone: no run, the rifle watched for in play
	Remove-Item -LiteralPath $cmRifleFile -Force
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = "$name-next"; Seconds = 12; Config = 'config:vr.HandsPrep=yes'
		Expect = @('VR hands: watching for the assault rifle in play')
		Reject = @('VR hands: no model yet', 'VR hands: .*, but ') }
	Copy-Item -LiteralPath $cmRifleSeed -Destination $cmRifleFile -Force
	if ($why) { return "the next start: $why" }
	return $null
}

# The run's log has a line for each regex (case-sensitive), each one after the one before: $null, else why not.
function Test-LogOrder([string]$name, [string[]]$regexes) {
	$after = 0
	foreach ($re in $regexes) {
		$hit = Select-String -LiteralPath (Get-RunLog $name) -Pattern $re -CaseSensitive | Where-Object { $_.LineNumber -gt $after } | Select-Object -First 1
		if (-not $hit) { return "no line '$re' after line $after" }
		$after = $hit.LineNumber
	}
	Write-Host "  PASS in order: $($regexes -join ', ')"
	return $null
}

# A setting's line in the run's emu.cfg.after (the settings it saved), the value after '='; $null without one.
function Get-SavedSetting([string]$name, [string]$key, [string]$file = 'emu.cfg.after') {
	$path = Join-Path $repo "build-win\vrtest\$name\$file"
	if (-not (Test-Path -LiteralPath $path)) { return $null }
	$hit = Select-String -LiteralPath $path -Pattern "^$([regex]::Escape($key))\s*=\s*(.*?)\s*$" | Select-Object -First 1
	return $(if ($hit) { $hit.Matches[0].Groups[1].Value } else { $null })
}

# $body played with build-win\emu.cfg holding these settings in its [config] section, as a player's saved settings
# (vr-test.ps1 keeps that as the run's emu.cfg.before); build-win\emu.cfg is put back as it was after.
function Invoke-SavedSettings([hashtable]$settings, [scriptblock]$body) {
	$cfg = Join-Path $repo 'build-win\emu.cfg'
	$hold = "$cfg.accept-hold"
	if (Test-Path -LiteralPath $hold) { return "$hold is there from a run that died: put it back first" }
	$had = Test-Path -LiteralPath $cfg
	if ($had) { Copy-Item -LiteralPath $cfg $hold }
	try {
		$keys = @($settings.Keys)
		$set = @($keys | ForEach-Object { "$_ = $($settings[$_])" })
		$lines = New-Object System.Collections.Generic.List[string]
		$placed = $false
		foreach ($line in @(if ($had) { Get-Content -LiteralPath $hold })) {
			if (@($keys | Where-Object { $line -match "^$([regex]::Escape($_))\s*=" }).Count) { continue }
			$lines.Add($line)
			if (-not $placed -and $line -eq '[config]') { $lines.AddRange([string[]]$set); $placed = $true }
		}
		if (-not $placed) { $lines.InsertRange(0, [string[]](@('[config]') + $set + '')) }
		Set-Content -LiteralPath $cfg -Value $lines -Encoding ascii
		return & $body
	}
	finally {
		if ($had) { Move-Item -LiteralPath $hold $cfg -Force } else { Remove-Item -LiteralPath $cfg -Force -ErrorAction SilentlyContinue }
	}
}

# CM VR 1.2 (Task 1): the three guns. The Gun row, round and round: the in-game pistol (vr.Gun 1, the default), then
# the folder guns, the bundled ones first in file-name order (re\make-own-models.py --namco-zapper in a bundled folder
# of the harness's own: namco.glb "Arcade gun", zapper.glb "Zapper"; the user's bundled models held aside), then the
# player's (the box gun alone). "list": the pistol in hand from the start; the right hand in front of the left eye from
# 20 s, pointing down past the panel (its ray off the rows: a gun change moves the muzzle, which would move the
# selection); the menu at 58 s, the stick down twice to Gun, then + three times (59.5, 60, 60.5 s): the Arcade gun, the
# Zapper, the box gun, in that order, saved as vr.Gun 4, vr.GunFile boxgun. 1.1's saved choices 0 (Auto), 2 (the
# cabinet gun) and 3 (the built-in arcade gun), saved in emu.cfg as a 1.1 player has them (Invoke-SavedSettings), read
# as the pistol, the saved value not rewritten ("migrate-<n>"); vr.Gun 4 with vr.GunFile NAMCO (1.1's Namco gun, the
# case folded) is the Arcade gun ("upgrade", Review Focus 1). vr.BuiltInGun (PC): the built-in gun though the bundled
# Arcade gun is there, no gun file read ("builtin"). Without the pistol's file (renamed .hold, back in finally): the
# Arcade gun ("nopistol"), and with no gun file either, the built-in gun ("nofiles").
function Test-CmGun {
	if (-not (Test-Path -LiteralPath $cmPistolFile)) { return "no ${cmPistolFile}: run cm-rip-pistol first (the rip scenarios make the guns; the full list does)" }
	$why = Invoke-OwnFolders -Fixture '--guns', '--only-box' -Bundled '--namco-zapper' {
		$name = 'accept-cm-gun'
		$menu = @('XR: menu Gun -> Arcade gun$', 'XR: menu Gun -> Zapper$', 'XR: menu Gun -> Box gun$')
		$plus = @('59.5', '60', '60.5' | ForEach-Object { "$_=stick right 1 0 150" })
		$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 64; XrSim = $true; Config = $cmWiden
			At = @('20=pose right 0.1 1.45 -0.3 0 -35') + @(Get-CmStartsXr) + '58=xr left/menu 1 150' +
				@('58.5=stick right 0 -1 150', '59=stick right 0 -1 150') + $plus + '62=xr right/b 1 150'
			Expect = @('XR: gun: in-game pistol \(chosen\)$') + $menu + 'XR: gun: Box gun \(chosen\)$'
			Reject = @('XR: initialisation failed', 'XR: gun: .*\(auto', 'XR: menu (Recenter|Laser|Haptics)',
				'XR: menu Gun -> (Auto|Cabinet gun|Agents)', 'bundled models.guns.boxgun') }
		if ($why) { return $why }
		$why = Test-LogOrder $name $menu
		if ($why) { return $why }
		foreach ($key in 'vr.Gun', 'vr.GunFile') {
			$got = Get-SavedSetting $name $key
			$want = @{ 'vr.Gun' = '4'; 'vr.GunFile' = 'boxgun' }[$key]
			Write-Host "  emu.cfg.after: $key = $got (want $want)"
			if ($got -cne $want) { return "the menu saved $key = $got, want $want" }
		}
		foreach ($n in 0, 2, 3) {
			$run = "accept-cm-gun-migrate-$n"
			$why = Invoke-SavedSettings @{ 'vr.Gun' = "$n" } {
				Invoke-VrTest @{ Game = 'confmiss'; Name = $run; Seconds = 15; XrSim = $true; Config = $cmWiden
					Expect = @('XR: gun: in-game pistol \(chosen\)$')
					Reject = @('XR: initialisation failed', 'XR: gun: (?!in-game pistol \(chosen\)$)') }
			}
			if ($why) { return "saved vr.Gun ${n}: $why" }
			# read as 1, nothing rewritten: the run's emu.cfg had vr.Gun = n, and still has it after
			$before = Get-SavedSetting $run 'vr.Gun' 'emu.cfg.before'
			$after = Get-SavedSetting $run 'vr.Gun'
			Write-Host "  saved vr.Gun ${n}: the run's emu.cfg had vr.Gun = $before, after it $after (want $n, $n)"
			if ($before -cne "$n") { return "saved vr.Gun ${n}: the run's emu.cfg had vr.Gun = $before (not the saved $n)" }
			if ($after -cne "$n") { return "saved vr.Gun ${n}: the run rewrote vr.Gun to $after" }
		}
		$why = Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-gun-upgrade'; Seconds = 15; XrSim = $true; Config = "$cmWiden,config:vr.Gun=4,config:vr.GunFile=NAMCO"
			Expect = @('XR: gun: Arcade gun \(chosen\)$')
			Reject = @('XR: initialisation failed', 'XR: gun: (?!Arcade gun \(chosen\)$)') }
		if ($why) { return "1.1's Namco gun (vr.GunFile NAMCO): $why" }
		return $null
	}
	if ($why) { return $why }
	# the bundled Arcade gun offered (the harness's namco.glb), and still the built-in gun, nothing read
	$why = Invoke-OwnFolders -Bundled '--namco-zapper' {
		Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-gun-builtin'; Seconds = 15; XrSim = $true; Config = $cmWidenArcade
			Expect = 'XR: own gun bundled models/guns/namco\.glb offered: "Arcade gun"', 'XR: gun: built-in gun \(chosen: vr\.BuiltInGun\)$'
			Reject = @('XR: initialisation failed', 'XR: gun: (?!built-in gun)', 'XR: own gun "') }
	}
	if ($why) { return "vr.BuiltInGun: $why" }
	$held = "$cmPistolFile.hold"
	try {
		if (-not $Reuse) { Move-Item -LiteralPath $cmPistolFile -Destination $held -Force }
		$why = Invoke-OwnFolders -Bundled '--namco-zapper' {
			Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-gun-nopistol'; Seconds = 15; XrSim = $true; Config = "$cmWiden,config:vr.Gun=1"
				Expect = @('XR: gun: Arcade gun \(chosen: the in-game pistol is not made yet\)$')
				Reject = @('XR: initialisation failed', 'XR: gun: in-game pistol') }
		}
		if ($why) { return "without the pistol's file: $why" }
		# no bundled folder, no player's: no gun file at all
		$why = Invoke-OwnFolders -Bundled @() {
			Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-gun-nofiles'; Seconds = 15; XrSim = $true; Config = $cmWiden
				Expect = @('XR: gun: built-in gun \(chosen: no gun file loads\)$')
				Reject = @('XR: initialisation failed', 'XR: gun: in-game pistol', 'XR: own gun ') }
		}
		if ($why) { return "without the pistol's file or a gun file: $why" }
	}
	finally {
		if (Test-Path -LiteralPath $held) {
			# (a pistol the run made meanwhile goes: the one held is put back)
			Remove-Item -LiteralPath $cmPistolFile -Force -ErrorAction SilentlyContinue
			Move-Item -LiteralPath $held -Destination $cmPistolFile -Force
		}
	}
	return $null
}

# Task 14: the assault rifle (the findings' ## Rifle). Task 4b's setMode(p, 2) as six pokes at $t into player p's
# block (0x0C1557B8 + 276 p): the rifle's per-frame and trigger handlers (+0x80, +0x88), its auto-fire countdown
# (+0x78 = 8), 45 rounds in a 45-round magazine (+0x4C, +0x50), the mode (+0x48 = 2) last. -Pistol: setMode(p, 0)'s
# (the pistol's handlers, 6 rounds of 6, mode 0). With the rifle's the game fires on its own, 12 a second while the
# trigger is held, and gives the pistol back itself at 0 rounds (4b's t4b-p2). POKE is absolute.
function Get-CmRiflePoke([double]$t, [int]$player = 0, [switch]$Pistol) {
	$b = 0x0C1557B8 + 276 * $player
	$writes = if ($Pistol) { @(@(0x80, '0c02b5a4'), @(0x88, '0c02bbfa'), @(0x78, '0'), @(0x50, '6'), @(0x4C, '6'), @(0x48, '0')) }
		else { @(@(0x80, '0c02b5c8'), @(0x88, '0c02bc64'), @(0x78, '8'), @(0x50, '2d'), @(0x4C, '2d'), @(0x48, '2')) }
	foreach ($w in $writes) { "$(T $t)=poke $('{0:x8}' -f ($b + $w[0])) 4 $($w[1])" }
}

# The log-clock times (seconds, as Get-LogCount's -From / -To) of the lines matching a regex.
function Get-LogTimes([string]$run, [string]$regex, [double]$From = 0, [double]$To = 1e9) {
	@(Select-String -LiteralPath (Get-RunLog $run) -Pattern $regex -CaseSensitive | ForEach-Object {
		if ($_.Line -match '^(\d+):(\d\d):(\d{3}) ') { [int]$Matches[1] * 60 + [int]$Matches[2] + [int]$Matches[3] / 1000 }
	} | Where-Object { $_ -ge $From -and $_ -lt $To })
}

# One held burst with the rifle, from -From to -To of a run's log: the rounds the game fired (player p's shot counter
# +0x60 watched with vr.WatchRam, a "VR: ram" line per rise) against the knocks in $hand: the pull's Shot (0.030 s,
# 1.0) once, then Auto knocks (0.020 s, 0.6), at least $minAuto, each knock over before the next (the gaps between
# the simulator's lines over the longest knock's 30 ms, 35 ms with a margin). $null when so (knocks = rounds +-1), the last
# knock within 0.1 s of the last round (the knocks keep up: no backlog), and the burst's flashes (the line the hand logs
# when the trigger is let go, "XR: P<n> rifle burst: <k> flashes") one per round, +-1 (Ruling S33).
function Test-RifleBurst([string]$run, [string]$hand, [int]$player, [double]$From, [double]$To, [int]$minAuto) {
	$counter = '{0:x8}' -f (0x0C1557B8 + 276 * $player + 0x60)
	$roundTimes = @(Get-LogTimes $run "VR: ram $counter " $From $To)
	$rounds = $roundTimes.Count
	$shots = @(Get-LogTimes $run "XRSIM: haptic $hand 0\.030 1\.000" $From $To).Count
	$auto = @(Get-LogTimes $run "XRSIM: haptic $hand 0\.020 0\.600" $From $To).Count
	$all = @(Get-LogTimes $run "XRSIM: haptic $hand " $From $To)
	$gaps = @(for ($i = 1; $i -lt $all.Count; $i++) { $all[$i] - $all[$i - 1] })
	$min = if ($gaps.Count) { ($gaps | Measure-Object -Minimum).Minimum } else { 0 }
	$median = if ($gaps.Count) { @($gaps | Sort-Object)[[int][Math]::Floor($gaps.Count / 2)] } else { 0 }
	Write-Host ("  {0} {1}-{2} s: {3} rounds, {4} knocks in the {5} hand ({6} Shot, {7} Auto), gaps min {8:0.000} s, median {9:0.000} s" -f
		$run, (T $From), (T $To), $rounds, $all.Count, $hand, $shots, $auto, $min, $median)
	if ($shots -ne 1) { return "$shots Shot knocks, want 1 (the pull)" }
	if ($all.Count -ne $shots + $auto) { return "$($all.Count - $shots - $auto) other knocks" }
	if ([Math]::Abs($all.Count - $rounds) -gt 1) { return "$($all.Count) knocks for $rounds rounds" }
	if ($auto -lt $minAuto) { return "$auto Auto knocks, want $minAuto or more" }
	if ($all.Count -gt 1 -and $min -lt 0.035) { return "two knocks $(T $min) s apart: they run into each other" }
	$lag = if ($all.Count -and $rounds) { $all[-1] - $roundTimes[-1] } else { 0 }
	Write-Host ("  the last knock {0:0.000} s after the last round (at most 0.1)" -f $lag)
	if ([Math]::Abs($lag) -gt 0.1) { return "the last knock $(T $lag) s after the last round: the knocks fall behind" }
	$burst = Select-String -LiteralPath (Get-RunLog $run) -Pattern "XR: P$($player + 1) rifle burst: (\d+) flashes" -CaseSensitive |
		Where-Object { $_.Line -match '^(\d+):(\d\d):(\d{3}) ' -and ($t = [int]$Matches[1] * 60 + [int]$Matches[2] + [int]$Matches[3] / 1000) -ge $From -and $t -lt $To + 1 } |
		Select-Object -First 1
	if (-not $burst) { return "no 'XR: P$($player + 1) rifle burst: <n> flashes' line: one flash for the whole burst?" }
	$flashes = [int]([regex]::Match($burst.Line, 'burst: (\d+) flashes').Groups[1].Value)
	Write-Host "  $flashes flashes for $rounds rounds"
	if ([Math]::Abs($flashes - $rounds) -gt 1) { return "$flashes flashes for $rounds rounds: want one each" }
	return $null
}

# The rifle in player 1's hand, through the simulator: the rifle poked at 52.6 s (stage 1's first room), the right
# hand in view pointing ahead; the trigger held 2 s from 53.5 s (24 rounds: a Shot and Auto knocks, at least
# floor(1.5 x 12) = 18) and 2.5 s from 57.5 s (the 21 left: the game gives the pistol back at ~59.3 s, the hand the
# chosen gun), then a pull at 61 s (the pistol's Shot). Window shots of the hand at 52.4 s (the pistol, before the poke:
# the same room, so a shot that differs shows the gun changed, not the scene; Ruling S33) and 56 s (the rifle).
function Get-CmRifleSimParams([string]$name, [double]$seconds) {
	@{ Game = 'confmiss'; Name = $name; Seconds = $seconds; XrSim = $true; Config = "$cmWiden,config:vr.WatchRam=0c155818:2"
		At = @('20=pose right 0.1 1.45 -0.3 0 0') + @(Get-CmStartsXr) + @(Get-CmRiflePoke 52.6) +
			'53.5=xr right/trigger 1 2000', '56=shot rifle', '57.5=xr right/trigger 1 2500', '61=xr right/trigger 1 100', '52.4=shot pistol'
		Reject = @('XR: initialisation failed', 'XRSIM: haptic right 0\.012') }
}
# With the rifle's file: its model in the hand ("XR: P1 hand: assault rifle (gun-cm-rifle.bin)"), the knocks, the
# pistol's feel back after; the two window shots differ in the hand's box. Without it (moved to .hold, back after):
# the arcade gun recoloured, which differs from the model at 56 s.
function Test-CmRifleSim {
	if (-not (Test-Path -LiteralPath $cmRifleFile)) { return "no ${cmRifleFile}: run cm-rip-rifle first (the full list does)" }
	$name = 'accept-cm-rifle-sim'
	$params = Get-CmRifleSimParams $name 64
	$params.Expect = @('VR: P1 weapon pistol -> rifle', 'XR: P1 hand: assault rifle \(gun-cm-rifle\.bin\)',
		'VR: P1 weapon rifle -> pistol', 'XR: P1 hand: back to the ')
	$why = Invoke-VrTest $params
	if ($why) { return $why }
	# (none before: the RAM's noise at boot, before the game clears it, is no rifle)
	$weapon = Get-LogCount $name 'VR: P[12] weapon '
	Write-Host "  $weapon weapon lines (want 2: player 1's pistol -> rifle and back)"
	if ($weapon -ne 2) { return "$weapon weapon lines, want 2" }
	# the rifle's model loaded at the start with the other guns, not mid-play at the pickup (52.6 s; Ruling S33)
	$loadedAt = Get-LogClock (Get-RunLog $name) "XR: the game's own model from .*gun-cm-rifle\.bin"
	Write-Host "  the rifle's model loaded at $(T $loadedAt) s (want before 50 s: the pickup is at 52.6 s, about 52.4 s on the log's clock)"
	if ($loadedAt -ge 50) { return "the rifle's model loaded at $(T $loadedAt) s, at the pickup: want it at the start" }
	foreach ($burst in @(53.2, 56.3), @(57.2, 60.8)) {
		$why = Test-RifleBurst $name 'right' 0 $burst[0] $burst[1] 18
		if ($why) { return "the burst from $(T $burst[0]) s: $why" }
	}
	$shot = @(Get-LogTimes $name 'XRSIM: haptic right 0\.030 1\.000' 60.8).Count
	$auto = @(Get-LogTimes $name 'XRSIM: haptic right 0\.020 0\.600' 60.8).Count
	Write-Host "  after the rifle: $shot Shot knocks (want 1), $auto Auto (want 0)"
	if ($shot -ne 1 -or $auto -ne 0) { return "after the rifle: $shot Shot and $auto Auto knocks, want 1 and 0" }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$why = Test-RunShots $name 't056-rifle.png', 't052.4-pistol.png'
	if ($why) { return $why }
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't056-rifle.png'), (Join-Path $dir 't052.4-pistol.png'), '--box', '0.5,0.5,0.8,0.95', '--min-diff', '2'
	if ($why) { return "the rifle and the pistol look alike: $why" }
	$arcade = 'accept-cm-rifle-sim-arcade'
	$held = "$cmRifleFile.hold"
	try {
		if (-not $Reuse) { Move-Item -LiteralPath $cmRifleFile -Destination $held -Force }
		$params = Get-CmRifleSimParams $arcade 57
		$params.Expect = @('XR: P1 hand: assault rifle \(arcade gun, recoloured\)')
		$params.Reject += 'XR: P1 hand: assault rifle \(gun-cm-rifle'
		$why = Invoke-VrTest $params
	}
	finally {
		if (Test-Path -LiteralPath $held) {
			# (a rifle the run made meanwhile goes: the one held is put back)
			Remove-Item -LiteralPath $cmRifleFile -Force -ErrorAction SilentlyContinue
			Move-Item -LiteralPath $held -Destination $cmRifleFile -Force
		}
	}
	if ($why) { return "without the rifle's file: $why" }
	$why = Test-RunShots $arcade 't056-rifle.png'
	if ($why) { return $why }
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't056-rifle.png'), (Join-Path $repo "build-win\vrtest\$arcade\t056-rifle.png"), '--box', '0.5,0.5,0.8,0.95', '--min-diff', '2'
	if ($why) { return "the rifle's model and the recoloured arcade gun look alike: $why" }
	return $null
}

# Player 2's rifle: X at 60 s (a gun in each hand), the rifle poked into player 2's block at 61 s, the left trigger
# held 2 s from 62 s (a Shot and Auto knocks in the left hand only, one per round), the pistol poked back at 65 s.
function Test-CmRifleP2 {
	$name = 'accept-cm-rifle-p2'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 67; XrSim = $true; Config = "$cmWiden,config:vr.WatchRam=0c15592c:2"
		At = @(Get-CmStartsXr) + '60=xr left/x 1 200' + @(Get-CmRiflePoke 61 1) + '62=xr left/trigger 1 2000' + @(Get-CmRiflePoke 65 1 -Pistol)
		Expect = 'XR: player 2 joins: a gun in each hand', 'VR: P2 weapon pistol -> rifle',
			'XR: P2 hand: assault rifle \((gun-cm-rifle\.bin|arcade gun, recoloured)\)', 'VR: P2 weapon rifle -> pistol', 'XR: P2 hand: back to the '
		Reject = @('XR: initialisation failed', 'XRSIM: haptic (left|right) 0\.012', 'XRSIM: haptic right 0\.020', 'XR: P1 hand: assault rifle') }
	if ($why) { return $why }
	$weapon = Get-LogCount $name 'VR: P[12] weapon '
	Write-Host "  $weapon weapon lines (want 2: player 2's pistol -> rifle and back)"
	if ($weapon -ne 2) { return "$weapon weapon lines, want 2" }
	return Test-RifleBurst $name 'left' 1 61.7 64.6 18
}

# The merge of a gun's captures (hands_build.cpp's mergeCaptures) on synthetic captures, built and run
# on its own (re\hands_build_test.cmd), then vrcheck.py's selftest; -Reuse changes nothing here.
function Test-CmMergeUnit {
	$out = @(& cmd /c (Join-Path $PSScriptRoot 're\hands_build_test.cmd') (Join-Path $repo 'build-win\vrtest\cm-merge-unit'))
	$code = $LASTEXITCODE
	$out | ForEach-Object { Write-Host "  $_" }
	if ($code -ne 0) {
		$why = ($out | Where-Object { $_ -match '^FAIL|error' } | Select-Object -First 3) -join '; '
		return "hands_build_test exit ${code}: $why"
	}
	return Invoke-VrCheck @('selftest') -Want @('^selftest OK$')
}

# Task 14b: the folders of the player's own models, guns\ and controllers\, next to build-win\flycast.exe and next to
# the zip ($env:VRLGH_GAMES), and the bundled models' folder build-win\data\models (S37: the PC zip's data\models, looked
# at last): moved aside (<folder>.accept-hold) while $body runs, back after. -Fixture: these arguments to
# re\make-own-models.py, which writes its fixtures into build-win meanwhile (gone after); its "muzzle x y z" line in
# $ownMuzzle, its "textures <stem> <bytes>" lines in $ownTextures. -Bundled: the same into build-win\data\models (a
# bundled set of the harness's own, gone after). $body's output is the scenario's ($null: passed).
$ownMuzzle = $null
$ownTextures = [ordered]@{}
$ownBundled = Join-Path $repo 'build-win\data\models'
function Invoke-OwnFolders([scriptblock]$body, [string[]]$Fixture = @(), [string[]]$Bundled = @()) {
	$held = @()
	$made = @()
	try {
		$dirs = @(foreach ($root in @((Join-Path $repo 'build-win'), $env:VRLGH_GAMES) | Where-Object { $_ }) {
			foreach ($sub in 'guns', 'controllers') { Join-Path $root $sub } }) + $ownBundled
		foreach ($dir in $dirs) {
			if (Test-Path -LiteralPath "$dir.accept-hold") { throw "$dir.accept-hold is there from a run that died: put it back as $dir first" }
			if (Test-Path -LiteralPath $dir) { Move-Item -LiteralPath $dir -Destination "$dir.accept-hold"; $held += $dir }
		}
		$maker = Join-Path $PSScriptRoot 're\make-own-models.py'
		if ($Bundled) {
			$made += $ownBundled
			$out = @(& python -I $maker $ownBundled @Bundled)
			if ($LASTEXITCODE -ne 0) { return "make-own-models.py (bundled) $($Bundled -join ' '): exit $LASTEXITCODE $out" }
		}
		if ($Fixture) {
			$made += @((Join-Path $repo 'build-win\guns'), (Join-Path $repo 'build-win\controllers'))
			$out = @(& python -I $maker (Join-Path $repo 'build-win') @Fixture)
			if ($LASTEXITCODE -ne 0) { return "make-own-models.py $($Fixture -join ' '): exit $LASTEXITCODE $out" }
			$script:ownMuzzle = @($out | Where-Object { $_ -match '^muzzle ' } | ForEach-Object { @(($_ -split ' ')[1..3] | ForEach-Object { [double]::Parse($_, $inv) }) })
			$script:ownTextures = [ordered]@{}
			foreach ($line in $out) { if ($line -match '^textures (\S+) (\d+)$') { $script:ownTextures[$Matches[1]] = [long]$Matches[2] } }
			$script:ownLabels = [ordered]@{}
			foreach ($line in $out) {
				if ($line -match '^label (\w+) (\w+) (\S+) (\S+) (\S+)$') {
					$script:ownLabels["$($Matches[1]) $($Matches[2])"] = @(3..5 | ForEach-Object { [double]::Parse($Matches[$_], $inv) })
				}
			}
		}
		return & $body
	}
	finally {
		# a fixture folder that can't be removed (a file kept open): a held folder stays in .accept-hold (moved back it
		# would land inside the leftover as guns\guns.accept-hold) and the scenario fails saying so
		$stuck = @()
		foreach ($dir in $made) {
			Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
			if (Test-Path -LiteralPath $dir) { $stuck += $dir }
		}
		foreach ($dir in $held) {
			if (Test-Path -LiteralPath $dir) {
				Write-Host "  !! $dir could not be removed: the player's folder is left in $dir.accept-hold; put it back by hand" -ForegroundColor Red
				$stuck += $dir
			}
			else { Move-Item -LiteralPath "$dir.accept-hold" -Destination $dir }
		}
		if ($stuck) { throw "the fixture folder $(@($stuck | Select-Object -Unique) -join ', ') could not be removed (any held folder left in .accept-hold)" }
	}
}

# The x y z after $prefix on the first line of a run's log that has it (metres, as the log prints them); $null without one.
function Get-LogVec([string]$run, [string]$prefix) {
	$hit = Select-String -LiteralPath (Get-RunLog $run) -Pattern "$prefix(-?[\d.]+) (-?[\d.]+) (-?[\d.]+)" -CaseSensitive | Select-Object -First 1
	if (-not $hit) { return $null }
	return @(1..3 | ForEach-Object { [double]::Parse($hit.Matches[0].Groups[$_].Value, $inv) })
}

# The player's own gun (Task 14b) through the simulator, widened, as cm-gun: re\make-own-models.py's box gun in guns\
# (boxgun.glb + .json: 3 parts, red, green and blue, the barrel textured; in made-up units, the barrel along +x) and
# zz-big.glb, over the cap of 200,000 triangles (skipped, why logged), with the bundled guns' stand-ins in a bundled folder
# of the harness's own (--namco-zapper: "Arcade gun", "Zapper"). The menu at 58 s, down twice to Gun, + three times from the
# in-game pistol (the hand's window shot at 59.6 s): the Arcade gun (61 s), the Zapper, then the player's guns in file-name
# order, the first one offered the box gun, as vr.Gun 4 (65 s); zz-big isn't offered. The muzzle the aim line and the flash start from (the hand's "XR: P1 muzzle: <gun> at
# x y z from the aim pose") is within 1 cm of the hand ("XR: right hand at") + the sidecar's muzzle as the fixture
# script turns and scales it (vr.HandScale 1). The box gun's shot differs from the Arcade gun's (all yellow), and has its 3
# colours (vrcheck hues, against the in-game pistol's shot at 59.6 s). Then without its sidecar (accept-cm-own-gun-nosidecar,
# vr.Gun=4): the box gun skipped with why, the hand's gun the pistol (or the Arcade gun) with "(chosen: boxgun is not there".
# Then a player's guns\zapper.glb over the bundled one (accept-cm-own-gun-override, Review Focus 2).
# Fix round 1: only the chosen gun is read and put on the GPU (mipmapped), the others only looked at ("offered");
# tex-a, tex-b, tex-c (128 MB of textures each, 384 MB in all): each offered, there is no total across the guns (S40:
# a gun's textures are capped on their own), and each gun's offered or skipped line logged once; the body's 8 green
# slabs merged (10 primitives in 3 parts); the sidecars with 1e39, a huge up and a far muzzle skipped with why.
$ownOldBudget = 256MB	# the folder total there was before S40
function Test-CmOwnGun {
	$why = Invoke-OwnFolders -Fixture '--guns' -Bundled '--namco-zapper' {
		$name = 'accept-cm-own-gun'
		$fixtureSum = [long]0
		$offeredLines = @()
		foreach ($stem in @($ownTextures.Keys | Sort-Object)) {
			$bytes = $ownTextures[$stem]
			$fixtureSum += $bytes
			$mb = [regex]::Escape(($bytes / 1MB).ToString('0.0', $inv))
			$offeredLines += "XR: own gun guns.$([regex]::Escape($stem))\.glb offered: .*, $mb MB\)$"
		}
		if ($fixtureSum -le $ownOldBudget) { return "the fixture's textures ($fixtureSum bytes) aren't over the old folder total ($ownOldBudget)" }
		Write-Host "  the fixture's textures: $fixtureSum bytes, over the old folder total of $($ownOldBudget): all offered"
		$menu = @('XR: menu Gun -> Arcade gun$', 'XR: menu Gun -> Zapper$', 'XR: menu Gun -> Box gun$')
		$plus = @(60, 62, 64 | ForEach-Object { "$_=stick right 1 0 150" })
		$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 68; XrSim = $true; Config = "$cmWiden,config:vr.HandScale=1"
			At = @('20=pose right 0.1 1.45 -0.3 0 -35') + @(Get-CmStartsXr) + '58=xr left/menu 1 150' +
				@('58.5=stick right 0 -1 150', '59=stick right 0 -1 150') + $plus + '59.6=shot pistol' + '61=shot arcade' + '65=shot own' + '66=xr right/b 1 150'
			Expect = @('XR: own gun "Box gun" from guns.boxgun\.glb \(120 triangles, 10 primitives in 3 parts #d91414 #1abf1f #2640ff, 1 texture',
				'XR: own gun "Box gun" on the GPU \(1 image, [\d.]+ MB, mipmapped\)$',
				'XR: own gun guns.zz-big\.glb skipped: over 200000 triangles \(204800\)',
				'XR: own gun guns.bad-inf\.glb skipped: "grip" is not 3 numbers from -1000000 to 1000000$',
				'XR: own gun guns.bad-up\.glb skipped: "up" is not 3 numbers from -1000000 to 1000000$',
				'XR: own gun guns.bad-far\.glb skipped: "muzzle" is [\d.]+ from the model, over twice its size \(2\)',
				'XR: gun: Box gun \(chosen\)$', 'XR: P1 muzzle: Box gun at ') + $menu + $offeredLines
			Reject = @('XR: initialisation failed', 'XR: menu Gun -> (Big grid|Auto|Cabinet gun)', 'XR: gun: Box gun \(auto', 'XR: menu (Recenter|Laser|Haptics)',
				'XR: own gun "(?!(Box gun|Arcade gun|Zapper)")[^"]*" (from|on the GPU)', 'would take the folder''s guns over') }
		if ($why) { return $why }
		$why = Test-LogOrder $name $menu
		if ($why) { return $why }
		# each gun's state logged once (a scan logs a gun again only when it changed; the menu's opening scans again)
		foreach ($stem in @($ownTextures.Keys) + 'zz-big', 'bad-inf', 'bad-up', 'bad-far') {
			$n = Get-LogCount $name "XR: own gun guns.$([regex]::Escape($stem))\.glb (offered|skipped)"
			if ($n -ne 1) { return "$n lines 'XR: own gun guns/$stem.glb offered/skipped', want 1" }
		}
		Write-Host "  each gun's offered or skipped line once"
		$palm = Get-LogVec $name 'XR: right hand at '
		$muzzle = Get-LogVec $name 'XR: P1 muzzle: Box gun at '
		if (-not $palm -or -not $muzzle -or -not $ownMuzzle) { return 'no hand, muzzle or fixture muzzle line to compare' }
		$off = [Math]::Sqrt((0..2 | ForEach-Object { [Math]::Pow($muzzle[$_] - ($palm[$_] + $ownMuzzle[$_]), 2) } | Measure-Object -Sum).Sum)
		Write-Host ("  the aim line starts {0:0.0000} m from the sidecar's muzzle (hand {1}, sidecar muzzle {2}; at most 0.01)" -f $off, ($palm -join ' '), ($ownMuzzle -join ' '))
		if ($off -gt 0.01) { return "the aim line starts $(T $off) m from the sidecar's muzzle" }
		$why = Test-RunShots $name 't059.6-pistol.png', 't061-arcade.png', 't065-own.png'
		if ($why) { return $why }
		$dir = Join-Path $repo "build-win\vrtest\$name"
		$why = Invoke-VrCheck 'differ', (Join-Path $dir 't065-own.png'), (Join-Path $dir 't061-arcade.png'), '--box', '0.5,0.5,0.8,0.95', '--min-diff', '2'
		if ($why) { return "the box gun looks like the Arcade gun: $why" }
		$why = Invoke-VrCheck 'hues', (Join-Path $dir 't065-own.png'), '--against', (Join-Path $dir 't059.6-pistol.png'), '--box', '0.4,0.4,0.9,1',
			'--rgb', '217,20,20', '--rgb', '26,191,31', '--rgb', '38,64,255'
		if ($why) { return "the box gun's 3 colours: $why" }
		return $null
	}
	if ($why) { return $why }
	$why = Invoke-OwnFolders -Fixture '--guns', '--no-sidecar' -Bundled '--namco-zapper' {
		Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-own-gun-nosidecar'; Seconds = 15; XrSim = $true; Config = "$cmWiden,config:vr.Gun=4,config:vr.GunFile=boxgun"
			Expect = @('XR: own gun guns.boxgun\.glb skipped: no sidecar guns.boxgun\.json',
				'XR: gun: (in-game pistol|Arcade gun) \(chosen: boxgun is not there')
			Reject = @('XR: initialisation failed', 'XR: gun: Box gun', 'XR: gun: built-in gun') }
	}
	if ($why) { return "without its sidecar: $why" }
	# Review Focus 2: a player's guns\zapper.glb (the bundled stand-in's files, its sidecar's name "My Zapper") goes instead
	# of the bundled one and keeps the Zapper's place, right after the Arcade gun, not among the player's own by name (where
	# it would come after boxgun.glb): + three times from the pistol reads Arcade gun, My Zapper, Box gun.
	return Invoke-OwnFolders -Fixture '--guns', '--only-box' -Bundled '--namco-zapper' {
		$guns = Join-Path $repo 'build-win\guns'
		Copy-Item -LiteralPath (Join-Path $ownBundled 'guns\zapper.glb') -Destination $guns
		$side = Get-Content -Raw -LiteralPath (Join-Path $ownBundled 'guns\zapper.json') | ConvertFrom-Json
		$side.name = 'My Zapper'
		[IO.File]::WriteAllText((Join-Path $guns 'zapper.json'), ($side | ConvertTo-Json -Compress))
		$name = 'accept-cm-own-gun-override'
		$menu = @('XR: menu Gun -> Arcade gun$', 'XR: menu Gun -> My Zapper$', 'XR: menu Gun -> Box gun$')
		$plus = @('59.5', '60', '60.5' | ForEach-Object { "$_=stick right 1 0 150" })
		$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 64; XrSim = $true; Config = $cmWiden
			At = @('20=pose right 0.1 1.45 -0.3 0 -35') + @(Get-CmStartsXr) + '58=xr left/menu 1 150' +
				@('58.5=stick right 0 -1 150', '59=stick right 0 -1 150') + $plus + '62=xr right/b 1 150'
			Expect = @('XR: own gun guns.zapper\.glb offered: "My Zapper"') + $menu + 'XR: gun: Box gun \(chosen\)$'
			Reject = @('XR: initialisation failed', 'bundled models.guns.zapper', 'XR: menu Gun -> Zapper$') }
		if ($why) { return $why }
		return Test-LogOrder $name $menu
	}
}

# The choice of a folder gun is its file, not its place in file-name order (Task 14b fix round 1, finding 4): vr.Gun and
# vr.GunFile as accept-cm-own-gun's menu saved them (its emu.cfg.after), played again (a restart) with aa-early.glb
# added, which sorts before boxgun.glb: the hand still holds the box gun.
function Test-CmOwnGunFile {
	$saved = Join-Path $repo 'build-win\vrtest\accept-cm-own-gun\emu.cfg.after'
	if (-not (Test-Path -LiteralPath $saved) -and -not $Reuse) {
		Write-Host '  (no accept-cm-own-gun emu.cfg.after: playing cm-own-gun)'
		$why = Test-CmOwnGun
		if ($why) { return "cm-own-gun: $why" }
	}
	if (-not (Test-Path -LiteralPath $saved)) { return "no $saved" }
	$config = $cmWiden
	foreach ($key in 'vr.Gun', 'vr.GunFile') {
		$hit = Select-String -LiteralPath $saved -Pattern "^$([regex]::Escape($key))\s*=\s*(.*?)\s*$" | Select-Object -First 1
		if ($hit) { $config += ",config:$key=$($hit.Matches[0].Groups[1].Value)" }
	}
	Write-Host "  as saved: $($config.Substring($cmWiden.Length))"
	return Invoke-OwnFolders -Fixture '--guns', '--early' {
		Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-own-gun-file'; Seconds = 15; XrSim = $true; Config = $config
			Expect = @('XR: gun: Box gun \(chosen\)$')
			Reject = @('XR: initialisation failed', 'XR: gun: Early gun', 'XR: own gun "Early gun" from') }
	}
}

# The harness's restore (Task 14b fix round 1, finding 7), no game run: a stand-in for the player's build-win\guns\ is
# held aside while a fixture is written there, one of the fixture's files kept open so its removal fails: the
# scenario must fail saying so, with the stand-in left in guns.accept-hold, not moved into the leftover fixture
# folder. A real build-win\guns\ (the player's) is moved to guns.own-folders-hold meanwhile and back after.
function Test-OwnFolders {
	$guns = Join-Path $repo 'build-win\guns'
	$aside = "$guns.own-folders-hold"
	foreach ($p in "$guns.accept-hold", $aside) { if (Test-Path -LiteralPath $p) { return "$p is there from a run that died: put it back first" } }
	$moved = Test-Path -LiteralPath $guns
	if ($moved) { Move-Item -LiteralPath $guns -Destination $aside }
	try {
		New-Item -ItemType Directory -Path $guns | Out-Null
		Set-Content -LiteralPath (Join-Path $guns 'player.txt') -Value 'a stand-in for the own models of a player'
		$script:ownLock = $null
		try {
			$why = Invoke-OwnFolders -Fixture '--guns' { $script:ownLock = [IO.File]::Open((Join-Path $guns 'boxgun.glb'), 'Open', 'Read', 'None'); $null }
		}
		catch { $why = "threw: $_" }
		finally { if ($script:ownLock) { $script:ownLock.Close() } }
		$nested = Test-Path -LiteralPath (Join-Path $guns 'guns.accept-hold')
		$kept = Test-Path -LiteralPath "$guns.accept-hold\player.txt"
		Write-Host "  Invoke-OwnFolders said: $why"
		Write-Host "  the stand-in kept in guns.accept-hold: $kept; moved into the leftover folder: $nested"
		# (all of it the test's own: the fixture's leftovers and the stand-in)
		Remove-Item -LiteralPath $guns, "$guns.accept-hold" -Recurse -Force -ErrorAction SilentlyContinue
		if ($nested) { return 'the held folder was moved into the fixture folder that could not be removed (guns\guns.accept-hold)' }
		if (-not $kept) { return 'the held folder is not in guns.accept-hold after the failed removal' }
		if ("$why" -notmatch 'could not be removed') { return "the scenario did not fail on the failed removal: $why" }
		return $null
	}
	finally {
		if ($moved) {
			Remove-Item -LiteralPath $guns -Recurse -Force -ErrorAction SilentlyContinue	# (the test's own leftovers, if any)
			if (Test-Path -LiteralPath $guns) { Write-Host "  !! $guns could not be removed: the player's guns are in $aside; put them back by hand" -ForegroundColor Red }
			else { Move-Item -LiteralPath $aside -Destination $guns }
		}
	}
}

# The player's own controllers (Task 14b) through the simulator, as cm-controllers: re\make-own-models.py's box controller
# (controllers\left.glb, right.glb + .json: in cm, two parts, its aim node 2 cm up and 6 cm forward, tilted 30 degrees) in
# both hands; the left hand in front of the eye at 24 s (before play starts) differs from the built-in controller there
# (cm-controllers' shot). Without left.glb (accept-cm-own-controllers-right): the left hand's file isn't read and its
# shot differs from the box's (the built-in shape is back), the right hand's still is.
# Task 15b: the how-to-play labels' lines end on the sidecars' "buttons" points (the box's left x, y, stick; right a, b,
# trigger, stick: each "XR: labels: <hand> <button> at (x, y, z) in the aim pose (from the file)", logged once, within 2 mm
# of the fixture's point placed as the model is); the left "menu" point, 5 m off (--bad-button), is refused with one log line
# and keeps the built-in place. In the -right run the sidecar has no "buttons" (--no-buttons): the built-in places, saying so.
function Test-CmOwnLabels([string]$name) {
	$why = @()
	foreach ($key in 'left x', 'left y', 'left stick', 'right a', 'right b', 'right trigger', 'right stick') {
		$hand, $button = $key -split ' '
		$want = $script:ownLabels[$key]
		$line = Select-String -LiteralPath (Get-RunLog $name) -Pattern "XR: labels: $hand $button at \((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\) in the aim pose \(from the file\)" -CaseSensitive | Select-Object -First 1
		if (-not $want) { $why += "no fixture point for $key"; continue }
		if (-not $line) { $why += "no 'XR: labels: $hand $button at (...) in the aim pose (from the file)'"; continue }
		$v = @(1..3 | ForEach-Object { [double]::Parse($line.Matches[0].Groups[$_].Value, $inv) })
		$d = [Math]::Sqrt([Math]::Pow($v[0] - $want[0], 2) + [Math]::Pow($v[1] - $want[1], 2) + [Math]::Pow($v[2] - $want[2], 2))
		$msg = "$key line ends at $($v -join ' '), the fixture's point $($want -join ' '): $([Math]::Round($d * 1000, 2)) mm"
		if ($d -le 0.002) { Write-Host "  PASS $msg" } else { $why += "FAIL $msg (over 2 mm)" }
		$n = Get-LogCount $name "XR: labels: $hand $button at "
		if ($n -ne 1) { $why += "$n lines 'XR: labels: $hand $button at' (want 1: logged once)" }
	}
	$why | ForEach-Object { Write-Host "  $_" }
	return $(if ($why) { $why -join '; ' } else { $null })
}
function Test-CmOwnControllers {
	$builtIn = Join-Path $repo 'build-win\vrtest\accept-cm-controllers\t025-in.png'
	if (-not (Test-Path -LiteralPath $builtIn) -and -not $Reuse) {
		Write-Host '  (no cm-controllers shot to compare with: playing accept-cm-controllers)'
		$why = Test-CmControllers
		if ($why) { return "cm-controllers: $why" }
	}
	$name = 'accept-cm-own-controllers'
	$why = Invoke-OwnFolders -Fixture '--controllers', '--bad-button' {
		$params = Get-CmControllersParams $name $cmWidenArcade
		$params.Expect = 'XR: controller model from controllers.left\.glb \(36 triangles, 3 primitives in 2 parts\)', 'XR: controller model from controllers.right\.glb \(36 triangles, 3 primitives in 2 parts\)',
			'XR: controller model from controllers.left\.glb: label point left out \(the built-in place\): "menu" is \S+ from the model, over a quarter of its size',
			'XR: labels: left menu at .* in the grip pose \(built in: no "menu" from controllers.left\.json\)$'
		$params.Reject += 'XR: controller model from controllers.right\.glb: label point left out'
		$why = Invoke-VrTest $params
		if ($why) { return $why }
		$why = Test-LogCount $name 'label point left out' 1
		if ($why) { return $why }
		$why = Test-CmOwnLabels $name
		if ($why) { return $why }
		$why = Test-RunShots $name 't025-in.png', 't026-out.png'
		if ($why) { return $why }
		$dir = Join-Path $repo "build-win\vrtest\$name"
		$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-in.png'), (Join-Path $dir 't026-out.png'), '--box', $cmControllerBox
		if ($why) { return "no controller in the left hand: $why" }
		$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-in.png'), $builtIn, '--box', $cmControllerBox
		if ($why) { return "the box controller looks like the built-in one: $why" }
		return $null
	}
	if ($why) { return $why }
	$right = 'accept-cm-own-controllers-right'
	return Invoke-OwnFolders -Fixture '--controllers', '--only-right', '--no-buttons' {
		$params = Get-CmControllersParams $right $cmWidenArcade
		$params.Expect = @('XR: controller model from controllers.right\.glb \(36 triangles, 3 primitives in 2 parts\)',
			'XR: labels: right a at .* in the grip pose \(built in: no "a" from controllers.right\.json\)$',
			'XR: labels: right stick at .* in the grip pose \(built in: no "stick" from controllers.right\.json\)$',
			'XR: labels: left x at .* in the grip pose \(built in\)$')
		$params.Reject += 'XR: controller model from controllers.left', 'XR: labels: .*\(from the file\)'
		$why = Invoke-VrTest $params
		if ($why) { return $why }
		$why = Test-RunShots $right 't025-in.png'
		if ($why) { return $why }
		$why = Invoke-VrCheck 'differ', (Join-Path $repo "build-win\vrtest\$right\t025-in.png"), (Join-Path $repo "build-win\vrtest\$name\t025-in.png"), '--box', $cmControllerBox
		if ($why) { return "without left.glb the left hand still looks like the box: $why" }
		return $null
	}
}

# The bundled models (S37) and the player's, merged file by file: re\make-own-models.py's fixtures in the player's
# folders (build-win\guns: the box gun alone; build-win\controllers: right.glb alone) and in a bundled folder of the
# harness's own, build-win\data\models (the box gun, aa-early "Early gun", both controllers). The player's boxgun.glb goes
# instead of the bundled one, the player's right.glb replaces only the right hand's (the left hand's comes from the
# bundled set), the bundled aa-early is offered and chosen (vr.Gun 4, vr.GunFile aa-early), and the log names the
# bundled files "bundled models/...". The user's own and bundled models are held aside meanwhile.
function Test-CmBundled {
	$why = Invoke-OwnFolders -Fixture '--guns', '--only-box', '--controllers', '--only-right' -Bundled '--guns', '--only-box', '--early', '--controllers' {
		Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-bundled'; Seconds = 20; XrSim = $true; Config = "$cmWiden,config:vr.Gun=4,config:vr.GunFile=aa-early"
			Expect = @('XR: own models from .*models.* \(2 guns, controllers',
				'XR: own gun bundled models.guns.aa-early\.glb offered: "Early gun"', 'XR: own gun guns.boxgun\.glb offered: "Box gun"',
				'XR: own gun "Early gun" from bundled models.guns.aa-early\.glb', 'XR: gun: Early gun \(chosen\)$',
				'XR: controller model from bundled models.controllers.left\.glb \(36 triangles', 'XR: controller model from controllers.right\.glb \(36 triangles')
			Reject = @('XR: initialisation failed', 'bundled models.guns.boxgun', 'bundled models.controllers.right') }
	}
	if ($why) { return $why }
	# A player's file named in another case over a bundled one (Ruling S45): the player's BoxGun.glb over the bundled
	# boxgun.glb, the choice saved as the bundled one's row saved it (vr.Gun 4, vr.GunFile boxgun). The scan folds the
	# case (the player's file goes instead of the bundled one), and so does the vr.GunFile match: the player's box gun is
	# the one chosen, not the in-game pistol.
	return Invoke-OwnFolders -Fixture '--guns', '--only-box' -Bundled '--guns', '--only-box' {
		$guns = Join-Path $repo 'build-win\guns'
		foreach ($ext in 'glb', 'json') {
			# a rename in case only, by way of another name (a case-blind file system may refuse it in one step)
			Move-Item -LiteralPath (Join-Path $guns "boxgun.$ext") -Destination (Join-Path $guns "boxgun-case.$ext")
			Move-Item -LiteralPath (Join-Path $guns "boxgun-case.$ext") -Destination (Join-Path $guns "BoxGun.$ext")
		}
		Invoke-VrTest @{ Game = 'confmiss'; Name = 'accept-cm-bundled-case'; Seconds = 15; XrSim = $true; Config = "$cmWiden,config:vr.Gun=4,config:vr.GunFile=boxgun"
			Expect = @('XR: own gun guns.BoxGun\.glb offered: "Box gun"', 'XR: own gun "Box gun" from guns.BoxGun\.glb',
				'XR: gun: Box gun \(chosen\)$')
			Reject = @('XR: initialisation failed', 'bundled models.guns.boxgun') }
	}
}

$scenarios = [ordered]@{ hotd2 = 'Test-Hotd2'; mok = 'Test-Mok'; 'cm-boot' = 'Test-CmBoot'; 'cm-widen' = 'Test-CmWiden'
	'cm-aim' = 'Test-CmAim'; 'cm-hud' = 'Test-CmHud'; 'cm-p2' = 'Test-CmP2'; 'cm-floor' = 'Test-CmFloor'; 'cm-sim' = 'Test-CmSim'
	'cm-buttons' = 'Test-CmButtons'; 'cm-buttons-pc' = 'Test-CmButtonsPc'; 'cm-pause' = 'Test-CmPause'
	'cm-haptics' = 'Test-CmHaptics'; 'cm-haptics-low' = 'Test-CmHapticsLow'; 'cm-haptics-off' = 'Test-CmHapticsOff'
	'cm-haptics-mag' = 'Test-CmHapticsMag'; 'cm-haptics-reload' = 'Test-CmHapticsReload'
	'cm-menu' = 'Test-CmMenu'; 'cm-menu-quit' = 'Test-CmMenuQuit'
	'cm-controllers' = 'Test-CmControllers'; 'cm-controllers-rt' = 'Test-CmControllersRt'
	'cm-hands' = 'Test-CmHands'; 'cm-hands-shot' = 'Test-CmHandsShot'
	# cm-rip-rifle first: it leaves the rifle's seed, which the next ones need (cm-rip-hand: cm-rip-start's pistol file)
	'cm-rip-rifle' = 'Test-CmRipRifle'; 'cm-rip-start' = 'Test-CmRipStart'; 'cm-rip-hand' = 'Test-CmRipHand'
	'cm-rip-skip' = 'Test-CmRipSkip'; 'cm-rip-skip-rifle' = 'Test-CmRipSkipRifle'
	'cm-rip-pistol' = 'Test-CmRipPistol'; 'cm-rip-pistol-one' = 'Test-CmRipPistolOne'
	# cm-gun after the rip scenarios: it needs the guns' files they leave
	'cm-gun' = 'Test-CmGun'
	'cm-rifle-sim' = 'Test-CmRifleSim'; 'cm-rifle-p2' = 'Test-CmRifleP2'
	'cm-merge-unit' = 'Test-CmMergeUnit'
	'cm-own-gun' = 'Test-CmOwnGun'; 'cm-own-gun-file' = 'Test-CmOwnGunFile'; 'cm-own-controllers' = 'Test-CmOwnControllers'
	'own-folders' = 'Test-OwnFolders'; 'cm-bundled' = 'Test-CmBundled'
	'spike-keys' = 'Test-SpikeKeys' }
# "hotd2,mok" arrives as one string through powershell -File
$names = if ($Only) { @($Only -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ }) } else { @($scenarios.Keys) }
$failed = 0
foreach ($name in $names) {
	if (-not $scenarios.Contains($name)) {
		Write-Output "FAIL ${name}: no such scenario (there are $($scenarios.Keys -join ', '))"
		$failed++
		continue
	}
	Get-Process flycast -ErrorAction SilentlyContinue | Stop-Process -Force
	try { $why = & $scenarios[$name] } catch { $why = "$_" }
	if ($why) { Write-Output "FAIL ${name}: $why"; $failed++ } else { Write-Output "PASS $name" }
}
exit $(if ($failed) { 1 } else { 0 })
