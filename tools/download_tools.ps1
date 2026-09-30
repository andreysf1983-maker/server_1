# ===========================================================================
#  LEGIONFORGE :: tools/download_tools.ps1
#  Автоматическая загрузка 100% портативных утилит в /tools
#  Использование:  powershell -File download_tools.ps1 [-Only "git cmake"]
# ===========================================================================
param([string]$Only = "all")

$ErrorActionPreference = "Stop"
$Root  = Split-Path -Parent $PSScriptRoot
$Tools = Join-Path $Root "tools"
$Cache = Join-Path $Tools "_cache"
New-Item -ItemType Directory -Force -Path $Tools, $Cache | Out-Null
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Want($name) { return ($Only -eq "all" -or $Only -like "*$name*") }

function Get-Portable {
    param([string]$Name, [string]$Url, [string]$Dest, [string]$Marker)
    $target = Join-Path $Tools $Dest
    if ((Test-Path (Join-Path $target $Marker))) { Write-Host "  [=] $Name уже установлен" -ForegroundColor DarkGreen; return }
    Write-Host "  [+] Скачиваю $Name ..." -ForegroundColor Cyan
    $archive = Join-Path $Cache ([IO.Path]::GetFileName(([Uri]$Url).AbsolutePath))
    if (-not (Test-Path $archive)) { Invoke-WebRequest -Uri $Url -OutFile $archive -UseBasicParsing }
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    if ($archive -like "*.zip") { Expand-Archive -Path $archive -DestinationPath $target -Force }
    elseif ($archive -like "*.7z") { & (Join-Path $Tools "7zip\7za.exe") x -y "-o$target" $archive | Out-Null }
    else { & tar -xf $archive -C $target }
    Write-Host "      распаковано в $target" -ForegroundColor DarkGray
}

# --- 7-Zip (нужен первым: им распаковываем остальные архивы) ---------------
if (Want "7zip") {
    Get-Portable "Portable 7-Zip" "https://www.7-zip.org/a/7zr.exe" "7zip" "7zr.exe"
    $dst = Join-Path $Tools "7zip"; New-Item -ItemType Directory -Force -Path $dst | Out-Null
    if (-not (Test-Path (Join-Path $dst "7za.exe"))) {
        Invoke-WebRequest "https://www.7-zip.org/a/7z2408-extra.7z" -OutFile (Join-Path $Cache "7z-extra.7z") -UseBasicParsing
    }
}

# --- Portable Git ----------------------------------------------------------
if (Want "git") {
    Get-Portable "Portable Git 2.45" "https://github.com/git-for-windows/git/releases/download/v2.45.2.windows.1/PortableGit-2.45.2-64-bit.7z.exe" "git" "cmd\git.exe"
}

# --- CMake + Ninja ---------------------------------------------------------
if (Want "cmake") {
    Get-Portable "CMake 3.30" "https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-windows-x86_64.zip" "cmake" "bin\cmake.exe"
    Get-Portable "Ninja 1.12" "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip" "cmake\bin" "ninja.exe"
}

# --- .NET 8 SDK (для LegionForge_Manager.exe) ------------------------------
if (Want "dotnet") {
    Get-Portable ".NET 8 SDK" "https://dotnetcli.azureedge.net/dotnet/Sdk/8.0.404/dotnet-sdk-8.0.404-win-x64.zip" "dotnet" "dotnet.exe"
}

# --- Node.js (для веб-панели) ----------------------------------------------
if (Want "nodejs") {
    Get-Portable "Node.js 20 LTS" "https://nodejs.org/dist/v20.18.1/node-v20.18.1-win-x64.zip" "nodejs" "node.exe"
}

# --- MySQL Server 8.0 (портативный, с преднастроенным my.ini) --------------
if (Want "mysql") {
    Get-Portable "MySQL 8.0 Server" "https://dev.mysql.com/get/Downloads/MySQL-8.0/mysql-8.0.40-winx64.zip" "mysql" "bin\mysqld.exe"
    $ini = Join-Path $Tools "mysql\my.ini"
    if (-not (Test-Path $ini)) { Copy-Item (Join-Path $Tools "my.ini.template") $ini -Force }
}

# --- OpenSSL 1.1.1+ x64 ----------------------------------------------------
if (Want "openssl") {
    Get-Portable "OpenSSL 3.0 x64" "https://slproweb.com/download/Win64OpenSSL-3_0_15.zip" "openssl" "bin\openssl.exe"
}

# --- Boost (заголовки + авто-сборка нужных библиотек) ----------------------
if (Want "boost") {
    Get-Portable "Boost 1.86" "https://archives.boost.io/release/1.86.0/source/boost_1_86_0.zip" "boost\src" "boost\version.hpp"
    Write-Host "  [+] Собираю Boost (system, filesystem, program_options, iostreams, thread) ..." -ForegroundColor Cyan
    & (Join-Path $Tools "compile_boost.bat")
}

# --- Visual Studio Build Tools (MSVC) --------------------------------------
if (Want "msvc") {
    $bt = Join-Path $Cache "vs_buildtools.exe"
    if (-not (Test-Path $bt)) {
        Invoke-WebRequest "https://aka.ms/vs/17/release/vs_BuildTools.exe" -OutFile $bt -UseBasicParsing
    }
    Start-Process $bt -ArgumentList "--quiet","--wait","--norestart","--nocache",
        "--add","Microsoft.VisualStudio.Workload.VCTools",
        "--add","Microsoft.VisualStudio.Component.Windows11SDK.22621" -Wait
}

Write-Host "  [OK] Портативная среда LEGIONFORGE готова." -ForegroundColor Green
