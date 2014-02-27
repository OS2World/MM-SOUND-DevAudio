SHELL=cmd.exe

CC=gcc
CFLAGS=-c -Zomf -s -O6 -Wall
#CFLAGS=-c -g -Wall

# Choose between next lines for stand-alone executable
# (that doesn't require EMX runtime DLLs) and EMX RT-dependent
#LDFLAGS=-Zomf -s -Zsys -Zsmall-conv
LDFLAGS=-Zomf -s -Zcrtdll
#LDFLAGS=-g -Zcrtdll

LIBS=-lmmpm2 -liberty

%.obj: %.cpp libDART.h
	$(CC) $(CFLAGS) -o $@ $<

devaudio.exe: libDART.obj devaudio.obj devaudio.def
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	-del *.obj
	-del *.exe
