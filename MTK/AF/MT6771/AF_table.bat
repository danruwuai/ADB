@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
cd %~dp0
cd ../../..
CALL "�������.bat"

echo +++++++++++++++++++++++++++++++++++++
echo �ȴ�5����
timeout /T 300

CALL "%~dp0\Fullscan.bat"

timeout 3>nul