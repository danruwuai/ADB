adb wait-for-device root
adb remount
adb wait-for-device
adb shell cat /proc/kmsg > af_kernel.txt