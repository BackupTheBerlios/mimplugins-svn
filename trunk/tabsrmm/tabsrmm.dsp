# Microsoft Developer Studio Project File - Name="tabSRMM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=tabSRMM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tabSRMM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tabSRMM.mak" CFG="tabSRMM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tabSRMM - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release - MATHMOD" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release-Unicode-MATHMOD" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" PRECOMP_VC7_TOBEREMOVED /Fp".\Debug/srmm.pch" /Fo".\Debug/" /Fd".\Debug/" /GZ /c /GX 
# ADD CPP /nologo /MDd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" PRECOMP_VC7_TOBEREMOVED /Fp".\Debug/srmm.pch" /Fo".\Debug/" /Fd".\Debug/" /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\srmm.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\srmm.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_DEBUG" 
# ADD RSC /l 2057 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /out:"..\..\Bin\Debug\Plugins\tabsrmm.dll" /incremental:no /debug /pdb:".\Debug\srmm.pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/srmm.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /out:"..\..\Bin\Debug\Plugins\tabsrmm.dll" /incremental:no /debug /pdb:".\Debug\srmm.pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/srmm.lib" /machine:ix86 

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release_Unicode"
# PROP BASE Intermediate_Dir ".\Release_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release_Unicode"
# PROP Intermediate_Dir ".\Release_Unicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /Ob1 /Os /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_USE_32BIT_TIME_T" /D "_UNICODE" /GF PRECOMP_VC7_TOBEREMOVED /Fp".\Release_Unicode/srmm.pch" /Fo".\Release_Unicode/" /Fd".\Release_Unicode/" /c /GX 
# ADD CPP /nologo /MT /W3 /O1 /Ob1 /Os /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_USE_32BIT_TIME_T" /D "_UNICODE" /GF PRECOMP_VC7_TOBEREMOVED /Fp".\Release_Unicode/srmm.pch" /Fo".\Release_Unicode/" /Fd".\Release_Unicode/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release_Unicode\srmm.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release_Unicode\srmm.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" /d "UNICODE" 
# ADD RSC /l 2057 /d "NDEBUG" /d "UNICODE" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm_unicode.dll" /incremental:no /pdbtype:sept /subsystem:windows /base:"0x6a540000" /implib:".\Release_Unicode/srmm.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm_unicode.dll" /incremental:no /pdbtype:sept /subsystem:windows /base:"0x6a540000" /implib:".\Release_Unicode/srmm.lib" /machine:ix86 

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_USE_32BIT_TIME_T" /D "_MBCS" /GF /Gy PRECOMP_VC7_TOBEREMOVED /Fp".\Release/srmm.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD CPP /nologo /MT /W3 /O1 /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_USE_32BIT_TIME_T" /D "_MBCS" /GF /Gy PRECOMP_VC7_TOBEREMOVED /Fp".\Release/srmm.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\srmm.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\srmm.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" 
# ADD RSC /l 2057 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /incremental:no /pdbtype:sept /subsystem:windows /base:"0x6a540000" /implib:".\Release/srmm.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /incremental:no /pdbtype:sept /subsystem:windows /base:"0x6a540000" /implib:".\Release/srmm.lib" /machine:ix86 

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug_Unicode"
# PROP BASE Intermediate_Dir ".\Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug_Unicode"
# PROP Intermediate_Dir ".\Debug_Unicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_USE_32BIT_TIME_T" /D "_UNICODE" PRECOMP_VC7_TOBEREMOVED /Fp".\Debug_Unicode/srmm.pch" /Fo".\Debug_Unicode/" /Fd".\Debug_Unicode/" /FR /GZ /c /GX 
# ADD CPP /nologo /MDd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_UNICODE" /D "_CRT_SECURE_NO_DEPRECATE" /D "_USE_32BIT_TIME_T" /D "_UNICODE" PRECOMP_VC7_TOBEREMOVED /Fp".\Debug_Unicode/srmm.pch" /Fo".\Debug_Unicode/" /Fd".\Debug_Unicode/" /FR /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug_Unicode\srmm.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug_Unicode\srmm.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_DEBUG" 
# ADD RSC /l 2057 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /dll /out:"..\..\Bin\Debug\Plugins\tabsrmm_unicode.dll" /incremental:no /debug /pdb:".\Debug_Unicode\srmm.pdb" /pdbtype:sept /subsystem:windows /base:"0x6a540000" /implib:".\Debug_Unicode/srmm.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /dll /out:"..\..\Bin\Debug\Plugins\tabsrmm_unicode.dll" /incremental:no /debug /pdb:".\Debug_Unicode\srmm.pdb" /pdbtype:sept /subsystem:windows /base:"0x6a540000" /implib:".\Debug_Unicode/srmm.lib" /machine:ix86 

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release - MATHMOD"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "$(ConfigurationName)"
# PROP BASE Intermediate_Dir "$(ConfigurationName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "$(ConfigurationName)"
# PROP Intermediate_Dir "$(ConfigurationName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MATHMOD_SUPPORT" /D "_MBCS" /GF /Gy PRECOMP_VC7_TOBEREMOVED /Fp".\Release/srmm.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD CPP /nologo /MT /W3 /O1 /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MATHMOD_SUPPORT" /D "_MBCS" /GF /Gy PRECOMP_VC7_TOBEREMOVED /Fp".\Release/srmm.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\srmm.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\srmm.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" 
# ADD RSC /l 2057 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /incremental:no /pdbtype:sept /subsystem:windows /implib:".\Release/srmm.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /incremental:no /pdbtype:sept /subsystem:windows /implib:".\Release/srmm.lib" /machine:ix86 

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release-Unicode-MATHMOD"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "$(ConfigurationName)"
# PROP BASE Intermediate_Dir "$(ConfigurationName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "$(ConfigurationName)"
# PROP Intermediate_Dir "$(ConfigurationName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /Ob1 /D "__MATHMOD_SUPPORT" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /GF /Gy PRECOMP_VC7_TOBEREMOVED /Fp".\Release_Unicode_mathmod/srmm.pch" /Fo".\Release_Unicode_Mathmod/" /Fd".\Release_Unicode_Mathmod/" /c /GX 
# ADD CPP /nologo /MT /W3 /O1 /Ob1 /D "__MATHMOD_SUPPORT" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /GF /Gy PRECOMP_VC7_TOBEREMOVED /Fp".\Release_Unicode_mathmod/srmm.pch" /Fo".\Release_Unicode_Mathmod/" /Fd".\Release_Unicode_Mathmod/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release_Unicode\srmm.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release_Unicode\srmm.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" /d "UNICODE" 
# ADD RSC /l 2057 /d "NDEBUG" /d "UNICODE" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm_mathmod_w.dll" /incremental:no /pdbtype:sept /subsystem:windows /implib:".\Release_Unicode/srmm.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /out:"..\..\Bin\Release\Plugins\tabsrmm_mathmod_w.dll" /incremental:no /pdbtype:sept /subsystem:windows /implib:".\Release_Unicode/srmm.lib" /machine:ix86 

