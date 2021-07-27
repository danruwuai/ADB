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
adb shell setprop presist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.pd.vpu.log.enable 1
adb shell setprop vendor.debug.camera.af.draw.lens 1
adb shell setprop debug.cam.drawid 1
adb shell setprop debug.cam.draw.ctrl 'DAC,MAGIC,ISO'
adb shell setprop vendor.debug.af.localfullscan 3
adb shell setprop vendor.debug.af.localfullscan.auto 1
adb shell setprop vendor.debug.hal3a.taskmgr 1
adb shell svc power stayon true
adb shell settings put system screen_off_timeout 1800000
adb shell sync
adb shell sync
adb shell sync
adb shell pkill camera*