@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0

adb shell setprop vendor.debug.af_motor.disable 1
rem Postion¶ÔÓ¦Î»ÖÃ
adb shell setprop vendor.debug.af_motor.position 117

timeout 4 >nul