# Run exact production input/ownership, suspension, service and JVS regressions.
param([string]$Output='')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Output){$Output=Join-Path $repo 'build-win/vrtest/player-assignment'}
$out=[IO.Path]::GetFullPath($Output)
# A dedicated test output may be on D: to avoid filling the source drive.
if($out -eq [IO.Path]::GetPathRoot($out)){throw 'Test output cannot be a drive root'}
& python (Join-Path $PSScriptRoot 're/host_production_test.py') $out
exit $LASTEXITCODE
