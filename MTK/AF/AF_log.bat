@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0

echo ¿ªÆôAF LOG
adb shell setprop vendor.debug.ae.enable 9
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.af.log.enable 1
adb shell setprop vendor.debug.af_mgr.enable 1
adb shell setprop vendor.debug.hal3av3.log 263
adb shell setprop vendor.debug.pd.enalbe 1
adb shell setprop vendor.debug.aaa_state.enable 1
adb logcat -G 16m
adb logcat -c

timeout 2 >nul