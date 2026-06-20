#!/bin/bash
set -e

echo "Building Zygisk module..."
ndk-build -C zygisk/jni -j$(nproc)

echo "Packaging..."
rm -rf out zygisk_adb_spoofer.zip
mkdir -p out/zygisk

cp -r module/* out/
cp zygisk/libs/arm64-v8a/libadb_spoofer.so out/zygisk/arm64-v8a.so

cd out
zip -r9 ../zygisk_adb_spoofer.zip ./*
cd ..

echo "Done! -> zygisk_adb_spoofer.zip"