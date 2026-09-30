[CmdletBinding()]
param(
    [ValidateSet('Prepare', 'Build', 'Run', 'Panel', 'Status', 'Stop')]
    [string]$Mode = 'Prepare',
    # Stop mode only: skip the graceful shutdown attempts and kill at once.
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
try { [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls13 } catch { }

$Root = Split-Path -Parent $PSScriptRoot
$ManifestPath = Join-Path $Root 'project.json'
if (-not (Test-Path $ManifestPath)) { throw "project.json is missing from $Root" }
$Manifest = Get-Content -Raw -Path $ManifestPath | ConvertFrom-Json

function RootPath([string]$Relative) { return Join-Path $Root $Relative }
function Step([string]$Message) { Write-Host "[LegionForge] $Message" -ForegroundColor Cyan }
function EnsureDirectory([string]$Path) { if (-not (Test-Path $Path)) { New-Item -ItemType Directory -Path $Path -Force | Out-Null } }
function Fail([string]$Message) { throw "[LegionForge] $Message" }
function Notice([string]$Message) { Write-Host "[LegionForge] $Message" -ForegroundColor Yellow }

# Full path of this script; Stop mode uses it to recognise other running
# bootstrap.ps1 instances (Prepare/Build/Run/Panel) started by the kit.
$BootstrapScript = $PSCommandPath

$Tools = RootPath $Manifest.directories.tools
$LegacyToolchain = RootPath 'toolchain'
$Downloads = RootPath $Manifest.directories.downloads
$ServerSource = RootPath $Manifest.directories.serverSource
$ServerBuild = RootPath $Manifest.directories.serverBuild
$ServerRuntime = RootPath $Manifest.directories.serverRuntime
$DatabaseHome = RootPath $Manifest.directories.database
$Logs = RootPath $Manifest.directories.logs
$Backups = RootPath $Manifest.directories.backups
$Patches = RootPath $Manifest.directories.patches
$Custom = RootPath $Manifest.directories.custom
$GitRoot = Join-Path $Tools 'git'
$CMakeRoot = Join-Path $Tools 'cmake'
# tools/download_tools.ps1 ставит портативный Node.js в tools\nodejs, а этот
# скрипт исторически ждал tools\node. Из-за расхождения PANEL/START не находили
# node.exe, даже когда среда уже была скачана. Принимаем оба варианта.
function ResolveNodeRoot {
    foreach ($Candidate in @('node', 'nodejs')) {
        $Path = Join-Path $Tools $Candidate
        if (Test-Path (Join-Path $Path 'node.exe')) { return $Path }
    }
    return (Join-Path $Tools 'node')
}

# Аналогично для БД: download_tools.ps1 ставит MySQL 8 в tools\mysql, а
# bootstrap ожидает MariaDB в tools\mariadb. Разрешаем оба расположения.
function ResolveDatabaseRoot {
    foreach ($Candidate in @('mariadb', 'mysql')) {
        $Path = Join-Path $Tools $Candidate
        if ((Test-Path (Join-Path $Path 'bin\mariadbd.exe')) -or (Test-Path (Join-Path $Path 'bin\mysqld.exe'))) { return $Path }
    }
    return (Join-Path $Tools 'mariadb')
}

# Login-сервер ядра 7.3.5 называется bnetserver; имя authserver оставлено как
# совместимость со старыми сборками.
function ResolveLoginServerBaseName {
    foreach ($Name in @('bnetserver', 'authserver')) {
        if (Test-Path (Join-Path $ServerRuntime "$Name.exe")) { return $Name }
    }
    if (Test-Path (Join-Path $ServerSource 'src\server\bnetserver\Main.cpp')) { return 'bnetserver' }
    return 'authserver'
}

$NodeRoot = ResolveNodeRoot
$BoostRoot = Join-Path $Tools 'boost\boost_1_86_0'
$OpenSSLRoot = Join-Path $Tools 'openssl\x64'
$InnoExtractRoot = Join-Path $Tools 'innoextract'
$MariaRoot = ResolveDatabaseRoot

# Имена исполняемых файлов отличаются: MariaDB -> mariadbd.exe/mariadb.exe,
# MySQL 8 -> mysqld.exe/mysql.exe.
function ResolveDatabaseServerExe {
    foreach ($Name in @('mariadbd.exe', 'mysqld.exe')) {
        $Path = Join-Path $MariaRoot "bin\$Name"
        if (Test-Path $Path) { return $Path }
    }
    return (Join-Path $MariaRoot 'bin\mariadbd.exe')
}
function ResolveDatabaseClientExe {
    foreach ($Name in @('mariadb.exe', 'mysql.exe')) {
        $Path = Join-Path $MariaRoot "bin\$Name"
        if (Test-Path $Path) { return $Path }
    }
    return (Join-Path $MariaRoot 'bin\mariadb.exe')
}
function GetDatabaseProcessNames { return @('mariadbd', 'mysqld') }

function MigrateLegacyToolchain {
    if (-not (Test-Path $LegacyToolchain)) { return }
    EnsureDirectory $Tools
    foreach ($Name in @('git', 'cmake', 'node', 'vs-buildtools', 'mariadb', 'boost', 'openssl')) {
        $OldPath = Join-Path $LegacyToolchain $Name
        $NewPath = Join-Path $Tools $Name
        if ((Test-Path $OldPath) -and (-not (Test-Path $NewPath))) {
            Step "Moving existing toolchain/$Name to tools/$Name"
            Move-Item -Path $OldPath -Destination $Tools
        }
    }
    if (-not (Get-ChildItem -Path $LegacyToolchain -Force -ErrorAction SilentlyContinue)) {
        Remove-Item -Path $LegacyToolchain -Force -ErrorAction SilentlyContinue
    }
}

$Paths = @($Tools, $Downloads, $ServerBuild, $ServerRuntime, $DatabaseHome, $Logs, $Backups, $Patches, $Custom)
foreach ($Path in $Paths) { EnsureDirectory $Path }
MigrateLegacyToolchain

function Download([string]$Url, [string]$Destination) {
    if ((Test-Path $Destination) -and ((Get-Item $Destination).Length -gt 0)) { return }
    EnsureDirectory (Split-Path -Parent $Destination)
    Step "Downloading $(Split-Path -Leaf $Destination)"
    $Partial = "$Destination.part"
    Remove-Item $Partial -Force -ErrorAction SilentlyContinue
    Invoke-WebRequest -Uri $Url -OutFile $Partial -UseBasicParsing
    if (-not (Test-Path $Partial) -or (Get-Item $Partial).Length -eq 0) { Fail "Download was empty: $Url" }
    Move-Item -Force $Partial $Destination
}

function GetCurlExecutable {
    $Candidates = @(
        (Join-Path $env:SystemRoot 'System32\curl.exe'),
        (Join-Path $GitRoot 'mingw64\bin\curl.exe'),
        (Join-Path $GitRoot 'usr\bin\curl.exe')
    )
    try {
        $Resolved = (Get-Command 'curl.exe' -ErrorAction SilentlyContinue | Select-Object -First 1).Source
        if ($Resolved) { $Candidates += $Resolved }
    } catch { }
    foreach ($Candidate in $Candidates) {
        if ($Candidate -and (Test-Path $Candidate)) { return $Candidate }
    }
    return $null
}

function TestWindowsExecutable([string]$Path, [int64]$MinimumBytes) {
    if (-not (Test-Path $Path)) { return $false }
    $Info = Get-Item $Path
    if ($Info.Length -lt $MinimumBytes) { return $false }
    $Stream = $null
    try {
        $Stream = [IO.File]::OpenRead($Path)
        return ($Stream.ReadByte() -eq 0x4D -and $Stream.ReadByte() -eq 0x5A)
    } catch {
        return $false
    } finally {
        if ($Stream) { $Stream.Dispose() }
    }
}

function DownloadLargeWindowsExecutable([string[]]$Urls, [string]$Destination, [int64]$MinimumBytes) {
    if (TestWindowsExecutable $Destination $MinimumBytes) { return }
    if (Test-Path $Destination) {
        $BadBytes = (Get-Item $Destination).Length
        Write-Host "[LegionForge] Removing incomplete cached file ($BadBytes bytes): $Destination" -ForegroundColor Yellow
        Remove-Item $Destination -Force -ErrorAction SilentlyContinue
    }

    $CurlExe = GetCurlExecutable
    if (-not $CurlExe) { Fail 'curl.exe is unavailable. Windows 10/11 or Portable Git should provide it.' }
    EnsureDirectory (Split-Path -Parent $Destination)
    $Partial = "$Destination.part"

    foreach ($Url in $Urls) {
        Remove-Item $Partial -Force -ErrorAction SilentlyContinue
        Step "Downloading $(Split-Path -Leaf $Destination) with curl from $(([uri]$Url).Host)"
        $PreviousErrorPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            & $CurlExe '--location' '--fail' '--show-error' '--retry' '5' '--retry-delay' '3' '--connect-timeout' '30' '--output' $Partial $Url
            $CurlExit = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $PreviousErrorPreference
        }

        if ($CurlExit -eq 0 -and (TestWindowsExecutable $Partial $MinimumBytes)) {
            Move-Item -Force $Partial $Destination
            $SizeMB = [math]::Round((Get-Item $Destination).Length / 1MB)
            Step "Download verified: $SizeMB MB, valid Windows executable"
            return
        }

        $Received = 0
        if (Test-Path $Partial) { $Received = (Get-Item $Partial).Length }
        Write-Host "[LegionForge] Mirror returned an invalid file ($Received bytes), trying the next mirror" -ForegroundColor Yellow
    }

    Remove-Item $Partial -Force -ErrorAction SilentlyContinue
    Fail "Unable to download a valid $(Split-Path -Leaf $Destination) from any configured mirror."
}

function GetGitHubAsset([string]$Repository, [string]$NamePattern) {
    $Headers = @{ 'User-Agent' = 'LegionForge-Developer-Kit' }
    $Release = Invoke-RestMethod -Headers $Headers -Uri "https://api.github.com/repos/$Repository/releases/latest"
    $Asset = @($Release.assets | Where-Object { $_.name -match $NamePattern } | Select-Object -First 1)
    if ($Asset.Count -ne 1) { Fail "No release asset matched '$NamePattern' in $Repository" }
    return $Asset[0]
}

function ResolveGit {
    # The server source is supplied by the user in server/source, so git is
    # no longer required and is never downloaded. It is only used when it is
    # already available: CMake then embeds the revision hash into the build
    # and a damaged dep/gsoap can be restored from a local git history.
    $GitExe = Join-Path $GitRoot 'cmd\git.exe'
    if (Test-Path $GitExe) {
        $env:Path = "$(Join-Path $GitRoot 'cmd');$env:Path"
        return $GitExe
    }
    $SystemGit = Get-Command git -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $SystemGit) { return $SystemGit.Source }
    return $null
}

