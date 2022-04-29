echo off
set path=.\
set packed_word=.packed_word
set raw=.raw
echo on
del %path%*%raw%
for %%i in (%path%*_10_*) do (
.\P40_PAK_UNP_v8.exe %path% %%~ni%packed_word% 1 1 1 0
mkdir %cd%\RAW_RESULT
move %path%*%raw% %cd%\RAW_RESULT\
)
::del /Q %path%*
pause
