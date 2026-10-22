<#
  Makes the agent's hands and pistol for the headset (hands.bin) from your own copy of
  The House of the Dead 2, and puts them on the Quest.

  Runs the Windows build (build-win.cmd) on your game: it starts Arcade mode and lets the
  agent lose (no input), and at the game over scene, where he kneels with his pistol in his
  hand, Flycast rips that frame (run\rip). assets\rip_hands.py turns it into hands.bin
  (with a preview), and when a headset is connected over adb it goes into the app's files.

  .\rip-hands.ps1             all of it (about two and a half minutes)
  .\rip-hands.ps1 -NoPush     keep hands.bin on the PC (run\hands.bin)
  .\rip-hands.ps1 -Manual     you play: the rip comes at the first frame with his pistol

  hands.bin holds the game's own models and textures: keep it to yourself.
  Needs Python with numpy and pillow (pip install -r assets\requirements.txt).
#>
param(
	[switch]$NoPush,
	[switch]$Manual,
	[int]$Port = 27016,
	[int]$TimeoutSeconds = 240
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$data = if ($env:HOTD2VR_DATA) { $env:HOTD2VR_DATA } else { Split-Path -Parent $repo }
$exe = Join-Path $repo 'build-win\flycast.exe'
if (-not (Test-Path $exe)) { throw "Build the Windows version first: hotd2-vr\build-win.cmd" }
$cue = Get-ChildItem (Join-Path $data 'game') -Include *.cue, *.gdi, *.chd -Recurse | Select-Object -First 1
if (-not $cue) { throw "No game found in $(Join-Path $data 'game')" }
$runDir = Join-Path $data 'run'
$ripDir = Join-Path $runDir 'rip'
$out = Join-Path $runDir 'hands.bin'
$preview = Join-Path $runDir 'hands-preview.png'
New-Item -ItemType Directory -Force $runDir | Out-Null
if (Test-Path $ripDir) { Get-ChildItem $ripDir -Filter '*.bin' | Remove-Item }

# The agent's pistol (HOTD2 PAL): rip the first frame with at least 20 of its polygons.
Set-Content -Path (Join-Path $runDir 'rip.request') -Value '1 1 0 5fe380 20' -NoNewline

# OpenGL with native depth and the reprojection on: the rip hooks into that path.
$cfg = 'config:pvr.rend=0,config:rend.NativeDepthInterpolation=yes,config:vr.Reproject=yes,config:aica.Volume=0,log:LogToFile=yes'
if (-not $Manual) { $cfg += ",input:device1=7,config:vr.UdpPort=$Port" }
$p = Start-Process -FilePath $exe -ArgumentList @('-config', $cfg, "`"$($cue.FullName)`"") -WorkingDirectory $runDir -PassThru
Write-Host 'Flycast started; waiting for the game over scene...'

# The light gun over UDP: Start a few times once the game has loaded (title, Arcade mode),
# then nothing, so the agent loses and kneels.
$udp = New-Object System.Net.Sockets.UdpClient
$t0 = Get-Date
$ripped = $null
try {
	while (-not $ripped) {
		$t = ((Get-Date) - $t0).TotalSeconds
		if ($t -gt $TimeoutSeconds) { throw "No rip after $TimeoutSeconds s. Try -Manual and play to the game over." }
		if ($p.HasExited) { throw "Flycast closed before the rip." }
		if (-not $Manual) {
			$press = $t -ge 44 -and $t -le 58.5 -and (($t - 44) % 2) -lt 0.2
			$msg = [Text.Encoding]::ASCII.GetBytes("LG 0 5000 5000 $(if ($press) { 4 } else { 0 })")
			[void]$udp.Send($msg, $msg.Length, '127.0.0.1', $Port)
		}
		Start-Sleep -Milliseconds 50
		$ripped = Get-ChildItem $ripDir -Filter 'pass*.bin' -ErrorAction SilentlyContinue | Select-Object -First 1
	}
	Start-Sleep -Milliseconds 500
} finally {
	$udp.Close()
	if (-not $p.HasExited) { Stop-Process -Id $p.Id }
	Remove-Item (Join-Path $runDir 'rip.request') -ErrorAction SilentlyContinue
}

$py = Join-Path $PSScriptRoot 'fingergun\.venv\Scripts\python.exe'
if (-not (Test-Path $py)) { $py = 'python' }
& $py (Join-Path $PSScriptRoot 'assets\rip_hands.py') $ripDir $out --preview $preview
if ($LASTEXITCODE -ne 0) { throw 'rip_hands.py failed' }
Write-Host "Preview: $preview"

if ($NoPush) { return }
$adb = Join-Path $(if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { "$env:LOCALAPPDATA\Android\Sdk" }) 'platform-tools\adb.exe'
if (-not (Test-Path $adb)) { $adb = 'adb' }
$devices = & $adb devices | Select-Object -Skip 1 | Where-Object { $_ -match '\tdevice$' }
if (-not $devices) {
	Write-Host "No headset over adb: hands.bin stays in $runDir. Later: adb push it to /data/local/tmp and copy it into the app's files with run-as (see README)."
	return
}
$pkg = 'com.vrlightgunhub.app'
& $adb push $out /data/local/tmp/hands.bin | Out-Null
& $adb shell "run-as $pkg sh -c 'cat /data/local/tmp/hands.bin > files/hands.bin' && rm /data/local/tmp/hands.bin"
& $adb shell am force-stop $pkg
Write-Host 'On the headset: start VR Light Gun Hub again to use them.'
