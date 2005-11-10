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

PROTOCOLDESCRIPTOR **g_protocols = NULL;
int g_protocount = 0;
char g_installed_protos[1030];
static int AV_QUEUESIZE = 0;
char *g_MetaName = NULL;

HANDLE hProtoAckHook = 0, hContactSettingChanged = 0, hEventChanged = 0, hMyAvatarChanged = 0;
HICON g_hIcon = 0;

static struct avatarCacheEntry *g_avatarCache = 0;
struct protoPicCacheEntry *g_ProtoPictures = 0;
struct protoPicCacheEntry *g_MyAvatars = 0;
static int g_curAvatar = 0;
CRITICAL_SECTION cachecs;
static CRITICAL_SECTION avcs;

BOOL g_imgDecoderAvail = 0;

HANDLE hAvatarThread = 0;
DWORD dwAvatarThreadID = 0;
BOOL bAvatarThreadRunning = TRUE;
BOOL g_MetaAvail = FALSE;
char *g_szMetaName = NULL;

HANDLE *avatarUpdateQueue = NULL;
DWORD dwPendingAvatarJobs = 0;

int ChangeAvatar(HANDLE hContact);

pfnImgNewDecoder ImgNewDecoder = 0;
pfnImgDeleteDecoder ImgDeleteDecoder = 0;
pfnImgNewDIBFromFile ImgNewDIBFromFile = 0;
pfnImgDeleteDIBSection ImgDeleteDIBSection = 0;
pfnImgGetHandle ImgGetHandle = 0;

PLUGININFO pluginInfo = {
    sizeof(PLUGININFO), "Avatar service", PLUGIN_MAKE_VERSION(0, 0, 1, 12), "Load and manage contact pictures for other plugins", "Nightwish", "", "Copyright 2000-2005 Miranda-IM project", "http://www.miranda-im.org", 0, 0
};

extern BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcAvatarOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 * output a notification message.
 * may accept a hContact to include the contacts nickname in the notification message...
 * the actual message is using printf() rules for formatting and passing the arguments...
 *
 * can display the message either as systray notification (baloon popup) or using the
 * popup plugin.
 */


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
    for(int i = 0; i < g_curAvatar; i++) {
        if(g_avatarCache[i].hContact == hContact) {
            DWORD dwFlags = g_avatarCache[i].dwFlags;

            g_avatarCache[i].dwFlags = mode ? g_avatarCache[i].dwFlags | attrib : g_avatarCache[i].dwFlags & ~attrib;
            if(g_avatarCache[i].dwFlags != dwFlags)
                NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)&g_avatarCache[i]);
            break;
        }
    }
    return 0;
}

// Correct alpha from bitmaps loaded without it (it cames with 0 and should be 255)
void CorrectBitmap32Alpha(HBITMAP hBitmap, BYTE *p, int width, int height)
{
	int x, y;

	for (y = 0; y < height; ++y) {
        BYTE *px = p + width * 4 * y;

        for (x = 0; x < width; ++x) 
		{
			px[3] = 255;
            px += 4;
        }
    }

	SetBitmapBits(hBitmap, width * height * 4, p);
}


HBITMAP CopyBitmapTo32(HBITMAP hBitmap)
{
	BITMAPINFO RGB32BitsBITMAPINFO; 
	BYTE * ptPixels;
	HBITMAP hDirectBitmap;

	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	dwLen = bmp.bmWidth * bmp.bmHeight * 4;
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return NULL;

	// Create bitmap
	ZeroMemory(&RGB32BitsBITMAPINFO, sizeof(BITMAPINFO));
	RGB32BitsBITMAPINFO.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RGB32BitsBITMAPINFO.bmiHeader.biWidth = bmp.bmWidth;
	RGB32BitsBITMAPINFO.bmiHeader.biHeight = bmp.bmHeight;
	RGB32BitsBITMAPINFO.bmiHeader.biPlanes = 1;
	RGB32BitsBITMAPINFO.bmiHeader.biBitCount = 32;

	hDirectBitmap = CreateDIBSection(NULL, 
									(BITMAPINFO *)&RGB32BitsBITMAPINFO, 
									DIB_RGB_COLORS,
									(void **)&ptPixels, 
									NULL, 0);

	// Copy data
	if (bmp.bmBitsPixel != 32)
	{
		HDC hdcOrig, hdcDest;
		HBITMAP oldOrig, oldDest;
		

		hdcOrig = CreateCompatibleDC(NULL);
		oldOrig = (HBITMAP) SelectObject(hdcOrig, hBitmap);

		hdcDest = CreateCompatibleDC(NULL);
		oldDest = (HBITMAP) SelectObject(hdcDest, hDirectBitmap);

		StretchBlt(hdcDest, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcOrig, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

		SelectObject(hdcDest, oldDest);
		DeleteObject(hdcDest);
		SelectObject(hdcOrig, oldOrig);
		DeleteObject(hdcOrig);

		// Set alpha
		GetBitmapBits(hDirectBitmap, dwLen, p);
		CorrectBitmap32Alpha(hDirectBitmap, p, bmp.bmWidth, bmp.bmHeight);
	}
	else
	{
		GetBitmapBits(hBitmap, dwLen, p);
		SetBitmapBits(hDirectBitmap, dwLen, p);
	}

	free(p);

	return hDirectBitmap;
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


BOOL GetColorForPoint(int colorDiff, BYTE *p, int width, int height, BOOL hasTransparency, 
					  int x0, int y0, int x1, int y1, int x2, int y2, BOOL *foundBkg, BYTE colors[][3])
{
#define GET_PIXEL(__X__, __Y__) ( p + width * 4 * (__Y__) + 4 * (__X__) )

	BYTE *px1, *px2, *px3; 

	px1 = GET_PIXEL(x0,y0);
	px2 = GET_PIXEL(x1,y1);
	px3 = GET_PIXEL(x2,y2);

	// If any of the corners have transparency, forget about it
	if (hasTransparency && (px1[3] != 255 || px2[3] != 255 || px3[3] != 255))
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


/*
 * See if finds a transparent background in image, and set its transparency
 * Return TRUE if found a transparent background
 */
static BOOL MakeTransparentBkg(HANDLE hContact, HBITMAP *hBitmap, BOOL hasAplpha)
{
	BYTE *p = NULL;
	DWORD dwLen;
    int width, height, i, j;
    BITMAP bmp;
	BYTE colors[8][3];
	BOOL foundBkg[8];
	BYTE *px1; 
	int count, maxCount, selectedColor;
	BOOL hasTransparency;
	HBITMAP hBmpTmp;
	int colorDiff;

	GetObject(*hBitmap, sizeof(bmp), &bmp);
    width = bmp.bmWidth;
	height = bmp.bmHeight;
	hasTransparency = (bmp.bmBitsPixel == 32) && hasAplpha;
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
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
						  0, 0, 0, 1, 1, 0, &foundBkg[0], &colors[0]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top center
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
						  width/2, 0, width/2-1, 0, width/2+1, 0, &foundBkg[1], &colors[1]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top Right
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
						  width-1, 0, width-1, 1, width-2, 0, &foundBkg[2], &colors[2]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
						  0, height/2, 0, height/2-1, 0, height/2+1, &foundBkg[3], &colors[3]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
						  width-1, height/2, width-1, height/2-1, width-1, height/2+1, &foundBkg[4], &colors[4]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom left
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
						  0, height-1, 0, height-2, 1, height-1, &foundBkg[5], &colors[5]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom center
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
						  width/2, height-1, width/2-1, height-1, width/2+1, height-1, &foundBkg[6], &colors[6]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom Right
	if (!GetColorForPoint(colorDiff, p, width, height, hasTransparency, 
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

	// **** Set alpha for the beckground color, from the borders

	if (hBmpTmp != *hBitmap)
	{
		DeleteObject(*hBitmap);

		*hBitmap = hBmpTmp;

		GetObject(*hBitmap, sizeof(bmp), &bmp);
		GetBitmapBits(*hBitmap, dwLen, p);
	}
	else if (!hasAplpha)
	{
		CorrectBitmap32Alpha(*hBitmap, p, width, height);
	}

	{
		// Set alpha from borders
		int x, y;
		int topPos = 0;
		int curPos = 0;
		int *stack = (int *)malloc(width * height * 2 * sizeof(int));

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
			px1 = GET_PIXEL(x, y);

			// It won't change the transparency if one exists
			// (This avoid an endless loop too)
			if (px1[3] == 255)
			{
				if (ColorsAreTheSame(colorDiff, px1, (BYTE *) &colors[selectedColor]))
				{
					px1[3] = (abs(px1[0] - colors[selectedColor][0]) 
							 + abs(px1[1] - colors[selectedColor][1]) 
							 + abs(px1[2] - colors[selectedColor][2])) / 3;

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

static BOOL PreMultiply(HBITMAP hBitmap, int mode)
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
    if(p) {
        GetBitmapBits(hBitmap, dwLen, p);

        for (y = 0; y < height; ++y) {
            BYTE *px = p + width * 4 * y;

            for (x = 0; x < width; ++x) {
                if(mode) {
                    alpha = px[3];

					if (alpha < 255) {
						transp  = TRUE;

						px[0] = px[0] * alpha/255;
						px[1] = px[1] * alpha/255;
						px[2] = px[2] * alpha/255;
					}
                }
                else
                    px[3] = 255;
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

static DWORD WINAPI AvatarUpdateThread(LPVOID vParam)
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
            return 0;
        /*
         * the thread is awake, processing the update quque one by one entry with a given delay of 5 seconds. The delay
         * ensures that no protocol will kick us because of flood protection(s)
         */
        hFound = 0;
        
        EnterCriticalSection(&avcs);
        for (i = 0; i < AV_QUEUESIZE; i++) {
            if (avatarUpdateQueue[i] != 0) {
                int status = ID_STATUS_OFFLINE;
                szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)avatarUpdateQueue[i], 0);
                if(g_MetaName && !strcmp(szProto, g_MetaName)) {
                    HANDLE hSubContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)avatarUpdateQueue[i], 0);
                    if(hSubContact)
                        szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hSubContact, 0);
                }
                if(szProto) {
                    status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
                    if(status != ID_STATUS_OFFLINE && status != ID_STATUS_INVISIBLE) {
                        hFound = avatarUpdateQueue[i];
                        avatarUpdateQueue[i] = 0;
                        dwPendingAvatarJobs--;
                        break;
                    }
                    else if(status == ID_STATUS_OFFLINE) {
                        avatarUpdateQueue[i] = 0;
                        dwPendingAvatarJobs--;
                    }
                }
            }
        }
        LeaveCriticalSection(&avcs);

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
        Sleep(5000L);
    } while ( bAvatarThreadRunning );
    return 0;
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

HBITMAP LoadPNG(struct avatarCacheEntry *ace, char *szFilename)
{
    LPVOID imgDecoder = NULL;
    LPVOID pImg = NULL;
    HBITMAP hBitmap = 0;
    LPVOID pBitmapBits = NULL;
    LPVOID m_pImgDecoder = NULL;

    if(!g_imgDecoderAvail)
        return 0;
    
    ImgNewDecoder(&m_pImgDecoder);
    if (!ImgNewDIBFromFile(m_pImgDecoder, szFilename, &pImg))
        ImgGetHandle(pImg, &hBitmap, (LPVOID *)&pBitmapBits);
    ImgDeleteDecoder(m_pImgDecoder);
    if(hBitmap == 0)
        return 0;
    ace->hbmPic = hBitmap;
    ace->lpDIBSection = pImg;
    return hBitmap;
}

// create the avatar in cache
// returns 0 if not created (no avatar), iIndex otherwise

int CreateAvatarInCache(HANDLE hContact, struct avatarCacheEntry *ace, int iIndex, char *szProto)
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
		if(hContact == 0) {				// protocol picture
			if(!DBGetContactSetting(NULL, PPICT_MODULE, szProto, &dbv)) {
				CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFilename);
				DBFreeVariant(&dbv);
			}
		}
		else if(hContact == (HANDLE)-1) {	// own avatar
			if(!DBGetContactSetting(NULL, szProto, "AvatarFile", &dbv)) {
				CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFilename);
				DBFreeVariant(&dbv);
			}
			else {
				char szTestFile[MAX_PATH];
				HANDLE hFFD;
				WIN32_FIND_DATAA ffd = {0};

				_snprintf(szTestFile, MAX_PATH, "MSN\\%s avatar.*", szProto);
				szTestFile[MAX_PATH - 1] = 0;
				CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)szTestFile, (LPARAM)szFilename);
				if((hFFD = FindFirstFileA(szFilename, &ffd)) != INVALID_HANDLE_VALUE) {
					_snprintf(szTestFile, MAX_PATH, "MSN\\%s", ffd.cFileName);
					CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)szTestFile, (LPARAM)szFilename);
					FindClose(hFFD);
					goto done;
				}
				_snprintf(szTestFile, MAX_PATH, "jabber\\%s avatar.*", szProto);
				szTestFile[MAX_PATH - 1] = 0;
				CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)szTestFile, (LPARAM)szFilename);
				if((hFFD = FindFirstFileA(szFilename, &ffd)) != INVALID_HANDLE_VALUE) {
					_snprintf(szTestFile, MAX_PATH, "jabber\\%s", ffd.cFileName);
					CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)szTestFile, (LPARAM)szFilename);
					FindClose(hFFD);
					goto done;
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
            return 1;

        _snprintf(szBuf, 256, "The selected contact picture is too large. The current file size limit for avatars are %d bytes.\nYou can increase that limit on the options page", sizeLimit * 1024);
        _snprintf(szTitle, 64, "Avatar Service warning (%s)", (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
        msn.szInfoTitle = szTitle;
        msn.szInfo = szBuf;
        msn.cbSize = sizeof(msn);
        msn.uTimeout = 10000;
        CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM)&msn);
        return -1;
    }
    szExt = &szFilename[lstrlenA(szFilename) - 4];
    if(!(!_stricmp(szExt, ".jpg") || (!_stricmp(szExt, ".gif") && !g_imgDecoderAvail) || !_stricmp(szExt, ".bmp") || !_stricmp(szExt, ".dat"))) {
		// Gif too can have transparency... image decoder today cant load it, but a gif with tranpsarency load by imagedecoder seens better than
		// one loaded from MS_UTILS_LOADBITMAP
        if(!_stricmp(szExt, ".png") || !_stricmp(szExt, ".gif")) { 
            struct avatarCacheEntry ace_private = {0};
            
            ace->hbmPic = LoadPNG(&ace_private, szFilename);
            if(ace->hbmPic == 0)
                return -1;
            ace->lpDIBSection = ace_private.lpDIBSection;
        }
        else
            return -1;
    }
    else {
        ace->hbmPic = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM)szFilename);
        ace->lpDIBSection = NULL;
    }
    if(ace->hbmPic != 0) {
        BITMAP bminfo;
		BOOL transpBkg;

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

		transpBkg = FALSE;
		if (DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0) && 
			DBGetContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", 1))
		{
			if (MakeTransparentBkg(hContact, &ace->hbmPic, ace->lpDIBSection != 0))
			{
				ace->dwFlags |= AVS_CUSTOMTRANSPBKG | AVS_HASTRANSPARENCY;
				transpBkg = TRUE;
				GetObject(ace->hbmPic, sizeof(bminfo), &bminfo);
			}
		}

        if(transpBkg || ace->lpDIBSection != 0) {
            int mode = bminfo.bmBitsPixel == 32 ? 1 : 0;
            if (PreMultiply(ace->hbmPic, mode))
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
        return iIndex;
    }
    else if(hContact != 0 && hContact != (HANDLE)-1){                     // don't update for pseudo avatars...
        UpdateAvatar(hContact);
        return -1;
    }
    return -1;
}

