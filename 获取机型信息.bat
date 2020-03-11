@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount
echo +++++++++++++++++++++++++++++++++++++
echo 手机机型 :
adb shell getprop ro.product.model
echo +++++++++++++++++++++++++++++++++++++
echo Android版本 :
adb shell getprop ro.build.version.release
echo +++++++++++++++++++++++++++++++++++++
echo 手机解锁状态 ：
adb shell getprop ro.boot.vbmeta.device_state
echo +++++++++++++++++++++++++++++++++++++
echo 屏幕分辨率 ：
adb shell wm size
echo +++++++++++++++++++++++++++++++++++++
echo 屏幕密度 ：
adb shell wm density
echo +++++++++++++++++++++++++++++++++++++
echo CPU信息 ：
adb shell cat /proc/cpuinfo | findstr Hardware
echo +++++++++++++++++++++++++++++++++++++
pause