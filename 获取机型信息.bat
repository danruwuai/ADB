@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo �����ֻ�adb
adb wait-for-device root
adb wait-for-device remount
echo +++++++++++++++++++++++++++++++++++++
echo �ֻ����� :
adb shell getprop ro.product.model
echo +++++++++++++++++++++++++++++++++++++
echo Android�汾 :
adb shell getprop ro.build.version.release
echo +++++++++++++++++++++++++++++++++++++
echo �ֻ�����״̬ ��
adb shell getprop ro.boot.vbmeta.device_state
echo +++++++++++++++++++++++++++++++++++++
echo ��Ļ�ֱ��� ��
adb shell wm size
echo +++++++++++++++++++++++++++++++++++++
echo ��Ļ�ܶ� ��
adb shell wm density
echo +++++++++++++++++++++++++++++++++++++
echo CPU��Ϣ ��
adb shell cat /proc/cpuinfo | findstr Hardware
echo +++++++++++++++++++++++++++++++++++++
pause