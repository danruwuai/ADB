adb wait-for-device root
adb remount
adb shell setprop vendor.debug.gma.log.enable 1
adb shell setprop vendor.debug.gma.log.level 3
adb shell setprop vendor.debug.gma.perf.enable 1
adb shell setprop vendor.autotesttool.enable 1
adb shell setprop vendor.debug.gma.algo.enable 1
adb shell setprop vendor.debug.gma.hdr10plus.enable 0
adb shell setprop vendor.debug.gma.db.enable 1
adb shell setprop vendor.debug.gma.hdr10plus.dynamic.enable 0
adb shell setprop vendor.debug.gma_alg.log.level 3
adb shell setprop vendor.debug.gma.apidump.enable 1
adb shell setprop vendor.debug.gma.curve.print 1
