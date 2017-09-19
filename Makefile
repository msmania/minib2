!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

all:
	@pushd src & nmake /nologo & popd

test:
	@pushd tests & nmake /nologo & popd
	@if exist tests\$(ARCH)\t.exe tests\$(ARCH)\t.exe

clean:
	@pushd tests & nmake /nologo clean & popd
	@pushd src & nmake /nologo clean & popd
