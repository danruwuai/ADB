@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
CALL "E:\workspace\ADB\�������.bat"

echo +++++++++++++++++++++++++++++++++++++
echo �ȴ�5����
timeout 300 >nul

echo +++++++++++++++++++++++++++++++++++++
echo ����adb����
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.af.frame 1
adb shell sync
adb shell sync
adb shell pkill camera*
timeout 3 >nul

echo +++++++++++++++++++++++++++++++++++++
echo �����
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0


echo +++++++++++++++++++++++++++++++++++++
echo ��������
:takepicture
adb shell input keyevent 27
pause
echo +++++++++++++++++++++++++++++++++++++
echo ��������
goto takepicture 