static int FindAvatarInCache(HANDLE hContact)
{
    int i, j;
    
    for(i = 0; i < g_curAvatar; i++) {
        if(g_avatarCache[i].hContact == hContact)
            return i;
    }
    // not found, create it in cache

    for(i = 0; i < g_curAvatar; i++) {
        if(g_avatarCache[i].hbmPic == 0 && g_avatarCache[i].hContact == 0)
            return CreateAvatarInCache(hContact, &g_avatarCache[i], i, NULL);
    }
    if(i == g_curAvatar) {
        g_curAvatar += CACHE_GROWSTEP;
        g_avatarCache = (struct avatarCacheEntry *)realloc(g_avatarCache, sizeof(struct avatarCacheEntry) * g_curAvatar);
        ZeroMemory((void *)&g_avatarCache[g_curAvatar - (CACHE_GROWSTEP + 1)], sizeof(struct avatarCacheEntry) * CACHE_GROWSTEP);
        // realloced the cache, pointers may be invalid, so invalidate them
        for(j = 0; j < g_curAvatar; j++) {
            if(g_avatarCache[j].hContact != 0)
                NotifyEventHooks(hEventChanged, (WPARAM)g_avatarCache[j].hContact, (LPARAM)&g_avatarCache[j]);
        }
    }
    if(CreateAvatarInCache(hContact, &g_avatarCache[i], i, NULL) != -1)
        return i;
    else
        return -1;
}


