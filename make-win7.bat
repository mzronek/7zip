SET curDir=%cd%

CALL %curDir%\..\build\cmsc\setenv.bat %curDir%\..\build\WinDDK\7600.16385.1 x64

cd %curDir%\CPP\7zip\Bundles\Alone

nmake PLATFORM=x64 MY_DYNAMIC_LINK=1

dir %curDir%\CPP\7zip\Bundles\Alone\x64\7za.exe

cd %curDir%