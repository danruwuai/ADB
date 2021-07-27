::打开3A exif信息,重启后命令失效
adb root
adb remount
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.camera.dump.p2.debuginfo 1
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop debug.cam.drawid 1
adb shell setprop debug.cam.draw.ctrl 'DAC,MAGIC,ISO'
adb shell stop camerahalserver
adb shell start camerahalserver
::pause
timeout 2 >nul