function EnsurePortableCMake {
    $CMakeExe = Join-Path $CMakeRoot 'bin\cmake.exe'
    if (-not (Test-Path $CMakeExe)) {
        $Asset = GetGitHubAsset 'Kitware/CMake' '^cmake-.*-windows-x86_64\.zip$'
        $Archive = Join-Path $Downloads $Asset.name
        Download $Asset.browser_download_url $Archive
        $Extract = Join-Path $Tools '.extract-cmake'
        Remove-Item $Extract -Recurse -Force -ErrorAction SilentlyContinue
        Expand-Archive -Path $Archive -DestinationPath $Extract -Force
        $Content = Get-ChildItem -Path $Extract -Directory | Select-Object -First 1
        if ($null -eq $Content) { Fail 'CMake archive has an unexpected layout' }
        EnsureDirectory $CMakeRoot
        Get-ChildItem -Path $Content.FullName | Move-Item -Destination $CMakeRoot -Force
        Remove-Item $Extract -Recurse -Force
    }
    if (-not (Test-Path $CMakeExe)) { Fail 'Portable CMake was not created correctly' }
    $env:Path = "$(Join-Path $CMakeRoot 'bin');$env:Path"
    return $CMakeExe
}

function EnsurePortableNode {
    $NodeExe = Join-Path $NodeRoot 'node.exe'
    if (-not (Test-Path $NodeExe)) {
        $Index = Invoke-RestMethod -Uri 'https://nodejs.org/download/release/index.json'
        $Release = @($Index | Where-Object { $_.version -match "^v$($Manifest.toolchain.nodeMajor)\." -and $_.lts -and ($_.files -contains 'win-x64-zip') } | Select-Object -First 1)
        if ($Release.Count -ne 1) { Fail "No supported Node.js $($Manifest.toolchain.nodeMajor) LTS ZIP was found" }
        $ArchiveName = "node-$($Release[0].version)-win-x64.zip"
        $Archive = Join-Path $Downloads $ArchiveName
        Download "https://nodejs.org/download/release/$($Release[0].version)/$ArchiveName" $Archive
        $Extract = Join-Path $Tools '.extract-node'
        Remove-Item $Extract -Recurse -Force -ErrorAction SilentlyContinue
        Expand-Archive -Path $Archive -DestinationPath $Extract -Force
        $Content = Get-ChildItem -Path $Extract -Directory | Select-Object -First 1
        if ($null -eq $Content) { Fail 'Node archive has an unexpected layout' }
        EnsureDirectory $NodeRoot
        Get-ChildItem -Path $Content.FullName | Move-Item -Destination $NodeRoot -Force
        Remove-Item $Extract -Recurse -Force
    }
    if (-not (Test-Path $NodeExe)) { Fail 'Portable Node.js was not created correctly' }
    $env:Path = "$NodeRoot;$env:Path"
    return $NodeExe
}

function DetectVisualStudio {
    Step 'Checking local Visual Studio installation'
    $VSWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $VSPath = $null
    if (Test-Path $VSWhere) {
        $Installations = & $VSWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($Installations) {
            $VSPath = ($Installations | Select-Object -First 1).ToString().Trim()
        }
    }
    if (-not $VSPath) {
        $StandardPaths = @(
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools",
            "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community",
            "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional",
            "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise",
            "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools"
        )
        foreach ($P in $StandardPaths) {
            if (Test-Path (Join-Path $P 'VC\Auxiliary\Build\vcvars64.bat')) {
                $VSPath = $P
                break
            }
        }
    }
    if ($VSPath) {
        Step "Detected local Visual Studio: $VSPath"
    } else {
        Step 'Using system default MSVC/Visual Studio 2022 toolset'
    }
    return $VSPath
}

function EnsureCustomMods([string]$GitExe) {
    $ModsDir = RootPath 'custom/mods'
    EnsureDirectory $ModsDir
    if ($null -ne $Manifest.customMods -and @($Manifest.customMods).Count -gt 0 -and -not $GitExe) {
        Notice 'project.json lists customMods, but git is not available - skipping them. Install Git or put tools/git in place to use customMods.'
        return
    }
    if ($null -ne $Manifest.customMods) {
        Step "Downloading and updating curated custom mods ($($Manifest.customMods.Count) items)"
        foreach ($Mod in $Manifest.customMods) {
            $Target = Join-Path $ModsDir $Mod.name
            if (-not (Test-Path (Join-Path $Target '.git'))) {
                Step "Cloning custom mod: $($Mod.name)"
                & $GitExe clone --depth 1 -b $Mod.branch $Mod.repo $Target
                if ($LASTEXITCODE -ne 0) {
                    Write-Host "[LegionForge] Notice: could not clone $($Mod.name). Continuing with other mods." -ForegroundColor Yellow
                }
            } else {
                Step "Checking updates for custom mod: $($Mod.name)"
                & $GitExe -C $Target fetch --all --prune
            }
        }
    }
}

function TestBoostInstalled {
    return ((Test-Path (Join-Path $BoostRoot 'boost\version.hpp')) -and
            (Test-Path (Join-Path $BoostRoot 'lib64-msvc-14.3')))
}

