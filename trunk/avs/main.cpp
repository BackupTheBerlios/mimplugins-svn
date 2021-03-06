/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
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
*/

#include "commonheaders.h"

HINSTANCE   g_hInst = 0;
PLUGINLINK  *pluginLink;

static int      g_shutDown = FALSE;
static          PROTOCOLDESCRIPTOR **g_protocols = NULL;
static char     g_szDBPath[MAX_PATH];		// database profile path (read at startup only)
static DWORD    dwMainThreadID;
static BOOL     g_MetaAvail = FALSE;
BOOL            g_AvatarHistoryAvail = FALSE;
static long     hwndSetMyAvatar = 0;
static HANDLE   hMyAvatarsFolder = 0;
static HANDLE   hProtocolAvatarsFolder = 0;
static HANDLE   hLoaderEvent = 0;
static HANDLE  hLoaderThread = 0;

int g_protocount = 0;

HANDLE  hProtoAckHook = 0, hContactSettingChanged = 0, hEventChanged = 0, hEventContactAvatarChanged = 0,
		hMyAvatarChanged = 0, hEventDeleted = 0, hUserInfoInitHook = 0;
HICON   g_hIcon = 0;

static struct   CacheNode *g_Cache = 0;
struct          protoPicCacheEntry *g_ProtoPictures = 0;
static struct   protoPicCacheEntry *g_MyAvatars = 0;

static  CRITICAL_SECTION cachecs, alloccs;
char    *g_szMetaName = NULL;


#ifndef SHVIEW_THUMBNAIL
# define SHVIEW_THUMBNAIL 0x702D
#endif

// Stores the id of the dialog

int         ChangeAvatar(HANDLE hContact, BOOL fLoad, BOOL fNotifyHist = FALSE, int pa_format = 0);
static int	ShutdownProc(WPARAM wParam, LPARAM lParam);
static int  OkToExitProc(WPARAM wParam, LPARAM lParam);
static int  OnDetailsInit(WPARAM wParam, LPARAM lParam);
static int  GetFileHash(char* filename);

BOOL Proto_IsAvatarsEnabled(char *proto);
BOOL Proto_IsAvatarFormatSupported(char *proto, int format);
void Proto_GetAvatarMaxSize(char *proto, int &width, int &height);
int Proto_AvatarImageProportion(char *proto);


PLUGININFO pluginInfo = {
    sizeof(PLUGININFO), 
#if defined(_UNICODE)
	"Avatar service (Unicode)",
#else
	"Avatar service",
#endif
	PLUGIN_MAKE_VERSION(0, 0, 2, 14), 
	"Load and manage contact pictures for other plugins", 
	"Nightwish, Pescuma", 
	"", 
	"Copyright 2000-2007 Miranda-IM project", 
	"http://www.miranda-im.org", 
	UNICODE_AWARE, 
	0
};

PLUGININFOEX pluginInfoEx = {
    sizeof(PLUGININFOEX), 
#if defined(_UNICODE)
	"Avatar service (Unicode)",
#else
	"Avatar service",
#endif
	PLUGIN_MAKE_VERSION(0, 0, 2, 13), 
	"Load and manage contact pictures for other plugins", 
	"Nightwish, Pescuma", 
	"", 
	"Copyright 2000-2005 Miranda-IM project", 
	"http://www.miranda-im.org", 
	UNICODE_AWARE, 
	0,
#if defined(_UNICODE)
// {E00F1643-263C-4599-B84B-053E5C511D28}
    { 0xe00f1643, 0x263c, 0x4599, { 0xb8, 0x4b, 0x5, 0x3e, 0x5c, 0x51, 0x1d, 0x28 } }
#else
// {C9E01EB0-A119-42d2-B340-E8678F5FEAD9}
    { 0xc9e01eb0, 0xa119, 0x42d2, { 0xb3, 0x40, 0xe8, 0x67, 0x8f, 0x5f, 0xea, 0xd9 } }
#endif	
};

extern BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcAvatarOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcAvatarUserInfo(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcAvatarProtoInfo(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


static int SetProtoMyAvatar(char *protocol, HBITMAP hBmp, char *originalFilename, int format);


// See if a protocol service exists
int ProtoServiceExists(const char *szModule,const char *szService)
{
	char str[MAXMODULELABELLENGTH * 2];
	strcpy(str,szModule);
	strcat(str,szService);
	return ServiceExists(str);
}


/*
 * output a notification message.
 * may accept a hContact to include the contacts nickname in the notification message...
 * the actual message is using printf() rules for formatting and passing the arguments...
 *
 * can display the message either as systray notification (baloon popup) or using the
 * popup plugin.
 */

#ifdef _DEBUG

int _DebugTrace(const char *fmt, ...)
{
    char    debug[2048];
    int     ibsize = 2047;
    va_list va;
    va_start(va, fmt);

	mir_snprintf(debug, sizeof(debug) - 10, " ***** AVS [%08d] [ID:%04x]: ", GetTickCount(), GetCurrentThreadId());
    OutputDebugStringA(debug);
	_vsnprintf(debug, ibsize, fmt, va);
    OutputDebugStringA(debug);
    OutputDebugStringA(" ***** \n");

	return 0;
}

int _DebugTrace(HANDLE hContact, const char *fmt, ...)
{
	char text[1024];
	size_t len;
    va_list va;

	char *name = NULL;
	char *proto = NULL;
	if (hContact != NULL)
	{
		name = (char*) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0);
		proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	}

	mir_snprintf(text, sizeof(text) - 10, " ***** AVS [%08d] [ID:%04x]: [%08d - %s - %s] ", 
		GetTickCount(), GetCurrentThreadId(), hContact, proto == NULL ? "" : proto, name == NULL ? "" : name);
	len = strlen(text);

    va_start(va, fmt);
    mir_vsnprintf(&text[len], sizeof(text) - len, fmt, va);
    va_end(va);

	OutputDebugStringA(text);
    OutputDebugStringA(" ***** \n");

	return 0;
}

#endif

/*
 * path utilities (make avatar paths relative to *PROFILE* directory, not miranda directory.
 * taken and modified from core services
 */

static int AVS_pathIsAbsolute(const char *path)
{
    if (!path || !(strlen(path) > 2)) 
        return 0;
    if ((path[1]==':'&&path[2]=='\\')||(path[0]=='\\'&&path[1]=='\\')) return 1;
    return 0;
}

int AVS_pathToRelative(const char *pSrc, char *pOut)
{
    if (!pSrc||!strlen(pSrc)||strlen(pSrc)>MAX_PATH) return 0;
    if (!AVS_pathIsAbsolute(pSrc)) {
        mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
        return strlen(pOut);
    }
    else {
        char szTmp[MAX_PATH];

        mir_snprintf(szTmp, SIZEOF(szTmp), "%s", pSrc);
        _strlwr(szTmp);
        if (strstr(szTmp, g_szDBPath)) {
            mir_snprintf(pOut, MAX_PATH, "%s", pSrc + strlen(g_szDBPath) + 1);
            return strlen(pOut);
        }
        else {
            mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
            return strlen(pOut);
        }
    }
}

int AVS_pathToAbsolute(const char *pSrc, char *pOut) 
{
    if (!pSrc||!strlen(pSrc)||strlen(pSrc)>MAX_PATH) return 0;
    if (AVS_pathIsAbsolute(pSrc)||!isalnum(pSrc[0])) {
        mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
        return strlen(pOut);
    }
    else {
        mir_snprintf(pOut, MAX_PATH, "%s\\%s", g_szDBPath, pSrc);
        return strlen(pOut);
    }
}

static void NotifyMetaAware(HANDLE hContact, struct CacheNode *node = NULL, AVATARCACHEENTRY *ace = (AVATARCACHEENTRY *)0xffffffff)
{
    if(ace == (AVATARCACHEENTRY *)0xffffffff)
        ace = &node->ace;

    NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)ace);
   
    if(g_MetaAvail && (node->dwFlags & MC_ISSUBCONTACT) && DBGetContactSettingByte(NULL, g_szMetaName, "Enabled", 0)) {
        HANDLE hMasterContact = (HANDLE)DBGetContactSettingDword(hContact, g_szMetaName, "Handle", 0);

        if(hMasterContact && (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)hMasterContact, 0) == hContact &&
           !DBGetContactSettingByte(hMasterContact, "ContactPhoto", "Locked", 0))
                NotifyEventHooks(hEventChanged, (WPARAM)hMasterContact, (LPARAM)ace);
    }
    if(node->dwFlags & AVH_MUSTNOTIFY) {
        // Fire the event for avatar history
        node->dwFlags &= ~AVH_MUSTNOTIFY;
        if(node->loaded) {
            CONTACTAVATARCHANGEDNOTIFICATION cacn = {0};
            cacn.cbSize = sizeof(CONTACTAVATARCHANGEDNOTIFICATION);
            cacn.hContact = hContact;
            cacn.format = node->pa_format;
            strncpy(cacn.filename, node->ace.szFilename, MAX_PATH);
            cacn.filename[MAX_PATH - 1] = 0;

			// Get hash
			char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
			if (szProto != NULL) {
				DBVARIANT dbv = {0};
				if (!DBGetContactSettingStringUtf(hContact, szProto, "AvatarHash", &dbv)) {
					if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_UTF8) {
						strncpy(cacn.hash, dbv.pszVal, sizeof(cacn.hash));
						DBFreeVariant(&dbv);
					} else if (dbv.type == DBVT_BLOB) {
						// Lets use base64 encode
						char *tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
						int i;
						for(i = 0; i < dbv.cpbVal / 3 && i*4+3 < sizeof(cacn.hash)-1; i++) {
							BYTE a = dbv.pbVal[i*3];
							BYTE b = i*3 + 1 < dbv.cpbVal ? dbv.pbVal[i*3 + 1] : 0;
							BYTE c = i*3 + 2 < dbv.cpbVal ? dbv.pbVal[i*3 + 2] : 0;

							cacn.hash[i*4] = tab[(a & 0xFC) >> 2];
							cacn.hash[i*4+1] = tab[((a & 0x3) << 4) + ((b & 0xF0) >> 4)];
							cacn.hash[i*4+2] = tab[((b & 0xF) << 2) + ((c & 0xC0) >> 6)];
							cacn.hash[i*4+3] = tab[c & 0x3F];
						}
						if (dbv.cpbVal % 3 != 0 && i*4+3 < sizeof(cacn.hash)-1) {
							BYTE a = dbv.pbVal[i*3];
							BYTE b = i*3 + 1 < dbv.cpbVal ? dbv.pbVal[i*3 + 1] : 0;

							cacn.hash[i*4] = tab[(a & 0xFC) >> 2];
							cacn.hash[i*4+1] = tab[((a & 0x3) << 4) + ((b & 0xF0) >> 4)];
							if (i + 1 < dbv.cpbVal)
								cacn.hash[i*4+2] = tab[((b & 0xF) << 4)];
							else
								cacn.hash[i*4+2] = '=';
							cacn.hash[i*4+3] = '=';
						}
					}
				}
			}

			// Default value
			if (cacn.hash[0] == '\0')
				mir_snprintf(cacn.hash, sizeof(cacn.hash), "AVS-HASH-%x", GetFileHash(cacn.filename));

            NotifyEventHooks(hEventContactAvatarChanged, (WPARAM)cacn.hContact, (LPARAM)&cacn);
        }
        else
            NotifyEventHooks(hEventContactAvatarChanged, (WPARAM)hContact, NULL);
    }
}

