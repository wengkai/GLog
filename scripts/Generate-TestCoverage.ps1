<#
.SYNOPSIS
    Run CTest under OpenCppCoverage (Windows) and write HTML + Cobertura reports.

.DESCRIPTION
    Requires a multi-config or single-config CMake build with PDBs (Debug or RelWithDebInfo).
    For MSVC + Qt, use a Developer shell so cmake/cl/msvc are on PATH, e.g.:
        & 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64
    From automation, add -SkipAutomaticLocation to avoid changing the working directory. Then:
        cmake -B build -G "Visual Studio 17 2022" -A x64
        cmake --build build --config Debug

    OpenCppCoverage must use --cover_children so CTest-spawned test executables are included.

.PARAMETER BuildDir
    CMake build directory (must contain CTestTestfile.cmake).

.PARAMETER Config
    CTest -C configuration (e.g. Debug, RelWithDebInfo).

.PARAMETER SourceRoot
    Repository root containing product sources. Default: parent of this script's directory.

.PARAMETER OpenCppCoverage
    Path or name of OpenCppCoverage.exe (PATH is used if bare name).

.PARAMETER OutDir
    Output folder for HTML report (index.html) and cobertura.xml. Default: <BuildDir>/coverage

.PARAMETER Parallel
    CTest parallel job count. 0 = default (ctest decides, typically serial for stability).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string] $BuildDir = './build',

    [Parameter(Mandatory = $false)]
    [string] $Config = 'Debug',

    [Parameter(Mandatory = $false)]
    [string] $SourceRoot = '',

    [Parameter(Mandatory = $false)]
    [string] $OpenCppCoverage = 'OpenCppCoverage.exe',

    [Parameter(Mandatory = $false)]
    [string] $OutDir = '',

    [Parameter(Mandatory = $false)]
    [int] $Parallel = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ExePath {
    param([string] $NameOrPath)
    if ([string]::IsNullOrWhiteSpace($NameOrPath)) {
        throw "Executable path is empty."
    }
    if ([System.IO.Path]::IsPathRooted($NameOrPath) -and (Test-Path -LiteralPath $NameOrPath)) {
        return (Resolve-Path -LiteralPath $NameOrPath).Path
    }
    $cmd = Get-Command -Name $NameOrPath -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Source) {
        return $cmd.Source
    }
    throw "Executable not found: $NameOrPath. Add it to PATH or pass a full path."
}

$ScriptDir = $PSScriptRoot
if (-not $ScriptDir) {
    $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}

if ([string]::IsNullOrWhiteSpace($SourceRoot)) {
    $SourceRoot = (Resolve-Path (Join-Path $ScriptDir '..')).Path
}
else {
    $SourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
}

if (-not (Test-Path -LiteralPath $BuildDir)) {
    throw "BuildDir does not exist: $BuildDir"
}
$BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path

$ctestFile = Join-Path $BuildDir 'CTestTestfile.cmake'
if (-not (Test-Path -LiteralPath $ctestFile)) {
    throw "Not a CMake build tree (missing CTestTestfile.cmake): $BuildDir"
}

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $BuildDir 'coverage'
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$OutDir = (Resolve-Path -LiteralPath $OutDir).Path

$coberturaPath = Join-Path $OutDir 'cobertura.xml'

$occPath = Resolve-ExePath -NameOrPath $OpenCppCoverage
$ctestPath = Resolve-ExePath -NameOrPath 'ctest'

# Patterns: OpenCppCoverage --sources / --excluded_sources use * wildcards on full paths.
$occArgs = @(
    '--cover_children',
    '--sources', $SourceRoot,
    '--excluded_sources', '*\build\*',
    '--excluded_sources', '*\Build\*',
    '--excluded_sources', '*\CMakeFiles\*',
    '--excluded_sources', '*\out\build\*',
    '--excluded_sources', '*\vcpkg\*',
    '--excluded_sources', '*\.cache\*',
    '--excluded_sources', '*\_deps\*',
    '--excluded_sources', '*\tests\*',
    '--excluded_sources', '*moc_*.cpp',
    '--excluded_sources', '*\moc_*.cpp',
    '--excluded_sources', '*_autogen\*',
    '--excluded_sources', '*\qrc_*.cpp',
    '--excluded_sources', '*\ui_*.h',
    '--export_type', "html:$OutDir",
    '--export_type', "cobertura:$coberturaPath",
    '--',
    $ctestPath,
    '--test-dir', $BuildDir,
    '-C', $Config,
    '--output-on-failure'
)

if ($Parallel -gt 0) {
    $occArgs += @('-j', "$Parallel")
}

Write-Host "OpenCppCoverage: $occPath"
Write-Host "SourceRoot:      $SourceRoot"
Write-Host "BuildDir:        $BuildDir"
Write-Host "Config:          $Config"
Write-Host "OutDir:          $OutDir"

& $occPath @occArgs
$exit = $LASTEXITCODE
if ($exit -ne 0) {
    exit $exit
}

$indexHtml = Join-Path $OutDir 'index.html'
if (-not (Test-Path -LiteralPath $indexHtml)) {
    throw "Expected HTML report missing: $indexHtml"
}
if (-not (Test-Path -LiteralPath $coberturaPath)) {
    throw "Expected Cobertura report missing: $coberturaPath"
}

Write-Host "Coverage OK: $indexHtml"
Write-Host "Coverage OK: $coberturaPath"
