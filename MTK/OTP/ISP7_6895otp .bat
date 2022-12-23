adb root
adb remount
adb shell setenforce 0
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1
adb shell setprop vendor.debug.lsc_mgr.dump 1
adb shell rm -rf /data/vendor/camera_dump
adb shell rm -rf /data/vendor/shading_otp
adb shell mkdir -p /data/vendor/camera_dump
adb shell mkdir -p /data/vendor/camera_dump/shading_CT
adb shell mkdir -p /data/vendor/camera_dump/shading_trfm
adb shell mkdir -p /data/vendor/camera_dump/tsf_batch
adb shell mkdir -p /data/vendor/camera_dump/tsf_batch_cap
adb shell mkdir -p /data/vendor/camera_dump/tsf_run
adb shell mkdir -p /data/vendor/camera_dump/tsf_keep
adb shell mkdir -p /data/vendor/camera_dump/GuAlignRaw
adb shell mkdir -p /data/vendor/camera_dump/GuMergeGain
adb shell mkdir -p /data/vendor/camera_dump/shading_reform
adb shell mkdir -p /data/vendor/camera_dump/shading_otp
adb shell mkdir -p /data/vendor/camera_dump/shading_align
adb shell mkdir -p /data/vendor/camera_dump/shading_merge
adb shell mkdir -p /data/vendor/camera_dump/pre_crop
adb shell mkdir -p /data/vendor/shading_otp
adb shell pkill camera*
