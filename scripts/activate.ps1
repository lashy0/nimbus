# Dot-source from the project root: . .\scripts\activate.ps1
param([string]$IdfConfigPath)
$ErrorActionPreference = 'Stop'

$nimbusLocalConfigPath = Join-Path (Split-Path -Parent $PSScriptRoot) '.cache\esp-idf.json'
$nimbusLocalConfig = $null
if (Test-Path -LiteralPath $nimbusLocalConfigPath -PathType Leaf) {
    $nimbusLocalConfig = Get-Content -LiteralPath $nimbusLocalConfigPath -Raw | ConvertFrom-Json
}

if (-not $IdfConfigPath -and $nimbusLocalConfig.idfConfigPath -and
    (Test-Path -LiteralPath $nimbusLocalConfig.idfConfigPath -PathType Leaf)) {
    $IdfConfigPath = $nimbusLocalConfig.idfConfigPath
}

if (-not $IdfConfigPath) {
    $nimbusConfigCandidates = @(
        if ($env:IDF_TOOLS_PATH) {
            Join-Path $env:IDF_TOOLS_PATH 'eim_idf.json'
            Join-Path $env:IDF_TOOLS_PATH 'tools\eim_idf.json'
        }
    )
    $IdfConfigPath = $nimbusConfigCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
}

if (-not $IdfConfigPath) {
    throw 'EIM configuration not found. Pass -IdfConfigPath <path to eim_idf.json> once or set IDF_TOOLS_PATH.'
}

if (-not (Test-Path -LiteralPath $IdfConfigPath -PathType Leaf)) {
    throw "EIM configuration file not found: $IdfConfigPath"
}

$nimbusIdfConfig = Get-Content -LiteralPath $IdfConfigPath -Raw | ConvertFrom-Json
$nimbusInstallations = @($nimbusIdfConfig.idfInstalled | Where-Object { $_.name -eq 'v6.1' })
if ($nimbusInstallations.Count -ne 1) {
    throw 'Expected exactly one ESP-IDF v6.1 installation in the EIM configuration.'
}

$nimbusInstallation = $nimbusInstallations[0]
$nimbusVersionFile = Join-Path $nimbusInstallation.path 'tools\cmake\version.cmake'
$nimbusVersion = Get-Content -LiteralPath $nimbusVersionFile -Raw
if ($nimbusVersion -notmatch 'set\(IDF_VERSION_MAJOR 6\)' -or
    $nimbusVersion -notmatch 'set\(IDF_VERSION_MINOR 1\)' -or
    $nimbusVersion -notmatch 'set\(IDF_VERSION_PATCH 0\)') {
    throw 'Nimbus requires ESP-IDF v6.1 (6.1.0). Check the EIM installation.'
}

if (-not $nimbusInstallation.activationScript -or
    -not (Test-Path -LiteralPath $nimbusInstallation.activationScript)) {
    throw 'ESP-IDF v6.1 activation script not found. Repair the EIM installation.'
}

$env:PYTHONUTF8 = '1'
. $nimbusInstallation.activationScript
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    throw 'ESP-IDF activation did not make idf.py available.'
}

$IdfConfigPath = (Resolve-Path -LiteralPath $IdfConfigPath).ProviderPath
if ($nimbusLocalConfig.idfConfigPath -ne $IdfConfigPath) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $nimbusLocalConfigPath) -Force | Out-Null
    @{ idfConfigPath = $IdfConfigPath } | ConvertTo-Json | Set-Content -LiteralPath $nimbusLocalConfigPath -Encoding UTF8
}
