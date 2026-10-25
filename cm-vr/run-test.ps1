<#
  Runs the Windows Flycast build on HOTD2 with VR test settings, grabs window
  screenshots at fixed times and closes the emulator again.

  .\cm-vr\run-test.ps1 -Name sway -Config 'config:vr.Animate=yes,config:vr.Yaw=12' -Shots 20,30,40

  -Keys takes "seconds=key" pairs, key being a .NET Keys name, e.g. '6=Return','14=Return'
  (Return is Start on Flycast's default keyboard map). Keys are posted to the Flycast
  window only (never global input) and held for 150 ms so the game's per-frame poll sees them.

  The game is looked for in a "game" folder next to this repository (or in
  $env:HOTD2VR_DATA\game); logs go to "run" and screenshots to "shots" there.
#>
param(
	[string]$Name = 'test',
	[string]$Config = '',
	[double[]]$Shots = @(20, 30, 40),
	[string[]]$Keys = @('6=Return'),
	[switch]$NoVr
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$data = if ($env:HOTD2VR_DATA) { $env:HOTD2VR_DATA } else { Split-Path -Parent $repo }
$exe = Join-Path $repo 'build-win\flycast.exe'
$cue = Get-ChildItem (Join-Path $data 'game') -Include *.cue, *.gdi, *.chd -Recurse | Select-Object -First 1
if (-not $cue) { throw "No game found in $(Join-Path $data 'game')" }
$runDir = Join-Path $data 'run'
$shotDir = Join-Path $data "shots\$Name"
New-Item -ItemType Directory -Force $runDir, $shotDir | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Win {
	[StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
	[DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
	[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
	[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
	[DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
	[DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
	[DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
	[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
}
'@
# Without this, a scaled display reports a shrunken client rect and shots get cropped.
[void][Win]::SetProcessDPIAware()
Add-Type -AssemblyName System.Windows.Forms

# OpenGL renderer + native depth interpolation are what the reprojection hooks into.
$base = 'config:pvr.rend=0,config:rend.NativeDepthInterpolation=yes,config:aica.Volume=0,log:LogToFile=yes'
# HOTD2 (PAL, MK-5100250) camera: projection matrix 0x4C65E0, viewport matrix 0x4C6708
if (-not $NoVr) { $base += ',config:vr.Reproject=yes,config:vr.ProjAddr=5006816,config:vr.ViewportAddr=5007112' }
if ($Config) { $base += ",$Config" }

Remove-Item (Join-Path $runDir 'flycast.log') -ErrorAction SilentlyContinue
$p = Start-Process -FilePath $exe -ArgumentList @('-config', $base, "`"$($cue.FullName)`"") -WorkingDirectory $runDir -PassThru
try {
	$t0 = Get-Date
	$events = @()
	foreach ($k in $Keys) { $at, $key = $k -split '=', 2; $events += [pscustomobject]@{ At = [double]$at; Key = $key } }
	foreach ($s in $Shots) { $events += [pscustomobject]@{ At = [double]$s; Key = $null } }
	foreach ($ev in ($events | Sort-Object At)) {
		while (((Get-Date) - $t0).TotalSeconds -lt $ev.At) { Start-Sleep -Milliseconds 100 }
		$p.Refresh()
		if ($p.HasExited) { throw "Flycast exited early (code $($p.ExitCode))" }
		$h = $p.MainWindowHandle
		if ($ev.Key) {
			$vk = [int][System.Windows.Forms.Keys]$ev.Key
			$scan = [long][Win]::MapVirtualKey($vk, 0)
			# SDL reads the scancode from lParam bits 16-23; key-up also sets bits 30 and 31.
			[void][Win]::PostMessage($h, 0x100, [IntPtr]$vk, [IntPtr](1 -bor ($scan -shl 16)))
			Start-Sleep -Milliseconds 150
			[void][Win]::PostMessage($h, 0x101, [IntPtr]$vk, [IntPtr](1 -bor ($scan -shl 16) -bor (3L -shl 30)))
			Write-Output ("key {0} at {1}s" -f $ev.Key, $ev.At)
			continue
		}
		$at = $ev.At
		# a minimized window has no client area to print: show it again, without taking focus
		if ([Win]::IsIconic($h)) { [void][Win]::ShowWindow($h, 4); Start-Sleep -Milliseconds 300 }
		$r = New-Object Win+RECT
		[void][Win]::GetClientRect($h, [ref]$r)
		$bmp = New-Object System.Drawing.Bitmap ([Math]::Max(1, $r.R)), ([Math]::Max(1, $r.B))
		$g = [System.Drawing.Graphics]::FromImage($bmp)
		$hdc = $g.GetHdc()
		[void][Win]::PrintWindow($h, $hdc, 3)   # client area, render full content
		$g.ReleaseHdc($hdc); $g.Dispose()
		$file = Join-Path $shotDir ('t{0:000.00}.png' -f $at)
		$bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
		Write-Output "shot $file"
	}
}
finally {
	if (-not $p.HasExited) { $p.CloseMainWindow() | Out-Null; Start-Sleep 2; if (-not $p.HasExited) { $p.Kill() } }
	Copy-Item (Join-Path $runDir 'flycast.log') (Join-Path $shotDir 'flycast.log') -ErrorAction SilentlyContinue
}
