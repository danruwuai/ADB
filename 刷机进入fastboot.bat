@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo �����ֻ�adb
adb wait-for-device root
adb wait-for-device remount

echo ����ˢ��ģʽ

adb reboot edl
timeout 2 >nul
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo �����ֻ�adb
adb wait-for-device root
adb wait-for-device remount