@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo �����ֻ�adb
adb wait-for-device root
adb wait-for-device remount
echo ��������
adb shell svc power stayon true
adb shell input keyevent 82
timeout 2 >nul
adb shell input keyevent 82
timeout 2 >nul

REM Start camera
echo ����Camera
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
echo ������Ļ�������
adb shell settings put system screen_brightness_mode 0
adb shell settings put system screen_off_timeout 600000000
adb shell settings put system screen_brightness 1024
timeout 2 >nul