static int g_maxBlock = 0, g_curBlock = 0;
static struct CacheNode **g_cacheBlocks = NULL;

/*
 * allocate a cache block and add it to the list of blocks
 * does not link the new block with the old block(s) - caller needs to do this
 */

static struct CacheNode *AllocCacheBlock()
{
	struct CacheNode *allocedBlock = NULL;

	allocedBlock = (struct CacheNode *)malloc(CACHE_BLOCKSIZE * sizeof(struct CacheNode));
	ZeroMemory((void *)allocedBlock, sizeof(struct CacheNode) * CACHE_BLOCKSIZE);

	for(int i = 0; i < CACHE_BLOCKSIZE - 1; i++)
	{
		//InitializeCriticalSection(&allocedBlock[i].cs);
		allocedBlock[i].pNextNode = &allocedBlock[i + 1];				// pre-link the alloced block
	}
	//InitializeCriticalSection(&allocedBlock[CACHE_BLOCKSIZE - 1].cs);

	if(g_Cache == NULL)													// first time only...
		g_Cache = allocedBlock;

	// add it to the list of blocks

	if(g_curBlock == g_maxBlock) {
		g_maxBlock += 10;
		g_cacheBlocks = (struct CacheNode **)realloc(g_cacheBlocks, g_maxBlock * sizeof(struct CacheNode *));
    }
	g_cacheBlocks[g_curBlock++] = allocedBlock;

	return(allocedBlock);
}

int _DebugPopup(HANDLE hContact, const char *fmt, ...)
{
    POPUPDATA ppd;
    va_list va;
    char    debug[1024];
    int     ibsize = 1023;
    
    if(!DBGetContactSettingByte(0, AVS_MODULE, "warnings", 0))
        return 0;
    
    va_start(va, fmt);
    _vsnprintf(debug, ibsize, fmt, va);
    
    if(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
        ZeroMemory((void *)&ppd, sizeof(ppd));
        ppd.lchContact = hContact;
        ppd.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        strncpy(ppd.lpzContactName, "Avatar Service Warning:", MAX_CONTACTNAME);
        mir_snprintf(ppd.lpzText, MAX_SECONDLINE - 5, "%s\nAffected contact: %s", debug, hContact != 0 ? (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0) : "Global");
        ppd.colorText = RGB(0,0,0);
        ppd.colorBack = RGB(255,0,0);
        CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
    }
    return 0;
}

int _TracePopup(HANDLE hContact, const char *fmt, ...)
{
    POPUPDATA ppd;
    va_list va;
    char    debug[1024];
    int     ibsize = 1023;
    
    va_start(va, fmt);
    _vsnprintf(debug, ibsize, fmt, va);
    
    ZeroMemory((void *)&ppd, sizeof(ppd));
    ppd.lchContact = hContact;
	ppd.lchIcon = g_hIcon;
    strncpy(ppd.lpzContactName, "Avatar Service TRACE:", MAX_CONTACTNAME);
    mir_snprintf(ppd.lpzText, MAX_SECONDLINE - 5, "%s\nAffected contact: %s", debug, hContact != 0 ? (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0) : "Global");
    ppd.colorText = RGB(0,0,0);
    ppd.colorBack = RGB(255,0,0);
    CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
    
    return 0;
}

int SetAvatarAttribute(HANDLE hContact, DWORD attrib, int mode)
{
	struct CacheNode *cacheNode = g_Cache;

	while(cacheNode) {
		if(cacheNode->ace.hContact == hContact) {
            DWORD dwFlags = cacheNode->ace.dwFlags;

            cacheNode->ace.dwFlags = mode ? cacheNode->ace.dwFlags | attrib : cacheNode->ace.dwFlags & ~attrib;
            if(cacheNode->ace.dwFlags != dwFlags)
                NotifyMetaAware(hContact, cacheNode);
            break;
        }
		cacheNode = cacheNode->pNextNode;
	}
    return 0;
}

/*
 * convert the avatar image path to a relative one...
 * given: contact handle, path to image
 */
void MakePathRelative(HANDLE hContact, char *path) 
{
	char szFinalPath[MAX_PATH];
	szFinalPath[0] = '\0';

	int result = AVS_pathToRelative(path, szFinalPath);
	if(result && lstrlenA(szFinalPath) > 0) {
	   DBWriteContactSettingString(hContact, "ContactPhoto", "RFile", szFinalPath);
	   if(!DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0))
		   DBWriteContactSettingString(hContact, "ContactPhoto", "Backup", szFinalPath);
	}
}

/*
 * convert the avatar image path to a relative one...
 * given: contact handle
 */

static void MakePathRelative(HANDLE hContact)
{
	DBVARIANT dbv = {0};

	if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
		if(dbv.type == DBVT_ASCIIZ) {
			MakePathRelative(hContact, dbv.pszVal);
		}
		DBFreeVariant(&dbv);
	}
}


static void ResetTranspSettings(HANDLE hContact)
{
	DBDeleteContactSetting(hContact, "ContactPhoto", "MakeTransparentBkg");
	DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgNumPoints");
	DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgColorDiff");
}


static char *getJGMailID(char *szProto)
{
	static char szJID[MAX_PATH+1];
	DBVARIANT dbva={0}, dbvb={0};

	szJID[0] = '\0';
	if(DBGetContactSetting(NULL, szProto, "LoginName", &dbva))
		return szJID;
	if(DBGetContactSetting(NULL, szProto, "LoginServer", &dbvb)) {
		DBFreeVariant(&dbva);
		return szJID;
	}

	mir_snprintf(szJID, sizeof(szJID), "%s@%s", dbva.pszVal, dbvb.pszVal);
	DBFreeVariant(&dbva);
	DBFreeVariant(&dbvb);
	return szJID;
}


// create the avatar in cache
// returns 0 if not created (no avatar), iIndex otherwise, -2 if has to request avatar
int CreateAvatarInCache(HANDLE hContact, struct avatarCacheEntry *ace, char *szProto)
{
    DBVARIANT dbv = {0};
    char *szExt = NULL;
    char szFilename[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSizeHigh = 0, dwFileSize = 0, sizeLimit = 0;
    
    szFilename[0] = 0;

    if(szProto == NULL) {
        if(DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0)
				&& !DBGetContactSetting(hContact, "ContactPhoto", "Backup", &dbv)) {
            AVS_pathToAbsolute(dbv.pszVal, szFilename);
            DBFreeVariant(&dbv);
        }
        else if(!DBGetContactSetting(hContact, "ContactPhoto", "RFile", &dbv)) {
            AVS_pathToAbsolute(dbv.pszVal, szFilename);
            DBFreeVariant(&dbv);
        }
        else if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
            AVS_pathToAbsolute(dbv.pszVal, szFilename);
            DBFreeVariant(&dbv);
        }
        else {
            return -2;
        }
    }
    else {
		if(hContact == 0) {				// create a protocol picture in the proto picture cache
			if(!DBGetContactSetting(NULL, PPICT_MODULE, szProto, &dbv)) {
                AVS_pathToAbsolute(dbv.pszVal, szFilename);
				DBFreeVariant(&dbv);
			}
		}
		else if(hContact == (HANDLE)-1) {	// create own picture - note, own avatars are not on demand, they are loaded once at
											// startup and everytime they are changed.
			if (ProtoServiceExists(szProto, PS_GETMYAVATAR)) {
				if (CallProtoService(szProto, PS_GETMYAVATAR, (WPARAM)szFilename, (LPARAM)MAX_PATH)) {
					szFilename[0] = '\0';
				}
			} else if(!DBGetContactSetting(NULL, szProto, "AvatarFile", &dbv)) {
                AVS_pathToAbsolute(dbv.pszVal, szFilename);
				DBFreeVariant(&dbv);
			}
			else {
				char szTestFile[MAX_PATH];
				HANDLE hFFD;
				WIN32_FIND_DATAA ffd = {0};
				char inipath[2048];

				// First try protocol avatars folder
				FoldersGetCustomPath(hProtocolAvatarsFolder, inipath, sizeof(inipath), g_szDBPath);

				_snprintf(szTestFile, MAX_PATH, "%s\\MSN\\%s avatar.*", inipath, szProto);
				szTestFile[MAX_PATH - 1] = 0;
				if((hFFD = FindFirstFileA(szTestFile, &ffd)) != INVALID_HANDLE_VALUE) {
					_snprintf(szFilename, MAX_PATH, "%s\\MSN\\%s", inipath, ffd.cFileName);
					FindClose(hFFD);
					goto done;
				}
				_snprintf(szTestFile, MAX_PATH, "%s\\jabber\\%s avatar.*", inipath, szProto);
				szTestFile[MAX_PATH - 1] = 0;
				if((hFFD = FindFirstFileA(szTestFile, &ffd)) != INVALID_HANDLE_VALUE) {
					_snprintf(szFilename, MAX_PATH, "%s\\jabber\\%s", inipath, ffd.cFileName);
					FindClose(hFFD);
					goto done;
				}
				_snprintf(szTestFile, MAX_PATH, "%s\\jabber\\%s avatar.*", inipath, getJGMailID(szProto));
				szTestFile[MAX_PATH - 1] = 0;
				if((hFFD = FindFirstFileA(szTestFile, &ffd)) != INVALID_HANDLE_VALUE) {
					_snprintf(szFilename, MAX_PATH, "%s\\jabber\\%s", inipath, ffd.cFileName);
					FindClose(hFFD);
					goto done;
				}
				// Now try profile folder
				if (_strcmpi(inipath, g_szDBPath)) {
					strcpy(inipath, g_szDBPath);

					_snprintf(szTestFile, MAX_PATH, "%s\\MSN\\%s avatar.*", inipath, szProto);
					szTestFile[MAX_PATH - 1] = 0;
					if((hFFD = FindFirstFileA(szTestFile, &ffd)) != INVALID_HANDLE_VALUE) {
						_snprintf(szFilename, MAX_PATH, "%s\\MSN\\%s", inipath, ffd.cFileName);
						FindClose(hFFD);
						goto done;
					}
					_snprintf(szTestFile, MAX_PATH, "%s\\jabber\\%s avatar.*", inipath, szProto);
					szTestFile[MAX_PATH - 1] = 0;
					if((hFFD = FindFirstFileA(szTestFile, &ffd)) != INVALID_HANDLE_VALUE) {
						_snprintf(szFilename, MAX_PATH, "%s\\jabber\\%s", inipath, ffd.cFileName);
						FindClose(hFFD);
						goto done;
					}
					_snprintf(szTestFile, MAX_PATH, "%s\\jabber\\%s avatar.*", inipath, getJGMailID(szProto));
					szTestFile[MAX_PATH - 1] = 0;
					if((hFFD = FindFirstFileA(szTestFile, &ffd)) != INVALID_HANDLE_VALUE) {
						_snprintf(szFilename, MAX_PATH, "%s\\jabber\\%s", inipath, ffd.cFileName);
						FindClose(hFFD);
						goto done;
					}
				}
				return -1;
			}
		}
    }