!ENDIF

# Begin Target

# Name "tabSRMM - Win32 Debug"
# Name "tabSRMM - Win32 Release Unicode"
# Name "tabSRMM - Win32 Release"
# Name "tabSRMM - Win32 Debug Unicode"
# Name "tabSRMM - Win32 Release - MATHMOD"
# Name "tabSRMM - Win32 Release-Unicode-MATHMOD"
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=commonheaders.h
# End Source File
# Begin Source File

SOURCE=functions.h
# End Source File
# Begin Source File

SOURCE=IcoLib.h
# End Source File
# Begin Source File

SOURCE=ImageDataObject.h
# End Source File
# Begin Source File

SOURCE=m_avatars.h
# End Source File
# Begin Source File

SOURCE=m_fontservice.h
# End Source File
# Begin Source File

SOURCE=m_ieview.h
# End Source File
# Begin Source File

SOURCE=m_MathModule.h
# End Source File
# Begin Source File

SOURCE=m_metacontacts.h
# End Source File
# Begin Source File

SOURCE=.\m_popup.h
# End Source File
# Begin Source File

SOURCE=m_smileyadd.h
# End Source File
# Begin Source File

SOURCE=m_Snapping_windows.h
# End Source File
# Begin Source File

SOURCE=m_tabsrmm.h
# End Source File
# Begin Source File

