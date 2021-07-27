title Read_otp.bat
adb logcat -c
adb logcat -G 16m
adb logcat | findstr -i "OTP"