done:
    if(lstrlenA(szFilename) < 4)
        return -1;

    if((hFile = CreateFileA(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        return -2;
    }

	sizeLimit = DBGetContactSettingDword(0, AVS_MODULE, "SizeLimit", 70);
    dwFileSize = GetFileSize(hFile, &dwFileSizeHigh) / 1024;
    CloseHandle(hFile);
    if(dwFileSize > 0 && dwFileSize > sizeLimit && sizeLimit > 0) {
        char szBuf[256];
        char szTitle[64];
        MIRANDASYSTRAYNOTIFY msn = {0};

        if(!DBGetContactSettingByte(0, AVS_MODULE, "warnings", 0))
            return -1;

        _snprintf(szBuf, 256, "The selected contact picture is too large. The current file size limit for avatars are %d bytes.\nYou can increase that limit on the options page", sizeLimit * 1024);
        _snprintf(szTitle, 64, "Avatar Service warning (%s)", (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
        msn.szInfoTitle = szTitle;
        msn.szInfo = szBuf;
        msn.cbSize = sizeof(msn);
        msn.uTimeout = 10000;
        CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM)&msn);
        return -1;
    }

	ace->hbmPic = (HBITMAP) BmpFilterLoadBitmap32(NULL, (LPARAM)szFilename);
	ace->dwFlags = 0;
	ace->bmHeight = 0;
	ace->bmWidth = 0;
	ace->lpDIBSection = NULL;
    if(ace->hbmPic != 0) {
        BITMAP bminfo;

        GetObject(ace->hbmPic, sizeof(bminfo), &bminfo);

        ace->cbSize = sizeof(struct avatarCacheEntry);
        ace->dwFlags = AVS_BITMAP_VALID;
        if(DBGetContactSettingByte(hContact, "ContactPhoto", "Hidden", 0))
            ace->dwFlags |= AVS_HIDEONCLIST;
        ace->hContact = hContact;
        ace->bmHeight = bminfo.bmHeight;
        ace->bmWidth = bminfo.bmWidth;
        strncpy(ace->szFilename, szFilename, MAX_PATH);
        ace->szFilename[MAX_PATH - 1] = 0;

		// Calc image hash
		if (hContact != 0 && hContact != (HANDLE)-1)
		{
			// Have to reset settings? -> do it if image changed
			DWORD imgHash = GetImgHash(ace->hbmPic);

			if (imgHash != DBGetContactSettingDword(hContact, "ContactPhoto", "ImageHash", 0))
			{
				ResetTranspSettings(hContact);
				DBWriteContactSettingDword(hContact, "ContactPhoto", "ImageHash", imgHash);
			}

			// Make transparent?
			if (DBGetContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", 
					DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0)))
			{
				if (MakeTransparentBkg(hContact, &ace->hbmPic))
				{
					ace->dwFlags |= AVS_CUSTOMTRANSPBKG | AVS_HASTRANSPARENCY;
					GetObject(ace->hbmPic, sizeof(bminfo), &bminfo);
				}
			}
		}
		else if (hContact == (HANDLE)-1) // My avatars
		{
			if (DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0)
				&& DBGetContactSettingByte(0, AVS_MODULE, "MakeMyAvatarsTransparent", 0))
			{
				if (MakeTransparentBkg(0, &ace->hbmPic))
				{
					ace->dwFlags |= AVS_CUSTOMTRANSPBKG | AVS_HASTRANSPARENCY;
					GetObject(ace->hbmPic, sizeof(bminfo), &bminfo);
				}
			}
		}

		if (DBGetContactSettingByte(0, AVS_MODULE, "MakeGrayscale", 0))
		{
			MakeGrayscale(hContact, &ace->hbmPic);
		}

		if (DBGetContactSettingByte(0, AVS_MODULE, "RemoveAllTransparency", 0))
		{
			CorrectBitmap32Alpha(ace->hbmPic, TRUE);
		}
        if (bminfo.bmBitsPixel == 32)
		{
			if (PreMultiply(ace->hbmPic))
			{
				ace->dwFlags |= AVS_HASTRANSPARENCY;
			}
			ace->dwFlags |= AVS_PREMULTIPLIED;
		}
        if(szProto) {
            struct protoPicCacheEntry *pAce = (struct protoPicCacheEntry *)ace;
			if(hContact == 0)
				pAce->dwFlags |= AVS_PROTOPIC;
			else if(hContact == (HANDLE)-1)
				pAce->dwFlags |= AVS_OWNAVATAR;
			strncpy(pAce->szProtoname, szProto, 100);
			pAce->szProtoname[99] = 0;
        }

        return 1;
    }
    return -1;
}

/*
 * link a new cache block with the already existing chain of blocks
 */

static struct CacheNode *AddToList(struct CacheNode *node) {
    struct CacheNode *pCurrent = g_Cache;

    while(pCurrent->pNextNode != 0)
        pCurrent = pCurrent->pNextNode;

    pCurrent->pNextNode = node;
    return pCurrent;
}

struct CacheNode *FindAvatarInCache(HANDLE hContact, BOOL add)
{
	struct CacheNode *cacheNode = g_Cache, *foundNode = NULL;

    EnterCriticalSection(&cachecs);

	while(cacheNode) 
	{
		if(cacheNode->ace.hContact == hContact)
		{
            cacheNode->ace.t_lastAccess = time(NULL);
            if(cacheNode->loaded) {
                LeaveCriticalSection(&cachecs);
                return cacheNode;
            }
            else {
                LeaveCriticalSection(&cachecs);
                return cacheNode;
            }
		}

		if(foundNode == NULL && cacheNode->ace.hContact == 0)
			foundNode = cacheNode;				// found an empty and usable node

		cacheNode = cacheNode->pNextNode;
    }

    // not found

	if (add)
	{
		if(foundNode == NULL) {					// no free entry found, create a new and append it to the list
            EnterCriticalSection(&alloccs);     // protect memory block allocation
			struct CacheNode *newNode = AllocCacheBlock();
			AddToList(newNode);
			foundNode = newNode;
            LeaveCriticalSection(&alloccs);
		}

		foundNode->ace.hContact = hContact;
        if(g_MetaAvail)
            foundNode->dwFlags |= (DBGetContactSettingByte(hContact, g_szMetaName, "IsSubcontact", 0) ? MC_ISSUBCONTACT : 0);
		foundNode->loaded = FALSE;
        foundNode->mustLoad = 1;                                 // pic loader will watch this and load images
        LeaveCriticalSection(&cachecs);
        SetEvent(hLoaderEvent);                                     // wake him up
        return NULL;
	}
	else
	{
		foundNode = NULL;
	}
    LeaveCriticalSection(&cachecs);
	return foundNode;
}

#define POLYNOMIAL (0x488781ED) /* This is the CRC Poly */
#define TOPBIT (1 << (WIDTH - 1)) /* MSB */
#define WIDTH 32

static int GetFileHash(char* filename)
{
   HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if(hFile == INVALID_HANDLE_VALUE)
	   return 0;

	int remainder = 0;
	char data[1024];
	DWORD dwRead;
	do
	{
		// Read file chunk
		dwRead = 0;
		ReadFile(hFile, data, 1024, &dwRead, NULL);

		/* loop through each byte of data */
		for (int byte = 0; byte < (int) dwRead; ++byte) {
			/* store the next byte into the remainder */
			remainder ^= (data[byte] << (WIDTH - 8));
			/* calculate for all 8 bits in the byte */
			for (int bit = 8; bit > 0; --bit) {
				/* check if MSB of remainder is a one */
				if (remainder & TOPBIT)
					remainder = (remainder << 1) ^ POLYNOMIAL;
				else
					remainder = (remainder << 1);
			}
		}
	} while(dwRead == 1024);

	CloseHandle(hFile);

	return remainder;
}

static int ProtocolAck(WPARAM wParam, LPARAM lParam)
{
    ACKDATA *ack = (ACKDATA *) lParam;

	if (ack != NULL && ack->type == ACKTYPE_AVATAR && ack->hContact != 0
		// Ignore metacontacts
		&& (!g_MetaAvail || strcmp(ack->szModule, g_szMetaName))) 
	{
        if (ack->result == ACKRESULT_SUCCESS) 
		{
			PROTO_AVATAR_INFORMATION *pai = (PROTO_AVATAR_INFORMATION *) ack->hProcess;
			if(pai == NULL)
				return 0;

			// Fix settings in DB
			DBDeleteContactSetting(pai->hContact, "ContactPhoto", "NeedUpdate");
			DBDeleteContactSetting(pai->hContact, "ContactPhoto", "RFile");
			if (!DBGetContactSettingByte(pai->hContact, "ContactPhoto", "Locked", 0))
				DBDeleteContactSetting(pai->hContact, "ContactPhoto", "Backup");
			DBWriteContactSettingString(pai->hContact, "ContactPhoto", "File", pai->filename);
			DBWriteContactSettingWord(pai->hContact, "ContactPhoto", "Format", pai->format);

			if (pai->format == PA_FORMAT_PNG || pai->format == PA_FORMAT_JPEG 
				|| pai->format == PA_FORMAT_ICON  || pai->format == PA_FORMAT_BMP
				|| pai->format == PA_FORMAT_GIF)
			{
				// We can load it!
				MakePathRelative(pai->hContact, pai->filename);
				ChangeAvatar(pai->hContact, TRUE, TRUE, pai->format);
			}
			else
			{
				// As we can't load it, notify as deleted
				ChangeAvatar(pai->hContact, FALSE, TRUE, pai->format);
			}
        }
        else if(ack->result == ACKRESULT_FAILED) 
		{
			if (DBGetContactSettingByte(NULL, AVS_MODULE, "RemoveAvatarOnFail", 0)) 
			{
				// Fix settings in DB
				DBDeleteContactSetting(ack->hContact, "ContactPhoto", "RFile");
				if (!DBGetContactSettingByte(ack->hContact, "ContactPhoto", "Locked", 0))
					DBDeleteContactSetting(ack->hContact, "ContactPhoto", "Backup");
				DBDeleteContactSetting(ack->hContact, "ContactPhoto", "File");
				DBDeleteContactSetting(ack->hContact, "ContactPhoto", "Format");
				// Fix cache
				ChangeAvatar(ack->hContact, FALSE, FALSE, 0);
			}
        }
		else if(ack->result == ACKRESULT_STATUS) 
		{
			DBWriteContactSettingByte(ack->hContact, "ContactPhoto", "NeedUpdate", 1);
			QueueAdd(requestQueue, ack->hContact);
		}
    }
    return 0;
}

int ProtectAvatar(WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact = (HANDLE)wParam;
    BYTE was_locked = DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0);

    if(was_locked == (BYTE)lParam)      // no need for redundant lockings...
        return 0;
    
    if(hContact) {
        if(!was_locked)
            MakePathRelative(hContact);
        DBWriteContactSettingByte(hContact, "ContactPhoto", "Locked", lParam ? 1 : 0);
        if(lParam == 0)
            MakePathRelative(hContact);
        ChangeAvatar(hContact, TRUE);
    }
    return 0;
}

/*
 * for subclassing the open file dialog...
 */
struct OpenFileSubclassData {
	BYTE *locking_request;
	BYTE setView;
};
static BOOL CALLBACK OpenFileSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_INITDIALOG: 
		{
            OPENFILENAMEA *ofn = (OPENFILENAMEA *)lParam;

			OpenFileSubclassData *data = (OpenFileSubclassData *) malloc(sizeof(OpenFileSubclassData));
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)data);
            data->locking_request = (BYTE *)ofn->lCustData;
			data->setView = TRUE;

            CheckDlgButton(hwnd, IDC_PROTECTAVATAR, *(data->locking_request));
            break;
        }
        case WM_COMMAND:
		{
            if(LOWORD(wParam) == IDC_PROTECTAVATAR) 
			{
                OpenFileSubclassData *data= (OpenFileSubclassData *)GetWindowLong(hwnd, GWL_USERDATA);
                *(data->locking_request) = IsDlgButtonChecked(hwnd, IDC_PROTECTAVATAR) ? TRUE : FALSE;
            }
            break;
		}
		case WM_NOTIFY:
		{
            OpenFileSubclassData *data= (OpenFileSubclassData *)GetWindowLong(hwnd, GWL_USERDATA);
			if (data->setView)
			{
				HWND hwndParent = GetParent(hwnd);
				HWND hwndLv = FindWindowEx(hwndParent, NULL, _T("SHELLDLL_DefView"), NULL) ;
				if (hwndLv != NULL) 
				{
					SendMessage(hwndLv, WM_COMMAND, SHVIEW_THUMBNAIL, 0);
					data->setView = FALSE;
				}
			}
			break;
		}
		case WM_NCDESTROY:
		{
            OpenFileSubclassData *data= (OpenFileSubclassData *)GetWindowLong(hwnd, GWL_USERDATA);
			free((OpenFileSubclassData *)data);
			SetWindowLong(hwnd, GWL_USERDATA, (LONG)0);
			break;
		}
	}

    return FALSE;
}

/*
 * set an avatar (service function)
 * if lParam == NULL, a open file dialog will be opened, otherwise, lParam is taken as a FULL
 * image filename (will be checked for existance, though)
 */

int SetAvatar(WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact = 0;
    BYTE is_locked = 0;
    char FileName[MAX_PATH], szBackupName[MAX_PATH];
    char *szFinalName = NULL;
    HANDLE hFile = 0;
    BYTE locking_request;
    
    if(wParam == 0)
        return 0;
    else
        hContact = (HANDLE)wParam;
    
    is_locked = DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0);
    
    if(lParam == 0) {
        OPENFILENAMEA ofn = {0};
		char filter[256];

		filter[0] = '\0';
		CallService(MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof(filter), (LPARAM) filter);

		if (IsWinVer2000Plus())
			ofn.lStructSize = sizeof(ofn);
		else
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
        ofn.hwndOwner = 0;
        ofn.lpstrFile = FileName;
		ofn.lpstrFilter = filter;
        ofn.nMaxFile = MAX_PATH;
        ofn.nMaxFileTitle = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLETEMPLATE | OFN_EXPLORER | OFN_ENABLESIZING | OFN_ENABLEHOOK;
        ofn.lpstrInitialDir = ".";
        *FileName = '\0';
        ofn.lpstrDefExt="";
        ofn.hInstance = g_hInst;
        ofn.lpTemplateName = MAKEINTRESOURCEA(IDD_OPENSUBCLASS);
        ofn.lpfnHook = (LPOFNHOOKPROC)OpenFileSubclass;
        locking_request = is_locked;
        ofn.lCustData = (LPARAM)&locking_request;
        if(GetOpenFileNameA(&ofn)) {
            szFinalName = FileName;
            is_locked = locking_request ? 1 : is_locked;
        }
        else
            return 0;
    }
    else
        szFinalName = (char *)lParam;

    /*
     * filename is now set, check it and perform all needed action
     */

    if((hFile = CreateFileA(szFinalName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        return 0;
        
    // file exists...

    CloseHandle(hFile);

    AVS_pathToRelative(szFinalName, szBackupName);
    DBWriteContactSettingString(hContact, "ContactPhoto", "Backup", szBackupName);

    DBWriteContactSettingByte(hContact, "ContactPhoto", "Locked", is_locked);
    DBWriteContactSettingString(hContact, "ContactPhoto", "File", szFinalName);
    MakePathRelative(hContact, szFinalName);
	// Fix cache
	ChangeAvatar(hContact, TRUE);

    return 0;
}

/*
 * see if is possible to set the avatar for the expecified protocol
 */
static int CanSetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *protocol = (char *) wParam;
    if(protocol == NULL)
        return 0;

	return ProtoServiceExists(protocol, PS_SETMYAVATAR);
}

/*
 * Callback to set thumbnaill view to open dialog
 */
static UINT_PTR CALLBACK OFNHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
		{
			InterlockedExchange(&hwndSetMyAvatar, (LONG) hwnd);

            SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);
            OPENFILENAMEA *ofn = (OPENFILENAMEA *)lParam;
			ofn->lCustData = TRUE;
			break;
		}
		case WM_NOTIFY:
		{
			OPENFILENAMEA *ofn = (OPENFILENAMEA *)GetWindowLong(hwnd, GWL_USERDATA);
			if (ofn->lCustData)
			{
				HWND hwndParent = GetParent(hwnd);
				HWND hwndLv = FindWindowEx(hwndParent, NULL, _T("SHELLDLL_DefView"), NULL) ;
				if (hwndLv != NULL) 
				{
					SendMessage(hwndLv, WM_COMMAND, SHVIEW_THUMBNAIL, 0);
					ofn->lCustData = FALSE;
				}
			}
			break;
		}
		case WM_DESTROY:
		{
			InterlockedExchange(&hwndSetMyAvatar, 0);
			break;
		}
	}

	return 0;
}


static int GetImageFormat(char *filename)
{
	size_t len = strlen(filename);

	if (len < 5)
	{
		return PA_FORMAT_UNKNOWN;
	}
	else if (strcmpi(".png", &filename[len-4]) == 0)
	{
		return PA_FORMAT_PNG;
	}
	else if (strcmpi(".jpg", &filename[len-4]) == 0 || strcmpi(".jpeg", &filename[len-4]) == 0)
	{
		return PA_FORMAT_JPEG;
	}
	else if (strcmpi(".ico", &filename[len-4]) == 0)
	{
		return PA_FORMAT_ICON;
	}
	else if (strcmpi(".bmp", &filename[len-4]) == 0 || strcmpi(".rle", &filename[len-4]) == 0)
	{
		return PA_FORMAT_BMP;
	}
	else if (strcmpi(".gif", &filename[len-4]) == 0)
	{
		return PA_FORMAT_GIF;
	}
	else if (strcmpi(".swf", &filename[len-4]) == 0)
	{
		return PA_FORMAT_SWF;
	}
	else if (strcmpi(".xml", &filename[len-4]) == 0)
	{
		return PA_FORMAT_XML;
	}
	else
	{
		return PA_FORMAT_UNKNOWN;
	}
}


