C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat

SET curDir=%cd%

cd %curDir%\CPP\7zip\Bundles\Alone

nmake PLATFORM=x64 MY_DYNAMIC_LINK=1

dir %curDir%\CPP\7zip\Bundles\Alone\x64\7za.exe

cd %curDir%