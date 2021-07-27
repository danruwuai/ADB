adb root
adb shell setenforce 0
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.af_mgr.enable 1
adb shell setprop vendor.debug.pd.enable 1
adb shell setprop vendor.debug.af.enable 1
adb shell setprop vendor.dump.bpci_tbl.enable 1
adb shell setprop vendor.debug.dump_pdaf_cali.enable 1
adb shell setprop vendor.pd.dump.enable 1
adb shell setprop vendor.vc.dump.enable 1
adb shell setprop vendor.debug.pdinfo.enable 1
adb shell pkill camera*
pause