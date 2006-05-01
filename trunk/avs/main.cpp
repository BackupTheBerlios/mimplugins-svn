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

HINSTANCE g_hInst = 0;
PLUGINLINK *pluginLink;
HANDLE hMyAvatarsFolder = 0;
HANDLE hProtocolAvatarsFolder = 0;
int g_shutDown = FALSE;

PROTOCOLDESCRIPTOR **g_protocols = NULL;
int g_protocount = 0;
char g_installed_protos[1030];
static int AV_QUEUESIZE = 0;
char *g_MetaName = NULL;

static char g_szDBPath[MAX_PATH];		// database profile path (read at startup only)

HANDLE hProtoAckHook = 0, hContactSettingChanged = 0, hEventChanged = 0, hMyAvatarChanged = 0;
HICON g_hIcon = 0;

static struct CacheNode *g_Cache = 0;
struct protoPicCacheEntry *g_ProtoPictures = 0;
struct protoPicCacheEntry *g_MyAvatars = 0;

CRITICAL_SECTION cachecs;
static CRITICAL_SECTION avcs;

HANDLE hAvatarThread = 0;
DWORD dwAvatarThreadID = 0;
BOOL bAvatarThreadRunning = TRUE;
BOOL g_MetaAvail = FALSE;
char *g_szMetaName = NULL;

HANDLE *avatarUpdateQueue = NULL;
DWORD dwPendingAvatarJobs = 0;

#ifndef SHVIEW_THUMBNAIL
# define SHVIEW_THUMBNAIL 0x702D
#endif

// Stores the id of the dialog
long hwndSetMyAvatar = 0;

int ChangeAvatar(HANDLE hContact);

PLUGININFO pluginInfo = {
    sizeof(PLUGININFO), 
#if defined(_UNICODE)
	"Avatar service (Unicode)",
#else
	"Avatar service",
#endif
	PLUGIN_MAKE_VERSION(0, 0, 1, 24), 
	"Load and manage contact pictures for other plugins", 
	"Nightwish, Pescuma", 
	"", 
	"Copyright 2000-2005 Miranda-IM project", 
	"http://www.miranda-im.org", 
	0, 
	0
};

extern BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcAvatarOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int SetProtoMyAvatar(char *protocol, HBITMAP hBmp);

#define GET_PIXEL(__P__, __X__, __Y__) ( __P__ + width * 4 * (__Y__) + 4 * (__X__) )


// Functions to set avatar for a protocol

/*
wParam=0
lParam=(const char *)Avatar file name
return=0 for sucess
*/
#define MSN_PS_SETMYAVATAR "/SetAvatar"

/*
wParam=0
lParam=(const char *)Avatar file name
return=0 for sucess
*/
#define PS_SETMYAVATAR "/SetMyAvatar"

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


/*
 * output a notification message.
 * may accept a hContact to include the contacts nickname in the notification message...
 * the actual message is using printf() rules for formatting and passing the arguments...
 *
 * can display the message either as systray notification (baloon popup) or using the
 * popup plugin.
 */


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
		allocedBlock[i].pNextNode = &allocedBlock[i + 1];				// pre-link the alloced block

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

int SetAvatarAttribute(HANDLE hContact, DWORD attrib, int mode)
{
	struct CacheNode *cacheNode = g_Cache;

	while(cacheNode) {
		if(cacheNode->ace.hContact == hContact) {
            DWORD dwFlags = cacheNode->ace.dwFlags;

            cacheNode->ace.dwFlags = mode ? cacheNode->ace.dwFlags | attrib : cacheNode->ace.dwFlags & ~attrib;
            if(cacheNode->ace.dwFlags != dwFlags)
                NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)&cacheNode->ace);
            break;
        }
		cacheNode = cacheNode->pNextNode;
	}
    return 0;
}

static BOOL ColorsAreTheSame(int colorDiff, BYTE *px1, BYTE *px2)
{
	return abs(px1[0] - px2[0]) <= colorDiff 
			&& abs(px1[1] - px2[1]) <= colorDiff 
			&& abs(px1[2] - px2[2])  <= colorDiff;
}


void AddToStack(int *stack, int *topPos, int x, int y)
{
	int i;

	// Already is in stack?
	for(i = 0 ; i < *topPos ; i += 2)
	{
		if (stack[i] == x && stack[i+1] == y)
			return;
	}

	stack[*topPos] = x; 
	(*topPos)++;

	stack[*topPos] = y; 
	(*topPos)++;
}


BOOL GetColorForPoint(int colorDiff, BYTE *p, int width, int height, 
					  int x0, int y0, int x1, int y1, int x2, int y2, BOOL *foundBkg, BYTE colors[][3])
{
	BYTE *px1, *px2, *px3; 

	px1 = GET_PIXEL(p, x0,y0);
	px2 = GET_PIXEL(p, x1,y1);
	px3 = GET_PIXEL(p, x2,y2);

	// If any of the corners have transparency, forget about it
	// Not using != 255 because some MSN bmps have 254 in some positions
	if (px1[3] < 253 || px2[3] < 253 || px3[3] < 253)
		return FALSE;

	// See if is the same color
	if (ColorsAreTheSame(colorDiff, px1, px2) && ColorsAreTheSame(colorDiff, px3, px2))
	{
		*foundBkg = TRUE;
		memmove(colors, px1, 3);
	}
	else
	{
		*foundBkg = FALSE;
	}

	return TRUE;
}


static DWORD GetImgHash(HBITMAP hBitmap)
{
	BITMAP bmp;
	DWORD dwLen;
	WORD *p;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (WORD *)malloc(dwLen);
	if (p == NULL)
		return 0;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	DWORD ret = 0;
	for (DWORD i = 0 ; i < dwLen/2 ; i++)
		ret += p[i];

	free(p);

	return ret;
}

/*
 * Changes the handle to a grayscale image
 */
static BOOL MakeGrayscale(HANDLE hContact, HBITMAP *hBitmap)
{
	BYTE *p = NULL;
	DWORD dwLen;
    int width, height;
    BITMAP bmp;

	GetObject(*hBitmap, sizeof(bmp), &bmp);
    width = bmp.bmWidth;
	height = bmp.bmHeight;

	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
    if (p == NULL) 
	{
		return FALSE;
	}

	if (bmp.bmBitsPixel != 32)
	{
		// Convert to 32 bpp
		HBITMAP hBmpTmp = CopyBitmapTo32(*hBitmap);
		DeleteObject(*hBitmap);
		*hBitmap = hBmpTmp;

		GetBitmapBits(*hBitmap, dwLen, p);
	} 
	else
	{
		GetBitmapBits(*hBitmap, dwLen, p);
	}

	// Make grayscale
	BYTE *p1;
	for (int y = 0 ; y < height ; y++)
	{
		for (int x = 0 ; x < width ; x++)
		{
			p1 = GET_PIXEL(p, x, y);
			p1[0] = p1[1] = p1[2] = ( p1[0] + p1[1] + p1[2] ) / 3;
		}
	}

    dwLen = SetBitmapBits(*hBitmap, dwLen, p);
    free(p);

	return TRUE;
}

