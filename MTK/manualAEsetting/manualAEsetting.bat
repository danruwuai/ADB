: ManualAE ON
adb shell setprop vendor.debug.ae_mgr.preview.update 1
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.ae_mgr.lock 0

:CCU AE
adb shell setprop vendor.debug.ae.manual.exposure 10000
adb shell setprop vendor.debug.ae.manual.afe 1024
adb shell setprop vendor.debug.ae.manual.isp 4096