/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

UNICODE done

*/
#include "commonheaders.h"

#define TRAYICON_ID_BASE    100
#define TIM_CALLBACK   (WM_USER+1857)
#define TIM_CREATE     (WM_USER+1858)

int EventsProcessTrayDoubleClick(void);
int ShowHide(WPARAM wParam, LPARAM lParam);
static VOID CALLBACK TrayCycleTimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime);

extern HIMAGELIST hCListImages;
extern HWND hwndContactList;
extern int currentStatusMenuItem, currentDesiredStatusMode;
extern BOOL(WINAPI *MySetProcessWorkingSetSize)(HANDLE, SIZE_T, SIZE_T);

extern struct CluiData g_CluiData;

static UINT WM_TASKBARCREATED;
static int cycleTimerId = 0,cycleStep = 0;

struct trayIconInfo_t {
    int id;
    char *szProto;
    HICON hBaseIcon;
    int isBase;
};
static struct trayIconInfo_t *trayIcon = NULL;
static int trayIconCount;

BOOL g_trayTooltipActive = FALSE;
POINT tray_hover_pos = {0};

// don't move to win2k.h, need new and old versions to work on 9x/2000/XP
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010
typedef struct {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    CHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    CHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    CHAR szInfoTitle[64];
    DWORD dwInfoFlags;
} NOTIFYICONDATA_NEW;

#if defined(_UNICODE)
typedef struct {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
} NOTIFYICONDATAW_NEW;
#endif
typedef struct _DllVersionInfo {
    DWORD cbSize;
    DWORD dwMajorVersion;                   // Major version
    DWORD dwMinorVersion;                   // Minor version
    DWORD dwBuildNumber;                    // Build number
    DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

static DLLVERSIONINFO dviShell;
static BOOL mToolTipTrayTips = FALSE;

static char szTip[2048];
static char * TrayIconMakeTooltip(const char *szPrefix, const char *szProto)
{
    char szProtoName[32];
    char *szStatus, *szSeparator;
    int tipSize;

    if(ServiceExists("mToolTip/ShowTip"))
        tipSize = 2048;
    else
        tipSize = 128;
    
    if(!mToolTipTrayTips)
        szSeparator = (IsWinVerMEPlus()) ? szSeparator = "\n" : " | ";
    else
        szSeparator = "\n";

    if (szProto == NULL) {
        PROTOCOLDESCRIPTOR **protos;
        int count, netProtoCount, i;
        CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
        for (i = 0,netProtoCount = 0; i < count; i++) {
            if (protos[i]->type == PROTOTYPE_PROTOCOL)
                netProtoCount++;
        }
        if (netProtoCount == 1)
            for (i = 0; i < count; i++) {
                if (protos[i]->type == PROTOTYPE_PROTOCOL)
                    return TrayIconMakeTooltip(szPrefix, protos[i]->szName);
            }
        if (szPrefix && szPrefix[0]) {
            lstrcpynA(szTip, szPrefix, tipSize);
            if (!DBGetContactSettingByte(NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT))
                return szTip;
        } else
            szTip[0] = '\0';
        szTip[tipSize - 1] = '\0';
        for (i = count - 1; i >= 0; i--) {
            if (protos[i]->type != PROTOTYPE_PROTOCOL || !GetProtocolVisibility(protos[i]->szName))
                continue;
            CallProtoService(protos[i]->szName, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName);
            szStatus = (char*) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0), 0);
            if(szStatus) {
                if(mToolTipTrayTips) {
                    char tipline[256];
                    mir_snprintf(tipline, 256, "<b>%-12.12s</b>\t%s", szProtoName, szStatus);
                    if (szTip[0])
                        strncat(szTip, szSeparator, tipSize - 1 - strlen(szTip));
                    strncat(szTip, tipline, tipSize - 1 - strlen(szTip));
                }
                else {
                    if (szTip[0])
                        strncat(szTip, szSeparator, tipSize - 1 - strlen(szTip));
                    strncat(szTip, szProtoName, tipSize - 1 - strlen(szTip));
                    strncat(szTip, " ", tipSize - 1 - strlen(szTip));
                    strncat(szTip, szStatus, tipSize - 1 - strlen(szTip));
                }
            }
        }
    } else {
        CallProtoService(szProto, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName);
        szStatus = (char*) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, CallProtoService(szProto, PS_GETSTATUS, 0, 0), 0);
        if (szPrefix && szPrefix[0]) {
            if (DBGetContactSettingByte(NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT)) {
                if(mToolTipTrayTips)
                    _snprintf(szTip, tipSize, "%s%s<b>%-12.12s</b>\t%s", szPrefix, szSeparator, szProtoName, szStatus);
                else
                    _snprintf(szTip, tipSize, "%s%s%s %s", szPrefix, szSeparator, szProtoName, szStatus);
            }
            else
                lstrcpynA(szTip, szPrefix, tipSize);
        } else {
            if(mToolTipTrayTips)
                _snprintf(szTip, tipSize, "<b>%-12-12s</b>\t%s", szProtoName, szStatus);
            else
                _snprintf(szTip, tipSize, "%s %s", szProtoName, szStatus);
        }
    }
    return szTip;
}

static int TrayIconAdd(HWND hwnd, const char *szProto, const char *szIconProto, int status)
{
    NOTIFYICONDATAA nid = {0};
    NOTIFYICONDATA_NEW nidn = {0};
    int i, iIcon = 0;
    HICON hIcon = 0;
    
    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            break;
    }
    trayIcon[i].id = TRAYICON_ID_BASE + i;
    trayIcon[i].szProto = (char *) szProto;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = trayIcon[i].id;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = TIM_CALLBACK;
    iIcon = IconFromStatusMode(szIconProto ? szIconProto : trayIcon[i].szProto, status, 0, &hIcon);
    if(hIcon == 0)
        trayIcon[i].hBaseIcon = ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL);
    else
        trayIcon[i].hBaseIcon = CopyIcon(hIcon);
    nid.hIcon = trayIcon[i].hBaseIcon;  

    if (dviShell.dwMajorVersion >= 5) {
        nidn.cbSize = sizeof(nidn);
        nidn.hWnd = nid.hWnd;
        nidn.uID = nid.uID;
        nidn.uFlags = nid.uFlags | NIF_INFO;
        nidn.uCallbackMessage = nid.uCallbackMessage;
        nidn.hIcon = nid.hIcon;
        TrayIconMakeTooltip(NULL, trayIcon[i].szProto);
        if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
            lstrcpynA(nidn.szTip, szTip, sizeof(nidn.szTip));
        Shell_NotifyIconA(NIM_ADD, (void*) &nidn);
    } else {
        TrayIconMakeTooltip(NULL, trayIcon[i].szProto);
        if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
            lstrcpynA(nid.szTip, szTip, sizeof(nid.szTip));
        Shell_NotifyIconA(NIM_ADD, &nid);
    }
    trayIcon[i]. isBase = 1;
    return i;
}

static void TrayIconRemove(HWND hwnd, const char *szProto)
{
    int i;
    NOTIFYICONDATA nid = {0};
    NOTIFYICONDATA_NEW nidn = {0};

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        if (lstrcmpA(szProto, trayIcon[i].szProto))
            continue;
        if (dviShell.dwMajorVersion >= 5) {
            nidn.cbSize = sizeof(nidn);
            nidn.hWnd = nid.hWnd;
            nidn.uID = trayIcon[i].id;
            Shell_NotifyIcon(NIM_DELETE, (void*) &nidn);
        } else {
            nid.uID = trayIcon[i].id;
            Shell_NotifyIcon(NIM_DELETE, &nid);
        }       
        DestroyIcon(trayIcon[i].hBaseIcon);
        trayIcon[i]. id = 0;
        break;
    }
}

