adb root
adb shell setenforce 0
adb shell setprop vendor.nrcore.dump.enable 2
adb shell setprop "persist.vendor.camera.aiasd" 128
adb shell setprop "persist.vendor.camera.aiasdValue" 1
adb shell logcat -G 200M
adb shell pkill camera*