adb root

adb shell setprop persist.mtk.camera.log_level 4

adb shell setprop persist.vendor.mtk.camera.log_level 4

adb shell setprop vendor.debug.mtkcam.loglevel 4

adb shell setprop debug.ae.enable 9

adb shell setprop debug.aaa.pvlog.enable 1

adb shell setprop debug.aaa_sensor_mgr.enable 1

adb shell setprop debug.ae_mgr.enable 1
adb shell setprop debug.aaa_hal.enable 1

adb shell setprop debug.aaa_state.enable 1

adb shell setprop debug.ae.plineinfo 9

adb shell setprop debug.cam.drawid 1

adb shell setprop vendor.flash_is_debug 1

adb shell setprop debug.cam.drawid 1

adb shell setprop vendor.debuglog.drv.ccu_drv 5

adb shell setprop vendor.debuglog.ccuif.ccu_drv 5

adb shell setprop vendor.debug.aaa.pvlog.enable 1

adb shell setprop vendor.debug.ae.enable 9

adb shell setprop vendor.debug.ae.plineinfo 9

adb shell setprop vendor.debug.aaa_sensor_mgr.enable 1

adb shell setprop vendor.debug.ae_mgr.enable 1

adb shell stop camerahalserver

adb shell start camerahalserver

adb root

adb shell setprop vendor.debug.aaa.perframe_prop.enable 1

adb shell setprop vendor.debug.mapping_mgr.enable 2

adb shell setprop vendor.debug.paramctrl.enable 1

adb shell pkill camera*

