@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
CALL "E:\workspace\ADB\�������.bat"

echo +++++++++++++++++++++++++++++++++++++
echo �ȴ�5����
timeout /T 300

CALL "Fullscan.bat"

timeout 3>nul