static int TrayIconInit(HWND hwnd)
{
    int count, netProtoCount, i;
    int averageMode = 0;
    PROTOCOLDESCRIPTOR **protos;

    if(ServiceExists("mToolTip/ShowTip"))
        mToolTipTrayTips = TRUE;
    
    if (cycleTimerId) {
        KillTimer(NULL, cycleTimerId); 
        cycleTimerId = 0;
    }
    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
    for (i = 0,netProtoCount = 0; i < count; i++) {
        if (protos[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        
        cycleStep = i;
        netProtoCount++;
        if (averageMode == 0)
            averageMode = CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0);
        else if (averageMode != CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0)) {
            averageMode = -1; break;
        }
    }
    trayIconCount = count;
    trayIcon = (struct trayIconInfo_t *) mir_alloc(sizeof(struct trayIconInfo_t) * count);
    ZeroMemory(trayIcon, sizeof(struct trayIconInfo_t) * count);
    if (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI && (averageMode <= 0 || DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT))) {
        int i;
        for (i = count - 1; i >= 0; i--) {
            if (!GetProtocolVisibility(protos[i]->szName))
                continue;
            if (protos[i]->type == PROTOTYPE_PROTOCOL)
                TrayIconAdd(hwnd, protos[i]->szName, NULL, CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0));
        }
    } else if (averageMode <= 0 && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_SINGLE) {
        DBVARIANT dbv = {DBVT_DELETED};
        char *szProto;
        if (DBGetContactSetting(NULL, "CList", "PrimaryStatus", &dbv))
            szProto = NULL;
        else
            szProto = dbv.pszVal;
        TrayIconAdd(hwnd, NULL, szProto, szProto ? CallProtoService(szProto, PS_GETSTATUS, 0, 0) : CallService(MS_CLIST_GETSTATUSMODE, 0, 0));
        DBFreeVariant(&dbv);
    } else
        TrayIconAdd(hwnd, NULL, NULL, averageMode);
    if (averageMode <= 0 && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_CYCLE)
        cycleTimerId = SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, TrayCycleTimerProc);
    return 0;
}

static void TrayIconDestroy(HWND hwnd)
{
    NOTIFYICONDATA nid = {
        0
    };
    int i;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        nid.uID = trayIcon[i].id;
        Shell_NotifyIcon(NIM_DELETE, &nid);
        DestroyIcon(trayIcon[i].hBaseIcon);
    }
    if (trayIcon)
        mir_free(trayIcon);
    trayIcon = NULL;
    trayIconCount = 0;
}

//called when Explorer crashes and the taskbar is remade
static void TrayIconTaskbarCreated(HWND hwnd)
{
    TrayIconDestroy(hwnd);
    TrayIconInit(hwnd);
}

int TrayIconUpdate(HICON hNewIcon, const char *szNewTip, const char *szPreferredProto, int isBase)
{
    NOTIFYICONDATAA nid = {0};
    NOTIFYICONDATA_NEW nidn = {0};
    int i;
    char szProto = 0;

    nid.cbSize = sizeof(nid);
    nid.hWnd = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);
    nid.uFlags = NIF_ICON | NIF_TIP;
    nid.hIcon = hNewIcon;           
    nidn.cbSize = sizeof(nidn);
    nidn.hWnd = nid.hWnd;
    nidn.uFlags = NIF_ICON | NIF_TIP;
    nidn.hIcon = nid.hIcon;

    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        if (lstrcmpA(trayIcon[i].szProto, szPreferredProto))
            continue;
        nid.uID = trayIcon[i].id;
        if (dviShell.dwMajorVersion >= 5) {
            nidn.uID = nid.uID;
            TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
            if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
                lstrcpynA(nidn.szTip, szTip, sizeof(nidn.szTip));
            Shell_NotifyIconA(NIM_MODIFY, (void*) &nidn);
        } else {
            TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
            if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
                lstrcpynA(nid.szTip, szTip, sizeof(nid.szTip));
            Shell_NotifyIconA(NIM_MODIFY, &nid);
        }
        trayIcon[i].isBase = isBase;
        return i;
    }
    //if there wasn't a suitable icon, change all the icons

    if(szPreferredProto != NULL && !GetProtocolVisibility((char *)szPreferredProto) && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI)
        return -1;

    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        nid.uID = trayIcon[i].id;
        if (dviShell.dwMajorVersion >= 5) {
            nidn.uID = nid.uID;
            TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
            if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
                lstrcpynA(nidn.szTip, szTip, sizeof(nidn.szTip));
            Shell_NotifyIconA(NIM_MODIFY, (void*) &nidn);
        } else {
            TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
            if(!mToolTipTrayTips && !g_CluiData.bNoTrayTips)
                lstrcpynA(nid.szTip, szTip, sizeof(nid.szTip));
            Shell_NotifyIconA(NIM_MODIFY, &nid);
        }
        trayIcon[i]. isBase = isBase;
    }
    return -1;
}

