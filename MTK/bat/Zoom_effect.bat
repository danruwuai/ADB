@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
echo +++++++++++++++++++++++++++++++++++++
echo L_bound INF
adb shell setprop vendor.debug.af.zeccalib 1
echo R_bound_bound Mac
adb shell setprop vendor.debug.af.zeccalib 2

timeout 3 >nul