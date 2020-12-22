adb wait-for-device root
adb remount
adb logcat -G 16m
adb logcat -c
adb logcat > %~dp0\af_log.txt