rem create folder with timestamp
for /f "delims=" %%a in ('wmic OS get localdatetime  ^| find "."') do set datetime=%%a
set "YYYY=%datetime:~0,4%"
set "MM=%datetime:~4,2%"
set "DD=%datetime:~6,2%"
set "HH=%datetime:~8,2%"
set "MI=%datetime:~10,2%"
set "SS=%datetime:~12,2%"
set mydir=%YYYY%-%MM%-%DD%-%HH%-%MI%-%SS%

adb root
adb remount
adb push camsys_converter.sh /data/vendor/camera_dump

adb shell "chmod a+x ./data/vendor/camera_dump/camsys_converter.sh"
adb shell "./data/vendor/camera_dump/camsys_converter.sh"

adb pull /data/vendor/camera_dump/ %cd%\%mydir%

pause