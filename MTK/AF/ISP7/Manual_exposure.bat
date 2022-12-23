adb wait-for-device root
adb remount
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.ae_mgr.lock 1
adb shell setprop vendor.debug.ae_mgr.preview.update 1
adb shell pkill camera*

adb shell setprop vendor.debug.ae_mgr.shutter 400000
adb shell setprop vendor.debug.ae_mgr.sensorgain 16384
adb shell setprop vendor.debug.ae_mgr.ispgain 4096