/*
 * See if finds a transparent background in image, and set its transparency
 * Return TRUE if found a transparent background
 */
static BOOL MakeTransparentBkg(HANDLE hContact, HBITMAP *hBitmap)
{
	BYTE *p = NULL;
	DWORD dwLen;
    int width, height, i, j;
    BITMAP bmp;
	BYTE colors[8][3];
	BOOL foundBkg[8];
	BYTE *px1; 
	int count, maxCount, selectedColor;
	HBITMAP hBmpTmp;
	int colorDiff;

	GetObject(*hBitmap, sizeof(bmp), &bmp);
    width = bmp.bmWidth;
	height = bmp.bmHeight;
	colorDiff = DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgColorDiff", 
					DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgColorDiff", 10));

	// Min 5x5 to easy things in loop
	if (width <= 4 || height <= 4)
		return FALSE;

	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
    if (p == NULL) 
	{
		return FALSE;
	}

	if (bmp.bmBitsPixel == 32)
	{
		hBmpTmp = *hBitmap;
	}
	else
	{
		// Convert to 32 bpp
		hBmpTmp = CopyBitmapTo32(*hBitmap);
	}

    GetBitmapBits(hBmpTmp, dwLen, p);

	// **** Get corner colors

	// Top left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  0, 0, 0, 1, 1, 0, &foundBkg[0], &colors[0]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top center
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width/2, 0, width/2-1, 0, width/2+1, 0, &foundBkg[1], &colors[1]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top Right
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width-1, 0, width-1, 1, width-2, 0, &foundBkg[2], &colors[2]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  0, height/2, 0, height/2-1, 0, height/2+1, &foundBkg[3], &colors[3]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width-1, height/2, width-1, height/2-1, width-1, height/2+1, &foundBkg[4], &colors[4]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  0, height-1, 0, height-2, 1, height-1, &foundBkg[5], &colors[5]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom center
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width/2, height-1, width/2-1, height-1, width/2+1, height-1, &foundBkg[6], &colors[6]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom Right
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width-1, height-1, width-1, height-2, width-2, height-1, &foundBkg[7], &colors[7]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}


	// **** X corners have to have the same color

	count = 0;
	for (i = 0 ; i < 8 ; i++)
	{
		if (foundBkg[i])
			count++;
	}

	if (count < DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", 
						DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5)))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Ok, X corners at least have a color, lets compare then
	maxCount = 0;
	for (i = 0 ; i < 8 ; i++)
	{
		if (foundBkg[i])
		{
			count = 0;

			for (j = 0 ; j < 8 ; j++)
			{
				if (foundBkg[j] && ColorsAreTheSame(colorDiff, (BYTE *) &colors[i], (BYTE *) &colors[j]))
					count++;
			}

			if (count > maxCount)
			{
				maxCount = count;
				selectedColor = i;
			}
		}
	}

	if (maxCount < DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", 
						DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5)))
	{
		// Not enought corners with the same color
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Get bkg color as mean of colors
	{
		int bkgColor[3];

		bkgColor[0] = 0;
		bkgColor[1] = 0;
		bkgColor[2] = 0;
		for (i = 0 ; i < 8 ; i++)
		{
			if (foundBkg[i] && ColorsAreTheSame(colorDiff, (BYTE *) &colors[i], (BYTE *) &colors[selectedColor]))
			{
				bkgColor[0] += colors[i][0];
				bkgColor[1] += colors[i][1];
				bkgColor[2] += colors[i][2];
			}
		}
		bkgColor[0] /= maxCount;
		bkgColor[1] /= maxCount;
		bkgColor[2] /= maxCount;

		colors[selectedColor][0] = bkgColor[0];
		colors[selectedColor][1] = bkgColor[1];
		colors[selectedColor][2] = bkgColor[2];
	}

	// **** Set alpha for the background color, from the borders

	if (hBmpTmp != *hBitmap)
	{
		DeleteObject(*hBitmap);

		*hBitmap = hBmpTmp;

		GetObject(*hBitmap, sizeof(bmp), &bmp);
		GetBitmapBits(*hBitmap, dwLen, p);
	}

	{
		// Set alpha from borders
		int x, y;
		int topPos = 0;
		int curPos = 0;
		int *stack = (int *)malloc(width * height * 2 * sizeof(int));
		bool transpProportional = (DBGetContactSettingByte(NULL, AVS_MODULE, "MakeTransparencyProportionalToColorDiff", 0) != 0);

		if (stack == NULL)
		{
			free(p);
			return FALSE;
		}

		// Put four corners
		AddToStack(stack, &topPos, 0, 0);
		AddToStack(stack, &topPos, width/2, 0);
		AddToStack(stack, &topPos, width-1, 0);
		AddToStack(stack, &topPos, 0, height/2);
		AddToStack(stack, &topPos, width-1, height/2);
		AddToStack(stack, &topPos, 0, height-1);
		AddToStack(stack, &topPos, width/2, height-1);
		AddToStack(stack, &topPos, width-1, height-1);

		while(curPos < topPos)
		{
			// Get pos
			x = stack[curPos]; curPos++;
			y = stack[curPos]; curPos++;

			// Get pixel
			px1 = GET_PIXEL(p, x, y);

			// It won't change the transparency if one exists
			// (This avoid an endless loop too)
			// Not using == 255 because some MSN bmps have 254 in some positions
			if (px1[3] >= 253)
			{
				if (ColorsAreTheSame(colorDiff, px1, (BYTE *) &colors[selectedColor]))
				{
					if (transpProportional)
					{
						px1[3] = min(252, 
								(abs(px1[0] - colors[selectedColor][0]) 
								+ abs(px1[1] - colors[selectedColor][1]) 
								+ abs(px1[2] - colors[selectedColor][2])) / 3);
					}
					else
					{
						px1[3] = 0;
					}

					// Add 4 neighbours

					if (x + 1 < width)
						AddToStack(stack, &topPos, x + 1, y);

					if (x - 1 >= 0)
						AddToStack(stack, &topPos, x - 1, y);

					if (y + 1 < height)
						AddToStack(stack, &topPos, x, y + 1);

					if (y - 1 >= 0)
						AddToStack(stack, &topPos, x, y - 1);
				}
			}
		}

		free(stack);
	}

    dwLen = SetBitmapBits(*hBitmap, dwLen, p);
    free(p);

	return TRUE;
}

/*
 * needed for per pixel transparent images. Such images should then be rendered by
 * using AlphaBlend() with AC_SRC_ALPHA
 * dwFlags will be set to AVS_PREMULTIPLIED
 * return TRUE if the image has at least one pixel with transparency
 */