SOURCE=m_toptoolbar.h
# End Source File
# Begin Source File

SOURCE=m_updater.h
# End Source File
# Begin Source File

SOURCE=msgdlgutils.h
# End Source File
# Begin Source File

SOURCE=msgs.h
# End Source File
# Begin Source File

SOURCE=nen.h
# End Source File
# Begin Source File

SOURCE=resource.h
# End Source File
# Begin Source File

SOURCE=sendqueue.h
# End Source File
# Begin Source File

SOURCE=templates.h
# End Source File
# Begin Source File

SOURCE=.\URLCtrl.h
# End Source File
# End Group
# Begin Group "Misc Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\docs\changelog.txt
# End Source File
# Begin Source File

SOURCE=docs\INSTALL
# End Source File
# Begin Source File

SOURCE=langpacks\langpack_tabsrmm_german.txt
# End Source File
# Begin Source File

SOURCE=.\Makefile.ansi
# End Source File
# Begin Source File

SOURCE=MAKEFILE.W32
# End Source File
# Begin Source File

SOURCE=.\docs\MetaContacts.TXT
# End Source File
# Begin Source File

SOURCE=.\docs\Popups.txt
# End Source File
# Begin Source File

SOURCE=.\docs\Readme.icons
# End Source File
# Begin Source File

SOURCE=.\docs\readme.txt
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\angeli-icons\Add.ico
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\addcontact.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\AddXP.ico
# End Source File
# Begin Source File

SOURCE=.\res\arrow-down.ico
# End Source File
# Begin Source File

SOURCE=.\res\blocked.ico
# End Source File
# Begin Source File

SOURCE=.\res\check.ico
# End Source File
# Begin Source File

SOURCE=.\res\checked.ico
# End Source File
# Begin Source File

SOURCE=.\res\Clock8.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Close.ico
# End Source File
# Begin Source File

SOURCE=.\res\delete.ico
# End Source File
# Begin Source File

SOURCE=res\Details32.ico
# End Source File
# Begin Source File

SOURCE=res\Details8.ico
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\dragcopy.cur
# End Source File
# Begin Source File

SOURCE=..\..\src\res\dragcopy.cur
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\dropuser.cur
# End Source File
# Begin Source File

SOURCE=..\..\src\res\dropuser.cur
# End Source File
# Begin Source File

SOURCE=.\res\error.ico
# End Source File
# Begin Source File

SOURCE=.\res\expand.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\History.ico
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\history.ico
# End Source File
# Begin Source File

SOURCE=res\History32.ico
# End Source File
# Begin Source File

SOURCE=res\History8.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\HistoryXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\hyperlin.cur
# End Source File
# Begin Source File

SOURCE=..\..\src\res\hyperlin.cur
# End Source File
# Begin Source File

SOURCE=..\..\src\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\in.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Incom.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Info.ico
# End Source File
# Begin Source File

SOURCE=.\res\leftarrow.ico
# End Source File
# Begin Source File

SOURCE=.\msgwindow.rc
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Multi.ico
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\multisend.ico
# End Source File
# Begin Source File

SOURCE=res\Multisend32.ico
# End Source File
# Begin Source File

SOURCE=res\Multisend8.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\MultiXP.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Opt.ico
# End Source File
# Begin Source File

SOURCE=.\res\options.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\OptXP.ico
# End Source File
# Begin Source File

SOURCE=.\res\out.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Outg.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Pencil.ico
# End Source File
# Begin Source File

SOURCE=.\res\Photo32.ico
# End Source File
# Begin Source File

SOURCE=.\res\Photo8.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Pic.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\PicXP.ico
# End Source File
# Begin Source File

SOURCE=.\res\pulldown.ico
# End Source File
# Begin Source File

SOURCE=.\res\pulldown1.ico
# End Source File
# Begin Source File

SOURCE=.\res\pullup.ico
# End Source File
# Begin Source File

SOURCE=.\res\quote.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Quote02.ico
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\rename.ico
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\res\rightarrow.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Save.ico
# End Source File
# Begin Source File

