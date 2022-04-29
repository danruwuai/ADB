@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
echo 输入AF_log开关
CALL "%~dp0\..\af.bat"
timeout 4
echo +++++++++++++++++++++++++++++++++++++
echo 关闭相机
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
echo 输入fullscan命令
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.af.localfullscan 3
adb shell setprop vendor.debug.af.fullscan.step 1
timeout 2 >nul

:begin
echo +++++++++++++++++++++++++++++++++++++
echo 关闭相机
adb shell pkill camera*
timeout 5 >nul
echo 开启相机
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
echo +++++++++++++++++++++++++++++++++++++
echo 等待拍照
timeout 55
adb shell input keyevent 27
echo 已拍照
echo +++++++++++++++++++++++++++++++++++++
echo 确定位置后重新触发对焦
pause 
goto begin