<#
  Makes VR Light Gun Hub's release key, once, before the first release:

    release.jks                  the key: RSA 4096, alias vrlightgunhub, valid 10000 days
    release-signing.properties   where the Quest build finds it, with its password

  in %USERPROFILE%\.vrlightgunhub, where the build looks for it (build.gradle.kts), or in -Dir;
  then point $env:VRLGH_SIGNING at that folder's release-signing.properties before building.

  Every update of the app must be signed with this same key: Android refuses an update signed
  with another one, and players would have to uninstall first, losing their settings and what the
  app made from their games. So back the folder up somewhere safe, and keep it private (never in
  the repository: anyone with it can sign an "update" of the app).

  .\make-release-key.ps1
  .\make-release-key.ps1 -Dir D:\keys\vrlightgunhub
#>
param([string]$Dir = (Join-Path $env:USERPROFILE '.vrlightgunhub'))
$ErrorActionPreference = 'Stop'

$jks = Join-Path $Dir 'release.jks'
$props = Join-Path $Dir 'release-signing.properties'
foreach ($f in $jks, $props) {
	if (Test-Path -LiteralPath $f) {
		Write-Host "$f is already there: no new key made (one made now could not update the app already out there)." -ForegroundColor Red
		exit 1
	}
}

# keytool: JAVA_HOME's, else a Microsoft JDK 17 (as build-quest.cmd picks up), else the PATH's
$keytool = @(
	if ($env:JAVA_HOME) { Join-Path $env:JAVA_HOME 'bin\keytool.exe' }
	Get-ChildItem (Join-Path $env:ProgramFiles 'Microsoft') -Directory -Filter 'jdk-17*' -ErrorAction SilentlyContinue |
		ForEach-Object { Join-Path $_.FullName 'bin\keytool.exe' }
) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $keytool) { $keytool = (Get-Command keytool.exe -ErrorAction SilentlyContinue).Source }
if (-not $keytool) { Write-Host 'keytool not found: set JAVA_HOME to a JDK (17 or later).' -ForegroundColor Red; exit 1 }

# a random password, 24 letters and digits (rejection sampling keeps each one equally likely)
$chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
$rng = [Security.Cryptography.RandomNumberGenerator]::Create()
$byte = New-Object byte[] 1
$password = ''
while ($password.Length -lt 24) {
	$rng.GetBytes($byte)
	if ($byte[0] -lt 248) { $password += $chars[$byte[0] % 62] }   # 248 = 4 * 62
}

New-Item -ItemType Directory -Force $Dir | Out-Null
# keytool reads the password from this process's environment, not from its command line
$env:VRLGH_KEY_PASSWORD = $password
$ErrorActionPreference = 'Continue'   # keytool reports progress on stderr
& $keytool -genkeypair -keystore $jks -storetype PKCS12 -alias vrlightgunhub -keyalg RSA -keysize 4096 `
	-validity 10000 -dname 'CN=VR Light Gun Hub' -storepass:env VRLGH_KEY_PASSWORD -keypass:env VRLGH_KEY_PASSWORD
$code = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
Remove-Item Env:VRLGH_KEY_PASSWORD
if ($code -ne 0 -or -not (Test-Path -LiteralPath $jks)) { Write-Host "keytool failed ($code)" -ForegroundColor Red; exit 1 }

# PKCS12 has one password for the store and the key
Set-Content -LiteralPath $props -Encoding ascii -Value @"
# VR Light Gun Hub's release key (cm-vr/make-release-key.ps1). Private: never commit or share it.
storeFile=release.jks
storePassword=$password
keyAlias=vrlightgunhub
keyPassword=$password
"@

Write-Host ''
Write-Host "Made $jks"
Write-Host "and  $props"
if ((Resolve-Path -LiteralPath $Dir).Path -ne (Join-Path $env:USERPROFILE '.vrlightgunhub')) {
	Write-Host "Not the default folder: set `$env:VRLGH_SIGNING = '$props' before building."
}
Write-Host ''
Write-Host "BACK UP THE FOLDER $Dir NOW (both files), somewhere safe and private." -ForegroundColor Yellow
Write-Host 'Losing it means no more updates: an app signed with a new key cannot install over the old one.' -ForegroundColor Yellow
