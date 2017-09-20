!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

OUTDIR=..\$(ARCH)
OBJDIR=$(ARCH)

CC=cl
RD=rd /s /q
RM=del /q
LINKER=link
TARGET=minib2.exe

OBJS=\
	$(OBJDIR)\addressbar.obj\
	$(OBJDIR)\bitmap.obj\
	$(OBJDIR)\blob.obj\
	$(OBJDIR)\eventsink.obj\
	$(OBJDIR)\main.obj\
	$(OBJDIR)\site.obj\
	$(OBJDIR)\minib2.res\

LIBS=\
	comctl32.lib\
	gdi32.lib\

# warning C4100: unreferenced formal parameter
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
	/wd4100\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:WINDOWS\

all: $(OUTDIR)\$(TARGET)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

.rc{$(OBJDIR)}.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	rc /nologo /fo "$@" $<

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:exe=ilk) $(RM) $(OUTDIR)\$(TARGET:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET:exe=pdb) $(RM) $(OUTDIR)\$(TARGET:exe=pdb)
