adb root
adb wait-for-device
timeout /t 3

adb shell input keyevent 4
adb shell input keyevent 4
adb shell "logcat -c "
adb shell "logcat -G 30m"

: ManualAE OFF
adb shell setprop vendor.debug.ae_mgr.preview.update 0
adb shell setprop vendor.debug.ae_mgr.lock 0
adb shell setprop vendor.debug.ae_mgr.enable 0
adb shell setprop vendor.debug.ae.manual.isp 0
adb shell setprop vendor.debug.ae.manual.afe 0
adb shell setprop vendor.debug.ae.manual.exposure 0

mode 110,30
adb shell "logcat |grep updateAEInfo2ISP"