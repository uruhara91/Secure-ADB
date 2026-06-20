$ErrorActionPreference = "Stop"

Write-Host "Building Zygisk module..." -ForegroundColor Cyan
ndk-build.cmd -C zygisk/jni -j$env:NUMBER_OF_PROCESSORS

Write-Host "Packaging..." -ForegroundColor Cyan
if (Test-Path "out") { Remove-Item -Recurse -Force "out" }
if (Test-Path "zygisk_adb_spoofer.zip") { Remove-Item -Force "zygisk_adb_spoofer.zip" }

New-Item -ItemType Directory -Force -Path "out/zygisk" | Out-Null

Copy-Item -Recurse -Force "module/*" "out/"
Copy-Item -Force "zygisk/libs/arm64-v8a/libadb_spoofer.so" "out/zygisk/arm64-v8a.so"

Compress-Archive -Path "out/*" -DestinationPath "zygisk_adb_spoofer.zip" -Force

Write-Host "Done! -> zygisk_adb_spoofer.zip" -ForegroundColor Green