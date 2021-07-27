adb root 
adb remount

adb shell rm -rf /sdcard/DCIM/Camera/*
adb shell rm -rf /data/vendor/camera_dump/*
adb shell rm -rf /data/log/mtklog/mobilelog/*
adb shell rm -rf /sdcard/Pictures/Screenshots/*

adb shell mkdir /data/vendor/camera_dump -p
adb shell setprop vendor.debug.cameng.force_sensormode 2

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
adb shell setprop vendor.debug.camera.cfg.ProcRaw 1
adb shell setprop vendor.debug.camera.raw.type 0

adb shell setprop vendor.debug.mfll.force 0
adb shell setprop vendor.mfll.force 0
adb shell setprop vendor.mediatek.mfll.force 0
adb shell setprop vendor.debug.mfll.iso 10000

adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.ae_mgr.lock 1
adb shell setprop vendor.debug.ae_mgr.preview.update 1
adb shell setprop vendor.debug.ae_mgr_ccu.enable 0

adb shell setprop vendor.debug.ae_mgr.shutter 22500
adb shell setprop vendor.debug.ae_mgr.sensorgain 2048
adb shell setprop vendor.debug.ae_mgr.ispgain 1024
adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause