#!/system/bin/sh

ui_print "********************************"
ui_print "      Zygisk ADB Spoofer       "
ui_print "********************************"

CURRENT_SDK="$(getprop ro.build.version.sdk)"
CURRENT_ABIS="$(getprop ro.product.cpu.abilist)"

ui_print "* SDK: $CURRENT_SDK"
ui_print "* ABI list: $CURRENT_ABIS"

# Memastikan perangkat mendukung arm64-v8a
printf '%s' "$CURRENT_ABIS" | grep -q 'arm64-v8a' || abort "! arm64-v8a is required"

[ -f "$MODPATH/zygisk/arm64-v8a.so" ] || abort "! Missing arm64 Zygisk library"

if [ "$APATCH" = "true" ] || [ "$KERNELPATCH" = "true" ]; then
  ui_print "* APatch/FolkPatch-compatible manager detected"
  ui_print "* Zygisk Next or ReZygisk is required"
elif [ -n "$KSU" ]; then
  ui_print "* KernelSU-compatible manager detected"
  ui_print "* Zygisk Next or ReZygisk is required"
else
  ui_print "* Magisk-compatible manager detected"
  ui_print "* Enable built-in Zygisk, Zygisk Next, or ReZygisk"
fi

ui_print "* Target package: Duck Detector (can be changed in code)"
ui_print "* Reboot is required after installation"

set_perm_recursive "$MODPATH" 0 0 0755 0644