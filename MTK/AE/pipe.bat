adb wait-for-device root
adb remount

adb shell setprop vendor.debug.ae.pipe.log.enable 1
adb shell setprop vendor.debug.ae.pipe.log.level 255
adb shell setprop vendor.debug.ae.pipe.perf.enable 1
adb shell setprop vendor.debug.ae.pipe.logi.enable 1
adb shell setprop vendor.algodbg.atrace.enable 1
adb shell pkill camera*