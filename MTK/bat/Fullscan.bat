@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0

echo �ر����
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
echo ����adb����
adb shell setprop vendor.debug.af.dumpdata 1
timeout 2 >nul
echo +++++++++++++++++++++++++++++++++++++
echo �������
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
echo ����fullscan����
adb shell setprop vendor.debug.af.fullscan.step 1

:begin
adb shell setprop vendor.debug.af.trigger 0
timeout 5 >nul
adb shell setprop vendor.debug.af.trigger 1
echo +++++++++++++++++++++++++++++++++++++
echo �ȴ�����
timeout 52 >nul
adb shell input keyevent 27
echo ������
echo +++++++++++++++++++++++++++++++++++++
echo ȷ��λ�ú����´����Խ�
pause 
goto begin