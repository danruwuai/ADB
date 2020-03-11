@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount

echo 进入刷机模式

adb reboot edl
timeout 2 >nul
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount