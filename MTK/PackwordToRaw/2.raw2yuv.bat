rem unpack6589raw.exe PASS1_FULLRAW_0_4176x3088_5220_3.raw 4176 3088 10 2
echo off
setlocal enabledelayedexpansion

set "raw_dir=%cd%\RAW_RESULT"
set raw=.raw
set bmp=.bmp
echo on

del %raw_dir%*%bmp%

::forѭ���ж��Ƿ���raw
for /R %raw_dir% %%i in (*__*.raw) do (
::��ȡraw���ļ���
	set "str=%%~ni"
	echo �����ļ�����!str!

pause
::��ȡPixel_id
if "!str:~-2,1!"=="_" (
	set Pixel_id=!str:~-1,1!
	echo Pixel_id is !Pixel_id!
) else (
	echo Pixel_id is error
)

::��ȡbitdepth
if "!str:~-5,1!"=="_" (
	set bitdepth=!str:~-4,2!
	echo bitdepth is !bitdepth!
) else if "!str:~-4,1!"=="_" (
	set height=!str:~-3,1!
	echo !bitdepth!
)else (
	echo bitdepth is error
)

::��ȡheight
if "!str:~-10,1!"=="x" (
	set height=!str:~-9,4!
	echo !height!
) else if "!str:~-9,1!"=="x" (
	set height=!str:~-8,3!
	echo !height!
) else (
	echo height is error
)

::��ȡwidth
if "!str:~-16,2!"=="__" (
	set width=!str:~-14,4!
	echo !width!
) else if "!str:~-15,2!"=="__" (
	set width=!str:~-13,4!
	echo !width!
) else (
	echo width is error
)

.\unpack6589raw.exe %cd%\RAW_RESULT\%%~ni%raw% !width! !height! !bitdepth! !Pixel_id!

)
::del /Q %path%*
pause