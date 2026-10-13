<#
.SYNOPSIS
    Automated build script for compiling BookBazzar Inno Setup Windows Installer.
.DESCRIPTION
    Detects Inno Setup Compiler (ISCC.exe) across standard registry and system paths,
    compiles BookBazzar.iss, and validates that frontend/dist/BookBazzar-Setup-0.1.0.exe
    is created without tampering with existing release ZIP files.
#>

[CmdletBinding()]
param()

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..\..")
$IssPath = Join-Path $ScriptDir "BookBazzar.iss"
$DistDir = Join-Path $ProjectRoot "frontend\dist"
$SetupExe = Join-Path $DistDir "BookBazzar-Setup-0.1.0.exe"
$ExistingZip = Join-Path $DistDir "BookBazzar-v0.1-Windows-x64.zip"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " BookBazzar Windows Installer Build Utility " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "Script: $IssPath"
Write-Host "Target: $SetupExe"

# 1. Search for Inno Setup Compiler (ISCC.exe)
$IsccCandidates = @(
    (Get-Command iscc.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source),
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe",
    "C:\Program Files (x86)\Inno Setup 5\ISCC.exe",
    "C:\Program Files\Inno Setup 5\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
)

# Registry search for Inno Setup installation directory
$regUninstallPaths = @(
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1",
    "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1",
    "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1"
)

foreach ($regPath in $regUninstallPaths) {
    if (Test-Path $regPath) {
        $installLocation = (Get-ItemProperty -Path $regPath -Name "InstallLocation" -ErrorAction SilentlyContinue).InstallLocation
        if ($installLocation) {
            $IsccCandidates += (Join-Path $installLocation "ISCC.exe")
        }
    }
}

$IsccPath = $null
foreach ($candidate in $IsccCandidates) {
    if ($candidate -and (Test-Path $candidate)) {
        $IsccPath = $candidate
        break
    }
}

# 2. Check if ISCC was found
if (-not $IsccPath) {
    Write-Host "`n[NOTICE] Inno Setup Compiler (ISCC.exe) was not found on this system." -ForegroundColor Yellow
    Write-Host "`nTo compile the installer, perform ONE of the following steps:" -ForegroundColor White
    Write-Host "  Option A (winget):" -ForegroundColor Cyan
    Write-Host "      winget install --id JRSoftware.InnoSetup -e --source winget"
    Write-Host "  Option B (Official Installer):" -ForegroundColor Cyan
    Write-Host "      Download and install Inno Setup 6 from https://jrsoftware.org/isdl.php"
    Write-Host "`nAfter installation, run:" -ForegroundColor White
    Write-Host "  & `"$ScriptDir\build_installer.ps1`"" -ForegroundColor Green
    Write-Host "Or run the compiler directly:" -ForegroundColor White
    Write-Host "  `"C:\Program Files (x86)\Inno Setup 6\ISCC.exe`" `"$IssPath`"" -ForegroundColor Green
    exit 1
}

Write-Host "`n[FOUND] Inno Setup compiler detected at: $IsccPath" -ForegroundColor Green

# 3. Verify packaged source folder exists
$SourceDist = Join-Path $DistDir "BookBazzar"
if (-not (Test-Path $SourceDist)) {
    Write-Error "[ERROR] Packaged distribution folder not found at: $SourceDist"
    exit 1
}

# 4. Compile the installer
Write-Host "`n[BUILD] Compiling BookBazzar.iss with Inno Setup..." -ForegroundColor Cyan
$compileProcess = Start-Process -FilePath $IsccPath -ArgumentList "`"$IssPath`"" -NoNewWindow -Wait -PassThru

if ($compileProcess.ExitCode -ne 0) {
    Write-Error "[ERROR] Inno Setup compilation failed with exit code $($compileProcess.ExitCode)."
    exit $compileProcess.ExitCode
}

# 5. Verify build outputs
if (Test-Path $SetupExe) {
    $setupItem = Get-Item $SetupExe
    $sizeMb = [math]::Round($setupItem.Length / 1MB, 2)
    Write-Host "`n[SUCCESS] Installer created successfully!" -ForegroundColor Green
    Write-Host "  Path: $($setupItem.FullName)"
    Write-Host "  Size: $sizeMb MB"
    Write-Host "  Modified: $($setupItem.LastWriteTime)"
} else {
    Write-Error "[ERROR] Expected installer binary not found at $SetupExe"
    exit 1
}

# 6. Verify existing ZIP preservation
if (Test-Path $ExistingZip) {
    $zipItem = Get-Item $ExistingZip
    $zipSizeMb = [math]::Round($zipItem.Length / 1MB, 2)
    Write-Host "`n[PRESERVED] Existing release archive intact: $ExistingZip ($zipSizeMb MB)" -ForegroundColor Green
}
