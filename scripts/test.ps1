param([switch]$Live)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/build.ps1" -CoreOnly -Test
Push-Location "$root/python"
try {
    & "$root/.venv/Scripts/python.exe" -m unittest discover -s tests -v
    if ($LASTEXITCODE) { throw 'Python tests failed.' }
} finally { Pop-Location }
if ($Live) {
    Push-Location $root
    try {
        Write-Host 'Live: publiczne zapytania Pointercrate/Geode/GD/MusicBrainz/Wikipedia. Bez prywatnej historii.'
        & "$root/build-core/gi_probe.exe" live
        if ($LASTEXITCODE) { throw 'Live probe process failed. Also inspect per-provider PASS/FAIL lines.' }
    } finally { Pop-Location }
}