function ExpandBoostWithInnoExtract([string]$Installer) {
    $Deps = $Manifest.toolchain.dependencies
    $Zip = Join-Path $Downloads 'innoextract-1.9-windows.zip'
    Download $Deps.innoextract.url $Zip
    $Extract = Join-Path $Tools '.extract-innoextract'
    Remove-Item $Extract -Recurse -Force -ErrorAction SilentlyContinue
    Expand-Archive -Path $Zip -DestinationPath $Extract -Force
    $Exe = Get-ChildItem -Path $Extract -Recurse -Filter 'innoextract.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $Exe) { Fail 'innoextract.exe was not found in the portable archive' }
    EnsureDirectory $InnoExtractRoot
    Copy-Item -Path $Exe.FullName -Destination (Join-Path $InnoExtractRoot 'innoextract.exe') -Force
    Remove-Item $Extract -Recurse -Force -ErrorAction SilentlyContinue

    Step 'Extracting Boost with portable innoextract (no administrator rights needed)'
    $InnoLog = Join-Path $Logs 'boost-innoextract.log'
    Remove-Item $InnoLog -Force -ErrorAction SilentlyContinue
    $PreviousErrorPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & (Join-Path $InnoExtractRoot 'innoextract.exe') --output-dir "$BoostRoot" "$Installer" *>&1 | Tee-Object -FilePath $InnoLog
    } finally {
        $ErrorActionPreference = $PreviousErrorPreference
    }
    # innoextract maps {app} to an "app" sub-directory; normalise the layout.
    $AppDir = Join-Path $BoostRoot 'app'
    if ((Test-Path (Join-Path $AppDir 'boost\version.hpp')) -and (-not (Test-Path (Join-Path $BoostRoot 'boost\version.hpp')))) {
        Get-ChildItem -Path $AppDir | Move-Item -Destination $BoostRoot -Force
        Remove-Item $AppDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function EnsurePortableBoost {
    if (TestBoostInstalled) { return $BoostRoot }
    $Deps = $Manifest.toolchain.dependencies
    $Installer = Join-Path $Downloads $Deps.boost.installer
    $SourceForgePath = "project/boost/boost-binaries/$($Deps.boost.version)/$($Deps.boost.installer)"
    $BoostUrls = @(
        $Deps.boost.url,
        "https://master.dl.sourceforge.net/$SourceForgePath",
        "https://pilotfiber.dl.sourceforge.net/$SourceForgePath",
        "https://psychz.dl.sourceforge.net/$SourceForgePath"
    )
    DownloadLargeWindowsExecutable $BoostUrls $Installer (150MB)

    Step "Installing prebuilt Boost $($Deps.boost.version) ($($Deps.boost.variant)) into tools/boost"
    EnsureDirectory $BoostRoot
    $BoostLog = Join-Path $Logs 'boost-install.log'
    Remove-Item $BoostLog -Force -ErrorAction SilentlyContinue
    $Proc = Start-Process -FilePath $Installer -ArgumentList @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART',
        ('/DIR="{0}"' -f $BoostRoot),
        ('/LOG="{0}"' -f $BoostLog)
    ) -Wait -PassThru

    if (($Proc.ExitCode -ne 0) -or (-not (TestBoostInstalled))) {
        Write-Host '[LegionForge] Silent Boost install did not complete, falling back to portable extraction' -ForegroundColor Yellow
        ExpandBoostWithInnoExtract $Installer
    }
    if (-not (TestBoostInstalled)) { Fail "Boost was not installed correctly. Review $BoostLog" }
    Step "Boost ready: $BoostRoot"
    return $BoostRoot
}

function EnsurePortableOpenSSL {
    $Marker = Join-Path $OpenSSLRoot 'include\openssl\ssl.h'
    if (Test-Path $Marker) { return $OpenSSLRoot }
    $Deps = $Manifest.toolchain.dependencies
    $Zip = Join-Path $Downloads ("openssl-{0}.zip" -f $Deps.openssl.version)
    Download $Deps.openssl.url $Zip
    Step "Extracting portable OpenSSL $($Deps.openssl.version)"
    $Extract = Join-Path $Tools '.extract-openssl'
    Remove-Item $Extract -Recurse -Force -ErrorAction SilentlyContinue
    Expand-Archive -Path $Zip -DestinationPath $Extract -Force
    $X64 = Join-Path $Extract 'x64'
    if (-not (Test-Path (Join-Path $X64 'include\openssl\ssl.h'))) { Fail 'OpenSSL archive has an unexpected layout' }
    EnsureDirectory (Split-Path -Parent $OpenSSLRoot)
    if (Test-Path $OpenSSLRoot) { Remove-Item $OpenSSLRoot -Recurse -Force }
    Move-Item -Path $X64 -Destination $OpenSSLRoot
    Remove-Item $Extract -Recurse -Force -ErrorAction SilentlyContinue
    return $OpenSSLRoot
}


function GetLauncherFileHash([string]$Path) {
    # Line endings are normalized (CRLF -> LF) before hashing, so the check
    # accepts both a ZIP download (LF) and a Git for Windows checkout with
    # core.autocrlf=true (CRLF). Latin-1 maps every byte 1:1 to a character,
    # so the content is otherwise hashed byte for byte.
    $Latin1 = [System.Text.Encoding]::GetEncoding(28591)
    $Text = $Latin1.GetString([System.IO.File]::ReadAllBytes($Path)).Replace("`r`n", "`n")
    $Sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString($Sha.ComputeHash($Latin1.GetBytes($Text))) -replace '-', '').ToLower()
    } finally {
        $Sha.Dispose()
    }
}

function VerifyLauncherIntegrity {
    $ManifestFile = Join-Path $PSScriptRoot 'launcher-manifest.sha256'
    if (-not (Test-Path $ManifestFile)) {
        Write-Host '[LegionForge] Notice: launcher manifest is missing, version check skipped.' -ForegroundColor Yellow
        return
    }

    $Mismatched = @()
    foreach ($Line in (Get-Content $ManifestFile)) {
        if ($Line -notmatch '^[0-9a-fA-F]{64}\s+\S+') { continue }
        $Parts = $Line -split '\s+', 2
        $Expected = $Parts[0].ToLower()
        $File = Join-Path $Root $Parts[1]
        if (-not (Test-Path $File)) {
            $Mismatched += $Parts[1] + ' (missing)'
            continue
        }
        $Actual = GetLauncherFileHash $File
        if ($Actual -ne $Expected) { $Mismatched += $Parts[1] }
    }

    if ($Mismatched.Count -gt 0) {
        Fail ("Launcher files do not match this project version: " + ($Mismatched -join ', ') +
              ". Copy the latest project.json, START.bat, START_PANEL.bat, arguscore.bin.bat, Stop.bat and the whole tools/ folder from the current project, then run START.bat again.")
    }
    Step 'Launcher integrity check passed'
}

function ShowLogTail([string]$LogPath, [int]$LineCount = 60) {
    if (-not (Test-Path $LogPath)) { return }
    Write-Host '----- BEGIN log excerpt (copy these lines when reporting the error) -----' -ForegroundColor Yellow
    Get-Content -Path $LogPath -Tail $LineCount | ForEach-Object { Write-Host $_ }
    Write-Host '----- END log excerpt -----' -ForegroundColor Yellow
}

function TryDownload([string]$Url, [string]$Destination) {
    if ((Test-Path $Destination) -and ((Get-Item $Destination).Length -gt 0)) { return $true }
    EnsureDirectory (Split-Path -Parent $Destination)
    $Partial = "$Destination.part"
    Remove-Item $Partial -Force -ErrorAction SilentlyContinue
    try {
        Invoke-WebRequest -Uri $Url -OutFile $Partial -UseBasicParsing
    } catch {
        Remove-Item $Partial -Force -ErrorAction SilentlyContinue
        return $false
    }
    if (-not (Test-Path $Partial) -or (Get-Item $Partial).Length -eq 0) {
        Remove-Item $Partial -Force -ErrorAction SilentlyContinue
        return $false
    }
    Move-Item -Force $Partial $Destination
    return $true
}

