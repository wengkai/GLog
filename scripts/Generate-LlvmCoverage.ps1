<#
.SYNOPSIS
    Run CTest under LLVM source-based coverage and write llvm-cov HTML + lcov reports.

.DESCRIPTION
    Requires a CMake build configured with the clang/LLVM toolchain and the source-based
    coverage flags, i.e. -fprofile-instr-generate -fcoverage-mapping in CXX/C flags and
    -fprofile-instr-generate in linker flags. On Windows + clang-cl, configure with the
    Developer shell on PATH, e.g.:
        & 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation
        cmake -B build-coverage -G Ninja -DCMAKE_CXX_COMPILER=clang-cl ...
        cmake --build build-coverage

    The script:
      1. clears <ProfRawDir> (unless -KeepProfraw is given),
      2. sets LLVM_PROFILE_FILE to '<ProfRawDir>/%p-%m.profraw' and runs ctest,
      3. merges all .profraw via 'llvm-profdata merge -sparse',
      4. invokes 'llvm-cov show' for HTML and 'llvm-cov report' for a text summary,
         using every .exe / .dll under <BuildDir> as a -object input.

.PARAMETER BuildDir
    CMake build directory (must contain CTestTestfile.cmake).

.PARAMETER SourceRoot
    Repository root containing product sources. Default: parent of this script's directory.

.PARAMETER LlvmCov
    Path or name of llvm-cov[.exe]. Default: 'llvm-cov' (resolved via PATH).

.PARAMETER LlvmProfdata
    Path or name of llvm-profdata[.exe]. Default: 'llvm-profdata' (resolved via PATH).

.PARAMETER OutDir
    HTML report output dir. Default: <BuildDir>/cov-html

.PARAMETER ProfRawDir
    Where .profraw files are collected. Default: <BuildDir>/profraw

.PARAMETER ProfData
    Output path of the merged .profdata. Default: <BuildDir>/coverage.profdata

.PARAMETER Parallel
    CTest parallel job count. 0 = ctest decides (typically serial).

.PARAMETER KeepProfraw
    Do not wipe <ProfRawDir> before running ctest (accumulate across runs).

.PARAMETER FailOnTestError
    If set, propagate a non-zero ctest exit code instead of continuing to generate the
    report from whatever .profraw files were produced.

.PARAMETER IgnoreRegex
    Value forwarded to llvm-cov as -ignore-filename-regex. Default excludes the build
    tree, third-party code, Qt autogen / moc / qrc / ui_*.h artifacts.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string] $BuildDir = './build-coverage',

    [Parameter(Mandatory = $false)]
    [string] $SourceRoot = '',

    [Parameter(Mandatory = $false)]
    [string] $LlvmCov = 'llvm-cov',

    [Parameter(Mandatory = $false)]
    [string] $LlvmProfdata = 'llvm-profdata',

    [Parameter(Mandatory = $false)]
    [string] $OutDir = '',

    [Parameter(Mandatory = $false)]
    [string] $ProfRawDir = '',

    [Parameter(Mandatory = $false)]
    [string] $ProfData = '',

    [Parameter(Mandatory = $false)]
    [int] $Parallel = 0,

    [switch] $KeepProfraw,
    [switch] $FailOnTestError,

    [Parameter(Mandatory = $false)]
    [string] $IgnoreRegex = 'build|thirdparty|_autogen|moc_|qrc_|FlexLexer|ui_.*\.h|CompilerIdC'
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

if ([string]::IsNullOrWhiteSpace($OutDir))     { $OutDir     = Join-Path $BuildDir 'cov-html' }
if ([string]::IsNullOrWhiteSpace($ProfRawDir)) { $ProfRawDir = Join-Path $BuildDir 'profraw'  }
if ([string]::IsNullOrWhiteSpace($ProfData))   { $ProfData   = Join-Path $BuildDir 'coverage.profdata' }