static void FilterGetStrings(char *filter, int bytesLeft, BOOL xml, BOOL swf)
{
	char *pfilter;
	int wParam = bytesLeft;

	lstrcpynA(filter,Translate("All Files"),bytesLeft); bytesLeft-=lstrlenA(filter);
	strncat(filter," (*.bmp;*.jpg;*.gif;*.png",bytesLeft); 
	if (swf) strcat(filter,";*.swf");
	if (xml) strcat(filter,";*.xml");
	strcat(filter,")");
	pfilter=filter+lstrlenA(filter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.BMP;*.RLE;*.JPG;*.JPEG;*.GIF;*.PNG",bytesLeft);
	if (swf) strcat(pfilter,";*.SWF");
	if (xml) strcat(pfilter,";*.XML");
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("Windows Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*.bmp;*.rle)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.BMP;*.RLE",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("JPEG Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*.jpg;*.jpeg)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.JPG;*.JPEG",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("GIF Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*.gif)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.GIF",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("PNG Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*.png)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.PNG",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	if (swf)
	{
		lstrcpynA(pfilter,Translate("Flash Animations"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
		strncat(pfilter," (*.swf)",bytesLeft);
		pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
		lstrcpynA(pfilter,"*.SWF",bytesLeft);
		pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	}

	if (xml)
	{
		lstrcpynA(pfilter,Translate("XML Files"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
		strncat(pfilter," (*.xml)",bytesLeft);
		pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
		lstrcpynA(pfilter,"*.XML",bytesLeft);
		pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	}

	if(bytesLeft) *pfilter='\0';
}

/*
 * set an avatar for a protocol (service function)
 * if lParam == NULL, a open file dialog will be opened, otherwise, lParam is taken as a FULL
 * image filename (will be checked for existance, though)
 */
static int SetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *protocol;
    char FileName[MAX_PATH];
    char *szFinalName = NULL;
    HANDLE hFile = 0;
	BOOL allAcceptXML;
	BOOL allAcceptSWF;
    
	protocol = (char *)wParam;
    
	// Protocol allow seting of avatar?
	if (protocol != NULL && !CanSetMyAvatar((WPARAM) protocol, 0))
		return -1;

	if (hwndSetMyAvatar != 0)
	{
		SetForegroundWindow((HWND) hwndSetMyAvatar);
		SetFocus((HWND) hwndSetMyAvatar);
 		ShowWindow((HWND) hwndSetMyAvatar, SW_SHOW);
		return -2;
	}

	// Check for XML and SWF
	if (protocol == NULL)
	{
		allAcceptXML = TRUE;
		allAcceptSWF = TRUE;

		PROTOCOLDESCRIPTOR **protos;
		int i,count;

		CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&count, (LPARAM)&protos);
		for (i = 0; i < count; i++)
		{
			if (protos[i]->type != PROTOTYPE_PROTOCOL)
				continue;

			if (protos[i]->szName == NULL || protos[i]->szName[0] == '\0')
				continue;

			if (!ProtoServiceExists(protos[i]->szName, PS_SETMYAVATAR))
				continue;

			if (!Proto_IsAvatarsEnabled(protos[i]->szName))
				continue;

			allAcceptXML = allAcceptXML && Proto_IsAvatarFormatSupported(protos[i]->szName, PA_FORMAT_XML);
			allAcceptSWF = allAcceptSWF && Proto_IsAvatarFormatSupported(protos[i]->szName, PA_FORMAT_SWF);
		}
	}
	else
	{
		allAcceptXML = Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_XML);
		allAcceptSWF = Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_SWF);
	}

    if(lParam == 0) {
        OPENFILENAMEA ofn = {0};
		char filter[512];
		char inipath[1024];

		filter[0] = '\0';
		FilterGetStrings(filter, sizeof(filter), allAcceptXML, allAcceptSWF);

		FoldersGetCustomPath(hMyAvatarsFolder, inipath, sizeof(inipath), ".");

		if (IsWinVer2000Plus())
			ofn.lStructSize = sizeof(ofn);
		else
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
        ofn.hwndOwner = 0;
        ofn.lpstrFile = FileName;
        ofn.lpstrFilter = filter;
        ofn.nMaxFile = MAX_PATH;
        ofn.nMaxFileTitle = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING | OFN_ENABLEHOOK;
		ofn.lpstrInitialDir = inipath;
		ofn.lpfnHook = OFNHookProc;

        *FileName = '\0';
        ofn.lpstrDefExt="";
        ofn.hInstance = g_hInst;

		char title[256];
		if (protocol == NULL)
			mir_snprintf(title, sizeof(title), Translate("Set My Avatar"));
		else
			mir_snprintf(title, sizeof(title), Translate("Set My Avatar for %s"), protocol);
		ofn.lpstrTitle = title;

        if(GetOpenFileNameA(&ofn)) 
            szFinalName = FileName;
        else
            return 1;
    }
    else
        szFinalName = (char *)lParam;

    /*
     * filename is now set, check it and perform all needed action
     */

	if (szFinalName[0] == '\0')
	{
		// Remove avatar
		int ret = 0;
		if (protocol != NULL)
		{
			if (ProtoServiceExists(protocol, PS_SETMYAVATAR))
				ret = CallProtoService(protocol, PS_SETMYAVATAR, 0, NULL);
			else
				ret = -3;
		}
		else
		{
			PROTOCOLDESCRIPTOR **protos;
			int i,count;

			CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&count, (LPARAM)&protos);
			for (i = 0; i < count; i++)
			{
				if (protos[i]->type != PROTOTYPE_PROTOCOL)
					continue;

				if (protos[i]->szName == NULL || protos[i]->szName[0] == '\0')
					continue;

				if (!ProtoServiceExists(protos[i]->szName, PS_SETMYAVATAR))
					continue;

				if (!Proto_IsAvatarsEnabled(protos[i]->szName))
					continue;
				
				// Found a protocol
				int retTmp = CallProtoService(protos[i]->szName, PS_SETMYAVATAR, 0, NULL);;
				if (retTmp != 0)
					ret = retTmp;
			}
		}

		return ret;
	}

	int format = GetImageFormat(szFinalName);
    if (format == PA_FORMAT_UNKNOWN || (hFile = CreateFileA(szFinalName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        return -3;
    
    CloseHandle(hFile);
        
    // file exists...

	HBITMAP hBmp = NULL;

	if (format == PA_FORMAT_SWF)
	{
		if (!allAcceptSWF)
			return -4;
	}
	else if (format == PA_FORMAT_XML)
	{
		if (!allAcceptXML)
			return -4;
	}
	else
	{
		// Try to open if is not a flash or XML
		hBmp = (HBITMAP) BmpFilterLoadBitmap32(NULL, (LPARAM)szFinalName);
		if (hBmp == NULL)
			return -4;

		BITMAP bminfo;
		GetObject(hBmp, sizeof(bminfo), &bminfo);

		if (bminfo.bmBitsPixel != 32)
		{
			HBITMAP hBmpTmp = CopyBitmapTo32(hBmp);
			DeleteObject(hBmp);
			if (hBmpTmp == 0)
				return -5;
			hBmp = hBmpTmp;
		}
	}

	int ret = 0;
	if (protocol != NULL)
	{
		ret = SetProtoMyAvatar(protocol, hBmp, szFinalName, format);
	}
	else
	{
		PROTOCOLDESCRIPTOR **protos;
		int i,count;

		CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&count, (LPARAM)&protos);
		for (i = 0; i < count; i++)
		{
			if (protos[i]->type != PROTOTYPE_PROTOCOL)
				continue;

			if (protos[i]->szName == NULL || protos[i]->szName[0] == '\0')
				continue;

			if (!ProtoServiceExists(protos[i]->szName, PS_SETMYAVATAR))
				continue;

			if (!Proto_IsAvatarsEnabled(protos[i]->szName))
				continue;
			
			int retTmp = SetProtoMyAvatar(protos[i]->szName, hBmp, szFinalName, format);
			if (retTmp != 0)
				ret = retTmp;
		}
	}

	DeleteObject(hBmp);

	return ret;
}
	
static int SetProtoMyAvatar(char *protocol, HBITMAP hBmp, char *originalFilename, int originalFormat)
{
	if (!ProtoServiceExists(protocol, PS_SETMYAVATAR))
		return -1;

	// If is swf or xml, just set it

	if (originalFormat == PA_FORMAT_SWF)
	{
		if (!Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_SWF))
			return -1;

		return CallProtoService(protocol, PS_SETMYAVATAR, 0, (LPARAM) originalFilename);
	}

	if (originalFormat == PA_FORMAT_XML)
	{
		if (!Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_XML))
			return -1;

		return CallProtoService(protocol, PS_SETMYAVATAR, 0, (LPARAM) originalFilename);
	}

	// Protocol has max size?
	int width, height;
	BOOL square = FALSE;	// Get user setting

	Proto_GetAvatarMaxSize(protocol, width, height);

	if (DBGetContactSettingByte(0, AVS_MODULE, "SetAllwaysMakeSquare", 0))
		square = TRUE;
	else 
		square = Proto_AvatarImageProportion(protocol) & PIP_SQUARE;

	ResizeBitmap rb;
	rb.size = sizeof(ResizeBitmap);
	rb.hBmp = hBmp;
	rb.max_height = (height <= 0 ? 300 : height);
	rb.max_width = (width <= 0 ? 300 : width);
	rb.fit = square ? RESIZEBITMAP_MAKE_SQUARE : RESIZEBITMAP_KEEP_PROPORTIONS;

	HBITMAP hBmpProto = (HBITMAP) BmpFilterResizeBitmap((WPARAM)&rb, 0);

	// Check if need to resize
	if (hBmpProto == hBmp && Proto_IsAvatarFormatSupported(protocol, originalFormat))
	{
		// Use original image
		return CallProtoService(protocol, PS_SETMYAVATAR, 0, (LPARAM) originalFilename);
	}

	// Need to resize...

	// Save to a temporary file
	char temp_file[MAX_PATH];
	temp_file[0] = '\0';
	if (GetTempPathA(MAX_PATH, temp_file) == 0)
	{
		if (hBmpProto != hBmp) DeleteObject(hBmpProto);
		return -1;
	}
	if (GetTempFileNameA(temp_file, "mir_av_", 0, temp_file) == 0)
	{
		if (hBmpProto != hBmp) DeleteObject(hBmpProto);
		return -1;
	}

	char image_file_name[MAX_PATH];

	bool saved = false;

	// What format?
	if (BmpFilterCanSaveBitmap(0, PA_FORMAT_PNG) // Png is default
		&& Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_PNG))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".png");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	if (!saved  // Jpeg is second
		&& BmpFilterCanSaveBitmap(0, PA_FORMAT_JPEG)
		&& Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_JPEG))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".jpg");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	if (!saved  // Gif
		&& BmpFilterCanSaveBitmap(0, PA_FORMAT_GIF)
		&& Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_GIF))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".gif");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	if (!saved   // Bitmap
		&& BmpFilterCanSaveBitmap(0, PA_FORMAT_BMP)
		&& Proto_IsAvatarFormatSupported(protocol, PA_FORMAT_BMP))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".bmp");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	int ret;
	if (saved)
	{
		// Call proto service
		ret = CallProtoService(protocol, PS_SETMYAVATAR, 0, (LPARAM)image_file_name);

		DeleteFileA(image_file_name);
	}
	else
	{
		ret = -1;
	}

	DeleteFileA(temp_file);

	if (hBmpProto != hBmp) 
		DeleteObject(hBmpProto);

	return ret;
}

static int ContactOptions(WPARAM wParam, LPARAM lParam)
{
    if(wParam)
        CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_AVATAROPTIONS), 0, DlgProcAvatarOptions, (LPARAM)wParam);
    return 0;
}

static int GetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	int i;
	char *szProto = (char *)lParam;

	if(wParam || g_shutDown)
		return 0;

	if(lParam == 0 || IsBadReadPtr((void *)lParam, 4))
		return 0;

	for(i = 0; i < g_protocount; i++) {
		if(!strcmp(szProto, g_MyAvatars[i].szProtoname) && g_MyAvatars[i].hbmPic != 0)
			return (int)&g_MyAvatars[i];
	}
	return 0;
}

static protoPicCacheEntry *GetProtoDefaultAvatar(HANDLE hContact)
{
	char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	if(szProto) {
		for(int i = 0; i < g_protocount; i++) {
			if(!strcmp(g_ProtoPictures[i].szProtoname, szProto) && g_ProtoPictures[i].hbmPic != NULL) {
				return &g_ProtoPictures[i];
			}
		}
	}
	return NULL;
}

HANDLE GetContactThatHaveTheAvatar(HANDLE hContact, int locked = -1) {
    if(g_MetaAvail && DBGetContactSettingByte(NULL, g_szMetaName, "Enabled", 0)) {
		if(DBGetContactSettingDword(hContact, g_szMetaName, "NumContacts", 0) >= 1) {
			if (locked == -1)
				locked = DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0);

			if (!locked)
				hContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)hContact, 0);
		}
    }
	return hContact;
}

