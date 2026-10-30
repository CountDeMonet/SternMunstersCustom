$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectRoot = Split-Path -Parent $projectRoot
$sdk = Join-Path $PSScriptRoot 'sdk'
$source = Join-Path $projectRoot 'modes\spot_baseline\spot_baseline.c'
$output = Join-Path $PSScriptRoot 'spot_baseline.so'
function Convert-ToWslPath([string]$path) {
    $full = [System.IO.Path]::GetFullPath($path)
    if ($full -notmatch '^([A-Za-z]):\\(.*)$') { throw "Expected an absolute drive path: $full" }
    return '/mnt/' + $Matches[1].ToLowerInvariant() + '/' + $Matches[2].Replace('\','/')
}
& wsl.exe -e bash (Convert-ToWslPath (Join-Path $sdk 'build_mode.sh')) -o (Convert-ToWslPath $output) (Convert-ToWslPath $source) (Convert-ToWslPath (Join-Path $sdk 'mode_file.c'))
if ($LASTEXITCODE -ne 0) { throw 'Stock Spot baseline build failed.' }
Write-Host "Built $output. No card image was modified."
