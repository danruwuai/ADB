adb root
adb remount
adb shell "echo 1 > /sys/module/camera_eeprom/parameters/cam_eeprom_debug"
adb shell pkill camera*
pause
adb shell “cat /proc/kmsg | grep -i "CAM_CAL" ”