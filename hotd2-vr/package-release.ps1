<#
  Builds VR Light Gun Hub for the Quest and the PC and puts what goes on the Releases page in dist\:

    vr-light-gun-hub-quest.apk   the Quest app (Quest 3, 3S, 2, Pro)
    vr-light-gun-hub-pcvr.zip    the PC version with VR on (flycast.exe, emu.cfg, README.txt, LICENSE.txt)

  Neither holds anything of a game: the players bring their own, and the app makes the hands or
  the staff from it on the spot. Both are checked for that (the names in their file lists: discs,
  keys, rips, frame grabs, models made from a game), and the APK for its signer: signed with the
  public debug key (no release key, see make-release-key.ps1) it is refused, since anyone could
  sign an "update" of the app with that key.

  .\package-release.ps1                  build both, then package
  .\package-release.ps1 -SkipBuild       package what was built last
  .\package-release.ps1 -AllowDebugKey   take a debug-signed APK too (a test build, not for release)
  .\package-release.ps1 -CheckOnly a.zip,b.apk   only check these archives' contents

  Uses the Android SDK's build-tools (ANDROID_HOME, default %LOCALAPPDATA%\Android\Sdk) and a JDK
  (JAVA_HOME, or a Microsoft JDK 17 in Program Files), as build-quest.cmd does.
#>
param([switch]$SkipBuild, [switch]$AllowDebugKey, [string[]]$CheckOnly)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $repo 'dist'
$cmd = Join-Path $env:SystemRoot 'System32\cmd.exe'
Add-Type -AssemblyName System.IO.Compression.FileSystem

# What must never be in a release: game discs and keys, rips (rip\), frame grabs, models made
# from a game (the app makes those on the player's device, from the player's game).
$bannedNames = '*.chd', '*.gdi', '*.cue', '*.cdi', '*.pic', 'shot_*.png', 'hands*.bin', 'staff*.bin', 'HND1*.bin'
$bannedDirs = 'rip'

# The entries of a zip or APK whose names are on the lists above.
function Find-GameData([string]$archive) {
	$zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $archive).Path)
	try {
		foreach ($e in $zip.Entries) {
			$parts = ($e.FullName -replace '\\', '/').Split('/')
			$name = $parts[-1]
			$dirs = if ($parts.Count -gt 1) { $parts[0..($parts.Count - 2)] } else { @() }
			if (($dirs | Where-Object { $bannedDirs -contains $_ }) -or
				($bannedNames | Where-Object { $name -like $_ })) { $e.FullName }
		}
	} finally { $zip.Dispose() }
}

function Test-Archives([string[]]$archives) {
	$bad = $false
	foreach ($a in $archives) {
		$hits = @(Find-GameData $a)
		if ($hits.Count) {
			$bad = $true
			Write-Host "$a holds what may be game data, not released:" -ForegroundColor Red
			$hits | Select-Object -First 20 | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
		} else {
			Write-Host "content check: $(Split-Path -Leaf $a) has no game data names"
		}
	}
	-not $bad
}

if ($CheckOnly) {
	# powershell -File passes "a.zip,b.apk" as one string
	if (Test-Archives ($CheckOnly | ForEach-Object { $_ -split ',' })) { exit 0 } else { exit 1 }
}

# the SDK's newest build-tools and a JDK, as build-quest.cmd finds them
$sdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA 'Android\Sdk' }
$tools = Get-ChildItem (Join-Path $sdk 'build-tools') -Directory -ErrorAction SilentlyContinue |
	Where-Object { Test-Path (Join-Path $_.FullName 'apksigner.bat') } |
	Sort-Object { [version]($_.Name -replace '[^0-9.].*$', '') } -Descending | Select-Object -First 1
if (-not $tools) { Write-Host "No build-tools with apksigner in $sdk (set ANDROID_HOME)." -ForegroundColor Red; exit 1 }
$jdk = @(Get-ChildItem (Join-Path $env:ProgramFiles 'Microsoft') -Directory -Filter 'jdk-17*' -ErrorAction SilentlyContinue |
	ForEach-Object { $_.FullName }) + @($env:JAVA_HOME) |
	Where-Object { $_ -and (Test-Path (Join-Path $_ 'bin\keytool.exe')) } | Select-Object -First 1
