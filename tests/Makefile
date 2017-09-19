!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

GTEST=G:\10_Git\googletest
O_GTEST=$(GTEST)\build\$(ARCH)\googlemock\gtest\Release
O_GMOCK=$(GTEST)\build\$(ARCH)\googlemock\Release
OUTDIR=$(ARCH)
OBJDIR=$(ARCH)

CC=cl
RD=rd /s /q
RM=del /q
LINKER=link
TARGET=t.exe

OBJS=\
	$(OBJDIR)\blob.obj\
	$(OBJDIR)\bitmap.obj\
	$(OBJDIR)\bitmap-test.obj\

LIBS=\
	gdi32.lib\
	user32.lib\
	gtest.lib\
	gtest_main.lib\

CFLAGS=\
	/nologo\
	/c\
	/DUNICODE\
	/O2\
	/W4\
	/Zi\
	/EHsc\
	/Fo"$(OBJDIR)\\"\
	/Fd"$(OBJDIR)\\"\
	/I..\src\
	/I$(GTEST)\googlemock\include\
	/I$(GTEST)\googletest\include\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:CONSOLE\
	/LIBPATH:$(O_GTEST)\
	/LIBPATH:$(O_GMOCK)\

all: $(OUTDIR)\$(TARGET)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

{../src}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