if (-not $KeepProfraw -and (Test-Path -LiteralPath $ProfRawDir)) {
    Get-ChildItem -LiteralPath $ProfRawDir -Filter '*.profraw' -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Force -Path $ProfRawDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir     | Out-Null
$ProfRawDir = (Resolve-Path -LiteralPath $ProfRawDir).Path
$OutDir     = (Resolve-Path -LiteralPath $OutDir).Path

$llvmCovPath      = Resolve-ExePath -NameOrPath $LlvmCov
$llvmProfdataPath = Resolve-ExePath -NameOrPath $LlvmProfdata
$ctestPath        = Resolve-ExePath -NameOrPath 'ctest'

Write-Host "llvm-cov:        $llvmCovPath"
Write-Host "llvm-profdata:   $llvmProfdataPath"
Write-Host "SourceRoot:      $SourceRoot"
Write-Host "BuildDir:        $BuildDir"
Write-Host "OutDir:          $OutDir"
Write-Host "ProfRawDir:      $ProfRawDir"
Write-Host "ProfData:        $ProfData"
Write-Host ""

# Step 1: run ctest with per-process .profraw output (so multi-test runs don't collide).
$prevProfileEnv = $env:LLVM_PROFILE_FILE
$env:LLVM_PROFILE_FILE = Join-Path $ProfRawDir '%p-%m.profraw'
try {
    $ctestArgs = @('--test-dir', $BuildDir, '--output-on-failure')
    if ($Parallel -gt 0) {
        $ctestArgs += @('-j', "$Parallel")
    }
    Write-Host "ctest $($ctestArgs -join ' ')"
    & $ctestPath @ctestArgs
    $ctestExit = $LASTEXITCODE
}
finally {
    $env:LLVM_PROFILE_FILE = $prevProfileEnv
}

if ($ctestExit -ne 0) {
    if ($FailOnTestError) {
        Write-Host "ctest exited with $ctestExit; aborting (-FailOnTestError)." -ForegroundColor Red
        exit $ctestExit
    }
    Write-Host "ctest exited with $ctestExit; continuing to build coverage report from available .profraw." -ForegroundColor Yellow
}

# Step 2: merge .profraw -> .profdata
$profrawFiles = Get-ChildItem -LiteralPath $ProfRawDir -Filter '*.profraw' -ErrorAction SilentlyContinue
if (-not $profrawFiles -or $profrawFiles.Count -eq 0) {
    throw "No .profraw produced in $ProfRawDir. Confirm the build was compiled with -fprofile-instr-generate -fcoverage-mapping."
}
$totalMB = [Math]::Round((($profrawFiles | Measure-Object Length -Sum).Sum / 1MB), 2)
Write-Host ""
Write-Host "profraw files: $($profrawFiles.Count) (~$totalMB MB)"

$mergeArgs = @('merge', '-sparse') + ($profrawFiles | ForEach-Object { $_.FullName }) + @('-o', $ProfData)
& $llvmProfdataPath @mergeArgs
if ($LASTEXITCODE -ne 0) {
    throw "llvm-profdata merge failed (exit $LASTEXITCODE)."
}
Write-Host "merged -> $ProfData"

# Step 3: collect every .exe / .dll in the build tree (except CMake internals) as -object inputs.
$bins = Get-ChildItem -Path $BuildDir -Recurse -Include *.exe,*.dll -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notmatch '\\CMakeFiles\\|CompilerIdC|CompilerIdCXX' } |
        Sort-Object FullName
if (-not $bins -or $bins.Count -eq 0) {
    throw "No instrumented .exe / .dll found in $BuildDir."
}
Write-Host "binary inputs: $($bins.Count)"

$mainBin = $bins[0].FullName
$objArgs = @()
foreach ($b in ($bins | Select-Object -Skip 1)) {
    $objArgs += @('-object', $b.FullName)
}

$commonArgs = @($mainBin) + $objArgs + @(
    "-instr-profile=$ProfData",
    "-ignore-filename-regex=$IgnoreRegex"
)

# Step 4: HTML report
$showArgs = @('show') + $commonArgs + @(
    '-format=html',
    "-output-dir=$OutDir"
)
& $llvmCovPath @showArgs
if ($LASTEXITCODE -ne 0) {
    throw "llvm-cov show failed (exit $LASTEXITCODE)."
}

$indexHtml = Join-Path $OutDir 'index.html'
if (-not (Test-Path -LiteralPath $indexHtml)) {
    throw "Expected HTML report missing: $indexHtml"
}

# Step 5: text summary (also prints overall TOTAL row, useful for CI logs)
$reportArgs = @('report') + $commonArgs
Write-Host ""
Write-Host "===== llvm-cov report ====="
& $llvmCovPath @reportArgs
if ($LASTEXITCODE -ne 0) {
    throw "llvm-cov report failed (exit $LASTEXITCODE)."
}

# Step 6: lcov export for CI tools that consume it (optional but cheap).
$lcovPath = Join-Path $OutDir 'coverage.lcov'
$exportArgs = @('export') + $commonArgs + @('-format=lcov')
& $llvmCovPath @exportArgs > $lcovPath
if ($LASTEXITCODE -ne 0) {
    throw "llvm-cov export failed (exit $LASTEXITCODE)."
}

Write-Host ""
Write-Host "Coverage OK: $indexHtml"
Write-Host "Coverage OK: $lcovPath"
