adb wait-for-device root
adb wait-for-device remount
adb pull /data/vendor/camera_dump %~dp0\camera_dump
timeout 5 >nul