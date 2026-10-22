param([switch]$CoreOnly, [switch]$Test)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    # Prefer an existing Visual Studio developer shell. The workspace's
    # portable tools are optional and are never installed globally.
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        $vc = Join-Path $root '.tooling/msvc/VC/Tools/MSVC'
        if (-not (Test-Path -LiteralPath $vc)) { throw 'Open Developer PowerShell for VS 2022/2026, then run this script.' }
        $v = Get-ChildItem -LiteralPath $vc -Directory | Sort-Object Name -Descending | Select-Object -First 1
        $sdk = Join-Path $root '.tooling/msvc/Windows Kits/10'
        $sv = Get-ChildItem -LiteralPath (Join-Path $sdk 'Include') -Directory | Sort-Object Name -Descending | Select-Object -First 1
        $env:PATH = "$($v.FullName)/bin/Hostx64/x64;$sdk/bin/$($sv.Name)/x64;$root/.tooling/ninja;$root/.tooling/cli;$env:PATH"
        $env:INCLUDE = "$($v.FullName)/include;$sdk/Include/$($sv.Name)/ucrt;$sdk/Include/$($sv.Name)/shared;$sdk/Include/$($sv.Name)/um;$sdk/Include/$($sv.Name)/winrt"
        $env:LIB = "$($v.FullName)/lib/x64;$sdk/Lib/$($sv.Name)/ucrt/x64;$sdk/Lib/$($sv.Name)/um/x64"
    }
    if (-not $env:GEODE_SDK -and (Test-Path '.tooling/geode')) { $env:GEODE_SDK = "$root/.tooling/geode" }
    if (-not $env:GEODE_BINDINGS_REPO_PATH -and (Test-Path '.tooling/bindings')) { $env:GEODE_BINDINGS_REPO_PATH = "$root/.tooling/bindings" }
    New-Item -ItemType Directory -Force "$root/.tooling/tmp" | Out-Null
    $env:TEMP = "$root/.tooling/tmp"
    $env:TMP = $env:TEMP
    $folder = if ($CoreOnly) {'build-core'} else {'build'}
    $mod = if ($CoreOnly) {'OFF'} else {'ON'}
    cmake -S . -B $folder -G Ninja -DCMAKE_BUILD_TYPE=Release "-DGI_BUILD_MOD=$mod" -DGEODE_DONT_INSTALL_MODS=ON -DGEODE_DONT_UPDATE_INDEX=OFF
    if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
    cmake --build $folder --config Release --parallel 4
    if ($LASTEXITCODE) { throw 'Compilation failed.' }
    if ($Test) {
        ctest --test-dir $folder -C Release --output-on-failure
        if ($LASTEXITCODE) { throw 'Tests failed.' }
    }
} finally { Pop-Location }
