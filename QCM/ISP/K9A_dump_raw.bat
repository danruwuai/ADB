adb root
adb remount

adb shell setprop persist.vendor.camera.autoImageDump 1
adb shell setprop persist.vendor.camera.autoInputImageDumpMask 4
::adb shell setprop persist.vendor.camera.betRegDumpMask 0x200000
::adb shell setprop persist.vendor.camera.betRegDumpMask 0xFFFFFFFF
adb shell setprop persist.vendor.camera.betRegDumpMask 0xF02F20A4
adb shell setprop persist.vendor.camera.autoImageDumpMask 0
adb shell "echo enableTuningMetadata=TRUE >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell "echo dumpDebugDataEveryProcessResult=1 >> /vendor/etc/camera/camxoverridesettings.txt"
adb shell setprop persist.vendor.camera.dumpInputatOutput TRUE
adb shell setprop persist.vendor.camera.rawDump 1
adb shell setprop persist.vendor.camera.mfnrDump 1
adb shell setprop persist.vendor.camera.offlineImageDumpOnly 1 
adb shell setprop persist.vendor.camera.debugdata.enable3A 1

pause
rem for /f "tokens=2" %%a in ('"adb shell ps -A| findstr mediaserver"') do adb shell kill %%a
rem for /f "tokens=2" %%a in ('"adb shell ps -A| findstr mm-qcamera-daemon"') do adb shell kill %%a
for /f "tokens=2" %%a in ('"adb shell ps -A| findstr android.hardware.camera.provider@2.4-service_64"') do adb shell kill  %%a
rem for /f "tokens=2" %%a in ('"adb shell ps -A| findstr camera"') do adb shell kill  %%a
timeout /t 5 /nobreak>nul
