adb root
adb shell setenforce 0
adb remount
adb disable-verity

adb shell "echo enable3ADebugData=TRUE >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell "echo enableTuningMetadata= TRUE >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell "echo dumpSensorEEPROMData = TRUE  >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell "echo autoImageDumpMask=4 >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell "echo offlineImageDumpOnly=TRUE >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell "echo reprocessDump=TRUE >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell "echo mfnrDump=TRUE >> /vendor/etc/camera/camxoverridesettings.txt"
::adb shell "echo autoImageDump=TRUE >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell cat /vendor/etc/camera/camxoverridesettings.txt

adb shell ls -l vendor/lib64/camera/*tuned* 

adb shell  setprop persist.vendor.camera.capture.photosoliddump 1
::adb shell  getprop persist.vendor.camera.capture.photosoliddump


echo ---------restarting cameraserver-------------
adb shell input keyevent 3

for /f "tokens=2" %%a in ('"adb shell ps -A| findstr android.hardware.camera.provider@2.4-service_64"') do adb shell kill  %%a

timeout /t 5 /nobreak>nul
::pause