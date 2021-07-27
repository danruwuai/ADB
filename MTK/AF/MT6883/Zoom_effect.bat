@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
cd %~dp0
cd ../../..
CALL "启动相机.bat"

echo +++++++++++++++++++++++++++++++++++++
echo 等待5分钟
timeout /T 300
echo 关闭相机
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.camera.dbginfo 1
echo L_bound INF
adb shell setprop vendor.debug.af.zeccalib 1
echo +++++++++++++++++++++++++++++++++++++
echo 开启相机
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
::timeout 3
::echo 点击屏幕
::adb shell input tap 550 1100
echo 延时40s
timeout 40
echo 拍照
adb shell input keyevent 27
timeout 3 >nul
echo +++++++++++++++++++++++++++++++++++++
echo 关闭相机
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
echo R_bound_bound Mac
adb shell setprop vendor.debug.af.zeccalib 2
echo +++++++++++++++++++++++++++++++++++++
echo 开启相机
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
::timeout 3
::echo 点击屏幕
::adb shell input tap 550 1100
echo 延时40s
timeout 40
echo 拍照
adb shell input keyevent 27
timeout 3 >nul