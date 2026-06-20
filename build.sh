#!/bin/bash
set -e

echo "Building Zygisk module..."

NDK_BUILD="ndk-build"
if [ -n "$ANDROID_NDK_HOME" ]; then
    NDK_BUILD="$ANDROID_NDK_HOME/ndk-build"
elif [ -n "$ANDROID_NDK_ROOT" ]; then
    NDK_BUILD="$ANDROID_NDK_ROOT/ndk-build"
fi

"$NDK_BUILD" -C zygisk/jni -j$(nproc)

echo "Packaging..."
rm -rf out zygisk_adb_spoofer.zip
mkdir -p out/zygisk

cp -r module/* out/
cp zygisk/libs/arm64-v8a/libadb_spoofer.so out/zygisk/arm64-v8a.so

cd out
zip -r9 ../zygisk_adb_spoofer.zip ./*
cd ..

echo "Done! -> zygisk_adb_spoofer.zip"