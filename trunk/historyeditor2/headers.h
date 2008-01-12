/*
astyle	--force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --unpad=paren									   

---------------------------------------------------------------------------
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
---------------------------------------------------------------------------

History Editor 2 Plugin for Miranda IM.
---------------------------------------

Originally written in 2004 by Jonathan Gordon.

Rewritten by Nightwish in 2008 to make it compatible with Miranda IM
0.7 and later.

$Id:$
*/

#ifndef _COMMONHEADERS_H
#define _COMMONHEADERS_H
#define _CRT_SECURE_NO_WARNINGS
#define _32BIT_TIME_T

#pragma warning( disable : 4786 ) // limitation in MSVC's debugger.
//=====================================================
//	Includes 
//=====================================================

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <string.h>
#include <newpluginapi.h>
#include <m_clist.h>
#include <m_clui.h>
#include <m_skin.h>
#include <m_langpack.h>
#include <m_protomod.h>
#include <m_database.h>
#include <m_system.h>
#include <m_protocols.h>
#include <m_userinfo.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_utils.h>
#include <m_ignore.h>
#include <m_clc.h>
#include <m_history.h>
#include <m_icolib.h>

//#include "../miranda_src/SDK/Headers_c/win2k.h"
#include "m_popup.h"
#include "m_file.h"

#include "resource.h"
#define WinVerMajor()      LOBYTE(LOWORD(GetVersion()))
#define WinVerMinor()      HIBYTE(LOWORD(GetVersion()))
#define IsWinVerNT()       ((GetVersion()&0x80000000)==0)
#define IsWinVerNT4Plus()  (WinVerMajor()>=5 || WinVerMinor()>0 || IsWinVerNT())
#define IsWinVer98Plus()   (LOWORD(GetVersion())!=4)
#define IsWinVerMEPlus()   (WinVerMajor()>=5 || WinVerMinor()>10)
#define IsWinVer2000Plus() (WinVerMajor()>=5)
#define IsWinVerXPPlus()   (WinVerMajor()>=5 && LOWORD(GetVersion())!=5)

#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
//=======================================================
//	Definitions
//=======================================================
#define modname			"HistoryEditor"
#define modFullname		"History Editor"
#define msg(a,b)		MessageBox(0,a,b,MB_OK)

//=======================================================
//	Event Definitions
//=======================================================
#define EVENTTYPE_LOGGEDSTATUSCHANGE 25368

//=======================================================
//  Variables
//=======================================================
PLUGINLINK *pluginLink;
HINSTANCE hInst;

typedef struct {
	HANDLE hContact;
	HANDLE hDbEvent;
	HANDLE hwndOwner;
} EditWindowStruct;

typedef struct {
	HANDLE hContact;
	HANDLE hFirstEvent, hLastEvent;
} HistoryWindowStruct;

//=======================================================
//  Functions
//=======================================================

// main.c
// threads.c
unsigned long forkthread (   void (__cdecl *threadcode)(void*),unsigned long stacksize,void *arg) ;
//dialog.c 
BOOL CALLBACK MainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// history.c
void setupHistoryList(HWND hwnd2List);
void populateHistory(HWND hwnd2List, HANDLE hContact);
void editHistory(HANDLE hContact, HANDLE hDbEvent, HWND hwndParent);
void deleteAllHistory(HANDLE hContact);
void copyHistory(HANDLE hContactFrom,HANDLE hContactTo);
HICON LoadIconEx(char *pszIcoLibName);

extern struct MM_INTERFACE mmi;

#define SIZEOF(a) (sizeof((a)) / sizeof((a[0])))
#define MODULE_NAME "histEdit2"
#define RWPF_HIDE 8
#define COLUMN_COUNT 4

#ifndef NDEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#pragma comment(lib,"comctl32.lib")

#endif //_COMMONHEADERS_H