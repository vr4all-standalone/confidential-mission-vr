<#
  Builds Confidential Mission VR for the Quest and the PC and puts what goes on the Releases page
  in dist\:

    confidential-mission-vr-quest.apk   the Quest app (Quest 3, 3S, 2, Pro)
    confidential-mission-vr-pcvr.zip    the PC version with VR on (flycast.exe, emu.cfg, README.txt,
                                        LICENSE.txt, Play Confidential Mission.cmd)

  Neither holds anything of the game: the players bring their own, and what the app makes from it
  is made on the spot. Both are checked for that (the names in their file lists: discs, keys,
  rips, frame grabs, models made from a game), and the APK for its signer: signed with the public
  debug key (no release key, see make-release-key.ps1) it is refused, since anyone could sign an
  "update" of the app with that key. The APK is also checked for being Confidential Mission VR
  (its package, version and label, and its start screen the only entry in the headset's library),
  and the zip for holding exactly its five files, with a README.txt that names no other game.

  A release is built from a commit, the one its published source is: the working tree must be
  clean (the gamesdk submodule's local change aside), and the APK's versionName (1.1.0-g<commit>),
  its libflycast.so and flycast.exe must all name HEAD (a -SkipBuild after a new commit fails
  that). -AllowDebugKey test builds skip both checks.

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

# The app: its application ID, label and version (shell\android-studio\flycast\build.gradle.kts and
# src\vr\AndroidManifest.xml), its library entry, and the PC zip's start script (hotd2-vr\assets).
$appId = 'com.vrlightgunhub.confmiss'
$appLabel = 'Confidential Mission VR'
$appVersion = '1.1.0'
$appLauncher = 'com.flycast.emulator.CmStartActivity'
$playCmd = 'Play Confidential Mission.cmd'

# What must never be in a release: game discs and keys, rips (rip\), frame grabs, models made
# from a game (the app makes those on the player's device, from the player's game), and archives
# or disc images inside it (a ROM set, a BIOS zip): neither the APK nor the zip holds one.
$bannedNames = '*.chd', '*.gdi', '*.cue', '*.cdi', '*.pic', 'shot_*.png', 'hands*.bin', 'staff*.bin', 'HND1*.bin',
	'gun-*.bin', '*.zip', '*.7z', '*.rar', '*.iso'
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

function Invoke-Git { $ErrorActionPreference = 'Continue'; & git -C $repo @args 2>$null }

# One entry of a zip or APK, as bytes ($null when it has none by that name).
function Read-ZipEntry([string]$archive, [string]$name) {
	$z = [IO.Compression.ZipFile]::OpenRead($archive)
	try {
		$e = $z.GetEntry($name)
		if (-not $e) { return $null }
		$ms = New-Object IO.MemoryStream
		$s = $e.Open()
		try { $s.CopyTo($ms) } finally { $s.Dispose() }
		, $ms.ToArray()
	} finally { $z.Dispose() }
}

# A built binary names this commit: the version core\version.h gave it (v<tag>-<n>-g<commit>).
function Test-NamesCommit([string]$what, [byte[]]$bytes, [string]$hash) {
	if ($bytes -and [Text.Encoding]::ASCII.GetString($bytes).Contains("-g$hash")) {
		Write-Host "version check: $what names $hash"
		return $true
	}
	Write-Host ("$what does not name HEAD ($hash): build again without -SkipBuild (if libflycast.so still " +
		"doesn't, delete shell\android-studio\flycast\.cxx so its CMake configure runs again).") -ForegroundColor Red
	$false
}

# The activities and aliases whose intent filters have the LAUNCHER category (an entry in the
# headset's library), from aapt2's dump of the APK's AndroidManifest.xml.
function Get-LauncherOwners([string[]]$tree) {
	$owner = $null; $ownerIndent = -1; $named = $false
	foreach ($line in $tree) {
		if ($line -match '^(\s*)E: (\S+)') {
			$indent = $Matches[1].Length
			if ($owner -ne $null -and $indent -le $ownerIndent) { $owner = $null }
			if ($Matches[2] -eq 'activity' -or $Matches[2] -eq 'activity-alias') {
				$owner = '(no name)'; $ownerIndent = $indent; $named = $false
			}
		} elseif ($owner -ne $null -and -not $named -and $line -match '^(\s*)A: \S*:name\([^)]*\)="([^"]+)"' -and
			$Matches[1].Length -eq $ownerIndent + 2) {
			$owner = $Matches[2]; $named = $true
		} elseif ($line -match '="android\.intent\.category\.LAUNCHER"') {
			if ($owner -ne $null) { $owner } else { '(outside an activity)' }
		}
	}
}

# the working tree as committed (the gamesdk submodule's local change aside)
$head = Invoke-Git rev-parse HEAD
if (-not $AllowDebugKey) {
	$dirty = @(Invoke-Git status --porcelain | Where-Object { $_ -and $_ -notmatch '^.. core/deps/gamesdk$' })
	if ($dirty.Count) {
		Write-Host 'The working tree has changes that are not committed; a release is built from a commit:' -ForegroundColor Red
		$dirty | Select-Object -First 20 | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
		exit 1
	}
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
	# The PC first: build-win.cmd runs CMake's configure every time, which writes core\version.h
	# (the version both builds embed) for HEAD. The Quest build's configure runs only when its
	# CMake files change, so it alone would keep an old commit's version.h; its ninja rebuilds
	# what includes the new one.
	& $cmd /c (Join-Path $PSScriptRoot 'build-win.cmd') Release
	if ($LASTEXITCODE -ne 0) { throw 'build-win.cmd failed' }
	$short = Invoke-Git rev-parse --short HEAD
	if ((Get-Content -Raw (Join-Path $repo 'core\version.h')) -notmatch "GIT_HASH\s+""$short""") { throw "core\version.h does not name HEAD ($short)" }
	& $cmd /c (Join-Path $PSScriptRoot 'build-quest.cmd')
	if ($LASTEXITCODE -ne 0) { throw 'build-quest.cmd failed' }
}
New-Item -ItemType Directory -Force $dist | Out-Null
$apkOut = Join-Path $dist 'confidential-mission-vr-quest.apk'
$zip = Join-Path $dist 'confidential-mission-vr-pcvr.zip'
$stage = Join-Path $dist 'confidential-mission-vr-pcvr'
foreach ($f in $apkOut, $zip) { if (Test-Path $f) { (Get-Item $f).Delete() } }   # never leave an old one

# the Quest app: who signed it
$apk = Join-Path $repo 'shell\android-studio\flycast\build\outputs\apk\vr\flycast-vr.apk'
$ErrorActionPreference = 'Continue'   # the tools may write warnings on stderr
$certs = & (Join-Path $tools.FullName 'apksigner.bat') verify --print-certs $apk 2>&1 | ForEach-Object { "$_" }
$signed = $LASTEXITCODE -eq 0
$badgingAll = @(& (Join-Path $tools.FullName 'aapt2.exe') dump badging $apk 2>&1 | ForEach-Object { "$_" })
$badging = $badgingAll | Where-Object { $_ -match '^package:' } | Select-Object -First 1   # not a warning line
$manifestTree = @(& (Join-Path $tools.FullName 'aapt2.exe') dump xmltree --file AndroidManifest.xml $apk 2>&1 |
	ForEach-Object { "$_" })
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
# the app it is: Confidential Mission VR's package, version and label, with its start screen the
# only entry in the headset's library (the VR activity is started from it) and no hub panel left
$short = Invoke-Git rev-parse --short HEAD
$wantVersion = [regex]::Escape("$appVersion-g") + $(if ($AllowDebugKey) { '[0-9a-f]+' } else { [regex]::Escape("$short") })
$launchers = @(Get-LauncherOwners $manifestTree)
$wrong = @()
if ($badging -notmatch "^package: name='$([regex]::Escape($appId))'") { $wrong += "the package is not $appId" }
if ($badging -notmatch "versionName='$wantVersion'") {
	$wrong += "the versionName is not $appVersion-g$(if ($AllowDebugKey) { '<commit>' } else { $short })"
}
if ($badgingAll -notcontains "application-label:'$appLabel'") { $wrong += "the label is not '$appLabel'" }
if (-not $launchers.Count -or @($launchers | Where-Object { $_ -ne $appLauncher }).Count) {
	$wrong += "the library entries (LAUNCHER) are '$($launchers -join "', '")', not $appLauncher alone"
}
if (@($manifestTree | Where-Object { $_ -match 'VrSetupActivity' }).Count) { $wrong += 'the manifest still has VrSetupActivity' }
if (-not @($manifestTree | Where-Object { $_ -match '"com\.oculus\.permission\.RENDER_MODEL"' }).Count) {
	$wrong += 'the manifest has no com.oculus.permission.RENDER_MODEL (the controllers drawn in the hands)'
}
if ($wrong.Count) {
	Write-Host "The APK is not $appLabel as released:" -ForegroundColor Red
	$wrong | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
	exit 1
}
Write-Host "app check: $appId, '$appLabel', its start screen the only library entry"
# built from HEAD: the APK's versionName, its libflycast.so and flycast.exe
if (-not $AllowDebugKey) {
	$built = if ($badging -match "versionName='[^']*-g([0-9a-f]+)'") { $Matches[1] } else { $null }
	$at = if ($built) { Invoke-Git rev-parse --verify --quiet "$built^{commit}" } else { $null }
	if (-not $built -or -not $head -or $at -ne $head) {
		Write-Host ("The APK was built from $(if ($built) { "commit $built" } else { 'no commit its versionName names' }), " +
			"not HEAD ($head): build it again (without -SkipBuild).") -ForegroundColor Red
		exit 1
	}
	if (-not (Test-NamesCommit 'libflycast.so in the APK' (Read-ZipEntry $apk 'lib/arm64-v8a/libflycast.so') $built) -or
		-not (Test-NamesCommit 'build-win\flycast.exe' ([IO.File]::ReadAllBytes((Join-Path $repo 'build-win\flycast.exe'))) $built)) {
		exit 1
	}
}
Copy-Item $apk $apkOut

# the PC version: VR on, OpenGL (the only renderer VR draws with)
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item (Join-Path $repo 'build-win\flycast.exe') $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "assets\$playCmd") $stage
Set-Content -Path (Join-Path $stage 'emu.cfg') -Encoding ascii -Value @'
[config]
pvr.rend = 0
rend.NativeDepthInterpolation = yes
vr.Xr = yes
'@
Set-Content -Path (Join-Path $stage 'README.txt') -Encoding ascii -Value @'
Confidential Mission VR, PC version

Sega's light-gun game Confidential Mission (arcade, Sega NAOMI, 2000) in VR. The original game
runs in the Flycast emulator; its 3D scenes are rebuilt around you and your controller is the
light gun. Built on mikermak's hotd2-vr (https://github.com/mikermak/hotd2-vr).

1. Make a folder named roms next to flycast.exe and put your own copy of the game in it, the
   MAME set confmiss:
     roms\confmiss.zip            the game, with its key inside
     roms\confmiss\gds-0001.chd   the disc, in a folder named confmiss
     roms\naomi.zip               the NAOMI BIOS (or have the BIOS inside confmiss.zip)
2. Connect your VR headset: start SteamVR, Quest Link / Air Link or Virtual Desktop, so it
   is the active OpenXR runtime.
3. Double-click "Play Confidential Mission.cmd". The game plays in the headset.

The controller is the light gun (Quest Touch): the trigger shoots, A is Start and B reloads.
X is player 2's Start: player 2 joins with a gun in your left hand, and Y reloads that one.
Shooting away from the game's view reloads too. The left menu button opens the settings menu;
a click on the left thumbstick recenters, one on the right thumbstick turns the laser off or
on. The project's readme has the other controllers and the rest. Without a headset the game
plays in the window. Settings are in emu.cfg (vr.Xr = yes is VR).

No game is included: you need your own copy. Unofficial fan project, not affiliated with
Sega.

Credits: Flycast by flyinghead and contributors (https://github.com/flyinghead/flycast,
GPL-2.0-or-later). hotd2-vr by mikermak, which this app is built on (GPL-2.0-or-later).
The gun: "Namco Arcade Gun" by Martoscar
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
# cgltf (core\deps\cgltf, it reads the controllers' models) is MIT: its notice goes with the build
$cgltf = ((Get-Content -Raw (Join-Path $repo 'core\deps\cgltf\LICENSE')) -replace "`r?`n", "`r`n").Trim()
if ($cgltf -notmatch '^Copyright \(c\) 2018-2021 Johannes Kuhlmann\s+Permission is hereby granted') { throw 'cgltf MIT notice not found' }
Set-Content -Path (Join-Path $stage 'LICENSE.txt') -Encoding ascii -NoNewline -Value (@'
Confidential Mission VR, PC version: licence

flycast.exe is built from Flycast (GPL-2.0-or-later), mikermak's hotd2-vr and Confidential
Mission VR's changes to it (GPL-2.0-or-later too), and the gun model "Namco Arcade Gun" by
Martoscar (CC BY 4.0, https://creativecommons.org/licenses/by/4.0/). CC BY 4.0 goes together
with GPL-3.0 but not GPL-2.0, so this build is distributed under the GNU General Public License
version 3, below. Its source code is published with every release, on the same page as this zip.

flycast.exe also contains cgltf by Johannes Kuhlmann (https://github.com/jkuhlmann/cgltf), which
reads the controllers' models, under the MIT licence:

'@ + "`r`n" + $cgltf + "`r`n`r`n" + $gpl3)
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip

# the zip: exactly its five files, and a README.txt about this game alone
$zipFiles = 'flycast.exe', 'emu.cfg', 'README.txt', 'LICENSE.txt', $playCmd
$z = [IO.Compression.ZipFile]::OpenRead($zip)
try { $inZip = @($z.Entries | ForEach-Object { $_.FullName }) } finally { $z.Dispose() }
$extra = @($inZip | Where-Object { $zipFiles -notcontains $_ })
$lacking = @($zipFiles | Where-Object { $inZip -notcontains $_ })
$readmeText = if ($lacking -notcontains 'README.txt') { [Text.Encoding]::ASCII.GetString((Read-ZipEntry $zip 'README.txt')) } else { '' }
$otherNames = @('Light Gun Hub', 'House of the Dead', 'Maze of the Kings' | Where-Object { $readmeText -match [regex]::Escape($_) })
$zipGood = $true
if ($extra.Count -or $lacking.Count) {
	Write-Host ("$(Split-Path -Leaf $zip) must hold exactly $($zipFiles -join ', '); also has: $($extra -join ', '); " +
		"lacks: $($lacking -join ', ')") -ForegroundColor Red
	$zipGood = $false
} elseif ($otherNames.Count) {
	Write-Host "README.txt names $($otherNames -join ', '): it is about $appLabel alone" -ForegroundColor Red
	$zipGood = $false
} else {
	Write-Host "zip check: $(Split-Path -Leaf $zip) holds exactly $($zipFiles -join ', '); README.txt names no other game"
}

if (-not (Test-Archives $apkOut, $zip) -or -not $zipGood) {
	foreach ($f in $apkOut, $zip) { if (Test-Path $f) { (Get-Item $f).Delete() } }
	exit 1
}
Get-ChildItem $dist -File | Select-Object Name, @{ n = 'MB'; e = { [math]::Round($_.Length / 1MB, 1) } } | Format-Table | Out-String | Write-Host
exit 0