static BOOL PreMultiply(HBITMAP hBitmap)
{
	BYTE *p = NULL;
	DWORD dwLen;
    int width, height, x, y;
    BITMAP bmp;
    BYTE alpha;
	BOOL transp = FALSE;
    
	GetObject(hBitmap, sizeof(bmp), &bmp);
    width = bmp.bmWidth;
	height = bmp.bmHeight;
	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
    if (p != NULL) 
	{
        GetBitmapBits(hBitmap, dwLen, p);

        for (y = 0; y < height; ++y) 
		{
            BYTE *px = p + width * 4 * y;

            for (x = 0; x < width; ++x) 
			{
                alpha = px[3];

				if (alpha < 255) 
				{
					transp  = TRUE;

					px[0] = px[0] * alpha/255;
					px[1] = px[1] * alpha/255;
					px[2] = px[2] * alpha/255;
				}

                px += 4;
            }
        }

        dwLen = SetBitmapBits(hBitmap, dwLen, p);
        free(p);
    }

	return transp;
}

/*
 * convert the avatar image path to a relative one...
 * given: contact handle
 */

static void MakePathRelative(HANDLE hContact)
{
   DBVARIANT dbv = {0};
   char szFinalPath[MAX_PATH];
   int result = 0;

   szFinalPath[0] = 0;
   
   if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
       if(dbv.type == DBVT_ASCIIZ) {
           result = CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)dbv.pszVal, (LPARAM)szFinalPath);
           if(result && lstrlenA(szFinalPath) > 0) {
               DBWriteContactSettingString(hContact, "ContactPhoto", "RFile", szFinalPath);
               if(!DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0))
                   DBWriteContactSettingString(hContact, "ContactPhoto", "Backup", szFinalPath);
           }
       }
       DBFreeVariant(&dbv);
   }
}

static void AvatarUpdateThread(LPVOID vParam)
{
    PROTO_AVATAR_INFORMATION pai_s;
    int result;
    char *szProto = NULL;
    int i;
    HANDLE hFound = 0;
    
    do {
        if(dwPendingAvatarJobs == 0 && bAvatarThreadRunning)
            SuspendThread(hAvatarThread);                       // nothing to do...
        if(!bAvatarThreadRunning)                           
            _endthread();
        /*
         * the thread is awake, processing the update quque one by one entry with a given delay of 5 seconds. The delay
         * ensures that no protocol will kick us because of flood protection(s)
         */
        hFound = 0;
        
        for (i = 0; i < AV_QUEUESIZE; i++) {
            if (avatarUpdateQueue[i] != 0) {
                int status = ID_STATUS_OFFLINE;
                szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)avatarUpdateQueue[i], 0);
                if(szProto && g_MetaName && !strcmp(szProto, g_MetaName)) {
                    HANDLE hSubContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)avatarUpdateQueue[i], 0);
                    if(hSubContact)
                        szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hSubContact, 0);
                }
                if(szProto) {
                    status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
                    if(status != ID_STATUS_OFFLINE && status != ID_STATUS_INVISIBLE) {
				        EnterCriticalSection(&avcs);
                        hFound = avatarUpdateQueue[i];
                        avatarUpdateQueue[i] = 0;
                        dwPendingAvatarJobs--;
				        LeaveCriticalSection(&avcs);
                        break;
                    }
                    else if(status == ID_STATUS_OFFLINE) {
				        EnterCriticalSection(&avcs);
                        avatarUpdateQueue[i] = 0;
                        dwPendingAvatarJobs--;
				        LeaveCriticalSection(&avcs);
                    }
                }
            }
        }

        if(hFound) {
            if(szProto) {
                if(DBGetContactSettingWord(hFound, szProto, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
                    pai_s.cbSize = sizeof(pai_s);
                    pai_s.hContact = hFound;
                    pai_s.format = PA_FORMAT_UNKNOWN;
                    strcpy(pai_s.filename, "X");
                    result = CallProtoService(szProto, PS_GETAVATARINFO, GAIF_FORCE, (LPARAM)&pai_s);
                    if(result == GAIR_SUCCESS) {
                        DBWriteContactSettingString(hFound, "ContactPhoto", "File", "****");
                        DBWriteContactSettingString(hFound, "ContactPhoto", "File", pai_s.filename);
                        if(DBGetContactSettingByte(hFound, "MetaContacts", "IsSubcontact", 0)) {
                            HANDLE hMaster = (HANDLE)DBGetContactSettingDword(hFound, "MetaContacts", "Handle", 0);
                            if(hMaster) {
                                DBWriteContactSettingString(hMaster, "ContactPhoto", "File", "****");
                                DBWriteContactSettingString(hMaster, "ContactPhoto", "File", pai_s.filename);
                            }
                        }
                    }
                }
            }
        }
		for(int n = 0; n < 5; n++) {
			if(!bAvatarThreadRunning)
				break;
			Sleep(1000L);
		}
    } while ( bAvatarThreadRunning );
	_endthread();
}

int UpdateAvatar(HANDLE hContact)
{
    int i;
    int pCaps = 0;
    char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
    int iFound = -1;

    if(hContact && szProto) {
        if(DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
            pCaps = CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_4, 0);
            if(pCaps & PF4_AVATARS && DBGetContactSettingByte(NULL, AVS_MODULE, szProto, 1) && DBGetContactSettingWord(hContact, szProto, "ApparentMode", 0) != ID_STATUS_OFFLINE) {
                EnterCriticalSection(&avcs);
                for (i = 0; i < AV_QUEUESIZE && avatarUpdateQueue[i] != hContact; i++) {
                    if(iFound == -1 && avatarUpdateQueue[i] == 0)
                        iFound = i;
                }
                if(i == AV_QUEUESIZE && iFound != -1) {
                    avatarUpdateQueue[iFound] = hContact;
                    dwPendingAvatarJobs++;
                    ResumeThread(hAvatarThread);            // make sure, the thread wakes up...
                }
                LeaveCriticalSection(&avcs);
            }
        }
    }
    return 0;
}


void ResetTranspSettings(HANDLE hContact)
{
	DBDeleteContactSetting(hContact, "ContactPhoto", "MakeTransparentBkg");
	DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgNumPoints");
	DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgColorDiff");
}


