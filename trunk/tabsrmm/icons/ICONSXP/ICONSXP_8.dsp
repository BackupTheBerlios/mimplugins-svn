# Microsoft Developer Studio Project File - Name="ICONSXP" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ICONSXP - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ICONSXP.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ICONSXP.mak" CFG="ICONSXP - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ICONSXP - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ICONSXP - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ICONSXP - Win32 Release Unicode 98" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ICONSXP - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "ICONSXP_EXPORTS" /D "_MBCS" /Gm /YX /GZ /c /GX 
# ADD CPP /nologo /MTd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "ICONSXP_EXPORTS" /D "_MBCS" /Gm /YX /GZ /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"Debug\ICONSXP.dll" /incremental:yes /debug /pdb:"Debug\ICONSXP.pdb" /pdbtype:sept /subsystem:windows /noentry /implib:"$(OutDir)/ICONSXP.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"Debug\ICONSXP.dll" /incremental:yes /debug /pdb:"Debug\ICONSXP.pdb" /pdbtype:sept /subsystem:windows /noentry /implib:"$(OutDir)/ICONSXP.lib" /machine:ix86 

!ELSEIF  "$(CFG)" == "ICONSXP - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /Zi /W3 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "ICONSXP_EXPORTS" /D "_MBCS" /YX /c /GX 
# ADD CPP /nologo /MT /Zi /W3 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "ICONSXP_EXPORTS" /D "_MBCS" /YX /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"Release\tabsrmm_icons.dll" /incremental:no /debug /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /noentry /implib:"$(OutDir)/ICONSXP.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"Release\tabsrmm_icons.dll" /incremental:no /debug /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /noentry /implib:"$(OutDir)/ICONSXP.lib" /machine:ix86 

!ELSEIF  "$(CFG)" == "ICONSXP - Win32 Release Unicode 98"

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
# ADD BASE CPP /nologo /MT /Zi /W3 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "ICONSXP_EXPORTS" /D "_MBCS" /YX /c /GX 
# ADD CPP /nologo /MT /Zi /W3 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "ICONSXP_EXPORTS" /D "_MBCS" /YX /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"$(ConfigurationName)\tabsrmm_icons.dll" /incremental:no /debug /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /noentry /implib:"$(OutDir)/ICONSXP.lib" /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"$(ConfigurationName)\tabsrmm_icons.dll" /incremental:no /debug /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /noentry /implib:"$(OutDir)/ICONSXP.lib" /machine:ix86 

!ENDIF

# Begin Target

# Name "ICONSXP - Win32 Debug"
# Name "ICONSXP - Win32 Release"
# Name "ICONSXP - Win32 Release Unicode 98"
# Begin Group "Quelldateien"

# PROP Default_Filter "cpp;c;cxx;def;odl;idl;hpj;bat;asm;asmx"
# End Group
# Begin Group "Headerdateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc;xsd"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx"
# Begin Source File

SOURCE=..\..\res\angeli-icons\AddXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\button.bold.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\button.fontface.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\button.italic.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\button.textcolor.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\button.underline.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\CloseXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\delete.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\empty.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\error.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\HistoryXP.ico
# End Source File
# Begin Source File

SOURCE=.\ICONSXP.rc
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\Incom.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\InfoXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\Logo.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\MultiXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\OptXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\Outg.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\Pencil.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\PicXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\Quote02.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\SaveXP.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\secureim_off.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\secureim_on.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\angeli-icons\Send.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\smbutton.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\statuschange.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\Typing32.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\unknown.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project

