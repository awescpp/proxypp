$ErrorActionPreference = "Stop"

. "$PSScriptRoot/detect-msvc.ps1"

$buildDir = $env:CMAKE_BUILD_DIR
if (-not $buildDir)
{
    $buildDir = "out/build/windows-msvc-debug-local"
}

Invoke-WithMsvc "cmake --build $buildDir && ctest --test-dir $buildDir --output-on-failure"
