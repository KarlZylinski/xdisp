set VC_VARS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat
call "%VC_VARS_PATH%" x86
cl.exe /O2 /D _HAS_EXCEPTIONS=0 /nologo /W4 /WX /EHsc /wd4055 /wd4054 /TC /wd4152 /wd4204 /D _CRT_SECURE_NO_WARNINGS /MT -c /Foxdisp.o xdisp.c
lib.exe /out:xdisp.lib /nodefaultlib kernel32.lib gdi32.lib user32.lib msvcrt.lib ucrt.lib opengl32.lib xdisp.o
