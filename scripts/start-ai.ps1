param([string]$Checkpoint = 'models/gi-tiny/model.pt', [int]$Port = 18765, [int]$Threads = 4)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path -LiteralPath "$root/.venv/Scripts/python.exe")) {
    & "$PSScriptRoot/setup-ai.ps1"
}
Push-Location "$root/python"
try {
    if (-not (Test-Path -LiteralPath $Checkpoint)) { throw 'Missing model weights. Unpack the full release ZIP or run Train_AI.cmd first.' }
    Write-Host 'Pozostaw to okno otwarte podczas korzystania z AI. Ctrl+C zatrzymuje serwer.'
    Write-Host "Mod: endpoint http://127.0.0.1:$Port/v1 ; model gi-tiny. Tylko lokalny komputer."
    & "$root/.venv/Scripts/python.exe" -m gi_model.serve --checkpoint $Checkpoint --port $Port --threads $Threads
    if ($LASTEXITCODE) { throw 'Server stopped with an error. Check the message above (e.g. port already in use).' }
} finally { Pop-Location }
