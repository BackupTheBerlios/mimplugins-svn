# Microsoft Developer Studio Generated NMAKE File, Based on avatars.dsp
!IF "$(CFG)" == ""
CFG=loadavatars - Win32 Debug Unicode
!MESSAGE No configuration specified. Defaulting to loadavatars - Win32 Debug Unicode.
!ENDIF 

!IF "$(CFG)" != "loadavatars - Win32 Release" && "$(CFG)" != "loadavatars - Win32 Debug" && "$(CFG)" != "loadavatars - Win32 Release Unicode" && "$(CFG)" != "loadavatars - Win32 Debug Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "avatars.mak" CFG="loadavatars - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "loadavatars - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "loadavatars - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "loadavatars - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "loadavatars - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "loadavatars - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\loadavatars.dll"


CLEAN :
	-@erase "$(INTDIR)\acc.obj"
	-@erase "$(INTDIR)\avatars.res"
	-@erase "$(INTDIR)\image_utils.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mir_dblists.obj"
	-@erase "$(INTDIR)\mir_memory.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\loadavatars.exp"
	-@erase "$(OUTDIR)\loadavatars.lib"
	-@erase "$(OUTDIR)\loadavatars.map"
	-@erase "$(OUTDIR)\loadavatars.pdb"
	-@erase "..\..\bin\release\plugins\loadavatars.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOADAVATARS_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\avatars.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\avatars.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib gdiplus.lib /nologo /base:"0x5130000" /dll /incremental:no /pdb:"$(OUTDIR)\loadavatars.pdb" /map:"$(INTDIR)\loadavatars.map" /debug /machine:I386 /out:"../../bin/release/plugins/loadavatars.dll" /implib:"$(OUTDIR)\loadavatars.lib" /IGNORE:4089 /filealign:512 
LINK32_OBJS= \
	"$(INTDIR)\acc.obj" \
	"$(INTDIR)\image_utils.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mir_dblists.obj" \
	"$(INTDIR)\mir_memory.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\avatars.res"

"..\..\bin\release\plugins\loadavatars.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\debug\plugins\loadavatars.dll" "$(OUTDIR)\avatars.bsc"


CLEAN :
	-@erase "$(INTDIR)\acc.obj"
	-@erase "$(INTDIR)\acc.sbr"
	-@erase "$(INTDIR)\avatars.res"
	-@erase "$(INTDIR)\image_utils.obj"
	-@erase "$(INTDIR)\image_utils.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mir_dblists.obj"
	-@erase "$(INTDIR)\mir_dblists.sbr"
	-@erase "$(INTDIR)\mir_memory.obj"
	-@erase "$(INTDIR)\mir_memory.sbr"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\options.sbr"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\poll.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\avatars.bsc"
	-@erase "$(OUTDIR)\loadavatars.exp"
	-@erase "$(OUTDIR)\loadavatars.lib"
	-@erase "$(OUTDIR)\loadavatars.map"
	-@erase "$(OUTDIR)\loadavatars.pdb"
	-@erase "..\..\bin\debug\plugins\loadavatars.dll"
	-@erase "..\..\bin\debug\plugins\loadavatars.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LOADAVATARS_EXPORTS" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\avatars.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\avatars.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\acc.sbr" \
	"$(INTDIR)\image_utils.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\mir_dblists.sbr" \
	"$(INTDIR)\mir_memory.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\poll.sbr"

"$(OUTDIR)\avatars.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib gdiplus.lib delayimp.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\loadavatars.pdb" /map:"$(INTDIR)\loadavatars.map" /debug /machine:I386 /out:"../../bin/debug/plugins/loadavatars.dll" /implib:"$(OUTDIR)\loadavatars.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\acc.obj" \
	"$(INTDIR)\image_utils.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mir_dblists.obj" \
	"$(INTDIR)\mir_memory.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\avatars.res"

"..\..\bin\debug\plugins\loadavatars.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode

ALL : "..\..\bin\release Unicode\plugins\loadavatars.dll"


CLEAN :
	-@erase "$(INTDIR)\acc.obj"
	-@erase "$(INTDIR)\avatars.res"
	-@erase "$(INTDIR)\image_utils.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mir_dblists.obj"
	-@erase "$(INTDIR)\mir_memory.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\loadavatars.exp"
	-@erase "$(OUTDIR)\loadavatars.lib"
	-@erase "$(OUTDIR)\loadavatars.map"
	-@erase "$(OUTDIR)\loadavatars.pdb"
	-@erase "..\..\bin\release Unicode\plugins\loadavatars.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "LOADAVATARS_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\avatars.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\avatars.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib gdiplus.lib /nologo /base:"0x5130000" /dll /incremental:no /pdb:"$(OUTDIR)\loadavatars.pdb" /map:"$(INTDIR)\loadavatars.map" /debug /machine:I386 /out:"../../bin/release Unicode/plugins/loadavatars.dll" /implib:"$(OUTDIR)\loadavatars.lib" /IGNORE:4089 /filealign:512 
LINK32_OBJS= \
	"$(INTDIR)\acc.obj" \
	"$(INTDIR)\image_utils.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mir_dblists.obj" \
	"$(INTDIR)\mir_memory.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\avatars.res"

