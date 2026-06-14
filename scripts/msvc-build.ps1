$ErrorActionPreference = "Stop"

. "$PSScriptRoot/detect-msvc.ps1"

$buildDir = $env:CMAKE_BUILD_DIR
if (-not $buildDir) {
  $buildDir = "cmake-build-debug"
}

Invoke-WithMsvc "cmake --build $buildDir"