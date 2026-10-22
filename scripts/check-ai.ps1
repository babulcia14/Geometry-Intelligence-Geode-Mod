param([int]$Port = 18765)
$ErrorActionPreference = 'Stop'
try {
    $health = Invoke-RestMethod -Uri "http://127.0.0.1:$Port/health" -TimeoutSec 5
    $models = Invoke-RestMethod -Uri "http://127.0.0.1:$Port/v1/models" -TimeoutSec 5
    $health | Format-List
    $models.data | Format-Table id, owned_by
    Write-Host 'Serwer odpowiada. To test polaczenia, nie ocena jakosci modelu.'
} catch {
    Write-Host 'Brak polaczenia. Najpierw uruchom Start_AI.cmd i pozostaw jego okno otwarte.'
    exit 1
}
