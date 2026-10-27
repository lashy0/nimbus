# Builds and runs the reference-style SDL/LVGL simulator with MinGW.
param(
    [switch]$BuildOnly,
    [switch]$RunOnly,
    [switch]$FreeRTOS,
    [ValidateRange(1, 64)]
    [int]$Jobs = 2,
    [string]$CompilerBin
)
$ErrorActionPreference = 'Stop'
if ($BuildOnly -and $RunOnly) { throw 'Use either -BuildOnly or -RunOnly, not both.' }
$projectRoot = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $projectRoot '.cache'
$build = Join-Path $cache $(if ($FreeRTOS) { 'preview-freertos' } else { 'preview' })
$executable = Join-Path $build 'bin/nimbus-sim.exe'
if ($RunOnly -and -not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw 'Simulator executable not found. Run scripts/preview.ps1 without -RunOnly first (with -FreeRTOS if needed).'
}
$originalPath = $env:PATH
$originalParallelLevel = $env:CMAKE_BUILD_PARALLEL_LEVEL

try {
    if (-not $CompilerBin) {
        $compiler = Get-Command gcc -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        $CompilerBin = @(
            if ($compiler) { Split-Path -Parent $compiler.Source }
            'C:\msys64\ucrt64\bin'
            'C:\msys64\mingw64\bin'
        ) | Where-Object {
            (Test-Path -LiteralPath (Join-Path $_ 'gcc.exe')) -and
            (Test-Path -LiteralPath (Join-Path $_ 'g++.exe')) -and
            (Test-Path -LiteralPath (Join-Path $_ 'mingw32-make.exe'))
        } | Select-Object -First 1
    }
    if (-not $CompilerBin) { throw 'Install MinGW gcc, g++ and mingw32-make, or pass -CompilerBin.' }
    $CompilerBin = (Resolve-Path -LiteralPath $CompilerBin).ProviderPath
    $env:PATH = "$CompilerBin;$env:PATH"
    $env:CMAKE_BUILD_PARALLEL_LEVEL = "$Jobs"

    if (-not $RunOnly) {
        $cmake = Get-Command cmake -CommandType Application | Select-Object -First 1 -ExpandProperty Source
        & $cmake -S (Join-Path $projectRoot 'simulator') -B $build -G 'MinGW Makefiles' `
            "-DCMAKE_C_COMPILER=$CompilerBin/gcc.exe" "-DCMAKE_CXX_COMPILER=$CompilerBin/g++.exe" `
            "-DCMAKE_MAKE_PROGRAM=$CompilerBin/mingw32-make.exe" `
            '-DCMAKE_BUILD_TYPE=Release' "-DNIMBUS_BUILD_JOBS=$Jobs" "-DUSE_FREERTOS=$(if ($FreeRTOS) { 'ON' } else { 'OFF' })"
        if ($LASTEXITCODE -ne 0) { throw 'Simulator configuration failed. Check the compiler, initialize submodules and allow SDL downloads.' }
        & $cmake --build $build --parallel $Jobs
        if ($LASTEXITCODE -ne 0) { throw 'Simulator build failed.' }
    }
    if (-not $BuildOnly) {
        & $executable
        if ($LASTEXITCODE -ne 0) { throw 'Simulator exited with an error.' }
    }
} finally {
    $env:PATH = $originalPath
    $env:CMAKE_BUILD_PARALLEL_LEVEL = $originalParallelLevel
}