// create the avatar in cache
// returns 0 if not created (no avatar), iIndex otherwise
int CreateAvatarInCache(HANDLE hContact, struct avatarCacheEntry *ace, char *szProto)
{
    DBVARIANT dbv = {0};
    char *szExt = NULL;
    char szFilename[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSizeHigh = 0, dwFileSize = 0, sizeLimit = 0;
    
    szFilename[0] = 0;

    if(szProto == NULL) {
        if(DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0)) {
            if(!DBGetContactSetting(hContact, "ContactPhoto", "Backup", &dbv)) {
                CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFilename);
                DBFreeVariant(&dbv);
                goto done;
            }
        }
        if(!DBGetContactSetting(hContact, "ContactPhoto", "RFile", &dbv)) {
            CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFilename);
            DBFreeVariant(&dbv);
        }
        else if(DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
            UpdateAvatar(hContact);
            return -1;
        }
        else {
            strncpy(szFilename, dbv.pszVal, MAX_PATH);
            szFilename[MAX_PATH - 1] = 0;
            DBFreeVariant(&dbv);
        }

    }
    else {
		if(hContact == 0) {				// create a protocol picture in the proto picture cache
			if(!DBGetContactSetting(NULL, PPICT_MODULE, szProto, &dbv)) {
				CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFilename);
				DBFreeVariant(&dbv);
			}
		}
		else if(hContact == (HANDLE)-1) {	// create own picture - note, own avatars are not on demand, they are loaded once at
											// startup and everytime they are changed.
			if(!DBGetContactSetting(NULL, szProto, "AvatarFile", &dbv)) {
				CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFilename);
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
				}
				return -1;
			}
		}
    }
