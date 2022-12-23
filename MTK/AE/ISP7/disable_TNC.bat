adb root

adb shell setprop vendor.debug.aaa.perframe_prop.enable 1
adb shell setprop vendor.debug.tnc.algo.enable 0
adb shell setprop vendor.debug.tnc.log.level 65535
adb shell setprop vendor.debug.tnc_alg.log.level 1

adb shell pkill camera*