int TrayIconSetBaseInfo(HICON hIcon, const char *szPreferredProto)
{
    int i;

    if(szPreferredProto != (const char *)NULL && !GetProtocolVisibility((char *)szPreferredProto))
        return -1;
    
    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        if (lstrcmpA(trayIcon[i].szProto, szPreferredProto))
            continue;
        DestroyIcon(trayIcon[i].hBaseIcon);
        trayIcon[i]. hBaseIcon = hIcon;
        return i;
    }
    //if there wasn't a specific icon, there will only be one suitable
    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        DestroyIcon(trayIcon[i].hBaseIcon);
        trayIcon[i]. hBaseIcon = hIcon;
        return i;
    }
    DestroyIcon(hIcon);
    return -1;
}

void TrayIconUpdateWithImageList(int iImage, const char *szNewTip, char *szPreferredProto, HANDLE hContact)
{
    HICON hIcon;

    // a meta contact has to flash in the tray, but the meta contact protocol is hidden by the protocol
    // configuration - so redirect it to the actual protocol icon
    
    hIcon = ImageList_GetIcon(hCListImages, iImage, ILD_NORMAL);
    TrayIconUpdate(hIcon, szNewTip, szPreferredProto, 0);
    DestroyIcon(hIcon);
}

static VOID CALLBACK TrayCycleTimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
    int count;
    PROTOCOLDESCRIPTOR **protos;

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
    for (cycleStep++; ; cycleStep++) {
        if (cycleStep >= count)
            cycleStep = 0;
        if (protos[cycleStep]->type == PROTOTYPE_PROTOCOL)
            break;
    }
    DestroyIcon(trayIcon[0].hBaseIcon);
    trayIcon[0].hBaseIcon = ImageList_GetIcon(hCListImages, IconFromStatusMode(protos[cycleStep]->szName, CallProtoService(protos[cycleStep]->szName, PS_GETSTATUS, 0, 0), 0, NULL), ILD_NORMAL);
    if (trayIcon[0].isBase)
        TrayIconUpdate(trayIcon[0].hBaseIcon, NULL, NULL, 1);
}

