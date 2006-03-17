/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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
*/

#include "chat.h"

//globals
struct MM_INTERFACE	mmi = {0};					// structure which keeps pointers to mirandas alloc, free and realloc
HANDLE			g_hWindowList;
HMENU			g_hMenu = NULL;

FONTINFO		aFonts[OPTIONS_FONTCOUNT];
HICON			hIcons[30];
BOOL			IEviewInstalled = FALSE;
HBRUSH			hEditBkgBrush = NULL;
HBRUSH			hListBkgBrush = NULL;

HIMAGELIST		hImageList = NULL;

struct GlobalLogSettings_t g_Settings;

HIMAGELIST		hIconsList = NULL;

char *			pszActiveWndID = 0;
char *			pszActiveWndModule = 0;
int             g_chat_integration_enabled = 0;

int Chat_Load(PLUGINLINK *link)
{
	BOOL bFlag = FALSE;

	#ifndef NDEBUG //mem leak detector :-) Thanks Tornado!
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif

    if(!DBGetContactSettingByte(NULL, SRMSGMOD_T, "enable_chat", 0))
        return 0;
    
    g_chat_integration_enabled = 1;
    
    UpgradeCheck();

	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &mmi);
	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU));
    //OleInitialize(NULL);
	HookEvents();
	CreateServiceFunctions();
	CreateHookableEvents();
	OptionsInit();
	TabsInit();
	return 0;
}


int Chat_Unload(void)
{
    if(!g_chat_integration_enabled)
        return 0;
    
    DBWriteContactSettingWord(NULL, "Chat", "SplitterX", (WORD)g_Settings.iSplitterX);
	DBWriteContactSettingWord(NULL, "Chat", "SplitterY", (WORD)g_Settings.iSplitterY);
	DBWriteContactSettingDword(NULL, "Chat", "roomx", g_Settings.iX);
	DBWriteContactSettingDword(NULL, "Chat", "roomy", g_Settings.iY);
	DBWriteContactSettingDword(NULL, "Chat", "roomwidth" , g_Settings.iWidth);
	DBWriteContactSettingDword(NULL, "Chat", "roomheight", g_Settings.iHeight);

	CList_SetAllOffline(TRUE);

//	RichUtil_Unload();
	if(pszActiveWndID)
		free(pszActiveWndID);
	if(pszActiveWndModule)
		free(pszActiveWndModule);

	DestroyMenu(g_hMenu);
	DestroyServiceFunctions();
	FreeIcons();
	OptionsUnInit();
	FreeLibrary(GetModuleHandleA("riched20.dll"));
	//OleUninitialize();
	UnhookEvents();
	return 0;
}

void UpgradeCheck(void)
{
	/*
	DWORD dwVersion = DBGetContactSettingDword(NULL, "Chat", "OldVersion", PLUGIN_MAKE_VERSION(0,2,9,9));
	if(	pluginInfo.version > dwVersion)
	{
		if(dwVersion < PLUGIN_MAKE_VERSION(0,3,0,0))
		{
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font18");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font18Col");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font18Set");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font18Size");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font18Sty");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font19");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font19Col");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font19Set");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font19Size");
			DBDeleteContactSetting(NULL, "ChatFonts",	"Font19Sty");
			DBDeleteContactSetting(NULL, "Chat",		"ColorNicklistLines");
			DBDeleteContactSetting(NULL, "Chat",		"NicklistIndent");
			DBDeleteContactSetting(NULL, "Chat",		"NicklistRowDist");
			DBDeleteContactSetting(NULL, "Chat",		"ShowFormatButtons");
			DBDeleteContactSetting(NULL, "Chat",		"ShowLines");
			DBDeleteContactSetting(NULL, "Chat",		"ShowName");
			DBDeleteContactSetting(NULL, "Chat",		"ShowTopButtons");
			DBDeleteContactSetting(NULL, "Chat",		"SplitterX");
			DBDeleteContactSetting(NULL, "Chat",		"SplitterY");
			DBDeleteContactSetting(NULL, "Chat",		"IconFlags");
			DBDeleteContactSetting(NULL, "Chat",		"LogIndentEnabled");
			
		}
		
	}
	DBWriteContactSettingDword(NULL, "Chat", "OldVersion", pluginInfo.version);
	*/
	return;
}