static int ProtocolAck(WPARAM wParam, LPARAM lParam)
{
    ACKDATA *ack = (ACKDATA *) lParam;

    if (ack->type == ACKTYPE_AVATAR && ack->hContact != 0) {
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

static BOOL CALLBACK OpenFileSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    OPENFILENAMEA *ofn = NULL;

    ofn = (OPENFILENAMEA *)GetWindowLong(hwnd, GWL_USERDATA);
    
    switch(msg) {
        case WM_INITDIALOG: {
            BYTE *locking_request;
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);
            ofn = (OPENFILENAMEA *)lParam;
            locking_request = (BYTE *)ofn->lCustData;
            CheckDlgButton(hwnd, IDC_PROTECTAVATAR, *locking_request);
            break;
        }
        case WM_COMMAND:
            if(LOWORD(wParam) == IDC_PROTECTAVATAR) {
                BYTE *locking_request = (BYTE *)ofn->lCustData;
                *locking_request = IsDlgButtonChecked(hwnd, IDC_PROTECTAVATAR) ? TRUE : FALSE;
            }
            break;
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

        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
        ofn.hwndOwner = 0;
        ofn.lpstrFile = FileName;
        if(g_imgDecoderAvail)
            ofn.lpstrFilter = "Image files\0*.gif;*.jpg;*.bmp;*.png\0\0";
        else
            ofn.lpstrFilter = "Image files\0*.gif;*.jpg;*.bmp\0\0";
        ofn.nMaxFile = MAX_PATH;
        ofn.nMaxFileTitle = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLETEMPLATE | OFN_EXPLORER | OFN_ENABLEHOOK;
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

	if(wParam)
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
    int iIndex, i;
    
    EnterCriticalSection(&cachecs);
    iIndex = FindAvatarInCache((HANDLE)wParam);
    LeaveCriticalSection(&cachecs);
    if(iIndex == -1 || iIndex >= g_curAvatar) {
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
        g_avatarCache[iIndex].t_lastAccess = time(NULL);
        return (int)&g_avatarCache[iIndex];
    }
}

int DeleteAvatar(HANDLE hContact)
{
    int i;
    char *szProto = NULL;
    
    EnterCriticalSection(&cachecs);
    for(i = 0; i < g_curAvatar; i++) {
        if(g_avatarCache[i].hContact == hContact) {
            if(g_avatarCache[i].lpDIBSection != NULL && ImgDeleteDIBSection)
                ImgDeleteDIBSection(g_avatarCache[i].lpDIBSection);
            if(g_avatarCache[i].hbmPic != 0)
                DeleteObject(g_avatarCache[i].hbmPic);
            ZeroMemory((void *)&g_avatarCache[i], sizeof(struct avatarCacheEntry));
            break;
        }
    }
    LeaveCriticalSection(&cachecs);
    // fallback with a protocol picture, if available...
    szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
    if(szProto) {
        for(i = 0; i < g_protocount; i++) {
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
    int i;

    EnterCriticalSection(&cachecs);
    for(i = 0; i < g_curAvatar; i++) {
        if(g_avatarCache[i].hContact == hContact) {
            if(g_avatarCache[i].lpDIBSection && ImgDeleteDIBSection)
                ImgDeleteDIBSection(g_avatarCache[i].lpDIBSection);
            if(g_avatarCache[i].hbmPic != 0)
                DeleteObject(g_avatarCache[i].hbmPic);
            ZeroMemory((void *)&g_avatarCache[i], sizeof(struct avatarCacheEntry));
            break;
        }
    }
    i = FindAvatarInCache(hContact);
    LeaveCriticalSection(&cachecs);
    if(i != -1 && i < g_curAvatar) {
        g_avatarCache[i].cbSize = sizeof(struct avatarCacheEntry);
        NotifyEventHooks(hEventChanged, (WPARAM)hContact, (LPARAM)&g_avatarCache[i]);
    }
    else {              // fallback with protocol picture
        int i;
        char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
        if(szProto) {
            for(i = 0; i < g_protocount; i++) {
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
        if(CreateAvatarInCache(0, (struct avatarCacheEntry *)&g_ProtoPictures[j], j, g_protocols[i]->szName) != -1)
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
        if(CreateAvatarInCache((HANDLE)-1, (struct avatarCacheEntry *)&g_MyAvatars[j], j, g_protocols[i]->szName) != -1)
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

static DWORD WINAPI ReloadMyAvatar(LPVOID lpParam)
{
	int i;
	char *szProto = (char *)lpParam;

	Sleep(5000);
	for(i = 0; i < g_protocount; i++) {
		if(!strcmp(g_MyAvatars[i].szProtoname, szProto) && lstrlenA(szProto) == lstrlenA(g_MyAvatars[i].szProtoname)) {
			if(g_MyAvatars[i].lpDIBSection && ImgDeleteDIBSection)
				ImgDeleteDIBSection(g_MyAvatars[i].lpDIBSection);
			if(g_MyAvatars[i].hbmPic)
				DeleteObject(g_MyAvatars[i].hbmPic);

			if(CreateAvatarInCache((HANDLE)-1, (struct avatarCacheEntry *)&g_MyAvatars[i], i, szProto) != -1)
				NotifyEventHooks(hMyAvatarChanged, (WPARAM)szProto, (LPARAM)&g_MyAvatars[i]);
			else
				NotifyEventHooks(hMyAvatarChanged, (WPARAM)szProto, 0);
		}
	}
	free(lpParam);
	return 0;
}

static int ContactSettingChanged(WPARAM wParam, LPARAM lParam)
{
    char *szProto;
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  

    if(cws == NULL)
        return 0;

	if(wParam == 0) {
		if(!strcmp(cws->szSetting, "AvatarFile") || !strcmp(cws->szSetting, "PictObject") || !strcmp(cws->szSetting, "AvatarHash")) {
			if(cws->szModule) {
				int i;

				for(i = 0; i < g_protocount; i++) {
					if(!strcmp(g_MyAvatars[i].szProtoname, cws->szModule) && lstrlenA(cws->szModule) == lstrlenA(g_MyAvatars[i].szProtoname)) {
						DWORD dwThreadid;
						LPVOID lpParam = 0;

						lpParam = (void *)malloc(lstrlenA(g_MyAvatars[i].szProtoname) + 2);
						strcpy((char *)lpParam, g_MyAvatars[i].szProtoname);
						CloseHandle(CreateThread(NULL, 16000, ReloadMyAvatar, lpParam, 0, &dwThreadid));
						return 0;
					}
				}
			}
		}
		return 0;
	}
    if(!strcmp(cws->szModule, "ContactPhoto")) {
        DBVARIANT dbv = {0};
        
        if(!strcmp(cws->szSetting, "Hidden") || !strcmp(cws->szSetting, "Locked"))
            return 0;
        
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
    if(szProto) {
        if(!strcmp(cws->szModule, szProto)) {
            if(!strcmp(cws->szSetting, "PictContext"))
                UpdateAvatar((HANDLE)wParam);
            if(!strcmp(cws->szSetting, "AvatarHash")) {
                UpdateAvatar((HANDLE)wParam);
            }
        }
    }
    return 0;
}

static int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
    int i;

    EnterCriticalSection(&cachecs);
    for(i = 0; i < g_curAvatar; i++) {
        if(g_avatarCache[i].hContact == (HANDLE)wParam) {
            if(g_avatarCache[i].lpDIBSection && ImgDeleteDIBSection)
                ImgDeleteDIBSection(g_avatarCache[i].lpDIBSection);
            if(g_avatarCache[i].hbmPic != 0)
                DeleteObject(g_avatarCache[i].hbmPic);
            ZeroMemory((void *)&g_avatarCache[i], sizeof(struct avatarCacheEntry));
            break;
        }
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

    DestroyServiceFunction(MS_AV_GETAVATARBITMAP);
    DestroyServiceFunction(MS_AV_PROTECTAVATAR);
    DestroyServiceFunction(MS_AV_SETAVATAR);
    DestroyServiceFunction(MS_AV_CONTACTOPTIONS);
    DestroyServiceFunction(MS_AV_DRAWAVATAR);
	DestroyServiceFunction(MS_AV_GETMYAVATAR);

    DestroyHookableEvent(hEventChanged);
	DestroyHookableEvent(hMyAvatarChanged);
    hEventChanged = 0;
	hMyAvatarChanged = 0;
    UnhookEvent(hContactSettingChanged);
    UnhookEvent(hProtoAckHook);
    UnhookEvent(hEventChanged);
    
    for(int i = 0; i < g_curAvatar; i++) {
        if(g_avatarCache[i].lpDIBSection && ImgDeleteDIBSection)
            ImgDeleteDIBSection(g_avatarCache[i].lpDIBSection);
        if(g_avatarCache[i].hbmPic != 0)
            DeleteObject(g_avatarCache[i].hbmPic);
    }
    for(int i = 0; i < g_protocount; i++) {
        if(g_ProtoPictures[i].lpDIBSection && ImgDeleteDIBSection)
            ImgDeleteDIBSection(g_ProtoPictures[i].lpDIBSection);
        if(g_ProtoPictures[i].hbmPic != 0)
            DeleteObject(g_ProtoPictures[i].hbmPic);

		if(g_MyAvatars[i].lpDIBSection && ImgDeleteDIBSection)
			ImgDeleteDIBSection(g_MyAvatars[i].lpDIBSection);
		if(g_MyAvatars[i].hbmPic != 0)
			DeleteObject(g_MyAvatars[i].hbmPic);
    }
    if(g_avatarCache)
        free(g_avatarCache);

    if(g_ProtoPictures)
        free(g_ProtoPictures);

	if(g_MyAvatars)
		free(g_MyAvatars);

    bAvatarThreadRunning = FALSE;
    ResumeThread(hAvatarThread);
    do {
        Sleep(100);
        GetExitCodeThread(hAvatarThread, &dwExitcode);
    } while (dwExitcode == STILL_ACTIVE);
    if(hAvatarThread)
        CloseHandle(hAvatarThread);

    DeleteCriticalSection(&avcs);
    DeleteCriticalSection(&cachecs);

    if(avatarUpdateQueue)
        free(avatarUpdateQueue);
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
            newWidth = (float)(targetHeight);
            dScale = (float)(targetHeight) / (float)bmWidth;
            newHeight = (float)bmHeight * dScale;
        }
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
    HMODULE hModule;

	HookEvent(ME_OPT_INITIALISE, OptInit);
    HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
    hContactSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ContactSettingChanged);
    HookEvent(ME_DB_CONTACT_DELETED, ContactDeleted);
    hProtoAckHook = (HANDLE) HookEvent(ME_PROTO_ACK, ProtocolAck);
    CreateServiceFunction(MS_AV_GETAVATARBITMAP, GetAvatarBitmap);
    CreateServiceFunction(MS_AV_PROTECTAVATAR, ProtectAvatar);
    CreateServiceFunction(MS_AV_SETAVATAR, SetAvatar);
    CreateServiceFunction(MS_AV_CONTACTOPTIONS, ContactOptions);
    CreateServiceFunction(MS_AV_DRAWAVATAR, DrawAvatarPicture);
	CreateServiceFunction(MS_AV_GETMYAVATAR, GetMyAvatar);
    hEventChanged = CreateHookableEvent(ME_AV_AVATARCHANGED);
	hMyAvatarChanged = CreateHookableEvent(ME_AV_MYAVATARCHANGED);
    g_avatarCache = (struct avatarCacheEntry *)malloc(sizeof(struct avatarCacheEntry) * INITIAL_AVATARCACHESIZE);
    ZeroMemory((void *)g_avatarCache, sizeof(struct avatarCacheEntry) * INITIAL_AVATARCACHESIZE);
    g_curAvatar = INITIAL_AVATARCACHESIZE;

    AV_QUEUESIZE = CallService(MS_DB_CONTACT_GETCOUNT, 0, 0);
    if(AV_QUEUESIZE < 300)
        AV_QUEUESIZE = 300;
    avatarUpdateQueue = (HANDLE *)malloc(sizeof(HANDLE) * (AV_QUEUESIZE + 10));
    ZeroMemory((void *)avatarUpdateQueue, sizeof(HANDLE) * (AV_QUEUESIZE + 10));

    InitializeCriticalSection(&avcs);
    InitializeCriticalSection(&cachecs);

    /*
     * initialize imgdecoder (if available)
     */
    
    if((hModule = LoadLibraryA("imgdecoder.dll")) == 0) {
        if((hModule = LoadLibraryA("plugins\\imgdecoder.dll")) != 0)
            g_imgDecoderAvail = TRUE;
    }
    else
        g_imgDecoderAvail = TRUE;

    if(hModule) {
        ImgNewDecoder = (pfnImgNewDecoder )GetProcAddress(hModule, "ImgNewDecoder");
        ImgDeleteDecoder=(pfnImgDeleteDecoder )GetProcAddress(hModule, "ImgDeleteDecoder");
        ImgNewDIBFromFile=(pfnImgNewDIBFromFile)GetProcAddress(hModule, "ImgNewDIBFromFile");
        ImgDeleteDIBSection=(pfnImgDeleteDIBSection)GetProcAddress(hModule, "ImgDeleteDIBSection");
        ImgGetHandle=(pfnImgGetHandle)GetProcAddress(hModule, "ImgGetHandle");
    }
    hAvatarThread = CreateThread(NULL, 16000, AvatarUpdateThread, NULL, 0, &dwAvatarThreadID);

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
    
    return(LoadAvatarModule());
}

extern "C" int __declspec(dllexport) Unload(void)
{
    return ShutdownProc(0, 0);
    //return 0;
}