void TrayIconUpdateBase(const char *szChangedProto)
{
    int i,count,netProtoCount,changed = -1;
    PROTOCOLDESCRIPTOR **protos;
    int averageMode = 0;
    HWND hwnd = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);

    if (cycleTimerId) {
        KillTimer(NULL, cycleTimerId); cycleTimerId = 0;
    }
    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
    for (i = 0,netProtoCount = 0; i < count; i++) {
        if (protos[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        netProtoCount++;
        if (!lstrcmpA(szChangedProto, protos[i]->szName))
            cycleStep = i;
        if (averageMode == 0)
            averageMode = CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0);
        else if (averageMode != CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0)) {
            averageMode = -1; break;
        }
    }
    if (netProtoCount > 1) {
        if (averageMode > 0) {
            if (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI) {
                if (DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT)) {
                    HICON hIcon = 0;
                    int iIcon = IconFromStatusMode(szChangedProto, averageMode, 0, &hIcon);
                    if(hIcon)
                        changed = TrayIconSetBaseInfo(CopyIcon(hIcon), szChangedProto);
                    else
                        changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), szChangedProto);
                }
                else if (trayIcon && trayIcon[0].szProto != NULL) {
                    TrayIconDestroy(hwnd);
                    TrayIconInit(hwnd);
                } else {
                    HICON hIcon = 0;
                    int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

                    if(hIcon)
                        changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
                    else
                        changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
                }
            } else {
                HICON hIcon = 0;
                int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

                if(hIcon)
                    changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
                else
                    changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
            }
        } else {
            switch (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT)) {
                case SETTING_TRAYICON_SINGLE:
                    {
                        DBVARIANT dbv = {DBVT_DELETED};
                        int iIcon = 0;
                        HICON hIcon = 0;
                        char *szProto;
                        if (DBGetContactSetting(NULL, "CList", "PrimaryStatus", &dbv))
                            szProto = NULL;
                        else
                            szProto = dbv.pszVal;
                        iIcon = IconFromStatusMode(szProto, szProto ? CallProtoService(szProto, PS_GETSTATUS, 0, 0) : CallService(MS_CLIST_GETSTATUSMODE, 0, 0), 0, &hIcon);
                        if(hIcon)
                            changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
                        else
                            changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
                        DBFreeVariant(&dbv);
                        break;
                    }
                case SETTING_TRAYICON_CYCLE:
                    {
                        HICON hIcon = 0;
                        int iIcon = IconFromStatusMode(szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0), 0, &hIcon);
                        
                        cycleTimerId = SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, TrayCycleTimerProc);
                        if(hIcon)
                            changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
                        else
                            changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
                        break;
                    }
                case SETTING_TRAYICON_MULTI:
                    if (!trayIcon) {
                        TrayIconRemove(NULL, NULL);
                    } else if (DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT)) {
                        HICON hIcon = 0;
                        int iIcon = IconFromStatusMode(szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0), 0, &hIcon);

                        if(hIcon)
                            changed = TrayIconSetBaseInfo(CopyIcon(hIcon), szChangedProto);
                        else
                            changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), szChangedProto);
                    }
                    else {
                        TrayIconDestroy(hwnd);
                        TrayIconInit(hwnd);
                    }
                    break;
            }
        }
    } else {
        HICON hIcon = 0;
        int iIcon = IconFromStatusMode(NULL, averageMode, 0, &hIcon);

        if(hIcon)
            changed = TrayIconSetBaseInfo(CopyIcon(hIcon), NULL);
        else
            changed = TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, iIcon, ILD_NORMAL), NULL);
    }
    if (changed != -1 && trayIcon[changed].isBase)
        TrayIconUpdate(trayIcon[changed].hBaseIcon, NULL, trayIcon[changed].szProto, 1);
}

void TrayIconSetToBase(char *szPreferredProto)
{
    int i;

    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        if (lstrcmpA(trayIcon[i].szProto, szPreferredProto))
            continue;
        TrayIconUpdate(trayIcon[i].hBaseIcon, NULL, szPreferredProto, 1);
        return;
    }
    //if there wasn't a specific icon, there will only be one suitable
    for (i = 0; i < trayIconCount; i++) {
        if (trayIcon[i].id == 0)
            continue;
        TrayIconUpdate(trayIcon[i].hBaseIcon, NULL, szPreferredProto, 1);
        return;
    }
}

void TrayIconIconsChanged(void)
{
    HWND hwnd = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);
    TrayIconDestroy(hwnd);
    TrayIconInit(hwnd);
}

static int autoHideTimerId;
static VOID CALLBACK TrayIconAutoHideTimer(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
    HWND hwndClui;
    KillTimer(hwnd, idEvent);
    hwndClui = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);
    if (GetActiveWindow() == hwndClui)
        return;
    ShowWindow(hwndClui, SW_HIDE);
    if (MySetProcessWorkingSetSize != NULL)
        MySetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
}

int TrayIconPauseAutoHide(WPARAM wParam, LPARAM lParam)
{
    if (DBGetContactSettingByte(NULL, "CList", "AutoHide", SETTING_AUTOHIDE_DEFAULT)) {
        if (GetActiveWindow() != (HWND) CallService(MS_CLUI_GETHWND, 0, 0)) {
            KillTimer(NULL, autoHideTimerId);
            autoHideTimerId = SetTimer(NULL, 0, 1000 * DBGetContactSettingWord(NULL, "CList", "HideTime", SETTING_HIDETIME_DEFAULT), TrayIconAutoHideTimer);
        }
    }
    return 0;
}

