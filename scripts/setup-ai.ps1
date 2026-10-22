param([string]$Python = 'python')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    Write-Host 'Geometry Intelligence: instalacja darmowego silnika CPU (bez pobierania gotowych wag).'
    New-Item -ItemType Directory -Force "$root/.tooling/tmp" | Out-Null
    $env:TEMP = "$root/.tooling/tmp"
    $env:TMP = $env:TEMP
    if (-not (Test-Path -LiteralPath "$root/.venv/Scripts/python.exe")) {
        & $Python -c "import sys; assert (3,11) <= sys.version_info[:2] <= (3,14), 'Use Python 3.11-3.14 x64'"
        if ($LASTEXITCODE) { throw 'Install Python 3.11-3.14 x64 with Add python.exe to PATH.' }
        & $Python -m venv .venv
        if ($LASTEXITCODE) { throw 'Cannot create .venv.' }
    }
    & "$root/.venv/Scripts/python.exe" -m pip install -r python/requirements.txt --index-url https://download.pytorch.org/whl/cpu
    if ($LASTEXITCODE) { throw 'Dependency installation failed. Check internet access; do not disable TLS verification.' }
    & "$root/.venv/Scripts/python.exe" -c "import torch; print('PyTorch:', torch.__version__, '| CPU ready')"
    if ($LASTEXITCODE) { throw 'PyTorch import failed.' }
    Write-Host 'Gotowe. Uruchom Start_AI.cmd. Nie potrzeba klucza API.'
} finally { Pop-Location }