static int GetAvatarBitmap(WPARAM wParam, LPARAM lParam)
{
    if(wParam == 0 || g_shutDown)
		return 0;

	HANDLE hContact = (HANDLE) wParam;
	hContact = GetContactThatHaveTheAvatar(hContact);

	// Get the node
    struct CacheNode *node = FindAvatarInCache(hContact, TRUE);
	if (node == NULL || !node->loaded)
        return (int) GetProtoDefaultAvatar(hContact);
    else
        return (int) &node->ace;
}


// Just delete an avatar from cache
// An cache entry is never deleted. What is deleted is the image handle inside it
// This is done this way to keep track of which avatars avs have to keep track
void DeleteAvatarFromCache(HANDLE hContact, BOOL forever)
{
    hContact = GetContactThatHaveTheAvatar(hContact);

	struct CacheNode *node = FindAvatarInCache(hContact, FALSE);
	if (node == NULL) {
        struct CacheNode temp_node = {0};
        if(g_MetaAvail)
            temp_node.dwFlags |= (DBGetContactSettingByte(hContact, g_szMetaName, "IsSubcontact", 0) ? MC_ISSUBCONTACT : 0);
        NotifyMetaAware(hContact, &temp_node, (AVATARCACHEENTRY *)GetProtoDefaultAvatar(hContact));
        return;
    }
    node->mustLoad = -1;                        // mark for deletion
    SetEvent(hLoaderEvent);
    if(forever)
        node->dwFlags |= AVS_DELETENODEFOREVER;
}

int ChangeAvatar(HANDLE hContact, BOOL fLoad, BOOL fNotifyHist, int pa_format)
{
    if(g_shutDown)
		return 0;

	hContact = GetContactThatHaveTheAvatar(hContact);

	// Get the node
    struct CacheNode *node = FindAvatarInCache(hContact, g_AvatarHistoryAvail && fNotifyHist);
	if (node == NULL) 
		return 0;

    if (fNotifyHist)
        node->dwFlags |= AVH_MUSTNOTIFY;

    node->mustLoad = fLoad ? 1 : -1;
    node->pa_format = pa_format;
    SetEvent(hLoaderEvent);
    return 0;
}

/*
 * this thread scans the cache and handles nodes which have mustLoad set to > 0 (must be loaded/reloaded) or
 * nodes where mustLoad is < 0 (must be deleted).
 * its waken up by the event and tries to lock the cache only when absolutely necessary.
 */

static DWORD WINAPI PicLoader(LPVOID param)
{
    DWORD dwDelay = DBGetContactSettingDword(NULL, AVS_MODULE, "picloader_sleeptime", 80);

	if(dwDelay < 30)
		dwDelay = 30;
	else if(dwDelay > 100)
		dwDelay = 100;

    while(!g_shutDown) {
        struct CacheNode *node = g_Cache;

        while(node) {
            if(node->mustLoad > 0 && node->ace.hContact) {
                node->mustLoad = 0;
                AVATARCACHEENTRY ace_temp;

                if (DBGetContactSettingByte(node->ace.hContact, "ContactPhoto", "NeedUpdate", 0))
                    QueueAdd(requestQueue, node->ace.hContact);

                CopyMemory(&ace_temp, &node->ace, sizeof(AVATARCACHEENTRY));
                ace_temp.hbmPic = 0;

                int result = CreateAvatarInCache(node->ace.hContact, &ace_temp, NULL);

                if (result == -2)
                    QueueAdd(requestQueue, node->ace.hContact);

				if(result == 1 && ace_temp.hbmPic != 0) {
                    HBITMAP oldPic = node->ace.hbmPic;

                    EnterCriticalSection(&cachecs);
                    CopyMemory(&node->ace, &ace_temp, sizeof(AVATARCACHEENTRY));
                    node->loaded = TRUE;
					LeaveCriticalSection(&cachecs);
                    if(oldPic)
                        DeleteObject(oldPic);
                    NotifyMetaAware(node->ace.hContact, node);
                }
                if(g_shutDown)
                    break;
                Sleep(dwDelay);
            }
            else if(node->mustLoad < 0 && node->ace.hContact) {         // delete this picture
                HANDLE hContact = node->ace.hContact;
                node->mustLoad = 0;
                node->loaded = 0;
                if(node->ace.hbmPic)
                    DeleteObject(node->ace.hbmPic);
                ZeroMemory(&node->ace, sizeof(AVATARCACHEENTRY));
                if(node->dwFlags & AVS_DELETENODEFOREVER)
                    node->dwFlags &= ~AVS_DELETENODEFOREVER;
                else {                                                  // restore node contact and notify events
                    node->ace.hContact = hContact;
                    NotifyMetaAware(hContact, node, (AVATARCACHEENTRY *)GetProtoDefaultAvatar(hContact));
                }
            }
            // protect this by changes from the cache block allocator as it can cause inconsistencies while working
            // on allocating a new block.
            EnterCriticalSection(&alloccs);
            node = node->pNextNode;
            LeaveCriticalSection(&alloccs);
        }
        WaitForSingleObject(hLoaderEvent, INFINITE);
        ResetEvent(hLoaderEvent);
    }
    return 0;
}

static int MetaChanged(WPARAM wParam, LPARAM lParam)
{
    if(wParam == 0 || g_shutDown)
        return 0;

    AVATARCACHEENTRY *ace;

    HANDLE hContact = (HANDLE) wParam;
    HANDLE hSubContact = GetContactThatHaveTheAvatar(hContact);

    // Get the node
    struct CacheNode *node = FindAvatarInCache(hSubContact, TRUE);
    if (node == NULL || !node->loaded) {
        ace = (AVATARCACHEENTRY *)GetProtoDefaultAvatar(hSubContact);
        QueueAdd(requestQueue, hSubContact);
    }
    else
        ace = &node->ace;

    NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)ace);
    return 0;
}

static DWORD dwPicLoaderID;
extern bool gdiPlusFail;

static int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    int i, j;
    DBVARIANT dbv = {0};
    TCHAR szEventName[100];

    if(gdiPlusFail)
        MessageBox(0, TranslateT("GDI+ is not installed on this system. Support for .GIF and .JPG avatars has been disabled."), TranslateT("Avatar Service warning"), MB_OK);

    InitPolls();
    mir_sntprintf(szEventName, 100, _T("avs_loaderthread_%d"), GetCurrentThreadId());
    hLoaderEvent = CreateEvent(NULL, TRUE, FALSE, szEventName);
    hLoaderThread = CreateThread(NULL, 0, PicLoader, 0, 0, &dwPicLoaderID);
    SetThreadPriority(hLoaderThread, THREAD_PRIORITY_IDLE);

    // Folders plugin support
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH))
	{
		hMyAvatarsFolder = (HANDLE) FoldersRegisterCustomPath(Translate("Avatars"), Translate("My Avatars"), 
													  PROFILE_PATH "\\" CURRENT_PROFILE "\\MyAvatars");

		hProtocolAvatarsFolder = (HANDLE) FoldersRegisterCustomPath(Translate("Avatars"), Translate("Protocol Avatars Cache"), 
													  FOLDER_AVATARS);
	}

    g_AvatarHistoryAvail = ServiceExists(MS_AVATARHISTORY_ENABLED);

    g_MetaAvail = ServiceExists(MS_MC_GETPROTOCOLNAME) ? TRUE : FALSE;
    if(g_MetaAvail) {
        g_szMetaName = (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0);
        if(g_szMetaName == NULL)
            g_MetaAvail = FALSE;
    }


    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&g_protocount, (LPARAM)&g_protocols);
    g_ProtoPictures = (struct protoPicCacheEntry *)malloc(sizeof(struct protoPicCacheEntry) * (g_protocount + 1));
    ZeroMemory((void *)g_ProtoPictures, sizeof(struct protoPicCacheEntry) * (g_protocount + 1));

	g_MyAvatars = (struct protoPicCacheEntry *)malloc(sizeof(struct protoPicCacheEntry) * (g_protocount + 1));
    ZeroMemory((void *)g_MyAvatars, sizeof(struct protoPicCacheEntry) * (g_protocount + 1));
    
    j = 0;
    for(i = 0; i < g_protocount; i++) {
        if(g_protocols[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        if(CreateAvatarInCache(0, (struct avatarCacheEntry *)&g_ProtoPictures[j], g_protocols[i]->szName) == 1)
            j++;
        else {
            strncpy(g_ProtoPictures[j].szProtoname, g_protocols[i]->szName, 100);
            g_ProtoPictures[j++].szProtoname[99] = 0;
            DBDeleteContactSetting(0, PPICT_MODULE, g_protocols[i]->szName);
        }
    }
	j = 0;
    for(i = 0; i < g_protocount; i++) {
        if(g_protocols[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        if(CreateAvatarInCache((HANDLE)-1, (struct avatarCacheEntry *)&g_MyAvatars[j], g_protocols[i]->szName) == 1)
            j++;
        else {
            strncpy(g_MyAvatars[j].szProtoname, g_protocols[i]->szName, 100);
            g_MyAvatars[j++].szProtoname[99] = 0;
        }
    }


    // updater plugin support
    if(ServiceExists(MS_UPDATE_REGISTER))
	{
		Update upd = {0};
		char szCurrentVersion[30];
#if defined(_UNICODE)
        char *szVersionUrl = "http://miranda.or.at/files/avs/versionW.txt";
        char *szUpdateUrl = "http://miranda.or.at/files/avs/avsW.zip";
        char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=2990";
        char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=2990";
#else
		char *szVersionUrl = "http://miranda.or.at/files/avs/version.txt";
		char *szUpdateUrl = "http://miranda.or.at/files/avs/avs.zip";
        char *szFLVersionUrl = "http://addons.miranda-im.org/details.php?action=viewfile&id=2361";
        char *szFLUpdateurl = "http://addons.miranda-im.org/feed.php?dlfile=2361";
#endif
		char *szPrefix = "avs ";

		upd.cbSize = sizeof(upd);
		upd.szComponentName = pluginInfo.shortName;
		upd.pbVersion = (BYTE *)CreateVersionStringPlugin(&pluginInfo, szCurrentVersion);
		upd.cpbVersion = strlen((char *)upd.pbVersion);
		upd.szVersionURL = szFLVersionUrl;
		upd.szUpdateURL = szFLUpdateurl;
#if defined(_UNICODE)
        upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">Avatar service (UNICODE) ";
#else
		upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">Avatar service ";
#endif

		upd.szBetaUpdateURL = szUpdateUrl;
		upd.szBetaVersionURL = szVersionUrl;
		upd.pbBetaVersionPrefix = (BYTE *)szPrefix;
		upd.pbVersion = (PBYTE)szCurrentVersion;
		upd.cpbVersion = lstrlenA(szCurrentVersion);

		upd.cpbVersionPrefix = strlen((char *)upd.pbVersionPrefix);
		upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);

        CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);
	}

	HookEvent(ME_SYSTEM_PRESHUTDOWN, ShutdownProc);
    HookEvent(ME_SYSTEM_OKTOEXIT, OkToExitProc);
	hUserInfoInitHook = HookEvent(ME_USERINFO_INITIALISE, OnDetailsInit);

	return 0;
}

static void ReloadMyAvatar(LPVOID lpParam)
{
	char *szProto = (char *)lpParam;
	
    Sleep(1000);
    for(int i = 0; i < g_protocount; i++) {
		if(!strcmp(g_MyAvatars[i].szProtoname, szProto)) {
			if(g_MyAvatars[i].hbmPic)
				DeleteObject(g_MyAvatars[i].hbmPic);
			g_MyAvatars[i].hbmPic = NULL;

			if(CreateAvatarInCache((HANDLE)-1, (struct avatarCacheEntry *)&g_MyAvatars[i], szProto) != -1)
				NotifyEventHooks(hMyAvatarChanged, (WPARAM)szProto, (LPARAM)&g_MyAvatars[i]);
			else
				NotifyEventHooks(hMyAvatarChanged, (WPARAM)szProto, 0);
		}
	}
	free(lpParam);
	_endthread();
}

static int ReportMyAvatarChanged(WPARAM wParam, LPARAM lParam)
{
	if (wParam == NULL)
		return -1;

	char *proto = (char *) wParam;

	int i;
	for(i = 0; i < g_protocount; i++) {
		if(!strcmp(g_MyAvatars[i].szProtoname, proto)) {
			LPVOID lpParam = (void *)malloc(lstrlenA(g_MyAvatars[i].szProtoname) + 2);
			strcpy((char *)lpParam, g_MyAvatars[i].szProtoname);
			_beginthread(ReloadMyAvatar, 0, lpParam);
			return 0;
		}
	}

	return -2;
}

static int ContactSettingChanged(WPARAM wParam, LPARAM lParam)
{
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  

    if(cws == NULL || g_shutDown)
        return 0;

	if(wParam == 0) {
		if(!strcmp(cws->szSetting, "AvatarFile") 
			|| !strcmp(cws->szSetting, "PictObject") 
			|| !strcmp(cws->szSetting, "AvatarHash")) 
		{
			ReportMyAvatarChanged((WPARAM) cws->szModule, 0);
		}
		return 0;
	}
    else if(g_MetaAvail && !strcmp(cws->szModule, g_szMetaName)) {
        if(lstrlenA(cws->szSetting) > 6 && !strncmp(cws->szSetting, "Status", 5))
           MetaChanged(wParam, 0);
    }
	return 0;
}

static int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
	DeleteAvatarFromCache((HANDLE)wParam, TRUE);

	return 0;
}

