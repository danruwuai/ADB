title Read_fullscan.bat
adb logcat -c
adb logcat -G 16m
adb logcat | findstr -i "Fullscan\]\[Peak"