done:    
    if(lstrlenA(szFilename) < 4)
        return -1;
    
    if((hFile = CreateFileA(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        if(hContact != 0 && hContact != (HANDLE)-1)
            UpdateAvatar(hContact);
        return -1;
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

		if ((hContact != 0 && hContact != (HANDLE)-1) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0))
		{
			// Have to reset settings? -> do it if image changed
			DWORD imgHash = GetImgHash(ace->hbmPic);

			if (imgHash != DBGetContactSettingDword(hContact, "ContactPhoto", "ImageHash", 0))
			{
				ResetTranspSettings(hContact);
				DBWriteContactSettingDword(hContact, "ContactPhoto", "ImageHash", imgHash);
			}
		}

		if (( (hContact != 0 && hContact != (HANDLE)-1) || DBGetContactSettingByte(0, AVS_MODULE, "MakeMyAvatarsTransparent", 0)) &&
			DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0) && 
			DBGetContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", 1))
		{
			if (MakeTransparentBkg(hContact, &ace->hbmPic))
			{
				ace->dwFlags |= AVS_CUSTOMTRANSPBKG | AVS_HASTRANSPARENCY;
				GetObject(ace->hbmPic, sizeof(bminfo), &bminfo);
			}
		}

		if (DBGetContactSettingByte(0, AVS_MODULE, "MakeGrayscale", 0))
		{
			MakeGrayscale(hContact, &ace->hbmPic);
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
    else if(hContact != 0 && hContact != (HANDLE)-1){                     // don't update for pseudo avatars...
        UpdateAvatar(hContact);
        return -1;
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

static struct CacheNode *FindAvatarInCache(HANDLE hContact)
{
	struct CacheNode *cacheNode = g_Cache, *foundNode = NULL;
    
	EnterCriticalSection(&cachecs);
	while(cacheNode) {
		if(cacheNode->ace.hContact == hContact) {
			LeaveCriticalSection(&cachecs);
			return cacheNode;
		}
		if(foundNode == NULL && cacheNode->ace.hbmPic == 0 && cacheNode->ace.hContact == 0)
			foundNode = cacheNode;				// found an empty and usable node
		cacheNode = cacheNode->pNextNode;
    }
    // not found, create it in cache

	if(foundNode == NULL) {					// no free entry found, create a new and append it to the list
		struct CacheNode *newNode = AllocCacheBlock();
		AddToList(newNode);
		foundNode = newNode;
	}
	if(CreateAvatarInCache(hContact, &foundNode->ace, NULL) != -1) {
	    LeaveCriticalSection(&cachecs);
        return foundNode;
	}
	else {
		ZeroMemory(&foundNode->ace, sizeof(struct avatarCacheEntry));			// mark the entry as unused
	    LeaveCriticalSection(&cachecs);
        return NULL;
	}
}


static int ProtocolAck(WPARAM wParam, LPARAM lParam)
{
    ACKDATA *ack = (ACKDATA *) lParam;

    if (ack != NULL && ack->type == ACKTYPE_AVATAR && ack->hContact != 0) {
        PROTO_AVATAR_INFORMATION *pai = (PROTO_AVATAR_INFORMATION *) ack->hProcess;
        if(pai == NULL)
            return 0;
        if (ack->result == ACKRESULT_SUCCESS) {
            DBWriteContactSettingString(ack->hContact, "ContactPhoto", "File", "****");
            DBWriteContactSettingString(ack->hContact, "ContactPhoto", "File", pai->filename);
            if(DBGetContactSettingByte(ack->hContact, "MetaContacts", "IsSubcontact", 0)) {
                HANDLE hMaster = (HANDLE)DBGetContactSettingDword(ack->hContact, "MetaContacts", "Handle", 0);
                if(hMaster) {
                    DBWriteContactSettingString(hMaster, "ContactPhoto", "File", "****");
                    DBWriteContactSettingString(hMaster, "ContactPhoto", "File", pai->filename);
                }
            }
        }
        else if(ack->result == ACKRESULT_FAILED) {
            DBDeleteContactSetting(ack->hContact, "ContactPhoto", "File");
            DBDeleteContactSetting(ack->hContact, "ContactPhoto", "RFile");
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
        if(wParam == 0)
            MakePathRelative(hContact);
        ChangeAvatar(hContact);
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
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);
            OPENFILENAMEA *ofn = (OPENFILENAMEA *)lParam;

			OpenFileSubclassData *data = (OpenFileSubclassData *) malloc(sizeof(OpenFileSubclassData));
            data->locking_request = (BYTE *)ofn->lCustData;
			data->setView = TRUE;

			ofn->lCustData = (LPARAM) data;

            CheckDlgButton(hwnd, IDC_PROTECTAVATAR, *(data->locking_request));
            break;
        }
        case WM_COMMAND:
		{
            if(LOWORD(wParam) == IDC_PROTECTAVATAR) 
			{
				OPENFILENAMEA *ofn = (OPENFILENAMEA *)GetWindowLong(hwnd, GWL_USERDATA);
                OpenFileSubclassData *data= (OpenFileSubclassData *)ofn->lCustData;
                *(data->locking_request) = IsDlgButtonChecked(hwnd, IDC_PROTECTAVATAR) ? TRUE : FALSE;
            }
            break;
		}
		case WM_NOTIFY:
		{
			OPENFILENAMEA *ofn = (OPENFILENAMEA *)GetWindowLong(hwnd, GWL_USERDATA);
			OpenFileSubclassData *data = (OpenFileSubclassData *)ofn->lCustData;
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
		case WM_DESTROY:
		{
			OPENFILENAMEA *ofn = (OPENFILENAMEA *)GetWindowLong(hwnd, GWL_USERDATA);
			free((OpenFileSubclassData *) ofn->lCustData);
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
    char FileName[MAX_PATH];
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
    DBWriteContactSettingByte(hContact, "ContactPhoto", "Locked", 0);       // force unlock it
    DBWriteContactSettingString(hContact, "ContactPhoto", "File", szFinalName);
    MakePathRelative(hContact);

    if(is_locked) {                                  // restore protection, if it was active
        ProtectAvatar((WPARAM)hContact, 1);
    }

    return 0;
}

// See if a protocol service exists
__inline static int ProtoServiceExists(const char *szModule,const char *szService)
{
	char str[MAXMODULELABELLENGTH * 2];
	strcpy(str,szModule);
	strcat(str,szService);
	return ServiceExists(str);
}

/*
 * see if is possible to set the avatar for the expecified protocol
 */
int CanSetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *protocol;

    if(wParam == 0)
        return 0;
	
	protocol = (char *)wParam;

	return ProtoServiceExists(protocol, PS_SETMYAVATAR) 
			|| ProtoServiceExists(protocol, MSN_PS_SETMYAVATAR); // TODO remove this when MSN change this
}

/*
 * Callback to set thumbnaill view to open dialog
 */
UINT_PTR CALLBACK OFNHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

/*
 * set an avatar for a protocol (service function)
 * if lParam == NULL, a open file dialog will be opened, otherwise, lParam is taken as a FULL
 * image filename (will be checked for existance, though)
 */
int SetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *protocol;
    char FileName[MAX_PATH];
    char *szFinalName = NULL;
    HANDLE hFile = 0;
    
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

    if(lParam == 0) {
        OPENFILENAMEA ofn = {0};
		char filter[256];
		char inipath[1024];

		filter[0] = '\0';
		CallService(MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof(filter), (LPARAM) filter);

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

    if((hFile = CreateFileA(szFinalName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        return -3;
    
    CloseHandle(hFile);
        
    // file exists...

	HBITMAP hBmp = (HBITMAP) BmpFilterLoadBitmap32(NULL, (LPARAM)szFinalName);
    if (hBmp == 0)
		return -4;

	BITMAP bminfo;
	GetObject(hBmp, sizeof(bminfo), &bminfo);

	if (bminfo.bmBitsPixel != 32)
	{
		HBITMAP hBmpTmp = CopyBitmapTo32(hBmp);
		if (hBmpTmp == 0)
			return -5;
		DeleteObject(hBmp);
		hBmp = hBmpTmp;
	}

	int ret = 0;
	if (protocol != NULL)
	{
		ret = SetProtoMyAvatar(protocol, hBmp);
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

			// Found a protocol
			if (CanSetMyAvatar((WPARAM) protos[i]->szName, 0))
			{
				int retTmp = SetProtoMyAvatar(protos[i]->szName, hBmp);
				if (retTmp != 0)
					ret = retTmp;
			}
		}
	}

	DeleteObject(hBmp);

	return ret;
}
	
int SetProtoMyAvatar(char *protocol, HBITMAP hBmp)
{
	// Protocol has max size?
	int width = -1, height = -1;
	bool square = false;	// Get user setting

	if (ProtoServiceExists(protocol, PS_GETMYAVATARMAXSIZE))
		CallProtoService(protocol, PS_GETMYAVATARMAXSIZE, (WPARAM) &width, (LPARAM) &height);

	if (DBGetContactSettingByte(0, AVS_MODULE, "SetAllwaysMakeSquare", 0))
		square = true;
	else if (ProtoServiceExists(protocol, PS_GETMYAVATARIMAGEPROPORTION))
		if (CallProtoService(protocol, PS_GETMYAVATARIMAGEPROPORTION, 0, 0) == PIP_SQUARE)
			square = true;

	ResizeBitmap rb;
	rb.size = sizeof(ResizeBitmap);
	rb.hBmp = hBmp;
	rb.max_height = (height <= 0 ? 300 : height);
	rb.max_width = (width <= 0 ? 300 : width);
	rb.fit = square ? RESIZEBITMAP_MAKE_SQUARE : RESIZEBITMAP_KEEP_PROPORTIONS;

	HBITMAP hBmpProto = (HBITMAP) BmpFilterResizeBitmap((WPARAM)&rb, 0);

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
		return -2;
	}

	char image_file_name[MAX_PATH];

	bool saved = false;

	// What format?
	if (BmpFilterCanSaveBitmap(0, PA_FORMAT_PNG) // Png is default
		&& (!ProtoServiceExists(protocol, PS_ISAVATARFORMATSUPPORTED) 
			|| CallProtoService(protocol, PS_ISAVATARFORMATSUPPORTED, 0, PA_FORMAT_PNG)))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".png");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	if (!saved  // Jpeg is second
		&& BmpFilterCanSaveBitmap(0, PA_FORMAT_JPEG)
		&& (!ProtoServiceExists(protocol, PS_ISAVATARFORMATSUPPORTED) 
			|| CallProtoService(protocol, PS_ISAVATARFORMATSUPPORTED, 0, PA_FORMAT_JPEG)))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".jpg");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	if (!saved  // Gif
		&& BmpFilterCanSaveBitmap(0, PA_FORMAT_GIF)
		&& (!ProtoServiceExists(protocol, PS_ISAVATARFORMATSUPPORTED) 
			|| CallProtoService(protocol, PS_ISAVATARFORMATSUPPORTED, 0, PA_FORMAT_GIF)))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".gif");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	if (!saved   // Bitmap
		&& BmpFilterCanSaveBitmap(0, PA_FORMAT_BMP)
		&& (!ProtoServiceExists(protocol, PS_ISAVATARFORMATSUPPORTED) 
			|| CallProtoService(protocol, PS_ISAVATARFORMATSUPPORTED, 0, PA_FORMAT_BMP)))
	{
		mir_snprintf(image_file_name, sizeof(image_file_name), "%s%s", temp_file, ".bmp");
		if (!BmpFilterSaveBitmap((WPARAM)hBmpProto, (LPARAM)image_file_name))
			saved = true;
	}
	
	int ret;

	if (saved)
	{
		// Call proto service
		if (ProtoServiceExists(protocol, PS_SETMYAVATAR))
			ret = CallProtoService(protocol, PS_SETMYAVATAR, 0, (LPARAM)image_file_name);
		else if (ProtoServiceExists(protocol, MSN_PS_SETMYAVATAR))
			ret = CallProtoService(protocol, MSN_PS_SETMYAVATAR, 0, (LPARAM)image_file_name); // TODO remove this when MSN change this

		DeleteFileA(image_file_name);
	}

	DeleteFileA(temp_file);

	if (hBmpProto != hBmp) 
		DeleteObject(hBmpProto);

	return saved ? ret : -3;
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

static int GetAvatarBitmap(WPARAM wParam, LPARAM lParam)
{
    int i;
	struct CacheNode *node = NULL;
    
    if(wParam == 0 || g_shutDown)
		return 0;

    node = FindAvatarInCache((HANDLE)wParam);
    if(node == NULL) {
        char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
        if(szProto) {
            for(i = 0; i < g_protocount; i++) {
                if(g_ProtoPictures[i].szProtoname[0] && !strcmp(g_ProtoPictures[i].szProtoname, szProto) && lstrlenA(szProto) == lstrlenA(g_ProtoPictures[i].szProtoname) && g_ProtoPictures[i].hbmPic != 0)
                    return (int)&g_ProtoPictures[i];
            }
        }
        return 0;
    }
    else {
		node->ace.t_lastAccess = time(NULL);
        return (int)&node->ace;
    }
}

int DeleteAvatar(HANDLE hContact)
{
    char *szProto = NULL;
	struct CacheNode *pNode = g_Cache;

    if(g_shutDown)
		return 0;

	EnterCriticalSection(&cachecs);
	while(pNode) {
        if(pNode->ace.hContact == hContact) {
            if(pNode->ace.hbmPic != 0)
                DeleteObject(pNode->ace.hbmPic);
            ZeroMemory((void *)&pNode->ace, sizeof(struct avatarCacheEntry));
            break;
        }
		pNode = pNode->pNextNode;
    }
    LeaveCriticalSection(&cachecs);
    // fallback with a protocol picture, if available...
    szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
    if(szProto) {
        for(int i = 0; i < g_protocount; i++) {
            if(g_ProtoPictures[i].szProtoname != NULL && !strcmp(g_ProtoPictures[i].szProtoname, szProto) && g_ProtoPictures[i].hbmPic != 0) {
                NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)&g_ProtoPictures[i]);
                return 0;
            }
        }
    }
    NotifyEventHooks(hEventChanged, (WPARAM)hContact, 0);
    return 0;
}

int ChangeAvatar(HANDLE hContact)
{
	struct CacheNode *pNode = g_Cache, *newNode = NULL;

    if(g_shutDown)
		return 0;

	EnterCriticalSection(&cachecs);
	while(pNode) {
        if(pNode->ace.hContact == hContact) {
            if(pNode->ace.hbmPic != 0)
                DeleteObject(pNode->ace.hbmPic);
            ZeroMemory((void *)&pNode->ace, sizeof(struct avatarCacheEntry));
            break;
        }
		pNode = pNode->pNextNode;
    }
    LeaveCriticalSection(&cachecs);
    newNode = FindAvatarInCache(hContact);
    if(newNode) {
        newNode->ace.cbSize = sizeof(struct avatarCacheEntry);
        NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)&newNode->ace);
    }
    else {              // fallback with protocol picture
        char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
        if(szProto) {
            for(int i = 0; i < g_protocount; i++) {
                if(g_ProtoPictures[i].szProtoname != NULL && !strcmp(g_ProtoPictures[i].szProtoname, szProto) && g_ProtoPictures[i].hbmPic != 0) {
                    NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)&g_ProtoPictures[i]);
                    return 0;
                }
            }
        }
        NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)NULL);
    }
    return 0;
}

