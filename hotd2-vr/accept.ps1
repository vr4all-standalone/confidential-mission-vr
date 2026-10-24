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
  and cm-hands-shot (accept-cm-hands, accept-cm-hands-shot). The gun run: cm-rip-cabinet plays
  accept-cm-rip-cabinet (it deletes build-win\data\gun-cm-*.bin first and leaves the cabinet gun's
  file there), cm-rip-hand accept-cm-rip-hand and -arcade (it needs that file), cm-rip-skip
  accept-cm-rip-skip and -skip2 (it deletes the files again). A CM run that starts a game with
  vr.GameHands on and no gun-cm-cabinet.bin makes the file while watching (the reload tutorial
  after Start); the XR scenarios from before it pin the arcade gun ($cmWidenArcade).
  spike-keys plays accept-spike-keys: the harness's scene keys (vr-test.ps1 -On, poke, vr.WatchRam).
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
# The XR scenarios from before the game's own guns (Task 10) pin the arcade gun: their aim, muzzle
# (vr.GunScale 0.68: cm-menu's Get-CmMenuAim) and shots assume it, and with vr.GameHands off a
# run neither draws gun-cm-cabinet.bin nor makes it, whatever ran before.
$cmWidenArcade = "$cmWiden,config:vr.GameHands=no"
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

# The controller models (xr_controllers.h) through the simulator, in stage 1's first room: the
# free (left) hand moved in front of the left eye at 56 s, a window shot with it there at 57 s,
# the hand moved away (below and behind the head) at 57.5 s and another shot at 58 s. The left
# eye's view differs where the controller was ($cmControllerBox: around it, below the enemy's
# feet, left of the gun's laser; without a controller the two shots differ there by ~0.3, with
# the built-in shape by ~17). The simulator lists no XR_FB_render_model: the built-in shape.
$cmControllerBox = '0.32,0.62,0.5,0.88'
function Get-CmControllersParams([string]$name, [string]$config) {
	@{ Game = 'confmiss'; Name = $name; Seconds = 62; XrSim = $true; Config = $config
		At = @(Get-CmStartsXr) + '56=pose left -0.1 1.45 -0.3 0 0', '57=shot in', '57.5=pose left -0.1 0.2 0.5 0 0', '58=shot out'
		Reject = @('XR: initialisation failed') }
}
function Test-CmControllers {
	$name = 'accept-cm-controllers'
	$params = Get-CmControllersParams $name $cmWidenArcade
	$params.Expect = 'XR: runtime extensions: ', 'XR: controller models built in \(the runtime has no XR_FB_render_model\)'
	$params.Reject += 'XR: runtime extensions: .*XR_FB_render_model', 'XR: controller models from the runtime'
	$why = Invoke-VrTest $params
	if ($why) { return $why }
	$why = Test-RunShots $name 't057-in.png', 't058-out.png'
	if ($why) { return $why }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't057-in.png'), (Join-Path $dir 't058-out.png'), '--box', $cmControllerBox
	if ($why) { return "no controller in the left hand: $why" }
	return $null
}

# The same with vr.XrSimRenderModel: the simulator lists XR_FB_render_model and serves the test
# GLB (re\make-test-glb.py: an 8 cm box and a 12 cm cylinder, 140 triangles, a 2x2 PNG) as both
# controllers' model; the hand in front of the eye then looks unlike the built-in shape. As a
# Quest, the simulator has no model until the interaction profile is bound (at the first
# xrSyncActions while focused, with its event): the models are unavailable at the first FOCUSED
# state and come with the retry at that event.
function Test-CmControllersRt {
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
	$why = Test-RunShots $name 't057-in.png', 't058-out.png'
	if ($why) { return $why }
	$builtIn = Join-Path $repo 'build-win\vrtest\accept-cm-controllers\t057-in.png'
	if (-not (Test-Path -LiteralPath $builtIn) -and -not $Reuse) {
		Write-Host '  (no cm-controllers shot to compare with: playing accept-cm-controllers)'
		$why = Test-CmControllers
		if ($why) { return "cm-controllers: $why" }
	}
	if (-not (Test-Path -LiteralPath $builtIn)) { return "no $builtIn to compare with" }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't057-in.png'), (Join-Path $dir 't058-out.png'), '--box', $cmControllerBox
	if ($why) { return "no controller in the left hand: $why" }
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't057-in.png'), $builtIn, '--box', $cmControllerBox
	if ($why) { return "the runtime's model looks like the built-in one: $why" }
	return $null
}

