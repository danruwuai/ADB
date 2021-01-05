adb wait-for-device root
adb remount
adb logcat -G 16m
adb logcat -c
adb logcat -v time > %~dp0\af_log.txt