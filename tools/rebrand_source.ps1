# ===========================================================================
#  LEGIONFORGE :: tools/rebrand_source.ps1
#  Тотальный ребрендинг исходников (white-labeling) под Windows.
#  Защищает имена таблиц БД (trinity_string) и игровой контент (Argus,
#  Sunwell, Nordrassil, Ashamane) - их трогать НЕЛЬЗЯ.
# ===========================================================================
param([Parameter(Mandatory=$true)][string]$Root)

$ErrorActionPreference = "Stop"
$exts = @("*.cpp","*.h","*.hpp","*.c","*.cc","*.inl","*.txt","*.cmake","*.conf",
          "*.dist","*.sql","*.rc","*.in","*.md","*.bat","*.ps1","*.cs","*.manifest","*.json")
$rules = @(
    @('trinity_string', '@@LFPROT_TS@@'),
    @('trinity_ctring',  '@@LFPROT_TCS@@'),
    @('LegionForgeCore', 'LegionForgeCore'), @('TRINITY_CORE', 'LEGIONFORGE_CORE'),
    @('TRINITYSERVER', 'LEGIONFORGESERVER'), @('TRINITY_', 'LEGIONFORGE_'),
    @('TrinityString', 'LegionForgeString'), @('Trinity', 'LegionForge'),
    @('TRINITY', 'LEGIONFORGE'), @('trinitycore', 'legionforgecore'),
    @('trinity', 'legionforge'), @('LegionForgeCore', 'LegionForge'),
    @('LEGIONFORGE', 'LEGIONFORGE'), @('LEGIONFORGE', 'LEGIONFORGE'),
    @('LEGIONFORGE', 'LEGIONFORGE'),
    @('http://www.azerothreborn.org/', 'https://legionforge.gg'),
    @('@@LFPROT_TS@@', 'trinity_string'),
    @('@@LFPROT_TCS@@', 'trinity_ctring')
)
$count = 0
foreach ($ext in $exts) {
    Get-ChildItem -Path $Root -Recurse -Filter $ext -File -ErrorAction SilentlyContinue | ForEach-Object {
        $txt = [IO.File]::ReadAllText($_.FullName)
        $orig = $txt
        foreach ($r in $rules) { $txt = $txt.Replace($r[0], $r[1]) }
        if ($txt -ne $orig) { [IO.File]::WriteAllText($_.FullName, $txt); $script:count++ }
    }
}
Write-Host "[rebrand] Обработано файлов: $count. Бренд: LEGIONFORGE / LegionForgeCore" -ForegroundColor Green
