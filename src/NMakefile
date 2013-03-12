default: fping.exe

LINK = link

.c.obj:
	$(CC) /c $(CFLAGS) /DSUPPORTXP /W4 /nologo $*.c

fping.exe: fping.obj getopt.obj
	$(LINK) /nologo /OUT:$@ $**
	-del *.obj

clean:
	-del /s /q *.obj fping.exe >NUL 2>NUL
