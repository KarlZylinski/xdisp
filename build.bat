cl.exe /D _HAS_EXCEPTIONS=0 /nologo /W4 /WX /EHsc /wd4055 /wd4054 /wd4152 /wd4204 /D _CRT_SECURE_NO_WARNINGS /Zi /MTd -c /Foxdisp.o xdisp.c
lib.exe /out:xdisp.lib kernel32.lib user32.lib opengl32.lib gdi32.lib xdisp.o
copy /Y xdisp.lib d:\projects\snake
rem link.exe /DEBUG /out:xdisp.exe xdisp.o kernel32.lib user32.lib opengl32.lib gdi32.lib