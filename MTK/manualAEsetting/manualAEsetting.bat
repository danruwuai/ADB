: ManualAE ON
adb shell setprop vendor.debug.ae_mgr.preview.update 1
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.ae_mgr.lock 0

:CCU AE
adb shell setprop vendor.debug.ae.manual.exposure 400000000
adb shell setprop vendor.debug.ae.manual.afe 16384
adb shell setprop vendor.debug.ae.manual.isp 16384