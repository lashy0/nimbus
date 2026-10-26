# Dot-source from the project root: . .\scripts\activate.ps1
$ErrorActionPreference = 'Stop'

if (-not $env:IDF_TOOLS_PATH) {
    throw 'IDF_TOOLS_PATH must point to the ESP-IDF tools installation.'
}

$nimbusInitScript = Join-Path $env:IDF_TOOLS_PATH 'Initialize-Idf.ps1'
$nimbusIdfConfigPath = Join-Path $env:IDF_TOOLS_PATH 'esp_idf.json'
foreach ($nimbusRequiredPath in @($nimbusInitScript, $nimbusIdfConfigPath)) {
    if (-not (Test-Path -LiteralPath $nimbusRequiredPath)) {
        throw "ESP-IDF installation file not found: $nimbusRequiredPath"
    }
}

$nimbusIdfConfig = Get-Content -LiteralPath $nimbusIdfConfigPath -Raw | ConvertFrom-Json
if (-not $nimbusIdfConfig.idfSelectedId) {
    throw 'Select ESP-IDF 5.5.3 in the Espressif installer before activation.'
}

. $nimbusInitScript -IdfId $nimbusIdfConfig.idfSelectedId
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    throw 'ESP-IDF activation did not make idf.py available.'
}
