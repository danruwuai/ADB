adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.pd.vpu.enable 1
adb shell setprop debug.af.log.enable 1
adb shell setprop debug.af.enable 1
adb shell setprop vendor.debug.af.log.enable 1
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.af_mgr.enable 1
adb shell setprop vendor.debug.pd.enable 1
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.pd.vpu.log.enable 1
adb shell setprop vendor.debug.camera.af.draw.lens 1
adb shell setprop debug.cam.drawid 1
adb shell setprop vendor.debug.drv.ccu_drv 4
adb shell setprop vendor.debug.ccuif.ccu_drv 4
adb shell setprop vendor.drv.ccu.log.ccuflowaf 1
adb shell setprop vendor.drv.ccu.log.ccuafalgo 1
adb shell setprop vendor.debug.ae.log.enable 1
timeout 2 > nul