adb root
adb remount
adb shell mkdir /data/vendor/camera_dump/shading_otp
adb shell rm /data/vendor/camera_dump/shading_otp
adb pull /data/vendor/camera_dump/shading_otp 
adb shell setprop vendor.debug.lsc_mgr.log 1 
adb shell setprop vendor.debug.lsc_mgr.dump 1
adb shell pkill camera*
#打开相机后再按回车
pause
adb pull /data/vendor/camera_dump/shading_otp %cd%
pause