function EnsureMariaDb {
    # Принимаем и уже установленный MySQL 8 из tools\mysql (его ставит
    # download_tools.ps1), чтобы не качать MariaDB повторно.
    $Server = ResolveDatabaseServerExe
    if (-not (Test-Path $Server)) {
        $Version = $Manifest.toolchain.mariadbVersion
        $MajorMinor = ($Version.Split('.')[0..1] -join '.')
        $ArchiveName = "mariadb-$Version-winx64.zip"
        $Archive = Join-Path $Downloads $ArchiveName
        # MariaDB changed its archive layout over time. The full-version path
        # (mariadb-11.4.10/...) is correct for current releases; the older
        # major.minor path (mariadb-11.4/...) is kept as a fallback.
        $Candidates = @(
            "https://archive.mariadb.org/mariadb-$Version/winx64-packages/$ArchiveName",
            "https://downloads.mariadb.org/interstitial/mariadb-$Version/winx64-packages/$ArchiveName",
            "https://archive.mariadb.org/mariadb-$MajorMinor/winx64-packages/$ArchiveName"
        )
        $Ok = $false
        foreach ($Url in $Candidates) {
            Step "Downloading $ArchiveName"
            if (TryDownload $Url $Archive) { $Ok = $true; break }
            Write-Host "[LegionForge] Not available at $Url" -ForegroundColor Yellow
        }
        if (-not $Ok) { Fail "Unable to download MariaDB $Version. Checked archive.mariadb.org and downloads.mariadb.org." }
        $Extract = Join-Path $Tools '.extract-mariadb'
        Remove-Item $Extract -Recurse -Force -ErrorAction SilentlyContinue
        Expand-Archive -Path $Archive -DestinationPath $Extract -Force
        $Content = Get-ChildItem -Path $Extract -Directory | Select-Object -First 1
        if ($null -eq $Content) { Fail 'MariaDB archive has an unexpected layout' }
        EnsureDirectory $MariaRoot
        Get-ChildItem -Path $Content.FullName | Move-Item -Destination $MariaRoot -Force
        Remove-Item $Extract -Recurse -Force
    }
    if (-not (Test-Path $Server)) { Fail 'Portable MariaDB was not created correctly' }
    return $Server
}

function EnsureSource {
    # The server source is NOT downloaded by the kit. The user copies their
    # own core into server/source; the kit only verifies the layout. Nothing
    # here touches the network or modifies the source tree.
    $Display = $ServerSource
    if (-not (Test-Path $ServerSource)) {
        EnsureDirectory $ServerSource
        Fail ("server\source is empty. Copy your server source code into:`n    $Display`n" +
              "so that CMakeLists.txt sits directly in that folder ($Display\CMakeLists.txt), then run START.bat again.")
    }
    if (-not (Test-Path (Join-Path $ServerSource 'CMakeLists.txt'))) {
        # A very common mistake: the archive was extracted into a subfolder,
        # e.g. server\source\LegionCore-7.3.5V2\CMakeLists.txt.
        $Nested = @(Get-ChildItem -Path $ServerSource -Directory -ErrorAction SilentlyContinue |
            Where-Object { Test-Path (Join-Path $_.FullName 'CMakeLists.txt') })
        if ($Nested.Count -ge 1) {
            Fail ("Your source is in a subfolder: $($Nested[0].FullName)`n" +
                  "Move the CONTENTS of that folder up one level, so that the file is at $Display\CMakeLists.txt, then run START.bat again.")
        }
        Fail ("server\source has no CMakeLists.txt. Copy your server source code into:`n    $Display`n" +
              "so that CMakeLists.txt sits directly in that folder, then run START.bat again.")
    }
    foreach ($Required in @('src\server', 'cmake')) {
        if (-not (Test-Path (Join-Path $ServerSource $Required))) {
            Fail "server\source does not look like a TrinityCore/LegionCore tree: the '$Required' folder is missing in $Display"
        }
    }
    Step "Using your server source from $Display"
}

function EnsureVendoredGsoapIntegrity([string]$GitExe) {
    # If the core vendors gSOAP in dep/gsoap and those files get truncated or
    # replaced with a stub (an interrupted copy, or antivirus quarantining the
    # raw-socket gSOAP sources - both are known to happen), bnetserver fails
    # with dozens of "not a member of soap" / "SOAP_C_UTFSTRING undeclared"
    # errors in LoginRESTService.cpp. Catch that before a long build starts.
    $GsoapDir = Join-Path $ServerSource 'dep\gsoap'
    if (-not (Test-Path $GsoapDir)) { return }   # this core does not vendor gSOAP

    # Members/macros present in every complete gSOAP 2.8 stdsoap2.h that
    # LoginRESTService.cpp relies on. A stub/truncated header lacks them.
    $RequiredMarkers = @('SOAP_C_UTFSTRING', 'fresponse', 'fposthdr', 'ssl_flags', 'SSL_CTX *ctx')

    function GetGsoapProblem {
        $Header = Join-Path $GsoapDir 'stdsoap2.h'
        $Source = Join-Path $GsoapDir 'stdsoap2.cpp'
        if (-not (Test-Path $Header)) { return 'stdsoap2.h is missing' }
        if (-not (Test-Path $Source)) { return 'stdsoap2.cpp is missing' }
        # Genuine files are ~150 KB (header) and ~590 KB (source).
        if ((Get-Item $Header).Length -lt 20000) { return "stdsoap2.h is only $((Get-Item $Header).Length) bytes (truncated)" }
        if ((Get-Item $Source).Length -lt 50000) { return "stdsoap2.cpp is only $((Get-Item $Source).Length) bytes (truncated)" }
        $HeaderText = [IO.File]::ReadAllText($Header)
        $Missing = @($RequiredMarkers | Where-Object { -not $HeaderText.Contains($_) })
        if ($Missing.Count -gt 0) { return "stdsoap2.h lacks: $($Missing -join ', ')" }
        return ''
    }

    $Problem = GetGsoapProblem
    if (-not $Problem) { return }

    Notice "dep/gsoap looks damaged: $Problem"
    if ($GitExe -and (Test-Path (Join-Path $ServerSource '.git'))) {
        Notice 'Your server/source is a git repository - restoring dep/gsoap from its local history.'
        & $GitExe -C $ServerSource checkout -- dep/gsoap
        & $GitExe -C $ServerSource clean -xfd dep/gsoap
        $Problem = GetGsoapProblem
        if (-not $Problem) {
            Step 'Restored dep/gsoap from the local git history of server/source'
            return
        }
    }
    Fail ("dep/gsoap in server\source is damaged ($Problem). The build would fail with SOAP errors in LoginRESTService.cpp.`n" +
          "  1. Check the antivirus quarantine for stdsoap2.cpp / stdsoap2.h and add an exclusion for the server folder.`n" +
          "  2. Copy the dep\gsoap folder again from your original, intact server source.")
}

function IntegrateModules {
    $BundledSource = RootPath 'custom/src'
    $TargetDir = Join-Path $ServerSource 'src\server\scripts\Custom'
    $LegacyExternalTarget = Join-Path $TargetDir 'IntegratedMods'

    # Older project revisions copied unported AzerothCore sources here. They
    # target a different core/client and can never be allowed into this build.
    if (Test-Path $LegacyExternalTarget) {
        Step 'Removing legacy unported external module sources'
        Remove-Item $LegacyExternalTarget -Recurse -Force
    }

    if (-not (Test-Path $BundledSource)) { Fail 'Bundled module source folder custom/src is missing' }
    EnsureDirectory $TargetDir

    $BundledFiles = @(Get-ChildItem -Path $BundledSource -File | Where-Object {
        $_.Name -match '^LegionForge_.*\.(cpp|h|hpp)$'
    })
    if ($BundledFiles.Count -eq 0) { Fail 'No bundled LegionForge C++ modules were found in custom/src' }

    Step "Integrating $($BundledFiles.Count) bundled LegionCore-native source files"
    foreach ($File in $BundledFiles) {
        Copy-Item -Path $File.FullName -Destination (Join-Path $TargetDir $File.Name) -Force
    }

    $Required = @(
        'LegionForge_Config.h',
        'LegionForge_Loader.cpp',
        'LegionForge_ItemTome.cpp',
        'LegionForge_ItemUpgrade.cpp',
        'LegionForge_VendorNPC.cpp',
        'LegionForge_OnlineReward.cpp',
        'LegionForge_BrokenQuests.cpp',
        'LegionForge_WorldBoss.cpp',
        'LegionForge_PlayerBots.cpp',
        'LegionForge_CatalogMods.cpp'
    )
    foreach ($Name in $Required) {
        if (-not (Test-Path (Join-Path $TargetDir $Name))) {
            Fail "Bundled module integration is incomplete: $Name is missing"
        }
    }
}

function ReplaceCompatText([string]$Content, [string]$Original, [string]$Replacement, [string]$Description) {
    # Compatibility patches target the upstream LegionCore code. With a
    # user-supplied source the code may already be adapted or look different,
    # so a patch that does not match exactly once is skipped, never forced.
    if ($Content.Contains($Replacement)) { return $Content }
    $Count = [regex]::Matches($Content, [regex]::Escape($Original)).Count
    if ($Count -ne 1) {
        Notice "Compatibility patch '$Description' skipped: the original code was found $Count time(s) instead of once (your source is probably already adapted)."
        return $Content
    }
    return $Content.Replace($Original, $Replacement)
}

