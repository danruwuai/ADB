adb wait-for-device
adb root
adb shell setenforce 0
rem adb shell setprop persist.vendor.mtk.camera.log_level 3
rem adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop debug.cam.drawid 1
adb shell setprop vendor.debug.aaa.pvlog.enable 1
adb shell setprop vendor.debug.ae_mgr.log.level 3
adb shell setprop vendor.debug.aesync_mgr.log.enable 1
adb shell setprop vendor.debug.ltm.dump.full 1
adb shell setprop vendor.debug.stt.aao_dump.enable 1

rem LTM Debug：
adb shell setprop vendor.debug.ltm.log.enable 1
adb shell setprop vendor.debug.ae.pipe.log.enable 1

rem TNC Debug：
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1
adb shell setprop vendor.debug.tnc.log.level 65535
adb shell setprop vendor.debug.tnc_alg.log.level 1

rem AE Algo Debug：
adb shell setprop vendor.debug.ae.log.enable 1

adb shell pkill camera*