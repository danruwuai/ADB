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
echo �ر����
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.camera.dbginfo 1
echo L_bound INF
adb shell setprop vendor.debug.af.zeccalib 1
echo +++++++++++++++++++++++++++++++++++++
echo �������
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
::timeout 3
::echo �����Ļ
::adb shell input tap 550 1100
echo ��ʱ40s
timeout 40
echo ����
adb shell input keyevent 27
timeout 3 >nul
echo +++++++++++++++++++++++++++++++++++++
echo �ر����
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
echo R_bound_bound Mac
adb shell setprop vendor.debug.af.zeccalib 2
echo +++++++++++++++++++++++++++++++++++++
echo �������
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
::timeout 3
::echo �����Ļ
::adb shell input tap 550 1100
echo ��ʱ40s
timeout 40
echo ����
adb shell input keyevent 27
timeout 3 >nul