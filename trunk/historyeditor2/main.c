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

#include "headers.h"

struct MM_INTERFACE mmi;
struct UTF8_INTERFACE utfi;

//========================
//  MirandaPluginInfo
//========================
PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	modFullname,
	PLUGIN_MAKE_VERSION(2, 0, 0, 0),
	"History editor lets you view (and edit!) all your history easily.",
	"Originally written by Jonathan Gordon",
	"jdgordy@gmail.com",
	"© 2003-2008 Jonathan Gordon, Updated for Miranda 0.7+ by Nightwish",
	"jdgordy.tk",		// www
#ifdef _UNICODE
	UNICODE_AWARE,
#else
	0,		//not transient
#endif
	0,		//doesn't replace anything built-in
#ifdef _UNICODE
	{ 0x40e285e8, 0x9462, 0x46f1, { 0x97, 0x85, 0xa5, 0x84, 0x1d, 0xd0, 0x1b, 0xe5 }}
#else
	{ 0xbffdeec0, 0x2bc9, 0x43a3, { 0x88, 0x2b, 0xc9, 0x33, 0x57, 0xd6, 0xdb, 0x6c }}
#endif
};

HANDLE hWindowList = 0;

__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 0))
		return NULL;
	return &pluginInfo;
}

static const MUUID interfaces[] = {{0x43f94ecf, 0x5666, 0x4472, { 0x9b, 0x59, 0xe5, 0x24, 0xa0, 0xbe, 0x39, 0xe1 }},
									MIID_LAST};

__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

//========================
//  WINAPI DllMain
//========================

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

//===================
// MainInit
//===================

int MainInit(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
 * wParam is contact handle
 */
static int HistoryEditorContactCommand(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;

	if((hwnd = WindowList_Find(hWindowList, (HANDLE)wParam)))
		SetForegroundWindow(hwnd);
	else
		hwnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAIN), 0, MainDlgProc, wParam);

	return 0;
}

static int HistoryEditorCopyHistoryCommand(WPARAM wParam, LPARAM lParam)
{
	copyHistory((HANDLE)wParam, (HANDLE)lParam);
	return 0;
}
//===========================
// Load (hook ModulesLoaded)
//===========================

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	CLISTMENUITEM mi;

#ifndef NDEBUG //mem leak detector :-) Thanks Tornado!
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	_CrtSetDbgFlag(flag); // Set flag to the new value
#endif

	pluginLink = link;
	mir_getMMI(&mmi);
	mir_getUTFI(&utfi);

	CreateServiceFunction("HistoryEditor/CopyHistory", HistoryEditorCopyHistoryCommand);
	CreateServiceFunction("HistoryEditor/ContactMenuCommand", HistoryEditorContactCommand);

	hWindowList = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.position = 1900000001;
	mi.flags = 0;
	mi.hIcon = LoadIconEx("core_main_10");
	mi.pszContactOwner = NULL;

	mi.pszName = Translate("View History with History Editor");
	mi.pszService = "HistoryEditor/ContactMenuCommand";
	CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	{	// add our modules to the KnownModules list
		DBVARIANT dbv;
		if (DBGetContactSetting(NULL, "KnownModules", modFullname, &dbv))
			DBWriteContactSettingString(NULL, "KnownModules", modFullname, modname);
		DBFreeVariant(&dbv);
	}

	return 0;
}

int __declspec(dllexport) Unload(void)
{
	return 0;
}

HICON LoadIconEx(char *pszIcoLibName)
{
	char szTemp[256];
	return (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)pszIcoLibName);
}
