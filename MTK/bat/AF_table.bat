@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
CALL "E:\workspace\ADB\启动相机.bat"

echo +++++++++++++++++++++++++++++++++++++
echo 等待5分钟
timeout 300 >nul

CALL "Fullscan.bat"

timeout 3>nul