function WriteIfChanged([string]$Path, [string]$Original, [string]$Updated) {
    # Rewriting an unchanged file would bump its timestamp and force Visual
    # Studio to recompile everything that includes it on every run.
    if ($Original -ceq $Updated) { return $false }
    [IO.File]::WriteAllText($Path, $Updated, [System.Text.UTF8Encoding]::new($false))
    return $true
}

function ApplyBoostAsioCompatibilityPatch {
    # Boost 1.86 changed asio::strand to a class template. The old core used
    # boost::asio::strand without template parameters and the removed wrap()
    # adapter. Store strand<io_service::executor_type> and post through the
    # modern free-function API instead.
    $LogHeader = Join-Path $ServerSource 'src\common\Logging\Log.h'
    $LogSource = Join-Path $ServerSource 'src\common\Logging\Log.cpp'
    if (-not (Test-Path $LogHeader) -or -not (Test-Path $LogSource)) {
        Notice 'Boost.Asio compatibility patch skipped: src/common/Logging/Log.h or Log.cpp not found in your source.'
        return
    }

    $HeaderOriginal = [IO.File]::ReadAllText($LogHeader)
    $HeaderText = ReplaceCompatText $HeaderOriginal 'boost::asio::strand* _strand;' 'boost::asio::strand<boost::asio::io_service::executor_type>* _strand;' 'Log.h strand member type'

    $SourceOriginal = [IO.File]::ReadAllText($LogSource)
    $SourceText = $SourceOriginal
    $SourceText = ReplaceCompatText $SourceText 'instance._strand = new boost::asio::strand(*ioService);' 'instance._strand = new boost::asio::strand<boost::asio::io_service::executor_type>(ioService->get_executor());' 'Log.cpp strand construction'
    $SourceText = ReplaceCompatText $SourceText '_ioService->post(_strand->wrap([logOperation]() { logOperation->call(); }));' 'boost::asio::post(*_strand, [logOperation]() { logOperation->call(); });' 'Log.cpp strand post'
    # boost::asio::post needs its header; add it only when the new API is used.
    if ($SourceText.Contains('boost::asio::post(') -and $SourceText -notmatch '(?m)^\s*#include\s*<boost/asio/post\.hpp>') {
        $IncludeAnchor = '#include "Log.h"'
        if ($SourceText.Contains($IncludeAnchor)) {
            $SourceText = $SourceText.Replace($IncludeAnchor, $IncludeAnchor + [Environment]::NewLine + '#include <boost/asio/post.hpp>')
        } else {
            Notice 'Boost.Asio compatibility patch: could not find #include "Log.h" in Log.cpp to add <boost/asio/post.hpp>.'
        }
    }

    $Changed = (WriteIfChanged $LogHeader $HeaderOriginal $HeaderText)
    $Changed = (WriteIfChanged $LogSource $SourceOriginal $SourceText) -or $Changed
    if ($Changed) { Step 'Applied Boost 1.86 Asio strand compatibility patch to Logging' }
}

function ApplyOpenSslVersionApiPatch {
    # OpenSSL 1.1.0 removed SSLeay_version()/SSLEAY_VERSION in favour of
    # OpenSSL_version()/OPENSSL_VERSION. Both server mains print the library
    # version in their startup banner with the removed API.
    $Targets = @(
        (Join-Path $ServerSource 'src\server\bnetserver\Main.cpp'),
        (Join-Path $ServerSource 'src\server\worldserver\Main.cpp')
    )
    $Changed = $false
    foreach ($File in $Targets) {
        if (-not (Test-Path $File)) { continue }
        $Original = [IO.File]::ReadAllText($File)
        if (-not $Original.Contains('SSLeay_version(SSLEAY_VERSION)')) { continue }   # already modern
        $Text = ReplaceCompatText $Original 'SSLeay_version(SSLEAY_VERSION)' 'OpenSSL_version(OPENSSL_VERSION)' ("OpenSSL version banner in " + (Split-Path -Leaf (Split-Path -Parent $File)))
        $Changed = (WriteIfChanged $File $Original $Text) -or $Changed
    }
    if ($Changed) { Step 'Applied OpenSSL 1.1+/3.x version-API patch to server banners' }
}

function BackupOverlayTargets([string]$Overlay) {
    # patches/overlay overwrites a few files of the core (ScriptLoader.cpp,
    # BattlePayHandler.cpp, ...). The source now belongs to the user, so keep
    # the user's original version of every file before it is replaced for
    # the first time: runtime/backups/source-originals/<same relative path>.
    $BackupRoot = Join-Path $Backups 'source-originals'
    $OverlayFull = (Resolve-Path $Overlay).Path.TrimEnd('\', '/')
    $Saved = 0
    foreach ($File in (Get-ChildItem -Path $OverlayFull -Recurse -File | Where-Object { $_.Name -ne '.gitkeep' })) {
        $Relative = $File.FullName.Substring($OverlayFull.Length).TrimStart('\', '/')
        $Target = Join-Path $ServerSource $Relative
        $Backup = Join-Path $BackupRoot $Relative
        if (-not (Test-Path $Target) -or (Test-Path $Backup)) { continue }
        if ((Get-FileHash $Target -Algorithm SHA256).Hash -eq (Get-FileHash $File.FullName -Algorithm SHA256).Hash) { continue }
        EnsureDirectory (Split-Path -Parent $Backup)
        Copy-Item -Path $Target -Destination $Backup -Force
        $Saved++
    }
    if ($Saved -gt 0) {
        Notice "Saved $Saved original file(s) of your source to runtime\backups\source-originals before applying patches/overlay."
    }
}

function ApplyOverlay {
    $Overlay = Join-Path $Patches 'overlay'
    if (-not (Test-Path $Overlay)) { return }
    BackupOverlayTargets $Overlay
    Step 'Applying patches/overlay without deleting upstream files'
    # /IS forces overwriting upstream files even when timestamps look newer.
    # Without it files with newer timestamps could silently win over our patches
    # (free transmog, BattlePay currency, FindMySQL, script loader).
    & robocopy $Overlay $ServerSource /E /IS /IT /XF .gitkeep /NFL /NDL /NJH /NJS | Out-Null
    if ($LASTEXITCODE -ge 8) { Fail "Overlay copy failed with robocopy exit code $LASTEXITCODE" }

    $LoaderMarker = Join-Path $ServerSource 'src\server\scripts\ScriptLoader.cpp'
    if (Test-Path $LoaderMarker) {
        if (-not (Select-String -Path $LoaderMarker -SimpleMatch 'AddSC_LegionForge_Custom' -Quiet)) {
            Fail 'Overlay did not apply: ScriptLoader.cpp is missing the LegionForge hook'
        }
    }

    ApplyBoostAsioCompatibilityPatch
    ApplyOpenSslVersionApiPatch
}

function WriteRuntimeConfig {
    $ConfigRoot = Join-Path $ServerRuntime 'config'
    EnsureDirectory $ConfigRoot
    $ConnectionFile = Join-Path $ConfigRoot 'local-database.env'
    if (-not (Test-Path $ConnectionFile)) {
        @(
            '# Generated local database endpoint. Do not commit credentials.',
            'DB_HOST=127.0.0.1',
            'DB_PORT=3307',
            'DB_USER=legion',
            'DB_PASSWORD=change-me-before-production',
            'DB_AUTH=legion_auth',
            'DB_CHARACTERS=legion_characters',
            'DB_WORLD=legion_world',
            'DB_HOTFIXES=legion_hotfixes'
        ) | Set-Content -Path $ConnectionFile -Encoding ascii
    }
}

function Prepare {
    Step 'Preparing the self-contained developer kit'
    VerifyLauncherIntegrity
    $GitExe = ResolveGit
    $null = EnsurePortableCMake
    $null = EnsurePortableNode
    $VisualStudioPath = DetectVisualStudio
    if (-not $VisualStudioPath) { Fail 'Visual Studio with the MSVC x64 toolset was not found' }
    $null = EnsureMariaDb
    $null = EnsurePortableBoost
    $null = EnsurePortableOpenSSL
    if (Test-Path (Join-Path $Tools 'vcpkg')) {
        Write-Host '[LegionForge] Notice: tools/vcpkg is obsolete (the vcpkg package manager was removed) and is no longer used. You can safely delete that folder to free disk space.' -ForegroundColor Yellow
    }
    EnsureSource
    EnsureVendoredGsoapIntegrity $GitExe
    EnsureCustomMods $GitExe
    IntegrateModules
    ApplyOverlay
    WriteRuntimeConfig
    WriteStatus 'ready'
}

function ResolveMySqlPaths {
    # The portable MariaDB Server archive already ships the client development
    # files (include/mysql + lib/libmariadb.lib + lib/libmariadb.dll), so no
    # separate Connector/C download is needed.
    $Header = Join-Path $MariaRoot 'include\mysql\mysql.h'
    if (-not (Test-Path $Header)) { Fail "mysql.h was not found under $MariaRoot\include\mysql. Re-run START.bat so portable MariaDB is extracted." }

    $Library = Join-Path $MariaRoot 'lib\libmariadb.lib'
    if (-not (Test-Path $Library)) { Fail "libmariadb.lib was not found under $MariaRoot\lib." }

    return [pscustomobject]@{
        IncludeDir = (Split-Path -Parent $Header)
        Library    = $Library
    }
}

function DisableLegacyFindModules {
    $MacroDir = Join-Path $ServerSource 'cmake\macros'
    foreach ($Name in @('FindBoost.cmake', 'FindOpenSSL.cmake')) {
        $Path = Join-Path $MacroDir $Name
        $Disabled = "$Path.upstream.disabled"
        if (Test-Path $Path) {
            # Keep the original next to it so the change is transparent and reversible.
            if (Test-Path $Disabled) { Remove-Item $Disabled -Force }
            Move-Item -Path $Path -Destination $Disabled -Force
            Step "Using CMake's built-in $Name instead of the legacy core copy"
        }
    }
}

function CopyRuntimeDependencies {
    EnsureDirectory $ServerRuntime
    Step 'Copying portable runtime libraries next to the server executables'
    foreach ($Dll in @(
        (Join-Path $OpenSSLRoot 'bin\libcrypto-3-x64.dll'),
        (Join-Path $OpenSSLRoot 'bin\libssl-3-x64.dll'),
        (Join-Path $MariaRoot 'lib\libmariadb.dll')
    )) {
        if (Test-Path $Dll) {
            Copy-Item -Path $Dll -Destination $ServerRuntime -Force
        } else {
            Write-Host "[LegionForge] WARNING: runtime library is missing: $Dll" -ForegroundColor Yellow
        }
    }

    # MariaDB client authentication plugins (used only if the server is
    # configured with matching auth methods; harmless to ship alongside).
    foreach ($Name in @('caching_sha2_password.dll', 'client_ed25519.dll', 'dialog.dll', 'mysql_clear_password.dll', 'sha256_password.dll')) {
        $Plugin = Join-Path $MariaRoot "lib\plugin\$Name"
        if (Test-Path $Plugin) { Copy-Item -Path $Plugin -Destination $ServerRuntime -Force }
    }
}

function ConfigureAndBuild {
    Prepare
    $CMakeExe = Join-Path $CMakeRoot 'bin\cmake.exe'
    if (-not (Test-Path (Join-Path $ServerSource 'CMakeLists.txt'))) { Fail 'server\source has no CMakeLists.txt - copy your server source code into server\source' }

    $MySql = ResolveMySqlPaths
    $BoostLibDir = Join-Path $BoostRoot 'lib64-msvc-14.3'
    Step "Using Boost headers: $BoostRoot"
    Step "Using Boost libraries: $BoostLibDir"
    Step "Using OpenSSL: $OpenSSLRoot"
    Step "Using MySQL/MariaDB headers: $($MySql.IncludeDir)"
    Step "Using MySQL/MariaDB library: $($MySql.Library)"

    # The dependency strategy changed from vcpkg to portable prebuilt trees,
    # so a CMake cache generated by an older revision must be discarded once.
    $DepsSignature = 'portable-deps;boost=1.86.0-msvc-14.3-64;openssl=3.5.8;mysql=portable-server'
    $DepsMarker = Join-Path $ServerBuild '.legionforge-deps.txt'
    $PreviousSignature = ''
    if (Test-Path $DepsMarker) { $PreviousSignature = (Get-Content -Raw $DepsMarker).Trim() }
    if ($PreviousSignature -ne $DepsSignature) {
        if (Test-Path (Join-Path $ServerBuild 'CMakeCache.txt')) {
            Step 'Dependency layout changed - clearing the previous CMake cache for a clean reconfigure'
            Remove-Item (Join-Path $ServerBuild 'CMakeCache.txt') -Force -ErrorAction SilentlyContinue
            Remove-Item (Join-Path $ServerBuild 'CMakeFiles') -Recurse -Force -ErrorAction SilentlyContinue
        }
        Set-Content -Path $DepsMarker -Value $DepsSignature -Encoding ascii
    }

    # A previously failed generation leaves NOTFOUND entries behind that would
    # be reused on the next run, so drop the cache and regenerate cleanly.
    $CacheFile = Join-Path $ServerBuild 'CMakeCache.txt'
    if (Test-Path $CacheFile) {
        if (Select-String -Path $CacheFile -SimpleMatch 'NOTFOUND' -Quiet) {
            Step 'Clearing incomplete CMake cache from a previous attempt'
            Remove-Item $CacheFile -Force -ErrorAction SilentlyContinue
            Remove-Item (Join-Path $ServerBuild 'CMakeFiles') -Recurse -Force -ErrorAction SilentlyContinue
        }
    }

    $ConfigureArgs = @(
        '-S', $ServerSource,
        '-B', $ServerBuild,
        '-G', $Manifest.build.generator,
        '-A', $Manifest.build.architecture,
        "-DCMAKE_INSTALL_PREFIX=$ServerRuntime",
        "-DCMAKE_PREFIX_PATH=$OpenSSLRoot",
        "-DMYSQL_INCLUDE_DIR=$($MySql.IncludeDir)",
        "-DMYSQL_LIBRARY=$($MySql.Library)",
        "-DBOOST_ROOT=$BoostRoot",
        "-DBOOST_LIBRARYDIR=$BoostLibDir",
        "-DBoost_NO_WARN_NEW_VERSIONS=ON",
        "-DOPENSSL_ROOT_DIR=$OpenSSLRoot",
        "-DOPENSSL_USE_STATIC_LIBS=OFF"
    )

    # Publish the portable trees as environment variables as well: legacy
    # core scripts inspect BOOST_ROOT / OPENSSL_ROOT_DIR directly.
    $env:BOOST_ROOT = $BoostRoot
    $env:BOOST_LIBRARYDIR = $BoostLibDir
    $env:OPENSSL_ROOT_DIR = $OpenSSLRoot

    # The core ships 2016-era copies of FindBoost.cmake / FindOpenSSL.cmake in
    # cmake/macros. They only know Boost versions up to ~1.6x and legacy
    # OpenSSL library names, so they cannot locate the portable trees. Disable
    # them so CMake falls back to its own, current Find modules.
    DisableLegacyFindModules

    Step 'Generating x64 Release build files'
    & $CMakeExe @ConfigureArgs
    if ($LASTEXITCODE -ne 0) { Fail 'CMake generation failed. See the output above.' }

    Step 'Building and installing Release x64'
    & $CMakeExe --build $ServerBuild --config $Manifest.build.configuration --target INSTALL -- /m
    if ($LASTEXITCODE -ne 0) { Fail 'CMake build failed. See the output above.' }

    CopyRuntimeDependencies
    PrepareServerConfig
    WriteStatus 'built'
}

function SetRuntimeConfigValue([string]$File, [string]$Key, [string]$Value) {
    if (-not (Test-Path $File)) { return }
    $Content = Get-Content -Raw -Path $File
    $Pattern = "(?m)^\s*$([regex]::Escape($Key))\s*=.*$"
    if ([regex]::IsMatch($Content, $Pattern)) {
        $Replacement = "$Key = `"$Value`""
        [regex]::Replace($Content, $Pattern, $Replacement) | Set-Content -Path $File -Encoding utf8
    }
}

function PrepareServerConfig {
    # Ядро Legion 7.3.5 собирает login-сервер как bnetserver.exe/bnetserver.conf.dist.
    # Имя authserver.* осталось от старых веток, поэтому сначала ищем bnetserver,
    # а authserver используем как запасной вариант. Раньше здесь был зашит только
    # authserver, из-за чего конфиг login-сервера никогда не создавался.
    $AuthName = ResolveLoginServerBaseName
    $AuthDist = Join-Path $ServerRuntime "$AuthName.conf.dist"
    $WorldDist = Join-Path $ServerRuntime 'worldserver.conf.dist'
    $AuthConf = Join-Path $ServerRuntime "$AuthName.conf"
    $WorldConf = Join-Path $ServerRuntime 'worldserver.conf'
    if ((Test-Path $AuthDist) -and (-not (Test-Path $AuthConf))) { Copy-Item $AuthDist $AuthConf }
    if ((Test-Path $WorldDist) -and (-not (Test-Path $WorldConf))) { Copy-Item $WorldDist $WorldConf }
    $Login = '127.0.0.1;3307;legion;change-me-before-production;legion_auth'
    $Characters = '127.0.0.1;3307;legion;change-me-before-production;legion_characters'
    $World = '127.0.0.1;3307;legion;change-me-before-production;legion_world'
    $Hotfixes = '127.0.0.1;3307;legion;change-me-before-production;legion_hotfixes'
    SetRuntimeConfigValue $AuthConf 'LoginDatabaseInfo' $Login
    SetRuntimeConfigValue $WorldConf 'LoginDatabaseInfo' $Login
    SetRuntimeConfigValue $WorldConf 'CharacterDatabaseInfo' $Characters
    SetRuntimeConfigValue $WorldConf 'WorldDatabaseInfo' $World
    SetRuntimeConfigValue $WorldConf 'HotfixDatabaseInfo' $Hotfixes
}

function StartDatabase {
    $DataDir = Join-Path $DatabaseHome 'data'
    $MyIni = Join-Path $DatabaseHome 'my.ini'
    $Server = ResolveDatabaseServerExe
    $Client = ResolveDatabaseClientExe
    EnsureDirectory $DataDir
    if (-not (Test-Path $MyIni)) {
        @(
            '[mysqld]',
            "basedir=$MariaRoot",
            "datadir=$DataDir",
            'port=3307',
            'bind-address=127.0.0.1',
            'character-set-server=utf8mb4',
            'collation-server=utf8mb4_unicode_ci',
            "log-error=$(Join-Path $Logs 'mariadb.log')",
            "pid-file=$(Join-Path $DatabaseHome 'mariadb.pid')"
        ) | Set-Content -Path $MyIni -Encoding ascii
    }
    if (-not (Test-Path (Join-Path $DataDir 'mysql'))) {
        Step 'Initialising the local MariaDB data directory'
        & $Server --defaults-file=$MyIni --initialize-insecure
        if ($LASTEXITCODE -ne 0) { Fail 'MariaDB data directory initialisation failed' }
    }
    $Running = Get-Process -Name (GetDatabaseProcessNames) -ErrorAction SilentlyContinue
    if ($null -eq $Running) {
        Step 'Starting portable MariaDB on 127.0.0.1:3307'
        Start-Process -FilePath $Server -ArgumentList "--defaults-file=$MyIni" -WindowStyle Hidden
    }
    if (-not (Test-Path $Client)) { Fail 'MariaDB client is missing' }
    $Ready = $false
    foreach ($Attempt in 1..15) {
        & $Client --protocol=tcp --host=127.0.0.1 --port=3307 --user=root -e 'SELECT 1' 2>$null
        if ($LASTEXITCODE -eq 0) { $Ready = $true; break }
        Start-Sleep -Seconds 1
    }
    if (-not $Ready) { Fail 'MariaDB did not become ready on port 3307' }
    $Sql = @'
CREATE DATABASE IF NOT EXISTS legion_auth CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS legion_characters CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS legion_world CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS legion_hotfixes CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS 'legion'@'127.0.0.1' IDENTIFIED BY 'change-me-before-production';
GRANT ALL PRIVILEGES ON legion_auth.* TO 'legion'@'127.0.0.1';
GRANT ALL PRIVILEGES ON legion_characters.* TO 'legion'@'127.0.0.1';
GRANT ALL PRIVILEGES ON legion_world.* TO 'legion'@'127.0.0.1';
GRANT ALL PRIVILEGES ON legion_hotfixes.* TO 'legion'@'127.0.0.1';
FLUSH PRIVILEGES;
'@
    & $Client --protocol=tcp --host=127.0.0.1 --port=3307 --user=root -e $Sql
    if ($LASTEXITCODE -ne 0) { Fail 'Unable to create local game databases' }
}

function StartServer {
    Prepare
    $Auth = Join-Path $ServerRuntime ((ResolveLoginServerBaseName) + '.exe')
    $World = Join-Path $ServerRuntime 'worldserver.exe'
    if (-not (Test-Path $Auth) -or -not (Test-Path $World)) { Fail 'No compiled runtime exists. Choose Build first.' }
    PrepareServerConfig
    StartDatabase
    Step "Starting $(ResolveLoginServerBaseName) and WorldServer"
    Start-Process -FilePath $Auth -WorkingDirectory $ServerRuntime
    Start-Sleep -Seconds 2
    Start-Process -FilePath $World -WorkingDirectory $ServerRuntime
    WriteStatus 'running'
}

function StartPanel {
    # The web Control Center is a static Next.js build. It only needs a
    # portable Node.js runtime, so we deliberately avoid the heavy server
    # Prepare step (cmake/MariaDB/Boost/OpenSSL/source checks) here. That keeps
    # "arguscore.bin" fast and prevents an unrelated server-side download
    # failure from blocking the panel from opening.
    Step 'Preparing the Control Center (web panel only)'
    EnsurePortableNode
    $Npm = Join-Path $NodeRoot 'npm.cmd'
    if (-not (Test-Path $Npm)) { Fail 'Portable npm is missing' }
    Push-Location $Root
    try {
        if (-not (Test-Path (Join-Path $Root 'node_modules\next'))) {
            Step 'Installing Control Center dependencies'
            & $Npm ci
            if ($LASTEXITCODE -ne 0) { Fail 'npm ci failed' }
        }
        if (-not (Test-Path (Join-Path $Root '.next\BUILD_ID'))) {
            Step 'Building the Control Center'
            & $Npm run build
            if ($LASTEXITCODE -ne 0) { Fail 'Control Center build failed' }
        }
        Start-Process 'http://localhost:3000'
        Step 'Control Center is starting at http://localhost:3000'
        & $Npm run start
    } finally { Pop-Location }
}

# ---------------------------------------------------------------------------
# Stop mode (Stop.bat): stops every process the kit started.
# A process belongs to the kit when its executable lives inside the project
# folder (tools\mariadb, tools\node, tools\cmake, tools\git, server\runtime,
# ...) or when it is another running instance of this bootstrap.ps1. Nothing
# outside the project folder is ever touched (a system MariaDB, another
# Node.js app or your IDE keep running).
# ---------------------------------------------------------------------------
function GetProjectProcesses {
    $RootPrefix = ([System.IO.Path]::GetFullPath($Root)).TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    $All = $null
    if (Get-Command Get-CimInstance -ErrorAction SilentlyContinue) {
        try { $All = @(Get-CimInstance -ClassName Win32_Process -ErrorAction Stop | ForEach-Object {
            [pscustomobject]@{ Id = [int]$_.ProcessId; Name = [string]$_.Name; Path = [string]$_.ExecutablePath; CommandLine = [string]$_.CommandLine }
        }) } catch { $All = $null }
    }
    if ($null -eq $All) {
        # Fallback when WMI/CIM is unavailable.
        $All = @(Get-Process | ForEach-Object {
            $Path = ''; $Cmd = ''
            try { $Path = [string]$_.Path } catch { }
            try { $Cmd = [string]$_.CommandLine } catch { }
            [pscustomobject]@{ Id = [int]$_.Id; Name = [string]$_.ProcessName; Path = $Path; CommandLine = $Cmd }
        })
    }
    return @($All | Where-Object {
        $_.Id -ne $PID -and (
            ($_.Path -and $_.Path.StartsWith($RootPrefix, [StringComparison]::OrdinalIgnoreCase)) -or
            ($BootstrapScript -and $_.CommandLine -and
             $_.CommandLine.IndexOf($BootstrapScript, [StringComparison]::OrdinalIgnoreCase) -ge 0 -and
             $_.CommandLine -notmatch '-Mode\s+Stop')
        )
    })
}

function GetShortProcessName($Process) {
    return ([System.IO.Path]::GetFileNameWithoutExtension([string]$Process.Name)).ToLowerInvariant()
}

function TestProcessAlive([int]$Id) {
    return ($null -ne (Get-Process -Id $Id -ErrorAction SilentlyContinue))
}

function WaitForExit([int[]]$Ids, [int]$Seconds) {
    $Deadline = (Get-Date).AddSeconds($Seconds)
    do {
        $Alive = @($Ids | Where-Object { TestProcessAlive $_ })
        if ($Alive.Count -eq 0) { return @() }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $Deadline)
    return $Alive
}

function HasTaskKill { return ($null -ne (Get-Command taskkill.exe -ErrorAction SilentlyContinue)) }

function RequestClose([int]$Id) {
    # Polite close request (WM_CLOSE). Console programs usually refuse it;
    # the caller then falls back to a forced stop after a short wait.
    if (-not (HasTaskKill)) { return $false }
    & taskkill.exe /PID $Id 2>&1 | Out-Null
    return ($LASTEXITCODE -eq 0)
}

function StopForcefully([int]$Id, [switch]$Tree) {
    if (-not (TestProcessAlive $Id)) { return }
    if (HasTaskKill) {
        # /T also ends child processes (MSBuild and cl.exe under cmake, ...).
        $TaskArgs = @('/PID', $Id)
        if ($Tree) { $TaskArgs += '/T' }
        $TaskArgs += '/F'
        & taskkill.exe @TaskArgs 2>&1 | Out-Null
    } else {
        Stop-Process -Id $Id -Force -ErrorAction SilentlyContinue
    }
}

function StopGroup([object[]]$Processes, [string]$Label, [int]$GraceSeconds, [switch]$Immediately, [switch]$Tree) {
    if ($Processes.Count -eq 0) { return }
    Step "Stopping $Label"
    $Ids = @($Processes | ForEach-Object { $_.Id })
    if (-not $Immediately -and $GraceSeconds -gt 0) {
        $Asked = $false
        foreach ($Id in $Ids) { if (RequestClose $Id) { $Asked = $true } }
        if ($Asked) { $Ids = @(WaitForExit $Ids $GraceSeconds) }
    }
    foreach ($Id in $Ids) { StopForcefully $Id -Tree:$Tree }
    $null = WaitForExit $Ids 10
}

function StopDatabaseGracefully([object[]]$Processes, [switch]$Immediately) {
    if ($Processes.Count -eq 0) { return }
    Step 'Stopping portable MariaDB'
    $Ids = @($Processes | ForEach-Object { $_.Id })
    if (-not $Immediately) {
        # A clean shutdown flushes InnoDB, so the next start needs no crash recovery.
        $Admin = @('mariadb-admin.exe', 'mysqladmin.exe') |
            ForEach-Object { Join-Path $MariaRoot "bin\$_" } |
            Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($Admin) {
            & $Admin --protocol=tcp --host=127.0.0.1 --port=3307 --user=root shutdown 2>$null
            if ($LASTEXITCODE -eq 0) {
                $Ids = @(WaitForExit $Ids 30)
            } else {
                Notice 'MariaDB refused the clean shutdown command (root password set?) - stopping it forcefully. InnoDB recovers automatically on the next start.'
            }
        }
    }
    foreach ($Id in $Ids) { StopForcefully $Id }
    $null = WaitForExit $Ids 10
}

function StopProjectProcesses([switch]$Immediately) {
    Step "Looking for processes started by LegionForge in $Root"
    $Found = @(GetProjectProcesses)
    if ($Found.Count -eq 0) {
        Step 'Nothing to stop: no LegionForge processes are running'
        try { WriteStatus 'stopped' } catch { }
        return
    }
    foreach ($P in $Found) { Write-Host ("  - {0,-22} PID {1,-7} {2}" -f $P.Name, $P.Id, $P.Path) }

    $GameNames = @('worldserver', 'bnetserver', 'authserver')
    $DbNames = @('mariadbd', 'mysqld')
    $World = @($Found | Where-Object { (GetShortProcessName $_) -eq 'worldserver' })
    $Login = @($Found | Where-Object { (GetShortProcessName $_) -in @('bnetserver', 'authserver') })
    $Database = @($Found | Where-Object { (GetShortProcessName $_) -in $DbNames })
    $Rest = @($Found | Where-Object { (GetShortProcessName $_) -notin ($GameNames + $DbNames) })
    # Tools (node, cmake, git, ...) first, their bootstrap.ps1 parents last.
    $Tools = @($Rest | Where-Object { (GetShortProcessName $_) -notin @('powershell', 'pwsh') })
    $Scripts = @($Rest | Where-Object { (GetShortProcessName $_) -in @('powershell', 'pwsh') })

    # Order matters: world first (it still talks to the login server and the
    # database), then the login server, then panel/build/tools, database last.
    if ($World.Count -gt 0) {
        Notice 'Tip: for a guaranteed save of online characters type ".server shutdown 1" in the worldserver console before Stop.bat.'
    }
    StopGroup $World 'WorldServer' 15 -Immediately:$Immediately
    StopGroup $Login 'login server (bnetserver/authserver)' 10 -Immediately:$Immediately
    StopGroup $Tools 'Control Center, build and tool processes' 0 -Immediately -Tree
    StopGroup $Scripts 'running LegionForge scripts' 0 -Immediately -Tree
    StopDatabaseGracefully $Database -Immediately:$Immediately

    $Left = @(GetProjectProcesses)
    if ($Left.Count -gt 0) {
        $List = ($Left | ForEach-Object { "$($_.Name) (PID $($_.Id))" }) -join ', '
        Fail "Could not stop: $List. If they were started as administrator, run Stop.bat as administrator too."
    }
    try { WriteStatus 'stopped' } catch { }
    Step "Stopped $($Found.Count) process(es). All LegionForge processes are down."
}

function WriteStatus([string]$Stage) {
    $State = [ordered]@{
        stage = $Stage
        updatedAt = (Get-Date).ToUniversalTime().ToString('o')
        clientBuild = $Manifest.project.clientBuild
        sourceMode = 'local'
        source = $ServerSource
        runtime = $ServerRuntime
        toolsRoot = $Tools
        installed = [ordered]@{
            git = (Test-Path (Join-Path $GitRoot 'cmd\git.exe'))
            cmake = (Test-Path (Join-Path $CMakeRoot 'bin\cmake.exe'))
            node = (Test-Path (Join-Path $NodeRoot 'node.exe'))
            visualStudio = $true
            boost = (Test-Path (Join-Path $BoostRoot 'boost\version.hpp'))
            openssl = (Test-Path (Join-Path $OpenSSLRoot 'include\openssl\ssl.h'))
            mysqlDev = (Test-Path (Join-Path $MariaRoot 'lib\libmariadb.lib'))
            mariadb = (Test-Path (ResolveDatabaseServerExe))
            bundledModules = (Test-Path (Join-Path $ServerSource 'src\server\scripts\Custom\LegionForge_Loader.cpp'))
            externalAzerothCoreSources = $false
        }
        compiled = (Test-Path (Join-Path $ServerRuntime 'worldserver.exe')) -and (Test-Path (Join-Path $ServerRuntime ((ResolveLoginServerBaseName) + '.exe')))
    }
    $State | ConvertTo-Json -Depth 4 | Set-Content -Path (Join-Path $DatabaseHome 'bootstrap-status.json') -Encoding utf8
}

try {
    switch ($Mode) {
        'Prepare' { Prepare }
        'Build' { ConfigureAndBuild }
        'Run' { StartServer }
        'Panel' { StartPanel }
        'Status' { WriteStatus 'checked'; Get-Content -Raw (Join-Path $DatabaseHome 'bootstrap-status.json') }
        'Stop' { StopProjectProcesses -Immediately:$Force }
    }
    if ($Mode -ne 'Panel') { Step "Completed: $Mode" }
    exit 0
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