# What each hand holds and the how-to-play labels (Task 9) through the simulator, widened: the
# right hand in front of the left eye from 20 s, a controller with its labels (shots at 25, 27 s); the
# first A (30 s) gives it the gun and the labels go (shot at 35 s). X lets player 2 join at 60 s
# (the left hand takes its gun), player 2 idles out at ~70 s (findings p2out: a controller
# again). The menu at 90 s, the right stick up twice (from Resume it wraps to Quit, then Show
# controls) and A: both hands controllers with labels again, until the right trigger's pull at
# 94 s gives the gun back. The menu again at 96 s, Show controls chosen by the right trigger and
# held through the close: the labels stay. The labels and the gun differ in the right hand's part
# of the view.
function Test-CmHands {
	$name = 'accept-cm-hands'
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 102; XrSim = $true; Config = $cmWidenArcade
		At = @('20=pose right 0.1 1.45 -0.3 0 0', '25=shot labels', '27=shot labels2') + @(Get-CmStartsXr) + @('35=shot gun', '60=xr left/x 1 200',
			'90=xr left/menu 1 150', '90.5=stick right 0 1 150', '91=stick right 0 1 150', '91.5=xr right/a 1 150',
			'94=xr right/trigger 1 100', '96=xr left/menu 1 150', "96.5=pose right 0.18 1.3 -0.35 $(Get-CmMenuAim -Row 8 -U 0.5)",
			'97=xr right/trigger 1 800')
		Expect = 'XR: labels off \(start\)', 'XR: right hand: controller -> gun \(start\)',
			'XR: left hand: controller -> gun \(player 2 joins\)', 'XR: left hand: gun -> controller \(player 2 out\)',
			'XR: show controls', 'XR: right hand: controller -> gun \(shot\)'
		Reject = @('XR: initialisation failed') }
	if ($why) { return $why }
	# Show controls chosen by the trigger (97 s), held through the menu's close: that pull isn't the
	# game's next one, so the labels stay (one "labels off (shot)", from 94 s, for two Show controls).
	$shows = Get-LogCount $name 'XR: show controls$'
	$offs = Get-LogCount $name 'XR: labels off \(shot\)'
	Write-Host "  $shows lines 'XR: show controls' (want 2), $offs lines 'XR: labels off (shot)' (want 1)"
	if ($shows -ne 2 -or $offs -ne 1) { return "Show controls by the trigger: $shows show controls, $offs labels off (shot)" }
	$why = Test-RunShots $name 't025-labels.png', 't027-labels2.png', 't035-gun.png'
	if ($why) { return $why }
	$dir = Join-Path $repo "build-win\vrtest\$name"
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-labels.png'), (Join-Path $dir 't035-gun.png'), '--box', '0.3,0.3,0.9,0.95'
	if ($why) { return "the labels and the gun look the same: $why" }
	# The game's screen changes between those two as well. The left hand's part of the view: its
	# controller the same in both, only its labels go (around them: their dark plates are near the
	# room's black, the letters do it); and with nothing done (25, 27 s) the same.
	$left = '0.25,0.74,0.43,0.9'
	$why = Invoke-VrCheck 'differ', (Join-Path $dir 't025-labels.png'), (Join-Path $dir 't035-gun.png'), '--box', $left
	if ($why) { return "the left hand's labels didn't go: $why" }
	$why = Invoke-VrCheck 'same', (Join-Path $dir 't025-labels.png'), (Join-Path $dir 't027-labels2.png'), '--box', $left
	if ($why) { return "the left hand's labels changed with nothing done: $why" }
	return Test-ArmingShot $name
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
# The gun run (Task 10, core/rend/vr/hands_rip.h): with no gun-cm-*.bin in build-win\data the first
# start runs the game by itself (on the PC with vr.HandsPrep): Start at 30 s of game, the reload
# tutorial, whose cabinet gun (tex 400000, 279 polygons) becomes gun-cm-cabinet.bin (HND1 v2, gun
# only), within 120 s of game. Every later Confidential Mission run draws that gun instead of the
# arcade gun (a run that starts a game without the file makes it too, at the tutorial).
#
$cmGunData = Join-Path $repo 'build-win\data'
$cmCabinetFile = Join-Path $cmGunData 'gun-cm-cabinet.bin'
function Remove-CmGuns {
	Get-ChildItem -LiteralPath $cmGunData -Filter 'gun-cm-*.bin*' -ErrorAction SilentlyContinue | Remove-Item -Force
}

