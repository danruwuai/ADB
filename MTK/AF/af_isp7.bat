@echo off

echo +++++++++++++++++++++++++++++++++++++
echo ´ò¿ªAF LOG
echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
adb shell setprop log.tag D
adb shell setprop persist.log.tag D
adb shell setprop vendor.debug.camera.dgbinfo 1
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.af_mgr.enable 1
adb shell setprop debug.cam.drawid 1
adb shell setprop debug.cam.draw.ctrl 'DAC,MAGIC,ISO,EXP'
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.af.log.enable 1
adb shell setprop vendor.debug.pd.enable 1
adb shell setprop vendor.debug.pd.vpu.log.enable 1
adb shell setprop vendor.debug.pd.vpu.enable 1
adb shell setprop vendor.debug.af.assist.ai.log.enable 3
adb shell sync
adb shell sync
adb shell pkill camera*
