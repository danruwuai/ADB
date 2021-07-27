@echo off

echo +++++++++++++++++++++++++++++++++++++
echo 打开AF LOG
echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
adb shell setprop log.tag D
adb shell setprop persist.log.tag D
adb shell setprop vendor.persist.log.tag.ae_mgr D
adb shell setprop vendor.persist.log.tag.AeAlgo D
adb shell setprop vendor.debug.ae_mgr.lock 0
adb shell setprop vendor.debug.aaa_sensor_mgr.enable 0
adb shell setprop vendor.debug.af_mgr.enable 1
adb shell setprop debug.cam.drawid 1
adb shell setprop debug.cam.draw.ctrl 'DAC,MAGIC,ISO'
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.debug.af.log.enable 1
adb shell setprop vendor.debug.pd.enable 1
adb shell setprop vendor.debug.pd.vpu.log.enable 1
adb shell setprop vendor.debug.pd.vpu.enable 1
adb shell setprop vendor.debug.af.assist.ai.log.enable 3
adb shell setprop persist.log.tag.af_mgr_v3 D
adb shell setprop persist.log.tag.pd_mgr D
adb shell setprop persist.log.tag.CcuDrv D
adb shell setprop persist.log.tag.AfAlgo D
adb shell setprop persist.log.tag.AfAlgoC D
adb shell setprop persist.log.tag.AfAlgoCtrl D
adb shell setprop persist.log.tag.af_assist_mgr D
adb shell setprop persist.log.tag.AfAssist D
adb shell setprop persist.log.tag.DualAfAssist D
adb shell setprop persist.log.tag.VPUAfAssist D
adb shell setprop persist.log.tag.PdAlgo D
adb shell setprop persist.log.tag.DualPdAlgo D
adb shell setprop persist.log.tag.SonyIMX D
adb shell setprop persist.log.tag.pd_buf D
adb shell setprop persist.log.tag.AIAfAssist D
adb shell setprop vendor.debug.aiawb.ga.type 1
adb shell setprop vendor.debug.aisa.disable 0
adb shell setprop vendor.debug.aiga.disable 0
adb shell setprop vendor.debug.aaa.pvlog.enable 1
adb shell setprop vendor.debug.ae.log.enable 1
adb shell setprop vendor.debug.cameradbginfo 1
adb shell "rm /data/vendor/camera_dump/*"
adb shell sync
adb shell sync
adb shell sync
adb shell pkill camera*
adb logcat -G 16m
adb logcat -c
echo +++++++++++++++++++++++++++++++++++++
echo 输出 log结束
echo +++++++++++++++++++++++++++++++++++++
timeout 2 > nul