# The run, flat: no files, a .part left by a run that died (written over); the cabinet gun saved,
# the run done by 120 s of game; the model's checks (vrcheck hnd).
function Test-CmRipCabinet {
	$name = 'accept-cm-rip-cabinet'
	if (-not $Reuse) {
		Remove-CmGuns
		[IO.File]::WriteAllText("$cmCabinetFile.part", 'junk from a run that died')
	}
	$why = Invoke-VrTest @{ Game = 'confmiss'; Name = $name; Seconds = 150; Config = 'config:vr.HandsPrep=yes'
		Expect = 'VR hands: no model yet, taking the guns from the game',
			'VR hands: the cabinet gun from this game, \d+ bytes to .*gun-cm-cabinet\.bin', 'VR hands: done after \d+ s of game'
		Reject = @('VR hands: .*, but ') }
	if ($why) { return $why }
	$hit = Select-String -LiteralPath (Get-RunLog $name) -Pattern 'VR hands: done after (\d+) s of game' -CaseSensitive | Select-Object -First 1
	$n = [int]$hit.Matches[0].Groups[1].Value
	Write-Host "  done after $n s of game (want 120 or less)"
	if ($n -gt 120) { return "the gun run took $n s of game, over 120" }
	if (Test-Path -LiteralPath "$cmCabinetFile.part") { return "$cmCabinetFile.part is still there" }
	if (-not (Test-Path -LiteralPath $cmCabinetFile)) { return "no $cmCabinetFile" }
	return Invoke-VrCheck 'hnd', $cmCabinetFile, '--polys', '279', '--length', '0.27', '--gun-only'
}

# The cabinet gun in the right hand through the simulator, widened (vr.Gun 2: the cabinet gun, which
# Task 13's Auto must keep): the hand in front of the left eye from 20 s, the first A (30 s) gives it
# the gun, a window shot at 35 s; against the same shot with the arcade gun (vr.GameHands off) in the
# right hand's part of the view.
function Get-CmRipHandParams([string]$name, [string]$config) {
	@{ Game = 'confmiss'; Name = $name; Seconds = 37; XrSim = $true; Config = $config
		At = @('20=pose right 0.1 1.45 -0.3 0 0') + @(Get-CmStartsXr | Where-Object { [double]($_ -split '=')[0] -lt 36 }) + '35=shot held'
		Reject = @('XR: initialisation failed') }
}
function Test-CmRipHand {
	if (-not (Test-Path -LiteralPath $cmCabinetFile)) { return "no $cmCabinetFile (cm-rip-cabinet makes it)" }
	$name = 'accept-cm-rip-hand'
	$params = Get-CmRipHandParams $name "$cmWiden,config:vr.Gun=2"
	$params.Expect = @('XR: the game''s own model from .*gun-cm-cabinet\.bin')
	$params.Reject += 'XR: no .*gun-cm-cabinet\.bin'
	$why = Invoke-VrTest $params
	if ($why) { return $why }
	$arcade = 'accept-cm-rip-hand-arcade'
	$params = Get-CmRipHandParams $arcade "$cmWiden,config:vr.GameHands=no"
	$params.Reject += 'XR: the game''s own model from'
	$why = Invoke-VrTest $params
	if ($why) { return "arcade gun: $why" }
	foreach ($run in $name, $arcade) {
		$why = Test-RunShots $run 't035-held.png'
		if ($why) { return $why }
	}
	$why = Invoke-VrCheck 'differ', (Join-Path $repo "build-win\vrtest\$name\t035-held.png"),
		(Join-Path $repo "build-win\vrtest\$arcade\t035-held.png"), '--box', '0.5,0.5,0.8,0.95', '--min-diff', '2'
	if ($why) { return "the cabinet gun looks like the arcade gun: $why" }
	return $null
}