static int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    int i, j;
    DBVARIANT dbv = {0};
    static Update upd = {0};
    static char *component = "Avatar service";
    static char szCurrentVersion[30];
    static char *szVersionUrl = "http://miranda.or.at/files/avs/version.txt";
    static char *szUpdateUrl = "http://miranda.or.at/files/avs/avs.zip";
    static char *szFLVersionUrl = "http://www.miranda-im.org/download/details.php?action=viewfile&id=2361";
    static char *szFLUpdateurl = "http://www.miranda-im.org/download/feed.php?dlfile=2361";
    static char *szPrefix = "avs ";

	// Start thread
	hAvatarThread = (HANDLE)_beginthread(AvatarUpdateThread, 0, NULL);

	// Folders plugin support
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH))
	{
		hMyAvatarsFolder = (HANDLE) FoldersRegisterCustomPath(Translate("Avatars"), Translate("My Avatars"), 
													  PROFILE_PATH "\\" CURRENT_PROFILE "\\MyAvatars");

		hProtocolAvatarsFolder = (HANDLE) FoldersRegisterCustomPath(Translate("Avatars"), Translate("Protocol Avatars Cache"), 
													  FOLDER_AVATARS);
	}

    CLISTMENUITEM mi;

    g_MetaAvail = ServiceExists(MS_MC_GETPROTOCOLNAME) ? TRUE : FALSE;
    if(g_MetaAvail)
        g_szMetaName = (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0);

    g_installed_protos[0] = 0;

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&g_protocount, (LPARAM)&g_protocols);
    g_ProtoPictures = (struct protoPicCacheEntry *)malloc(sizeof(struct protoPicCacheEntry) * (g_protocount + 1));
    ZeroMemory((void *)g_ProtoPictures, sizeof(struct protoPicCacheEntry) * (g_protocount + 1));

	g_MyAvatars = (struct protoPicCacheEntry *)malloc(sizeof(struct protoPicCacheEntry) * (g_protocount + 1));
    ZeroMemory((void *)g_MyAvatars, sizeof(struct protoPicCacheEntry) * (g_protocount + 1));
    
    j = 0;
    for(i = 0; i < g_protocount; i++) {
        if(g_protocols[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        strncat(g_installed_protos, "[", 1024);
        strncat(g_installed_protos, g_protocols[i]->szName, 1024);
        strncat(g_installed_protos, "] ", 1024);
        if(CreateAvatarInCache(0, (struct avatarCacheEntry *)&g_ProtoPictures[j], g_protocols[i]->szName) != -1)
            j++;
        else {
            strncpy(g_ProtoPictures[j].szProtoname, g_protocols[i]->szName, 100);
            g_ProtoPictures[j++].szProtoname[99] = 0;
        }
    }
	j = 0;
    for(i = 0; i < g_protocount; i++) {
        if(g_protocols[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        if(CreateAvatarInCache((HANDLE)-1, (struct avatarCacheEntry *)&g_MyAvatars[j], g_protocols[i]->szName) != -1)
            j++;
        else {
            strncpy(g_MyAvatars[j].szProtoname, g_protocols[i]->szName, 100);
            g_MyAvatars[j++].szProtoname[99] = 0;
        }
    }
    DBWriteContactSettingString(NULL, AVS_MODULE, "InstalledProtos", g_installed_protos);
    if(ServiceExists(MS_MC_GETMOSTONLINECONTACT))
        g_MetaName = (char *)CallService(MS_MC_GETPROTOCOLNAME, 0, 0);
    else
        g_MetaName = NULL;


    // updater plugin support

    upd.cbSize = sizeof(upd);
    upd.szComponentName = pluginInfo.shortName;
    upd.pbVersion = (BYTE *)CreateVersionStringPlugin(&pluginInfo, szCurrentVersion);
    upd.cpbVersion = strlen((char *)upd.pbVersion);
    upd.szVersionURL = szFLVersionUrl;
    upd.szUpdateURL = szFLUpdateurl;
    upd.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">Updater ";

    upd.szBetaUpdateURL = szUpdateUrl;
    upd.szBetaVersionURL = szVersionUrl;
    upd.pbBetaVersionPrefix = (BYTE *)szPrefix;
    upd.pbVersion = (PBYTE)szCurrentVersion;
    upd.cpbVersion = lstrlenA(szCurrentVersion);

    upd.cpbVersionPrefix = strlen((char *)upd.pbVersionPrefix);
    upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);

    if(ServiceExists(MS_UPDATE_REGISTER))
        CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);

    g_hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_AVATAR));
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);

    mi.position = 2000080000;
    mi.flags = 0;
    mi.hIcon = g_hIcon;
    mi.pszContactOwner = NULL;    //on every contact
    mi.pszName = Translate("Contact picture...");
    mi.pszService = MS_AV_CONTACTOPTIONS;
    CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) &mi);

	return 0;
}

