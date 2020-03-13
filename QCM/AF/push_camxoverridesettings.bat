@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount
adb shell umount -nlf /vendor/etc
adb shell umount -nlf /vendor/lib64
adb shell umount -nlf /vendor/lib
adb push %~dp0\camxoverridesettings.txt /vendor/etc/camera/
adb shell cat /vendor/etc/camera/camxoverridesettings.txt
adb shell setprop persist.camera.exif3a.decompress 1
timeout 5 >nul
adb shell killall -9 android.hardware.camera.provider@2.4-service_64