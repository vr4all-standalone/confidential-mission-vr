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
  four runs (accept-cm-tour, -off, -edges, -hall), each played once per call.
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
# intro), the intro to ~85 s, then the agent loses (CONTINUE? at ~100 s).
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
$cmPoints = @(@(5000, 5000), @(1000, 1000), @(9000, 1000), @(1000, 9000), @(9000, 9000), @(5000, 1000), @(5000, 9000), @(1000, 5000), @(9000, 5000))

function Get-CmStarts { for ($t = 30; $t -le 52; $t += 2) { "$t=start 0" } }

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

# The shared runs (vr-test.ps1 parameters, no checks: each scenario checks its own lines).
$cmRunDefs = [ordered]@{
	# boot to the attract's 3D: Start to 52 s, then nothing (t3-stage1d, t5-gate2's timeline),
	# viewed yawed and pitched (t4y-*), the game's own frames every 4 s
	'accept-cm-tour' = @{ Game = 'confmiss'; Name = 'accept-cm-tour'; Seconds = 156; DebugShots = 4
		Config = "$cmWiden,config:vr.Yaw=25,config:vr.Pitch=-15"
		At = @(Get-CmStarts) + '46.3=shot cutscene', '49=shot cutscene', '55=shot stage1', '60=shot stage1', '65=shot continue',
			'79=shot', '95=shot', '108=shot attract', '118=shot attract', '130=shot attract', '142=shot attract', '148=shot attract', '154=shot attract' }
	# widening off, the marker at the gun: t5-g2-off (shots on the adjustment screen included).
	# The player 2 prompt stays: Task 6's hiding of it also hides flat polygons on its line at the
	# bottom right while it blinks on, which took the lower half of the marker at 576,432 here
	# (t5-g2-off, before Task 6, had all of it); with the defaults the marker is hidden anyway.
	'accept-cm-off' = @{ Game = 'confmiss'; Name = 'accept-cm-off'; Seconds = 60
		Config = 'config:vr.DropShotMarker=no,config:vr.DropShotFlash=no,config:vr.HideP2Prompt=no'
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

# Lines matching a regex in a run's log, at most $max: $null when so.
function Test-LogCount([string]$name, [string]$re, [int]$max) {
	$n = @(Select-String -LiteralPath (Get-RunLog $name) -Pattern $re -CaseSensitive).Count
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
	$why = Invoke-VrCheck 'focal', $log, '--from', '45.5', '--to', '46.5', '--expect', '224.7'
	if ($why) { $why = Invoke-VrCheck 'focal', $log, '--from', '45.5', '--to', '46.5', '--expect', '436.4' }
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
	$why = Invoke-VrCheck 'shots', $edges, '--tex', '5e8000', '--union', '--max-off', '3', '--from', '52.5', '--to', '55' -Want '^shots 4 matched 4 '
	if ($why) { return "edges: $why" }
	return Invoke-VrCheck 'shots', $hall, '--tex', '5e8000', '--union', '--max-ratio', '0.9'
}

# The 2D plane: layers, sights, the letterbox and the flat screens go onto it; no more 3D
# polygons kept with a corner on its W than Task 4 saw (1 a run; the hall route 2 in Task 5);
# each frame's nearest W after it the 3D's near plane or the plane's own; the shot marker
# hidden but not the game's own white fade (tex 404800, ~119 s on the hall route); yawed window shots.
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

$scenarios = [ordered]@{ hotd2 = 'Test-Hotd2'; mok = 'Test-Mok'; 'cm-boot' = 'Test-CmBoot'; 'cm-widen' = 'Test-CmWiden'
	'cm-aim' = 'Test-CmAim'; 'cm-hud' = 'Test-CmHud'; 'cm-p2' = 'Test-CmP2' }
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
