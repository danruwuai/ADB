@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
echo ����AF_log����
CALL "%~dp0\..\af.bat"
timeout 4
echo +++++++++++++++++++++++++++++++++++++
echo �ر����
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
echo ����fullscan����
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.af.localfullscan 3
adb shell setprop vendor.debug.af.fullscan.step 1
timeout 2 >nul

:begin
echo +++++++++++++++++++++++++++++++++++++
echo �ر����
adb shell pkill camera*
timeout 5 >nul
echo �������
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
echo +++++++++++++++++++++++++++++++++++++
echo �ȴ�����
timeout 55
adb shell input keyevent 27
echo ������
echo +++++++++++++++++++++++++++++++++++++
echo ȷ��λ�ú����´����Խ�
pause 
goto begin