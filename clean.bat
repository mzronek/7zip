SET curDir=%cd%

for /d /r %%i in (*x64) do @rmdir /s /q "%%i"
