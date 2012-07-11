PROGRAM = GemfireSvcLauncher.exe
INCLUDEDIRS =
CPPSOURCES = GemfireSvcLauncher.cpp
CPPOBJECTS = GemfireSvcLauncher.obj
CPPFLAGS = /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _CONSOLE /D _MBCS /D _CRT_SECURE_NO_WARNINGS /c GemfireSvcLauncher.c
LINKFLAGS = Advapi32.lib

$(PROGRAM): $(CPPOBJECTS)
  link.exe /out:$@ $(CPPOBJECTS)  $(LINKFLAGS)
GemfireSvcLauncher.obj: GemfireSvcLauncher.cpp 
clean:
  del $(CPPOBJECTS) $(PROGRAM) *.tlh
run:
  $(PROGRAM)  

