@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
CALL "E:\workspace\ADB\启动相机.bat"

echo +++++++++++++++++++++++++++++++++++++
echo 等待5分钟
timeout 300 >nul

echo +++++++++++++++++++++++++++++++++++++
echo 输入adb命令
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.af.frame 1
adb shell sync
adb shell sync
adb shell pkill camera*
timeout 3 >nul

echo +++++++++++++++++++++++++++++++++++++
echo 打开相机
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0


echo +++++++++++++++++++++++++++++++++++++
echo 触发拍照
:takepicture
adb shell input keyevent 27
pause
echo +++++++++++++++++++++++++++++++++++++
echo 触发拍照
goto takepicture 