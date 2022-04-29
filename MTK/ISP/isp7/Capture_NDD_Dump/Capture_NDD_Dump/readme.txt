1、本脚本可以dump如下Capture Feature相关的image buffer文件。
Capture_lpnr
Capture_cshot_lpnr
Capture_simple
Capture_cshot
Capture_mfnr
Capture_mfnr_single_nr
Capture_aihdr_single_nr
Capture_ainr_single_nr
Capture_aishutter1
Capture_aishutter2

2、在open camera之前，先执行NDD_init.bat，再执行NDD_Capture_config.bat。
   NDD_init.bat中的adb shell setprop vendor.debug.ndd.subdir（0 or 1）可以控制是否创建子文件夹。
     ● 如果设成0，dump的文件都放在default_dump文件夹。
	 ● 如果设成1，每次进入camera就会创建一个子文件夹，例如：UKey143530399。

3、open camera之后，点击拍照就可以dump对应Feature的image buffer。 