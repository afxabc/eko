@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 8\VC\bin\vcvars32.bat"
call lib.exe /out:debug\libeko.lib debug\base.lib debug\net.lib
call lib.exe /out:release\libeko.lib release\base.lib release\net.lib
  
echo 按任意键退出...
pause > nul
