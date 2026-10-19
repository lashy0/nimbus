# Formats project C sources with .clang-format: .\format.ps1 [-Check]
param([switch]$Check)
$ErrorActionPreference = 'Stop'

if (-not $env:IDF_TOOLS_PATH) {
    throw 'IDF_TOOLS_PATH must point to the ESP-IDF tools installation.'
}
$clangFormat = Get-ChildItem (Join-Path $env:IDF_TOOLS_PATH 'tools\esp-clang') -Recurse -Filter 'clang-format.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $clangFormat) {
    throw 'clang-format.exe not found; install esp-clang with the ESP-IDF tools.'
}

$sources = Get-ChildItem -Path (Join-Path $PSScriptRoot 'firmware\main'), (Join-Path $PSScriptRoot 'firmware\components') `
    -Recurse -Include '*.c', '*.h' | Select-Object -ExpandProperty FullName
if ($Check) {
    & $clangFormat --dry-run --Werror $sources
} else {
    & $clangFormat -i $sources
}
exit $LASTEXITCODE
