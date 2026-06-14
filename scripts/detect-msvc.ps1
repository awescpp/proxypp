$ErrorActionPreference = "Stop"

function Get-MsvcInfo {
  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

  if (-not (Test-Path $vswhere)) {
    throw "Cannot find vswhere.exe. Please install Visual Studio Installer."
  }

  $vsInstallPath = & $vswhere `
    -latest `
    -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath

  if (-not $vsInstallPath) {
    throw "Cannot find Visual Studio installation with MSVC C++ tools."
  }

  $vsDisplayName = & $vswhere `
    -latest `
    -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property displayName

  $vsVersion = & $vswhere `
    -latest `
    -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationVersion

  $vsDevCmd = Join-Path $vsInstallPath "Common7\Tools\VsDevCmd.bat"

  if (-not (Test-Path $vsDevCmd)) {
    throw "Cannot find VsDevCmd.bat at: $vsDevCmd"
  }

  return [PSCustomObject]@{
    DisplayName = $vsDisplayName
    Version     = $vsVersion
    InstallPath = $vsInstallPath
    DevCmd      = $vsDevCmd
  }
}

function Show-MsvcInfo {
  param (
    [Parameter(Mandatory = $true)]
    $MsvcInfo
  )

  Write-Host "Using Visual Studio:"
  Write-Host "  Name:    $($MsvcInfo.DisplayName)"
  Write-Host "  Version: $($MsvcInfo.Version)"
  Write-Host "  Path:    $($MsvcInfo.InstallPath)"
  Write-Host "  DevCmd:  $($MsvcInfo.DevCmd)"
  Write-Host ""
}

function Invoke-WithMsvc {
  param (
    [Parameter(Mandatory = $true)]
    [string]$Command
  )

  $msvcInfo = Get-MsvcInfo
  Show-MsvcInfo $msvcInfo

  cmd /v:on /c "call `"$($msvcInfo.DevCmd)`" -arch=x64 && echo MSVC Developer Environment: !VSCMD_VER! && echo MSVC Toolset Version: !VCToolsVersion! && echo Target Arch: !VSCMD_ARG_TGT_ARCH! && $Command"

  if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
  }
}