"..\..\bin\release Unicode\plugins\loadavatars.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"

OUTDIR=.\Debug_Unicode
INTDIR=.\Debug_Unicode
# Begin Custom Macros
OutDir=.\Debug_Unicode
# End Custom Macros

ALL : "..\..\bin\debug Unicode\plugins\loadavatars.dll" "$(OUTDIR)\avatars.bsc"


CLEAN :
	-@erase "$(INTDIR)\acc.obj"
	-@erase "$(INTDIR)\acc.sbr"
	-@erase "$(INTDIR)\avatars.res"
	-@erase "$(INTDIR)\image_utils.obj"
	-@erase "$(INTDIR)\image_utils.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mir_dblists.obj"
	-@erase "$(INTDIR)\mir_dblists.sbr"
	-@erase "$(INTDIR)\mir_memory.obj"
	-@erase "$(INTDIR)\mir_memory.sbr"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\options.sbr"
	-@erase "$(INTDIR)\poll.obj"
	-@erase "$(INTDIR)\poll.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\avatars.bsc"
	-@erase "$(OUTDIR)\loadavatars.exp"
	-@erase "$(OUTDIR)\loadavatars.lib"
	-@erase "$(OUTDIR)\loadavatars.map"
	-@erase "$(OUTDIR)\loadavatars.pdb"
	-@erase "..\..\bin\debug Unicode\plugins\loadavatars.dll"
	-@erase "..\..\bin\debug Unicode\plugins\loadavatars.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "LOADAVATARS_EXPORTS" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\avatars.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\avatars.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\acc.sbr" \
	"$(INTDIR)\image_utils.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\mir_dblists.sbr" \
	"$(INTDIR)\mir_memory.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\poll.sbr"

"$(OUTDIR)\avatars.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib gdiplus.lib delayimp.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\loadavatars.pdb" /map:"$(INTDIR)\loadavatars.map" /debug /machine:I386 /out:"../../bin/debug Unicode/plugins/loadavatars.dll" /implib:"$(OUTDIR)\loadavatars.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\acc.obj" \
	"$(INTDIR)\image_utils.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mir_dblists.obj" \
	"$(INTDIR)\mir_memory.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\poll.obj" \
	"$(INTDIR)\avatars.res"

"..\..\bin\debug Unicode\plugins\loadavatars.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("avatars.dep")
!INCLUDE "avatars.dep"
!ELSE 
!MESSAGE Warning: cannot find "avatars.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "loadavatars - Win32 Release" || "$(CFG)" == "loadavatars - Win32 Debug" || "$(CFG)" == "loadavatars - Win32 Release Unicode" || "$(CFG)" == "loadavatars - Win32 Debug Unicode"
SOURCE=.\acc.cpp

!IF  "$(CFG)" == "loadavatars - Win32 Release"


"$(INTDIR)\acc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"


"$(INTDIR)\acc.obj"	"$(INTDIR)\acc.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"


"$(INTDIR)\acc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"


"$(INTDIR)\acc.obj"	"$(INTDIR)\acc.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\image_utils.cpp

!IF  "$(CFG)" == "loadavatars - Win32 Release"


"$(INTDIR)\image_utils.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"


"$(INTDIR)\image_utils.obj"	"$(INTDIR)\image_utils.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"


"$(INTDIR)\image_utils.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"


"$(INTDIR)\image_utils.obj"	"$(INTDIR)\image_utils.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\main.cpp

!IF  "$(CFG)" == "loadavatars - Win32 Release"


"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"


"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"


"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"


"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mir_dblists.cpp

!IF  "$(CFG)" == "loadavatars - Win32 Release"


"$(INTDIR)\mir_dblists.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"


"$(INTDIR)\mir_dblists.obj"	"$(INTDIR)\mir_dblists.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"


"$(INTDIR)\mir_dblists.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"


"$(INTDIR)\mir_dblists.obj"	"$(INTDIR)\mir_dblists.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mir_memory.cpp

!IF  "$(CFG)" == "loadavatars - Win32 Release"


"$(INTDIR)\mir_memory.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"


"$(INTDIR)\mir_memory.obj"	"$(INTDIR)\mir_memory.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"


"$(INTDIR)\mir_memory.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"


"$(INTDIR)\mir_memory.obj"	"$(INTDIR)\mir_memory.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\options.cpp

!IF  "$(CFG)" == "loadavatars - Win32 Release"


"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"


"$(INTDIR)\options.obj"	"$(INTDIR)\options.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"


"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"


"$(INTDIR)\options.obj"	"$(INTDIR)\options.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\poll.cpp

!IF  "$(CFG)" == "loadavatars - Win32 Release"


"$(INTDIR)\poll.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug"


"$(INTDIR)\poll.obj"	"$(INTDIR)\poll.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Release Unicode"


"$(INTDIR)\poll.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "loadavatars - Win32 Debug Unicode"


"$(INTDIR)\poll.obj"	"$(INTDIR)\poll.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\avatars.rc

"$(INTDIR)\avatars.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 