void CALLBACK TrayToolTipTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD elapsed)
{
    if(!g_trayTooltipActive && !g_CluiData.bNoTrayTips) {
        CLCINFOTIP ti = {0};
        POINT pt;
        GetCursorPos(&pt);
        if(pt.x == tray_hover_pos.x && pt.y == tray_hover_pos.y) {
            ti.cbSize = sizeof(ti);
            ti.isTreeFocused = GetFocus() == hwndContactList ? 1 : 0;
            CallService("mToolTip/ShowTip", (WPARAM)szTip, (LPARAM)&ti);
            GetCursorPos(&tray_hover_pos);
            g_trayTooltipActive = TRUE;
        }
    }
    KillTimer(hwnd, id);
}

int TrayIconProcessMessage(WPARAM wParam, LPARAM lParam)
{
    MSG *msg = (MSG *) wParam;
    switch (msg->message) {
        case WM_CREATE:
            {
                WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");
                PostMessage(msg->hwnd, TIM_CREATE, 0, 0);           
                break;
            }
        case TIM_CREATE:
            TrayIconInit(msg->hwnd);
            break;
        case WM_ACTIVATE:
            if (DBGetContactSettingByte(NULL, "CList", "AutoHide", SETTING_AUTOHIDE_DEFAULT)) {
                if (LOWORD(msg->wParam) == WA_INACTIVE)
                    autoHideTimerId = SetTimer(NULL, 0, 1000 * DBGetContactSettingWord(NULL, "CList", "HideTime", SETTING_HIDETIME_DEFAULT), TrayIconAutoHideTimer);
                else
                    KillTimer(NULL, autoHideTimerId);
            }
            break;
        case WM_DESTROY:
            TrayIconDestroy(msg->hwnd);
            break;
        case TIM_CALLBACK:
            if (msg->lParam == WM_RBUTTONDOWN || msg->lParam == WM_LBUTTONDOWN) {
                KillTimer(hwndContactList, TIMERID_TRAYHOVER);
                CallService("mToolTip/HideTip", 0, 0);
            }
            if (msg->lParam == WM_MBUTTONUP) {
                ShowHide(0, 0);
            } else if (msg->lParam == (DBGetContactSettingByte(NULL, "CList", "Tray1Click", SETTING_TRAY1CLICK_DEFAULT) ? WM_LBUTTONUP : WM_LBUTTONDBLCLK)) {
                if ((GetAsyncKeyState(VK_CONTROL) & 0x8000))
                    ShowHide(0, 0);
                else {
                    if (EventsProcessTrayDoubleClick())
                        ShowHide(0, 0);
                }
            } 
            else if (msg->lParam == WM_MOUSEMOVE) {
                if(g_trayTooltipActive) {
                    POINT pt;
                    GetCursorPos(&pt);
                    if(pt.x != tray_hover_pos.x || pt.y != tray_hover_pos.y) {
                        CallService("mToolTip/HideTip", 0, 0);
                        g_trayTooltipActive = FALSE;
                    }
                    break;
                }
                else {
                    GetCursorPos(&tray_hover_pos);
                    SetTimer(hwndContactList, TIMERID_TRAYHOVER, 600, TrayToolTipTimerProc);
                }
            }
            else if (msg->lParam == WM_RBUTTONUP) {
                HMENU hMenu;
                MENUITEMINFOA mi;
                POINT pt;
                hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
                hMenu = GetSubMenu(hMenu, 0);
                CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hMenu, 0);

                ZeroMemory(&mi, sizeof(mi));
                mi.cbSize = MENUITEMINFO_V4_SIZE;
                mi.fMask = MIIM_SUBMENU | MIIM_TYPE;
                mi.fType = MFT_STRING;
                mi.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);
                mi.dwTypeData = Translate("&Main Menu");
                InsertMenuItemA(hMenu, 1, TRUE, &mi);
                mi.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
                mi.dwTypeData = Translate("&Status");
                InsertMenuItemA(hMenu, 2, TRUE, &mi);
                SetMenuDefaultItem(hMenu, ID_TRAY_HIDE, FALSE);

                SetForegroundWindow(msg->hwnd);
                SetFocus(msg->hwnd);
                GetCursorPos(&pt);
                TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, msg->hwnd, NULL);
            }
            *((LRESULT *) lParam) = 0;
            return TRUE;
        default:
            if (msg->message == WM_TASKBARCREATED) {
                TrayIconTaskbarCreated(msg->hwnd);
                *((LRESULT *) lParam) = 0;
                return TRUE;
            }
    }

    return FALSE;
}

