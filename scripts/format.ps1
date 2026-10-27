# Formats project C sources with .clang-format: .\scripts\format.ps1 [-Check]
param(
    [switch]$Check,
    [string]$IdfConfigPath
)
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'activate.ps1') -IdfConfigPath $IdfConfigPath

$clangFormat = Get-Command clang-format.exe -CommandType Application -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty Source
$clangRoot = Join-Path $env:IDF_TOOLS_PATH 'esp-clang'
if (-not $clangFormat -or -not $clangFormat.StartsWith($clangRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'clang-format.exe not found; install esp-clang with the ESP-IDF tools.'
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$sources = Get-ChildItem -Path (Join-Path $projectRoot 'firmware\main'), (Join-Path $projectRoot 'firmware\components'), (Join-Path $projectRoot 'simulator\main\src'), (Join-Path $projectRoot 'simulator\config') `
    -Recurse -Include '*.c', '*.h', '*.cpp' |
    Where-Object { $_.Name -ne 'mouse_cursor_icon.c' } |
    Select-Object -ExpandProperty FullName
if ($Check) {
    & $clangFormat --dry-run --Werror $sources
} else {
    & $clangFormat -i $sources
}
exit $LASTEXITCODE