void LoadLogIcons(void)
{
	hIcons[ICON_ACTION] = LoadIconEx(IDI_ACTION, "log_action", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_ACTION),IMAGE_ICON,0,0,0);
	hIcons[ICON_ADDSTATUS] = LoadIconEx(IDI_ADDSTATUS, "log_addstatus", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_ADDSTATUS),IMAGE_ICON,0,0,0);
	hIcons[ICON_HIGHLIGHT] = LoadIconEx(IDI_HIGHLIGHT, "log_highlight", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_HIGHLIGHT),IMAGE_ICON,0,0,0);
	hIcons[ICON_INFO] = LoadIconEx(IDI_INFO, "log_info", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_INFO),IMAGE_ICON,0,0,0);
	hIcons[ICON_JOIN] = LoadIconEx(IDI_JOIN, "log_join", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_JOIN),IMAGE_ICON,0,0,0);
	hIcons[ICON_KICK] = LoadIconEx(IDI_KICK, "log_kick", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_KICK),IMAGE_ICON,0,0,0);
	hIcons[ICON_MESSAGE] = LoadIconEx(IDI_MESSAGE, "log_message_in", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_MESSAGE),IMAGE_ICON,0,0,0);
	hIcons[ICON_MESSAGEOUT] = LoadIconEx(IDI_MESSAGEOUT, "log_message_out", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_MESSAGEOUT),IMAGE_ICON,0,0,0);
	hIcons[ICON_NICK] = LoadIconEx(IDI_NICK, "log_nick", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_NICK),IMAGE_ICON,0,0,0);
	hIcons[ICON_NOTICE] = LoadIconEx(IDI_NOTICE, "log_notice", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_NOTICE),IMAGE_ICON,0,0,0);
	hIcons[ICON_PART] = LoadIconEx(IDI_PART, "log_part", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_PART),IMAGE_ICON,0,0,0);
	hIcons[ICON_QUIT] = LoadIconEx(IDI_QUIT, "log_quit", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_QUIT),IMAGE_ICON,0,0,0);
	hIcons[ICON_REMSTATUS] = LoadIconEx(IDI_REMSTATUS, "log_removestatus", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_REMSTATUS),IMAGE_ICON,0,0,0);
	hIcons[ICON_TOPIC] = LoadIconEx(IDI_TOPIC, "log_topic", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_TOPIC),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS1] = LoadIconEx(IDI_STATUS1, "status1", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS1),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS2] = LoadIconEx(IDI_STATUS2, "status2", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS2),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS3] = LoadIconEx(IDI_STATUS3, "status3", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS3),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS4] = LoadIconEx(IDI_STATUS4, "status4", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS4),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS0] = LoadIconEx(IDI_STATUS0, "status0", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS0),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS5] = LoadIconEx(IDI_STATUS5, "status5", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS5),IMAGE_ICON,0,0,0);

	return;
}
void LoadIcons(void)
{
	int i;

	for(i = 0; i < 20; i++)
		hIcons[i] = NULL;

	LoadLogIcons();
	
	LoadMsgLogBitmaps();

	hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),IsWinVerXPPlus()? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK,0,3);
	hIconsList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),IsWinVerXPPlus()? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK,0,100);
	ImageList_AddIcon(hIconsList,LoadSkinnedIcon( SKINICON_EVENT_MESSAGE));
	ImageList_AddIcon(hIconsList,LoadIconEx(IDI_OVERLAY, "overlay", 0, 0));
	ImageList_SetOverlayImage(hIconsList, 1, 1);
	ImageList_AddIcon(hImageList,LoadImage(g_hInst,MAKEINTRESOURCE(IDI_BLANK),IMAGE_ICON,0,0,0));
	ImageList_AddIcon(hImageList,LoadImage(g_hInst,MAKEINTRESOURCE(IDI_BLANK),IMAGE_ICON,0,0,0));
	return ;
}

void FreeIcons(void)
{
	FreeMsgLogBitmaps();
	ImageList_Destroy(hImageList);
	ImageList_Destroy(hIconsList);
	return;
}