SOURCE=.\res\save.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\SaveXP.ico
# End Source File
# Begin Source File

SOURCE=.\res\angeli-icons\Send.ico
# End Source File
# Begin Source File

SOURCE=.\res\smbutton.ico
# End Source File
# Begin Source File

SOURCE=.\res\smiley_xp.ico
# End Source File
# Begin Source File

SOURCE=.\res\status.ico
# End Source File
# Begin Source File

SOURCE=res\Typing32.ico
# End Source File
# Begin Source File

SOURCE=res\Typing8.ico
# End Source File
# Begin Source File

SOURCE=.\res\unchecked.ico
# End Source File
# Begin Source File

SOURCE=..\..\Miranda-IM\res\viewdetails.ico
# End Source File
# Begin Source File

SOURCE=.\res\visible.ico
# End Source File
# Begin Source File

SOURCE=.\res\xstatus.bmp
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\container.c
# End Source File
# Begin Source File

SOURCE=.\containeroptions.c
# End Source File
# Begin Source File

SOURCE=eventpopups.c
# End Source File
# Begin Source File

SOURCE=.\formatting.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode 98"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=.\hotkeyhandler.c
# End Source File
# Begin Source File

SOURCE=ImageDataObject.cpp
# End Source File
# Begin Source File

SOURCE=msgdialog.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /Od /FR /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release - MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release-Unicode-MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "__MATHMOD_SUPPORT" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode 98"

# ADD CPP /nologo /O1 /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=msgdlgutils.c
# End Source File
# Begin Source File

SOURCE=msglog.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /O2 /Ot /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /Od /FR /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release - MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release-Unicode-MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "__MATHMOD_SUPPORT" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode 98"

# ADD CPP /nologo /O1 /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=msgoptions.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /Od /FR /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release - MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release-Unicode-MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode 98"

# ADD CPP /nologo /O1 /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=msgs.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /Od /FR /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release - MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release-Unicode-MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "__MATHMOD_SUPPORT" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode 98"

# ADD CPP /nologo /O1 /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=.\selectcontainer.c
# End Source File
# Begin Source File

SOURCE=sendqueue.c
# End Source File
# Begin Source File

SOURCE=srmm.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /O1 /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /Od /FR /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release - MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release-Unicode-MATHMOD"

# ADD CPP /nologo /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "__MATHMOD_SUPPORT" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode 98"

# ADD CPP /nologo /O1 /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=.\tabctrl.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# PROP Intermediate_Dir "$(IntDir)/$(InputName)1.obj"
# ADD CPP /nologo /Fo"$(IntDir)/$(InputName)1.obj" /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# PROP Intermediate_Dir "$(IntDir)/$(InputName)1.obj"
# ADD CPP /nologo /Fo"$(IntDir)/$(InputName)1.obj" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# PROP Intermediate_Dir "$(IntDir)/$(InputName)1.obj"
# ADD CPP /nologo /Fo"$(IntDir)/$(InputName)1.obj" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# PROP Intermediate_Dir "$(IntDir)/$(InputName)1.obj"
# ADD CPP /nologo /Fo"$(IntDir)/$(InputName)1.obj" /GZ /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release - MATHMOD"

# PROP Intermediate_Dir "$(IntDir)/$(InputName)1.obj"
# ADD CPP /nologo /Fo"$(IntDir)/$(InputName)1.obj" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release-Unicode-MATHMOD"

# PROP Intermediate_Dir "$(IntDir)/$(InputName)1.obj"
# ADD CPP /nologo /Fo"$(IntDir)/$(InputName)1.obj" /GX 
!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode 98"

# PROP Intermediate_Dir "$(IntDir)/$(InputName)1.obj"
# ADD CPP /nologo /Fo"$(IntDir)/$(InputName)1.obj" /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=templates.c
# End Source File
# Begin Source File

SOURCE=.\themes.c
# End Source File
# Begin Source File

SOURCE=trayicon.c
# End Source File
# Begin Source File

SOURCE=TSButton.c
# End Source File
# Begin Source File

SOURCE=.\URLCtrl.c
# End Source File
# Begin Source File

SOURCE=userprefs.c
# End Source File
# End Group
# End Target
# End Project