static int OptInit(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp;

    ZeroMemory(&odp, sizeof(odp));
    odp.cbSize = sizeof(odp);
    odp.position = 0;
    odp.hInstance = g_hInst;
    odp.pszGroup = Translate("Customize");
    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
    odp.pszTitle = Translate("Contact pictures");
    odp.pfnDlgProc = DlgProcOptions;
    odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);
    return 0;
}

static HANDLE hSvc_MS_AV_GETAVATARBITMAP = 0, hSvc_MS_AV_PROTECTAVATAR = 0,
	hSvc_MS_AV_SETAVATAR = 0, hSvc_MS_AV_SETMYAVATAR = 0, hSvc_MS_AV_CANSETMYAVATAR, hSvc_MS_AV_CONTACTOPTIONS,
	hSvc_MS_AV_DRAWAVATAR = 0, hSvc_MS_AV_GETMYAVATAR = 0, hSvc_MS_AV_REPORTMYAVATARCHANGED = 0,
	hSvc_MS_AV_LOADBITMAP32 = 0, hSvc_MS_AV_SAVEBITMAP = 0, hSvc_MS_AV_CANSAVEBITMAP = 0,
	hSvc_MS_AV_RESIZEBITMAP = 0;

static int OkToExitProc(WPARAM wParam, LPARAM lParam)
{
    EnterCriticalSection(&cachecs);
    g_shutDown = TRUE;

    UnhookEvent(hContactSettingChanged);
    UnhookEvent(hProtoAckHook);
	UnhookEvent(hUserInfoInitHook);

    DestroyServiceFunction(hSvc_MS_AV_GETAVATARBITMAP);
    DestroyServiceFunction(hSvc_MS_AV_PROTECTAVATAR);
    DestroyServiceFunction(hSvc_MS_AV_SETAVATAR);
    DestroyServiceFunction(hSvc_MS_AV_SETMYAVATAR);
    DestroyServiceFunction(hSvc_MS_AV_CANSETMYAVATAR);
    DestroyServiceFunction(hSvc_MS_AV_CONTACTOPTIONS);
    DestroyServiceFunction(hSvc_MS_AV_DRAWAVATAR);
    DestroyServiceFunction(hSvc_MS_AV_GETMYAVATAR);
    DestroyServiceFunction(hSvc_MS_AV_REPORTMYAVATARCHANGED);
    DestroyServiceFunction(hSvc_MS_AV_LOADBITMAP32);
    DestroyServiceFunction(hSvc_MS_AV_SAVEBITMAP);
    DestroyServiceFunction(hSvc_MS_AV_CANSAVEBITMAP);
    DestroyServiceFunction(hSvc_MS_AV_RESIZEBITMAP);

    DestroyHookableEvent(hEventChanged);
	DestroyHookableEvent(hEventContactAvatarChanged);
    DestroyHookableEvent(hMyAvatarChanged);
    hEventChanged = 0;
	hEventContactAvatarChanged = 0;
    hMyAvatarChanged = 0;
	UnhookEvent(hEventDeleted);
    LeaveCriticalSection(&cachecs);

    SetEvent(hLoaderEvent);
    WaitForSingleObject(hLoaderThread, 1000);
    CloseHandle(hLoaderThread);
    CloseHandle(hLoaderEvent);
	FreePolls();


    return 0;
}

static int ShutdownProc(WPARAM wParam, LPARAM lParam)
{
    DeleteCriticalSection(&cachecs);
    DeleteCriticalSection(&alloccs);
	return 0;
}

static int DrawAvatarPicture(WPARAM wParam, LPARAM lParam)
{
    AVATARDRAWREQUEST *r = (AVATARDRAWREQUEST *)lParam;
    AVATARCACHEENTRY *ace = NULL;
    
    if(r == 0 || IsBadReadPtr((void *)r, sizeof(AVATARDRAWREQUEST)))
        return 0;

    if(r->cbSize != sizeof(AVATARDRAWREQUEST))
        return 0;

    if(r->dwFlags & AVDRQ_PROTOPICT) {
        int i = 0;

        if(r->szProto == NULL)
            return 0;
        
        for(i = 0; i < g_protocount; i++) {
            if(g_ProtoPictures[i].szProtoname[0] && !strcmp(g_ProtoPictures[i].szProtoname, r->szProto) && lstrlenA(r->szProto) == lstrlenA(g_ProtoPictures[i].szProtoname) && g_ProtoPictures[i].hbmPic != 0) {
                ace = (AVATARCACHEENTRY *)&g_ProtoPictures[i];
                break;
            }
        }
    }
	else if(r->dwFlags & AVDRQ_OWNPIC) {
        int i = 0;

        if(r->szProto == NULL)
            return 0;
        
		ace = (AVATARCACHEENTRY *)GetMyAvatar(0, (LPARAM)r->szProto);
	}
    else
        ace = (AVATARCACHEENTRY *)GetAvatarBitmap((WPARAM)r->hContact, 0);

	if(ace && (!(r->dwFlags & AVDRQ_RESPECTHIDDEN) || !(ace->dwFlags & AVS_HIDEONCLIST))) {
        float dScale = 0;
        int newHeight, newWidth;
        HDC hdcAvatar;
        HBITMAP hbmMem;
        DWORD topoffset = 0, leftoffset = 0;
        LONG bmWidth, bmHeight;
        HBITMAP hbm;
        HRGN rgn = 0, oldRgn = 0;
        int targetWidth = r->rcDraw.right - r->rcDraw.left;
        int targetHeight = r->rcDraw.bottom - r->rcDraw.top;
        BLENDFUNCTION bf = {0};
        
        bmHeight = ace->bmHeight;
        bmWidth = ace->bmWidth;
        hbm = ace->hbmPic;
        ace->t_lastAccess = time(NULL);

        if(bmHeight == 0 || bmWidth == 0 || hbm == 0)
            return 0;

        hdcAvatar = CreateCompatibleDC(r->hTargetDC);
        hbmMem = (HBITMAP)SelectObject(hdcAvatar, hbm);

		if ((r->dwFlags & AVDRQ_DONTRESIZEIFSMALLER) && ace->bmHeight <= targetHeight && ace->bmWidth <= targetWidth) {
			newHeight = bmHeight;
			newWidth = bmWidth;
		} 
        else if (bmHeight >= bmWidth) {
            dScale = targetHeight / (float)bmHeight;
            newHeight = targetHeight;
            newWidth = (int) (bmWidth * dScale);
        }
        else {
            dScale = targetWidth / (float)bmWidth;
            newWidth = targetWidth;
            newHeight = (int) (bmHeight * dScale);
        }

        topoffset = targetHeight > newHeight ? (targetHeight - newHeight) / 2 : 0;
        leftoffset = targetWidth > newWidth ? (targetWidth - newWidth) / 2 : 0;
        
        // create the region for the avatar border - use the same region for clipping, if needed.

		oldRgn = CreateRectRgn(0,0,1,1);
		if (GetClipRgn(r->hTargetDC, oldRgn) != 1)
		{
			DeleteObject(oldRgn);
			oldRgn = NULL;
		}

        if(r->dwFlags & AVDRQ_ROUNDEDCORNER)
            rgn = CreateRoundRectRgn(r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, r->rcDraw.left + leftoffset + newWidth + 1, r->rcDraw.top + topoffset + newHeight + 1, 2 * r->radius, 2 * r->radius);
        else
            rgn = CreateRectRgn(r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, r->rcDraw.left + leftoffset + newWidth, r->rcDraw.top + topoffset + newHeight);

		ExtSelectClipRgn(r->hTargetDC, rgn, RGN_AND);

        bf.SourceConstantAlpha = r->alpha > 0 ? r->alpha : 255;
        bf.AlphaFormat = ace->dwFlags & AVS_PREMULTIPLIED ? AC_SRC_ALPHA : 0;

        SetStretchBltMode(r->hTargetDC, HALFTONE);
        if(bf.SourceConstantAlpha == 255 && bf.AlphaFormat == 0) {
            StretchBlt(r->hTargetDC, r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, newWidth, newHeight, hdcAvatar, 0, 0, bmWidth, bmHeight, SRCCOPY);
        }
        else {
            /*
             * get around SUCKY AlphaBlend() rescaling quality...
             */
            HDC hdcTemp = CreateCompatibleDC(r->hTargetDC);
            HBITMAP hbmTemp;
            HBITMAP hbmTempOld;//

            hbmTemp = CreateCompatibleBitmap(hdcAvatar, bmWidth, bmHeight);
            hbmTempOld = (HBITMAP)SelectObject(hdcTemp, hbmTemp);

            //SetBkMode(hdcTemp, TRANSPARENT);
            SetStretchBltMode(hdcTemp, HALFTONE);
            StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, r->hTargetDC, r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, newWidth, newHeight, SRCCOPY);
            AlphaBlend(hdcTemp, 0, 0, bmWidth, bmHeight, hdcAvatar, 0, 0, bmWidth, bmHeight, bf);
            StretchBlt(r->hTargetDC, r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, newWidth, newHeight, hdcTemp, 0, 0, bmWidth, bmHeight, SRCCOPY);
            SelectObject(hdcTemp, hbmTempOld);
            DeleteObject(hbmTemp);
            DeleteDC(hdcTemp);
        }

		if((r->dwFlags & AVDRQ_DRAWBORDER) && !((r->dwFlags & AVDRQ_HIDEBORDERONTRANSPARENCY) && (ace->dwFlags & AVS_HASTRANSPARENCY))) {
            HBRUSH br = CreateSolidBrush(r->clrBorder);
            HBRUSH brOld = (HBRUSH)SelectObject(r->hTargetDC, br);
            FrameRgn(r->hTargetDC, rgn, br, 1, 1);
            SelectObject(r->hTargetDC, brOld);
            DeleteObject(br);
        }

        SelectClipRgn(r->hTargetDC, oldRgn);
        DeleteObject(rgn);
		if (oldRgn) DeleteObject(oldRgn);

        SelectObject(hdcAvatar, hbmMem);
        DeleteDC(hdcAvatar);
        return 1;
    }
    else
        return 0;
}

