# Generates the LVGL fonts in firmware/assets/fonts from artwork/fonts.
# Needs Node.js (npx) and SF Pro Rounded in artwork/fonts.
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$fonts = @(
    @{ Name = 'bold_10'; File = 'SF-Pro-Rounded-Bold.otf'; Size = 10; Range = '0x20,0x25,0x2D,0x30-0x39' }
    @{ Name = 'semibold_15'; File = 'SF-Pro-Rounded-Semibold.otf'; Size = 15; Range = '0x20-0x7E,0x401,0x410-0x44F,0x451' }
    @{ Name = 'semibold_30'; File = 'SF-Pro-Rounded-Semibold.otf'; Size = 30; Range = '0x41-0x5A,0x61-0x7A' }
    @{ Name = 'semibold_50'; File = 'SF-Pro-Rounded-Semibold.otf'; Size = 50; Range = '0x25,0x30-0x39' }
    @{ Name = 'semibold_60'; File = 'SF-Pro-Rounded-Semibold.otf'; Size = 60; Range = '0x25,0x2D,0x30-0x39,0xB0' }
)

foreach ($font in $fonts) {
    $source = Join-Path $root "artwork/fonts/$($font.File)"
    $output = Join-Path $root "firmware/assets/fonts/$($font.Name).fnt"
    npx -y lv_font_conv@1.5.3 --bpp 4 --no-compress --format bin `
        --size $font.Size --font $source --range $font.Range -o $output
    if ($LASTEXITCODE -ne 0) { throw "lv_font_conv failed for $($font.Name)" }
    Write-Host "$($font.Name).fnt"
}