int CListTrayNotify(WPARAM wParam, LPARAM lParam)
{
    MIRANDASYSTRAYNOTIFY *msn = (MIRANDASYSTRAYNOTIFY *) lParam;

#if defined(_UNICODE)
    if(msn->dwInfoFlags & NIIF_INTERN_UNICODE) {
        if (msn && msn->cbSize == sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
            if (trayIcon) {
                NOTIFYICONDATAW_NEW nid = {0};
                nid.cbSize = sizeof(nid);
                nid.hWnd = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);
                if (msn->szProto) {
                    int j;
                    for (j = 0; j < trayIconCount; j++) {
                        if (trayIcon[j].szProto != NULL) {
                            if (strcmp(msn->szProto, trayIcon[j].szProto) == 0) {
                                nid.uID = trayIcon[j].id;
                                j = trayIconCount;
                                continue;
                            }
                        } else {
                            if (trayIcon[j].isBase) {
                                nid.uID = trayIcon[j].id;
                                j = trayIconCount;
                                continue;
                            }
                        }
                    } //for
                } else {
                    nid.uID = trayIcon[0].id;
                }
                nid.uFlags = NIF_INFO;
                lstrcpynW(nid.szInfo, (WCHAR *)msn->szInfo, safe_sizeof(nid.szInfo));
                lstrcpynW(nid.szInfoTitle, (WCHAR *)msn->szInfoTitle, safe_sizeof(nid.szInfoTitle));
                nid.szInfo[safe_sizeof(nid.szInfo) - 1] = 0;
                nid.szInfoTitle[safe_sizeof(nid.szInfoTitle) - 1] = 0;
                nid.uTimeout = msn->uTimeout;
                nid.dwInfoFlags = (msn->dwInfoFlags & ~NIIF_INTERN_UNICODE);
                return Shell_NotifyIconW(NIM_MODIFY, (void*) &nid) == 0;
            }
            return 2;
        }
    }
    else {
#endif        
        if (msn && msn->cbSize == sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
            if (trayIcon) {
                NOTIFYICONDATA_NEW nid = {0};
                nid.cbSize = sizeof(nid);
                nid.hWnd = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);
                if (msn->szProto) {
                    int j;
                    for (j = 0; j < trayIconCount; j++) {
                        if (trayIcon[j].szProto != NULL) {
                            if (strcmp(msn->szProto, trayIcon[j].szProto) == 0) {
                                nid.uID = trayIcon[j].id;
                                j = trayIconCount;
                                continue;
                            }
                        } else {
                            if (trayIcon[j].isBase) {
                                nid.uID = trayIcon[j].id;
                                j = trayIconCount;
                                continue;
                            }
                        }
                    } //for
                } else {
                    nid.uID = trayIcon[0].id;
                }
                nid.uFlags = NIF_INFO;
                lstrcpynA(nid.szInfo, msn->szInfo, sizeof(nid.szInfo));
                lstrcpynA(nid.szInfoTitle, msn->szInfoTitle, sizeof(nid.szInfoTitle));
                nid.szInfo[sizeof(nid.szInfo) - 1] = 0;
                nid.szInfoTitle[sizeof(nid.szInfoTitle) - 1] = 0;
                nid.uTimeout = msn->uTimeout;
                nid.dwInfoFlags = msn->dwInfoFlags;
                return Shell_NotifyIconA(NIM_MODIFY, (void*) &nid) == 0;
            }
            return 2;
        }
#if defined(_UNICODE)
    }
#endif    
    return 1;
}

void InitTray(void)
{
    HINSTANCE hLib;

    hLib = LoadLibraryA("shell32.dll");
    if (hLib) {
        DLLGETVERSIONPROC proc;
        dviShell.cbSize = sizeof(dviShell);
        proc = (DLLGETVERSIONPROC) GetProcAddress(hLib, "DllGetVersion");
        if (proc) {
            proc(&dviShell);
        }
        FreeLibrary(hLib);
    }
    if (dviShell.dwMajorVersion >= 5) {
        CreateServiceFunction(MS_CLIST_SYSTRAY_NOTIFY, CListTrayNotify);
    }
    return;
}