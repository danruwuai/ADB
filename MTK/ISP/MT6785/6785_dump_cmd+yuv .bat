adb root
adb remount
adb shell setenforce 0
adb shell rm -rf /data/vendor/camera_dump
adb shell mkdir /data/vendor/camera_dump -p
adb shell setprop vendor.debug.camera.dump.campipe 1
adb shell setprop vendor.debug.camera.bss.dump 1 
adb shell setprop vendor.debug.camera.p2.dump 1
adb shell setprop vendor.debug.camera.SttBufQ.enable 60
adb shell setprop vendor.debug.camera.dump.JpegNode 1
adb shell setprop vendor.debug.camera.AAO.dump 1
adb shell setprop vendor.debug.camera.dump.p2 1
adb shell setprop vendor.debug.mfll.dump.raw 1
adb shell setprop vendor.debug.camera.cammode 1
adb shell setprop vendor.debug.camera.dump.p2.profile 0
adb shell setprop vendor.debug.camera.dump.p2.debuginfo 2
adb shell setprop vendor.debug.camera.dump.lsc2.preview 1 
adb shell setprop vendor.debug.camera.dump.p1.lsc 0
adb shell setprop vendor.debug.camera.copy.p1.lsc 1
adb shell setprop vendor.debug.dumpregister.enable 2
adb shell setprop vendor.debug.camera.dump.p2.out 0
adb shell setprop vendor.debug.camera.dump.p2.in 5
adb shell setprop vendor.debug.camera.dumpin.en 1
adb shell setprop vendor.debug.camera.ufo_off 1
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.camera.mdp.dump 1
adb shell setprop vendor.dp.dumpreg.enable 2 
adb shell setprop vendor.debug.camera.dump.en 1



::adb shell setprop vendor.debug.camera.p2.dump.only_raw 1  
::adb shell setprop vendor.debug.camera.upkraw.dump 1
::dumpYUV
#adb shell setenforce 0
#adb shell setprop vendor.debug.camera.p2.dump 1
#adb shell setprop vendor.debug.camera.dump.campipe 1 
#adb shell setprop vendor.debug.camera.mfnr.yuv.dump 1 
#adb shell setprop vendor.mfll.dump.mixer 1
#adb shell setprop vendor.mfll.dump.mixer.out 1
pause