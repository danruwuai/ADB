adb wait-for-device root
adb remount
adb shell setprop vendor.debug.tnc.perframe_prop.enable 1
adb shell setprop vendor.debug.tnc.log.enable 1
adb shell setprop vendor.debug.tnc.log.level 3
adb shell setprop vendor.debug.tnc.perf.enable 1
adb shell setprop vendor.debug.tnc.algo.enable 1
adb shell setprop vendor.debug.tnc.crop.enable 1
adb shell setprop vendor.debug.tnc.db.enable 1
adb shell setprop vendor.debug.tnc.bce.enable 1
adb shell setprop vendor.debug.tnc_alg.log.level 3
adb shell setprop vendor.debug.tnc.apidump.enable 1
adb shell setprop vendor.debug.tnc.dump.enable 1
adb shell setprop vendor.debug.tnc.dbg.enable 1
adb shell setprop vendor.debug.tnc.gt.tps.en 1
adb shell setprop vendor.debug.tnc.hwgtm.enable 1
adb shell setprop vendor.debug.tnc.drawblk.enable 0
adb shell setprop vendor.debug.tnc.buf.dump 1
adb shell setprop vendor.debug.tnc.buf.enable 1
::/data/vendor/camera_dump/tncdumpdata