if (-not $jdk) { Write-Host 'No JDK found (set JAVA_HOME).' -ForegroundColor Red; exit 1 }
$env:JAVA_HOME = $jdk   # this process only: apksigner.bat runs on it

if (-not $SkipBuild) {
	& $cmd /c (Join-Path $PSScriptRoot 'build-quest.cmd')
	if ($LASTEXITCODE -ne 0) { throw 'build-quest.cmd failed' }
	& $cmd /c (Join-Path $PSScriptRoot 'build-win.cmd') Release
	if ($LASTEXITCODE -ne 0) { throw 'build-win.cmd failed' }
}
New-Item -ItemType Directory -Force $dist | Out-Null
$apkOut = Join-Path $dist 'vr-light-gun-hub-quest.apk'
$zip = Join-Path $dist 'vr-light-gun-hub-pcvr.zip'
$stage = Join-Path $dist 'vr-light-gun-hub-pcvr'
foreach ($f in $apkOut, $zip) { if (Test-Path $f) { (Get-Item $f).Delete() } }   # never leave an old one

# the Quest app: who signed it
$apk = Join-Path $repo 'shell\android-studio\flycast\build\outputs\apk\vr\flycast-vr.apk'
$ErrorActionPreference = 'Continue'   # the tools may write warnings on stderr
$certs = & (Join-Path $tools.FullName 'apksigner.bat') verify --print-certs $apk 2>&1 | ForEach-Object { "$_" }
$signed = $LASTEXITCODE -eq 0
$badging = & (Join-Path $tools.FullName 'aapt2.exe') dump badging $apk 2>&1 | Select-Object -First 1
$tmp = Join-Path $env:TEMP "vrlgh-debug-$PID.cer"
& (Join-Path $jdk 'bin\keytool.exe') -exportcert -keystore (Join-Path $repo 'shell\android-studio\debug.keystore') `
	-storepass android -alias androiddebugkey -file $tmp 2>&1 | Out-Null
$ErrorActionPreference = 'Stop'
if (-not $signed -or -not (Test-Path $tmp)) {
	Write-Host "Could not check the APK's signer:" -ForegroundColor Red; $certs | Write-Host; exit 1
}
$sha = [Security.Cryptography.SHA256]::Create()
$debugDigest = (($sha.ComputeHash([IO.File]::ReadAllBytes($tmp)) | ForEach-Object { $_.ToString('x2') }) -join '')
Remove-Item $tmp
$digests = @($certs | Select-String 'Signer #\d+ certificate SHA-256 digest: ([0-9a-f]+)' |
	ForEach-Object { $_.Matches[0].Groups[1].Value })
if (-not $digests.Count) { Write-Host 'No signer certificate in the APK:' -ForegroundColor Red; $certs | Write-Host; exit 1 }
Write-Host $badging
$certs | Select-String 'certificate DN' | ForEach-Object { Write-Host "$_" }
if ($digests -contains $debugDigest) {
	if (-not $AllowDebugKey) {
		Write-Host ("The APK is signed with the public debug key (shell\android-studio\debug.keystore): not released. " +
			'Make the release key with hotd2-vr\make-release-key.ps1 and build again (or -AllowDebugKey for a test build).') -ForegroundColor Red
		exit 1
	}
	Write-Host 'Signed with the public debug key: a test build, do not publish it.' -ForegroundColor Yellow
}
Copy-Item $apk $apkOut

# the PC version: VR on, OpenGL (the only renderer VR draws with)
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item (Join-Path $repo 'build-win\flycast.exe') $stage
Set-Content -Path (Join-Path $stage 'emu.cfg') -Encoding ascii -Value @'
[config]
pvr.rend = 0
rend.NativeDepthInterpolation = yes
vr.Xr = yes
'@
Set-Content -Path (Join-Path $stage 'README.txt') -Encoding ascii -Value @'
VR Light Gun Hub, PC version

Light-gun games in VR: The House of the Dead 2 (Dreamcast), The Maze of the Kings and
Confidential Mission (arcade, Sega NAOMI). The original games run in the Flycast emulator;
their 3D scenes are rebuilt around you and your controller is the light gun. A fork of
mikermak's The House of the Dead 2 VR (https://github.com/mikermak/hotd2-vr).

1. Connect your VR headset: start SteamVR, Quest Link / Air Link or Virtual Desktop, so it
   is the active OpenXR runtime.
2. Start flycast.exe. In its window, add the folder with your own copies of the games and
   start one. It plays in the headset.
   - The House of the Dead 2: the European or US version, a .chd, .gdi, .cue or .cdi.
   - The Maze of the Kings: the MAME set, mok.zip (its key inside) and the folder mok with
     gds-0022.chd next to it.
   - Confidential Mission: the MAME set, confmiss.zip (its key inside) and the folder
     confmiss with gds-0001.chd next to it.
   The NAOMI BIOS for both: inside the game's zip, or naomi.zip next to flycast.exe or in
   the data folder next to it.
3. The first time The House of the Dead 2 or The Maze of the Kings starts, the app makes the
   agent's hands (or the hero's staff) from your game: it plays itself for a short while,
   fast, behind a panel that says so. Once only; B skips it.

The controller is the light gun: the trigger shoots, shooting away from the game's view
reloads, A is Start, the left controller's menu button lets player 2 join (a gun in each
hand), X or a click on the left thumbstick recenters. The project's readme has the rest.
Without a headset the game plays in the window. Settings are in emu.cfg (vr.Xr = yes is VR).

No game is included: you need your own copies. Unofficial fan project, not affiliated with
Sega.

Credits: Flycast by flyinghead and contributors (https://github.com/flyinghead/flycast,
GPL-2.0-or-later). The House of the Dead 2 VR by mikermak, which this app is a fork of
(GPL-2.0-or-later). The gun: "Namco Arcade Gun" by Martoscar
(https://sketchfab.com/3d-models/namco-arcade-gun-15fbd5b9add94a34b5c21746e3dd32be), CC BY 4.0
(https://creativecommons.org/licenses/by/4.0/); changed: re-oriented, quantised, textures and
materials dropped, recoloured, converted to C++. The OpenXR loader by Khronos, Apache-2.0.

Licence: this build is distributed under the GNU GPL version 3 (LICENSE.txt). Its source code
is published with every release, on the same page as this zip.
'@
# the licence: GPL-3.0 for the build (the gun model's CC BY 4.0 goes with GPL-3.0, not GPL-2.0;
# Flycast's "or later" allows it). Its text: the copy that comes with Flycast's picoTCP.
$gpl3 = Get-Content -Raw (Join-Path $repo 'core\deps\picotcp\LICENSE.GPLv3')
if ($gpl3 -notmatch 'GNU GENERAL PUBLIC LICENSE\s+Version 3, 29 June 2007') { throw 'GPL-3.0 text not found' }
Set-Content -Path (Join-Path $stage 'LICENSE.txt') -Encoding ascii -NoNewline -Value (@'
VR Light Gun Hub, PC version: licence

flycast.exe is built from Flycast (GPL-2.0-or-later), mikermak's The House of the Dead 2 VR and
VR Light Gun Hub's changes to it (GPL-2.0-or-later too), and the gun model "Namco Arcade Gun" by
Martoscar (CC BY 4.0, https://creativecommons.org/licenses/by/4.0/). CC BY 4.0 goes together
with GPL-3.0 but not GPL-2.0, so this build is distributed under the GNU General Public License
version 3, below. Its source code is published with every release, on the same page as this zip.

'@ + $gpl3)
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip

if (-not (Test-Archives $apkOut, $zip)) {
	foreach ($f in $apkOut, $zip) { if (Test-Path $f) { (Get-Item $f).Delete() } }
	exit 1
}
Get-ChildItem $dist -File | Select-Object Name, @{ n = 'MB'; e = { [math]::Round($_.Length / 1MB, 1) } } | Format-Table | Out-String | Write-Host
exit 0