# B skips the run and the menu button does nothing during it, both keyed on the run's first line
# (the gun comes up ~6 s in, at 30 s of game). Skipped holds for the session (S23): Start from
# 22 s after the skip (30 s of game or more) and on through the reload tutorial, where watching
# would make the gun: still the arcade gun, and no file. The next start runs it again.
function Test-CmRipSkip {
	if (-not $Reuse) { Remove-CmGuns }
	$first = 'VR hands: no model yet, taking the guns from the game'
	$keys = @("$first|1|xr left/menu 1 150", "$first|2|xr right/b 1 200")
	$starts = @(22, 25, 28 | ForEach-Object { "VR hands: skipped after|$_|xr right/a 1 200" })
	$params = @{ Game = 'confmiss'; Name = 'accept-cm-rip-skip'; Seconds = 48; XrSim = $true; Config = "$cmWiden,config:vr.HandsPrep=yes"
		On = @($keys + $starts)
		Expect = $first, 'VR hands: skipped after', 'XR: menu: not while the guns are being made'
		# (the loader says "no ..., the arcade gun it is" at INFO, which the log leaves out)
		Reject = @('XR: initialisation failed', 'XR: menu open', 'XR: the game''s own model from',
			'VR hands: the cabinet gun from this game') }
	$why = Invoke-VrTest $params
	if ($why) { return $why }
	if (Test-Path -LiteralPath $cmCabinetFile) { return "$cmCabinetFile made though the run was skipped" }
	$params.Name = 'accept-cm-rip-skip2'
	$params.Seconds = 10
	$params.On = $keys
	$params.Expect = @($first)
	$why = Invoke-VrTest $params
	if ($why) { return "the next start: $why" }
	return $null
}

$scenarios = [ordered]@{ hotd2 = 'Test-Hotd2'; mok = 'Test-Mok'; 'cm-boot' = 'Test-CmBoot'; 'cm-widen' = 'Test-CmWiden'
	'cm-aim' = 'Test-CmAim'; 'cm-hud' = 'Test-CmHud'; 'cm-p2' = 'Test-CmP2'; 'cm-floor' = 'Test-CmFloor'; 'cm-sim' = 'Test-CmSim'
	'cm-buttons' = 'Test-CmButtons'; 'cm-buttons-pc' = 'Test-CmButtonsPc'; 'cm-pause' = 'Test-CmPause'
	'cm-haptics' = 'Test-CmHaptics'; 'cm-haptics-low' = 'Test-CmHapticsLow'; 'cm-haptics-off' = 'Test-CmHapticsOff'
	'cm-haptics-mag' = 'Test-CmHapticsMag'; 'cm-haptics-reload' = 'Test-CmHapticsReload'
	'cm-menu' = 'Test-CmMenu'; 'cm-menu-quit' = 'Test-CmMenuQuit'
	'cm-controllers' = 'Test-CmControllers'; 'cm-controllers-rt' = 'Test-CmControllersRt'
	'cm-hands' = 'Test-CmHands'; 'cm-hands-shot' = 'Test-CmHandsShot'
	'cm-rip-cabinet' = 'Test-CmRipCabinet'; 'cm-rip-hand' = 'Test-CmRipHand'; 'cm-rip-skip' = 'Test-CmRipSkip'
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
