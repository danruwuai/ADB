adb root
adb remount
adb shell setprop persist.vendor.hal3a.log_level 4
adb shell setprop vendor.debug.awb_log.enable 1
adb shell setprop vendor.debug.awb_mgr.enable 1
adb shell setprop vendor.debug.awb.enable 1
