param([int]$Steps = 4000, [int]$Threads = 4, [string]$Resume = '', [string]$Output = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path -LiteralPath "$root/.venv/Scripts/python.exe")) { & "$PSScriptRoot/setup-ai.ps1" }
Push-Location "$root/python"
try {
    if (-not $Output) { $Output = 'models/training-' + (Get-Date -Format 'yyyyMMdd-HHmmss') }
    $trainArgs = @('-m', 'gi_model.train', '--data', 'data/domain.jsonl', '--out', $Output, '--steps', "$Steps", '--threads', "$Threads")
    if ($Resume) { $trainArgs += @('--resume', $Resume) }
    Write-Host 'Trening prawdziwych wag. Nowy trening zaczyna od losowych parametrow; --resume kontynuuje nasze wagi.'
    Write-Host 'Ten maly korpus nie wystarcza do dobrej ogolnej rozmowy. Sam dluzszy trening moze tylko przeuczyc model.'
    & "$root/.venv/Scripts/python.exe" @trainArgs
    if ($LASTEXITCODE) { throw 'Training failed. Previous inference weights have not been changed.' }
    if (-not (Test-Path -LiteralPath "$Output/best.pt")) { throw 'No best checkpoint. Resume the interrupted run first.' }
    & "$root/.venv/Scripts/python.exe" export_model.py --checkpoint "$Output/best.pt" --out models/gi-tiny/model.pt
    if ($LASTEXITCODE) { throw 'Export failed.' }
    Write-Host 'Wagi zapisane. Zrestartuj Start_AI.cmd, aby ich uzywac. Raport treningu jest w:' $Output
} finally { Pop-Location }
