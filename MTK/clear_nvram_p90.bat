adb wait-for-device root
adb remount
adb shell rm -rf /data/nvram
adb shell rm -rf nvcfg
adb shell rm -r vendor/nvcfg/camera
adb shell rm -r vendor/nvcfg/*
adb shell pkill camera*
adb shell pkill camerahal
adb shell pkill cameraserver
adb shell pkill camerahalserver

adb shell rm -rf /mnt/vendor/nvcfg/camera
::adb root
::adb shell "ps -A | grep cameraserver | grep -v grep | sed 's/ [ ]*/:/g' | cut -d: -f2 | xargs kill -9"
::adb reboot
::pause
adb shell killall -9 com.android.camera
timeout 5 > nul