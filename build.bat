call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
cl.exe /D _HAS_EXCEPTIONS=0 /nologo /W4 /WX /EHsc /wd4055 /wd4054 /TC /wd4152 /wd4204 /d2noftol3 /D _CRT_SECURE_NO_WARNINGS /Zi /MTd -c /Foxdisp.o xdisp.c
lib.exe /out:xdisp.lib /nodefaultlib kernel32.lib gdi32.lib user32.lib opengl32.lib xdisp.o
copy /Y xdisp.lib d:\projects\snake
copy /Y vc140.pdb d:\projects\snake
copy /Y xdisp.pdb d:\projects\snake
copy /Y xdisp.c d:\projects\snake
rem kernel32.lib gdi32.lib user32.lib opengl32.lib
rem link.exe /DEBUG /out:xdisp.exe xdisp.o kernel32.lib user32.lib opengl32.lib gdi32.lib