static int OnDetailsInit(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) lParam;
	if (hContact == NULL) 
	{
		// User dialog
		OPTIONSDIALOGPAGE odp = {0};
		odp.cbSize = sizeof(odp);
		odp.hIcon = g_hIcon;
		odp.hInstance = g_hInst;
		odp.pfnDlgProc = DlgProcAvatarProtoInfo;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_PROTO_AVATARS);
		odp.pszTitle = Translate("Avatar");
		CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
	}
	else
	{
		// Contact dialog
		OPTIONSDIALOGPAGE odp = {0};
		odp.cbSize = sizeof(odp);
		odp.hIcon = g_hIcon;
		odp.hInstance = g_hInst;
		odp.pfnDlgProc = DlgProcAvatarUserInfo;
		odp.position = -2000000000;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_USER_AVATAR);
		odp.pszTitle = Translate("Avatar");
		CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
	}

	return 0;
}

static int LoadAvatarModule()
{
	init_mir_malloc();
	init_list_interface();

	InitializeCriticalSection(&cachecs);
    InitializeCriticalSection(&alloccs);

	HookEvent(ME_OPT_INITIALISE, OptInit);
    HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
    hContactSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ContactSettingChanged);
    hEventDeleted = HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);
    hProtoAckHook = HookEvent(ME_PROTO_ACK, ProtocolAck);

    hSvc_MS_AV_GETAVATARBITMAP = CreateServiceFunction(MS_AV_GETAVATARBITMAP, GetAvatarBitmap);
    hSvc_MS_AV_PROTECTAVATAR = CreateServiceFunction(MS_AV_PROTECTAVATAR, ProtectAvatar);
    hSvc_MS_AV_SETAVATAR = CreateServiceFunction(MS_AV_SETAVATAR, SetAvatar);
    hSvc_MS_AV_SETMYAVATAR = CreateServiceFunction(MS_AV_SETMYAVATAR, SetMyAvatar);
    hSvc_MS_AV_CANSETMYAVATAR = CreateServiceFunction(MS_AV_CANSETMYAVATAR, CanSetMyAvatar);
    hSvc_MS_AV_CONTACTOPTIONS = CreateServiceFunction(MS_AV_CONTACTOPTIONS, ContactOptions);
    hSvc_MS_AV_DRAWAVATAR = CreateServiceFunction(MS_AV_DRAWAVATAR, DrawAvatarPicture);
	hSvc_MS_AV_GETMYAVATAR = CreateServiceFunction(MS_AV_GETMYAVATAR, GetMyAvatar);
	hSvc_MS_AV_REPORTMYAVATARCHANGED = CreateServiceFunction(MS_AV_REPORTMYAVATARCHANGED, ReportMyAvatarChanged);
	hSvc_MS_AV_LOADBITMAP32 = CreateServiceFunction(MS_AV_LOADBITMAP32, BmpFilterLoadBitmap32);
	hSvc_MS_AV_SAVEBITMAP = CreateServiceFunction(MS_AV_SAVEBITMAP, BmpFilterSaveBitmap);
	hSvc_MS_AV_CANSAVEBITMAP = CreateServiceFunction(MS_AV_CANSAVEBITMAP, BmpFilterCanSaveBitmap);
	hSvc_MS_AV_RESIZEBITMAP =CreateServiceFunction(MS_AV_RESIZEBITMAP, BmpFilterResizeBitmap);
    
	hEventChanged = CreateHookableEvent(ME_AV_AVATARCHANGED);
	hEventContactAvatarChanged = CreateHookableEvent(ME_AV_CONTACTAVATARCHANGED);
	hMyAvatarChanged = CreateHookableEvent(ME_AV_MYAVATARCHANGED);

	AllocCacheBlock();

    CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)g_szDBPath);
    _strlwr(g_szDBPath);

	return 0;
}


extern "C" BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID reserved)
{
    g_hInst = hInstDLL;
    return TRUE;
}


extern "C" __declspec(dllexport) PLUGININFO * MirandaPluginInfo(DWORD mirandaVersion)
{
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0))
        return NULL;
    return &pluginInfo;
}

extern "C" __declspec(dllexport) PLUGININFOEX * MirandaPluginInfoEx(DWORD mirandaVersion)
{
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0))
        return NULL;
    return &pluginInfoEx;
}


static const MUUID interfaces[] = { { 0xece29554, 0x1cf0, 0x41da, { 0x85, 0x82, 0xfb, 0xe8, 0x45, 0x5c, 0x6b, 0xec } }, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID * MirandaPluginInterfaces(void)
{
	return interfaces;
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK * link)
{
    pluginLink = link;
    dwMainThreadID = GetCurrentThreadId();
	LoadGdiPlus();
    InitGdiPlus();
	LoadACC();

    return LoadAvatarModule();
}

extern "C" int __declspec(dllexport) Unload(void)
{
	struct CacheNode *pNode = g_Cache;

	while(pNode) {
		//DeleteCriticalSection(&pNode->cs);
        if(pNode->ace.hbmPic != 0)
            DeleteObject(pNode->ace.hbmPic);
		pNode = pNode->pNextNode;
    }

	int i;
	for(i = 0; i < g_curBlock; i++)
		free(g_cacheBlocks[i]);
	free(g_cacheBlocks);

	for(i = 0; i < g_protocount; i++) {
        if(g_ProtoPictures[i].hbmPic != 0)
            DeleteObject(g_ProtoPictures[i].hbmPic);
		if(g_MyAvatars[i].hbmPic != 0)
			DeleteObject(g_MyAvatars[i].hbmPic);
    }
    if(g_ProtoPictures)
        free(g_ProtoPictures);

	if(g_MyAvatars)
		free(g_MyAvatars);
    ShutdownGdiPlus();
    FreeGdiPlus();
    return 0;
}


/*
wParam=(int *)max width of avatar - will be set (-1 for no max)
lParam=(int *)max height of avatar - will be set (-1 for no max)
return=0 for sucess
*/
#define PS_GETMYAVATARMAXSIZE "/GetMyAvatarMaxSize"

/*
wParam=0
lParam=0
return=One of PIP_SQUARE, PIP_FREEPROPORTIONS
*/
#define PIP_FREEPROPORTIONS	0
#define PIP_SQUARE			1
#define PS_GETMYAVATARIMAGEPROPORTION "/GetMyAvatarImageProportion"

/*
wParam = 0
lParam = PA_FORMAT_*   // avatar format
return = 1 (supported) or 0 (not supported)
*/
#define PS_ISAVATARFORMATSUPPORTED "/IsAvatarFormatSupported"



BOOL Proto_IsAvatarsEnabled(char *proto)
{
	if (ProtoServiceExists(proto, PS_GETAVATARCAPS))
		return CallProtoService(proto, PS_GETAVATARCAPS, AF_ENABLED, 0);
		
	return TRUE;
}

BOOL Proto_IsAvatarFormatSupported(char *proto, int format)
{
	if (ProtoServiceExists(proto, PS_GETAVATARCAPS))
		return CallProtoService(proto, PS_GETAVATARCAPS, AF_FORMATSUPPORTED, format);

	if (ProtoServiceExists(proto, PS_ISAVATARFORMATSUPPORTED))
		return CallProtoService(proto, PS_ISAVATARFORMATSUPPORTED, 0, format);

	if (format >= PA_FORMAT_SWF)
		return FALSE;

	return TRUE;
}

int Proto_AvatarImageProportion(char *proto)
{
	if (ProtoServiceExists(proto, PS_GETAVATARCAPS))
		return CallProtoService(proto, PS_GETAVATARCAPS, AF_PROPORTION, 0);

	if (ProtoServiceExists(proto, PS_GETMYAVATARIMAGEPROPORTION))
		return CallProtoService(proto, PS_GETMYAVATARIMAGEPROPORTION, 0, 0);

	return 0;
}

void Proto_GetAvatarMaxSize(char *proto, int &width, int &height)
{
	if (ProtoServiceExists(proto, PS_GETAVATARCAPS))
	{
		POINT maxSize;
		CallProtoService(proto, PS_GETAVATARCAPS, AF_MAXSIZE, (LPARAM) &maxSize);
		width = maxSize.y;
		height = maxSize.x;
	}
	else if (ProtoServiceExists(proto, PS_GETMYAVATARMAXSIZE))
	{
		CallProtoService(proto, PS_GETMYAVATARMAXSIZE, (WPARAM) &width, (LPARAM) &height);
	}
	else
	{
		width = 300;
		height = 300;
	}

	if (width < 0)
		width = 0;
	else if (width > 300)
		width = 300;

	if (height < 0)
		height = 0;
	else if (height > 300)
		height = 300;
}
