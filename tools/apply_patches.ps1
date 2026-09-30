# ===========================================================================
#  LEGIONFORGE :: tools/apply_patches.ps1
#  Накладывает overlay-патчи из /patches/overlay поверх /source и проверяет
#  жёсткую привязку к клиентскому билду 26124.
# ===========================================================================
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = "Stop"
$Source = Join-Path $Root "source"
$Overlay = Join-Path $Root "patches\overlay"

if (Test-Path $Overlay) {
    Write-Host "[patches] Копирую overlay -> source ..." -ForegroundColor Cyan
    Copy-Item -Path (Join-Path $Overlay "*") -Destination $Source -Recurse -Force
}

# Жёсткая привязка к билду 26124
$buildFiles = @(
    "src\server\shared\Realm\RealmList.cpp",
    "src\server\worldserver\Main.cpp"
)
foreach ($f in $buildFiles) {
    $p = Join-Path $Source $f
    if (-not (Test-Path $p)) { continue }
    $txt = [IO.File]::ReadAllText($p)
    $txt = $txt -replace 'GetIntDefault\("Game\.Build\.Version",\s*\d+\)', 'GetIntDefault("Game.Build.Version", LEGIONFORGE_CLIENT_BUILD)'
    [IO.File]::WriteAllText($p, $txt)
}

foreach ($f in @("src\server\worldserver\worldserver.conf.dist","src\server\bnetserver\bnetserver.conf.dist")) {
    $p = Join-Path $Source $f
    if (Test-Path $p) {
        (Get-Content $p -Raw) -replace 'Game\.Build\.Version\s*=\s*\d+', 'Game.Build.Version = 26124' | Set-Content $p -NoNewline
    }
}
Write-Host "[patches] Билд зафиксирован: 26124" -ForegroundColor Green