static void ReloadMyAvatar(LPVOID lpParam)
{
	char *szProto = (char *)lpParam;
	
	Sleep(5000);
	for(int i = 0; i < g_protocount; i++) {
		if(!strcmp(g_MyAvatars[i].szProtoname, szProto) && lstrlenA(szProto) == lstrlenA(g_MyAvatars[i].szProtoname)) {
			if(g_MyAvatars[i].hbmPic)
				DeleteObject(g_MyAvatars[i].hbmPic);

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
		if(!strcmp(g_MyAvatars[i].szProtoname, proto) && lstrlenA(proto) == lstrlenA(g_MyAvatars[i].szProtoname)) {
			LPVOID lpParam = 0;

			lpParam = (void *)malloc(lstrlenA(g_MyAvatars[i].szProtoname) + 2);
			strcpy((char *)lpParam, g_MyAvatars[i].szProtoname);
			_beginthread(ReloadMyAvatar, 0, lpParam);
			return 0;
		}
	}

	return -2;
}

static int ContactSettingChanged(WPARAM wParam, LPARAM lParam)
{
    char *szProto;
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  

    if(cws == NULL || g_shutDown)
        return 0;

	if(wParam == 0) {
		if(!strcmp(cws->szSetting, "AvatarFile") || !strcmp(cws->szSetting, "PictObject") || !strcmp(cws->szSetting, "AvatarHash")) {
			ReportMyAvatarChanged((WPARAM) cws->szModule, 0);
		}
		return 0;
	}
    if(!strcmp(cws->szModule, "ContactPhoto")) {
        DBVARIANT dbv = {0};
        
        if(!strcmp(cws->szSetting, "File") && !DBGetContactSetting((HANDLE)wParam, "ContactPhoto", "File", &dbv)) {
            HANDLE hFile;
            if(strcmp(dbv.pszVal, "****")) {
                if((hFile = CreateFileA(dbv.pszVal, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
                    MakePathRelative((HANDLE)wParam);
                    CloseHandle(hFile);
                    ChangeAvatar((HANDLE)wParam);
                }
            }
            DBFreeVariant(&dbv);
        }
        return 0;
    }
    szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
    if(szProto && !strcmp(cws->szModule, szProto)) {
        if(!strcmp(cws->szSetting, "PictContext"))
            UpdateAvatar((HANDLE)wParam);
        if(!strcmp(cws->szSetting, "AvatarHash")) {
            UpdateAvatar((HANDLE)wParam);
        }
    }
    return 0;
}

static int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
	struct CacheNode *pNode = g_Cache;

    EnterCriticalSection(&cachecs);
	while(pNode) {
        if(pNode->ace.hContact == (HANDLE)wParam) {
            if(pNode->ace.hbmPic != 0)
                DeleteObject(pNode->ace.hbmPic);
            ZeroMemory((void *)&pNode->ace, sizeof(struct avatarCacheEntry));
            break;
        }
		pNode = pNode->pNextNode;
    }
    LeaveCriticalSection(&cachecs);
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

static int ShutdownProc(WPARAM wParam, LPARAM lParam)
{
    DWORD dwExitcode = 0;

    EnterCriticalSection(&cachecs);
	g_shutDown = TRUE;
    LeaveCriticalSection(&cachecs);

	DestroyServiceFunction(MS_AV_GETAVATARBITMAP);
    DestroyServiceFunction(MS_AV_PROTECTAVATAR);
    DestroyServiceFunction(MS_AV_SETAVATAR);
    DestroyServiceFunction(MS_AV_SETMYAVATAR);
    DestroyServiceFunction(MS_AV_CANSETMYAVATAR);
    DestroyServiceFunction(MS_AV_CONTACTOPTIONS);
    DestroyServiceFunction(MS_AV_DRAWAVATAR);
	DestroyServiceFunction(MS_AV_GETMYAVATAR);
	DestroyServiceFunction(MS_AV_REPORTMYAVATARCHANGED);
	DestroyServiceFunction(MS_AV_LOADBITMAP32);
	DestroyServiceFunction(MS_AV_SAVEBITMAP);
	DestroyServiceFunction(MS_AV_CANSAVEBITMAP);
	DestroyServiceFunction(MS_AV_RESIZEBITMAP);

    DestroyHookableEvent(hEventChanged);
	DestroyHookableEvent(hMyAvatarChanged);
    hEventChanged = 0;
	hMyAvatarChanged = 0;
    UnhookEvent(hContactSettingChanged);
    UnhookEvent(hProtoAckHook);
    
	bAvatarThreadRunning = FALSE;
    ResumeThread(hAvatarThread);

	WaitForSingleObject(hAvatarThread, 6000);

    DeleteCriticalSection(&avcs);
    
	EnterCriticalSection(&cachecs);

	if(avatarUpdateQueue)
        free(avatarUpdateQueue);

	struct CacheNode *pNode = g_Cache;
	while(pNode) {
        if(pNode->ace.hbmPic != 0)
            DeleteObject(pNode->ace.hbmPic);
		pNode = pNode->pNextNode;
    }

	int i;
	for(i = 0; i < g_curBlock; i++)
		free(g_cacheBlocks[i]);

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

    LeaveCriticalSection(&cachecs);
    DeleteCriticalSection(&cachecs);
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
    if(ace) {
        float dScale = 0.;
        float newHeight, newWidth;
        HDC hdcAvatar;
        HBITMAP hbmMem;
        DWORD topoffset = 0, leftoffset = 0;
        LONG bmWidth, bmHeight;
        float dAspect;
        HBITMAP hbm;
        HRGN rgn = 0, oldRgn = 0;
        int targetWidth = r->rcDraw.right - r->rcDraw.left;
        int targetHeight = r->rcDraw.bottom - r->rcDraw.top;
        BLENDFUNCTION bf = {0};
        
        bmHeight = ace->bmHeight;
        bmWidth = ace->bmWidth;
        if(bmWidth != 0)
            dAspect = (float)bmHeight / (float)bmWidth;
        else
            dAspect = 1.0;
        hbm = ace->hbmPic;
        ace->t_lastAccess = time(NULL);

        if(bmHeight == 0 || bmWidth == 0 || hbm == 0)
            return 0;

        hdcAvatar = CreateCompatibleDC(r->hTargetDC);
        hbmMem = (HBITMAP)SelectObject(hdcAvatar, hbm);

        if(dAspect >= 1.0) {            // height > width
            dScale = (float)(targetHeight) / (float)bmHeight;
            newHeight = (float)(targetHeight);
            newWidth = (float)bmWidth * dScale;
        }
        else {
            /* ??? */
            dScale = (float)(targetWidth) / (float)bmWidth;
            newWidth = (float)(targetWidth);
            newHeight = (float)bmHeight * dScale;
            /* ??? */
        }
		/*
		if(dAspect >= 1.0) {            // height > width
            dScale = (float)(targetHeight) / (float)bmHeight;
            newHeight = (float)(targetHeight);
            newWidth = (float)bmWidth * dScale;
        }
        else {
            newWidth = (float)(targetHeight);
            dScale = (float)(targetHeight) / (float)bmWidth;
            newHeight = (float)bmHeight * dScale;
        }*/

        topoffset = targetHeight > (int)newHeight ? (targetHeight - (int)newHeight) / 2 : 0;

        // create the region for the avatar border - use the same region for clipping, if needed.

        leftoffset = targetWidth > (int)newWidth ? (targetWidth - (int)newWidth) / 2 : 0;
        
		oldRgn = CreateRectRgn(0,0,1,1);
		if (GetClipRgn(r->hTargetDC, oldRgn) != 1)
		{
			DeleteObject(oldRgn);
			oldRgn = NULL;
		}

        if(r->dwFlags & AVDRQ_ROUNDEDCORNER)
            rgn = CreateRoundRectRgn(r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, r->rcDraw.left + leftoffset + (int)newWidth + 1, r->rcDraw.top + topoffset + (int)newHeight + 1, 2 * r->radius, 2 * r->radius);
        else
            rgn = CreateRectRgn(r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, r->rcDraw.left + leftoffset + (int)newWidth, r->rcDraw.top + topoffset + (int)newHeight);

		ExtSelectClipRgn(r->hTargetDC, rgn, RGN_AND);

        bf.SourceConstantAlpha = r->alpha > 0 ? r->alpha : 255;
        bf.AlphaFormat = ace->dwFlags & AVS_PREMULTIPLIED ? AC_SRC_ALPHA : 0;

        SetStretchBltMode(r->hTargetDC, HALFTONE);
        if(bf.SourceConstantAlpha == 255 && bf.AlphaFormat == 0) {
            StretchBlt(r->hTargetDC, r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, (int)newWidth, (int)newHeight, hdcAvatar, 0, 0, bmWidth, bmHeight, SRCCOPY);
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
            StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, r->hTargetDC, r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, (int)newWidth, (int)newHeight, SRCCOPY);
            AlphaBlend(hdcTemp, 0, 0, bmWidth, bmHeight, hdcAvatar, 0, 0, bmWidth, bmHeight, bf);
            StretchBlt(r->hTargetDC, r->rcDraw.left + leftoffset, r->rcDraw.top + topoffset, (int)newWidth, (int)newHeight, hdcTemp, 0, 0, bmWidth, bmHeight, SRCCOPY);
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

static int LoadAvatarModule()
{
//    HMODULE hModule;
	HookEvent(ME_OPT_INITIALISE, OptInit);
    HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
    hContactSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ContactSettingChanged);
    HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);
    hProtoAckHook = (HANDLE) HookEvent(ME_PROTO_ACK, ProtocolAck);
    CreateServiceFunction(MS_AV_GETAVATARBITMAP, GetAvatarBitmap);
    CreateServiceFunction(MS_AV_PROTECTAVATAR, ProtectAvatar);
    CreateServiceFunction(MS_AV_SETAVATAR, SetAvatar);
    CreateServiceFunction(MS_AV_SETMYAVATAR, SetMyAvatar);
    CreateServiceFunction(MS_AV_CANSETMYAVATAR, CanSetMyAvatar);
    CreateServiceFunction(MS_AV_CONTACTOPTIONS, ContactOptions);
    CreateServiceFunction(MS_AV_DRAWAVATAR, DrawAvatarPicture);
	CreateServiceFunction(MS_AV_GETMYAVATAR, GetMyAvatar);
	CreateServiceFunction(MS_AV_REPORTMYAVATARCHANGED, ReportMyAvatarChanged);
	CreateServiceFunction(MS_AV_LOADBITMAP32, BmpFilterLoadBitmap32);
	CreateServiceFunction(MS_AV_SAVEBITMAP, BmpFilterSaveBitmap);
	CreateServiceFunction(MS_AV_CANSAVEBITMAP, BmpFilterCanSaveBitmap);
	CreateServiceFunction(MS_AV_RESIZEBITMAP, BmpFilterResizeBitmap);
    hEventChanged = CreateHookableEvent(ME_AV_AVATARCHANGED);
	hMyAvatarChanged = CreateHookableEvent(ME_AV_MYAVATARCHANGED);
	AllocCacheBlock();
    AV_QUEUESIZE = CallService(MS_DB_CONTACT_GETCOUNT, 0, 0);
    if(AV_QUEUESIZE < 300)
        AV_QUEUESIZE = 300;
    avatarUpdateQueue = (HANDLE *)malloc(sizeof(HANDLE) * (AV_QUEUESIZE + 10));
    ZeroMemory((void *)avatarUpdateQueue, sizeof(HANDLE) * (AV_QUEUESIZE + 10));

    InitializeCriticalSection(&avcs);
    InitializeCriticalSection(&cachecs);

    CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)g_szDBPath);

	hAvatarThread = 0;
	//hAvatarThread = (HANDLE)_beginthread(AvatarUpdateThread, 0, NULL);
	//hAvatarThread = CreateThread(NULL, 16000, AvatarUpdateThread, NULL, 0, &dwAvatarThreadID);
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

static int systemModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    ModulesLoaded(wParam, lParam);
    return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK * link)
{
    pluginLink = link;
    
	LoadGdiPlus();

    return(LoadAvatarModule());
}

extern "C" int __declspec(dllexport) Unload(void)
{
	FreeGdiPlus();

    return ShutdownProc(0, 0);
    //return 0;
}

