# Zygisk ADB Spoofer

A highly optimized Zygisk module to spoof `sys.usb.config` and `persist.sys.usb.config` properties natively. This module bypasses strict ADB / USB Debugging detectors (like Duck Detector, Momo, etc.) by intercepting both legacy `__system_property_get` and modern `__system_property_read_callback`.

## Features
- **Zero-Allocation**: No `malloc` or heap memory leaks during hot-path execution.
- **Double Barrier Hook**: Intercepts both legacy and modern Android properties API.
- **Targeted Injection**: Only injects into processes defined in `target.txt`.
- **arm64-v8a Optimized**: Built with LTO, strict aliasing, and no STL overhead.

## How to Configure
Edit the `module/target.txt` file inside the ZIP or before flashing to add your target application package names (one package per line).

## Requirements
- Android 8.0+ (API 26+)
- arm64-v8a device
- Magisk with Zygisk enabled, KernelSU with ZygiskNext, or APatch.