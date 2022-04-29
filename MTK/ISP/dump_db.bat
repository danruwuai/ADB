@echo off

echo +++++++++++++++++++++++++++++++++++++
echo pull DB
echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb remount
adb shell setenforce 0
adb pull vendor/bin/crossbuild/DataSet/SQLiteModule/db %cd%\%mydir%