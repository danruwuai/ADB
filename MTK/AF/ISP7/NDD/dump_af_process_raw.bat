@echo off

echo +++++++++++++++++++++++++++++++++++++
echo 关闭相机
echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb remount
adb shell pkill camera*
timeout 5 >nul
adb shell setenforce 0
adb shell setprop vendor.debug.ndd.thdnum 2
adb shell setprop vendor.debug.ndd.subdir 0
adb shell setprop vendor.debug.camera.pipemgr.bufdump 1
adb shell setprop vendor.debug.camera.imBuf.enFC 1
adb shell setprop vendor.debug.file_cache.storage_size 5368709120
adb shell setprop vendor.debug.camera.scenarioRecorder.enable 1
adb shell setprop vendor.debug.camera.afdump 1
adb shell pkill camera
adb shell logcat -c
adb shell logcat -G 30M
adb push preview_ndd.cfg data/vendor/camera_dump
adb shell chmod 777 data/vendor/camera_dump/preview_ndd.cfg
adb shell setprop vendor.debug.ndd.cfgpath "/data/vendor/camera_dump/preview_ndd.cfg"

echo 打开相机?
echo +++++++++++++++++++++++++++++++++++++
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
echo +++++++++++++++++++++++++++++++++++++

adb shell rm -rf data/vendor/camera_dump/default_dump/*
adb shell setprop vendor.debug.ndd.prv_ready 1
timeout 2 >nul
adb shell setprop vendor.debug.ndd.prv_ready 0
adb pull data/vendor/camera_dump/default_dump/ %cd%\%mydir%
pause