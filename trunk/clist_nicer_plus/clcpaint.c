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

#define _GDITEXTRENDERING

#include "commonheaders.h"

extern struct avatarCache *g_avatarCache;
extern int g_curAvatar;

extern struct ExtraCache *g_ExtraCache;
extern int g_nextExtraCacheEntry, g_maxExtraCacheEntry;

extern int hClcProtoCount;
extern ClcProtoStatus *clcProto;
extern HIMAGELIST himlCListClc, hCListImages;
extern struct CluiData g_CluiData;
static BYTE divide3[765] = {255};
extern char *im_clients[];
extern HICON im_clienthIcons[];
extern HICON overlayicons[];
extern HWND hwndContactTree, hwndContactList;
#if defined(_UNICODE)
    extern TCHAR statusNames[12][124];
#else
    extern TCHAR *statusNames[];
#endif

extern LONG g_cxsmIcon, g_cysmIcon;
extern DWORD g_gdiplusToken;
extern StatusItems_t *StatusItems;
//pfnDrawAvatar DrawAvatar = NULL;
pfnDrawAlpha pDrawAlpha = NULL;

void DrawWithGDIp(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, UCHAR alpha, struct ClcContact *contact);
void RemoveFromImgCache(HANDLE hContact, struct avatarCacheEntry *ace);
int DrawTextHQ(HDC hdc, HFONT hFont, RECT *rc, WCHAR *szwText, COLORREF colorref, int fontHeight, int g_RTL);
void CreateG(HDC hdc), DeleteG();
int MeasureTextHQ(HDC hdc, HFONT hFont, RECT *rc, WCHAR *szwText);

int g_hottrack, g_center, g_ignoreselforgroups, g_selectiveIcon, g_exIconSpacing, g_gdiPlus, g_gdiPlusText;
HWND g_focusWnd;
BYTE selBlend;
BYTE saved_alpha;
int my_status;

HFONT __fastcall ChangeToFont(HDC hdc, struct ClcData *dat, int id, int *fontHeight)
{
    HFONT hOldFont = 0;
    
    hOldFont = SelectObject(hdc, dat->fontInfo[id].hFont);
    SetTextColor(hdc, dat->fontInfo[id].colour);
    if (fontHeight)
        *fontHeight = dat->fontInfo[id].fontHeight;

    dat->currentFontID = id;
    return hOldFont;
}

static void __inline SetHotTrackColour(HDC hdc, struct ClcData *dat)
{
    if (dat->gammaCorrection) {
        COLORREF oldCol, newCol;
        int oldLum, newLum;

        oldCol = GetTextColor(hdc);
        oldLum = (GetRValue(oldCol) * 30 + GetGValue(oldCol) * 59 + GetBValue(oldCol) * 11) / 100;
        newLum = (GetRValue(dat->hotTextColour) * 30 + GetGValue(dat->hotTextColour) * 59 + GetBValue(dat->hotTextColour) * 11) / 100;
        if (newLum == 0) {
            SetTextColor(hdc, dat->hotTextColour);
            return;
        }
        if (newLum >= oldLum + 20) {
            oldLum += 20;
            newCol = RGB(GetRValue(dat->hotTextColour) * oldLum / newLum, GetGValue(dat->hotTextColour) * oldLum / newLum, GetBValue(dat->hotTextColour) * oldLum / newLum);
        } else if (newLum <= oldLum) {
            int r, g, b;
            r = GetRValue(dat->hotTextColour) * oldLum / newLum;
            g = GetGValue(dat->hotTextColour) * oldLum / newLum;
            b = GetBValue(dat->hotTextColour) * oldLum / newLum;
            if (r > 255) {
                g +=(r-255)*3 / 7;
                b +=(r-255)*3 / 7;
                r = 255;
            }
            if (g > 255) {
                r +=(g-255)*59 / 41;
                if (r > 255)
                    r = 255;
                b +=(g-255)*59 / 41;
                g = 255;
            }
            if (b > 255) {
                r +=(b-255)*11 / 89;
                if (r > 255)
                    r = 255;
                g +=(b-255)*11 / 89;
                if (g > 255)
                    g = 255;
                b = 255;
            }
            newCol = RGB(r, g, b);
        } else
            newCol = dat->hotTextColour;
        SetTextColor(hdc, newCol);
    } else
        SetTextColor(hdc, dat->hotTextColour);
}

int __fastcall GetStatusOnlineness(int status)
{
    if(status >= ID_STATUS_CONNECTING && status < ID_STATUS_OFFLINE)
        return 120;
    
    switch (status) {
        case ID_STATUS_FREECHAT:
            return 110;
        case ID_STATUS_ONLINE:
            return 100;
        case ID_STATUS_OCCUPIED:
            return 60;
        case ID_STATUS_ONTHEPHONE:
            return 50;
        case ID_STATUS_DND:
            return 40;
        case ID_STATUS_AWAY:
            return 30;
        case ID_STATUS_OUTTOLUNCH:
            return 20;
        case ID_STATUS_NA:
            return 10;
        case ID_STATUS_INVISIBLE:
            return 5;
    }
    return 0;
}

static int __fastcall GetGeneralisedStatus(void)
{
    int i, status, thisStatus, statusOnlineness, thisOnlineness;

    status = ID_STATUS_OFFLINE;
    statusOnlineness = 0;

    for (i = 0; i < hClcProtoCount; i++) {
        thisStatus = clcProto[i].dwStatus;
        if (thisStatus == ID_STATUS_INVISIBLE)
            return ID_STATUS_INVISIBLE;
        thisOnlineness = GetStatusOnlineness(thisStatus);
        if (thisOnlineness > statusOnlineness) {
            status = thisStatus;
            statusOnlineness = thisOnlineness;
        }
    }
    return status;
}

static int __fastcall GetRealStatus(struct ClcContact *contact, int status)
{
    int i;
    char *szProto = contact->proto;
    if (!szProto)
        return status;
    for (i = 0; i < hClcProtoCount; i++) {
        if (!lstrcmpA(clcProto[i].szProto, szProto)) {
            return clcProto[i].dwStatus;
        }
    }
    return status;
}

int GetBasicFontID(struct ClcContact * contact)
{
	switch (contact->type)
	{
		case CLCIT_GROUP:
	        return FONTID_GROUPS;
			break;
		case CLCIT_INFO:
			if(contact->flags & CLCIIF_GROUPFONT)
				return FONTID_GROUPS;
			else 
				return FONTID_CONTACTS;
			break;
		case CLCIT_DIVIDER:
			return FONTID_DIVIDERS;
			break;
		case CLCIT_CONTACT:
			if (contact->flags & CONTACTF_NOTONLIST)
                return FONTID_NOTONLIST;
			else if ((contact->flags&CONTACTF_INVISTO && GetRealStatus(contact, ID_STATUS_OFFLINE) != ID_STATUS_INVISIBLE)
                 || (contact->flags&CONTACTF_VISTO && GetRealStatus(contact, ID_STATUS_OFFLINE) == ID_STATUS_INVISIBLE))
                    return contact->flags & CONTACTF_ONLINE ? FONTID_INVIS : FONTID_OFFINVIS;
			else
				return contact->flags & CONTACTF_ONLINE ? FONTID_CONTACTS : FONTID_OFFLINE;
			break;
		default:
			return FONTID_CONTACTS;
	}
}

static HMODULE themeAPIHandle = NULL; // handle to uxtheme.dll
HANDLE   (WINAPI *MyOpenThemeData)(HWND, LPCWSTR) = 0;
HRESULT  (WINAPI *MyCloseThemeData)(HANDLE) = 0;
HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE, HDC, int, int, const RECT *, const RECT *) = 0;

#define MGPROC(x) GetProcAddress(themeAPIHandle,x)

static void InitThemeAPI()
{
    themeAPIHandle = GetModuleHandleA("uxtheme");
    if (themeAPIHandle) {
        MyOpenThemeData = (HANDLE(WINAPI *)(HWND, LPCWSTR))MGPROC("OpenThemeData");
        MyCloseThemeData = (HRESULT(WINAPI *)(HANDLE))MGPROC("CloseThemeData");
        MyDrawThemeBackground = (HRESULT(WINAPI *)(HANDLE, HDC, int, int, const RECT *, const RECT *))MGPROC("DrawThemeBackground");
    }
}

void PaintNotifyArea(HDC hDC, RECT *rc)
{
    struct ClcData *dat = (struct ClcData *) GetWindowLong(hwndContactTree, 0);
    int iCount;
    static int ev_lastIcon = 0;

	rc->left += 26;             // button
    iCount = GetMenuItemCount(g_CluiData.hMenuNotify);
    if (g_CluiData.hUpdateContact != 0) {
        TCHAR *szName = GetContactDisplayNameW(g_CluiData.hUpdateContact, 0);
        int iIcon = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) g_CluiData.hUpdateContact, 0);

        ImageList_DrawEx(hCListImages, iIcon, hDC, rc->left, (rc->bottom + rc->top - g_cysmIcon) / 2, g_cxsmIcon, g_cysmIcon, CLR_NONE, CLR_NONE, ILD_NORMAL);
        rc->left += 18;
        DrawText(hDC, szName, -1, rc, DT_VCENTER | DT_SINGLELINE);
        ImageList_DrawEx(hCListImages, (int)g_CluiData.hIconNotify, hDC, 4, (rc->bottom + rc->top - 16) / 2, 16, 16, CLR_NONE, CLR_NONE, ILD_NORMAL);
        ev_lastIcon = g_CluiData.hIconNotify;
    } else if (iCount > 0) {
        MENUITEMINFO mii = {0};
        struct NotifyMenuItemExData *nmi;
        TCHAR *szName;
        int iIcon;

		mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA;
        GetMenuItemInfo(g_CluiData.hMenuNotify, iCount - 1, TRUE, &mii);
        nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
        szName = GetContactDisplayNameW(nmi->hContact, 0);
        iIcon = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) nmi->hContact, 0);
        ImageList_DrawEx(hCListImages, iIcon, hDC, rc->left, (rc->bottom + rc->top - g_cysmIcon) / 2, g_cxsmIcon, g_cysmIcon, CLR_NONE, CLR_NONE, ILD_NORMAL);
        rc->left += 18;
        ImageList_DrawEx(hCListImages, nmi->iIcon, hDC, 4, (rc->bottom + rc->top) / 2 - 8, 16, 16, CLR_NONE, CLR_NONE, ILD_NORMAL);
        DrawText(hDC, szName, -1, rc, DT_VCENTER | DT_SINGLELINE);
        ev_lastIcon = (int)nmi->hIcon;
    } else {
        HICON hIcon = LoadImage(g_hInst, MAKEINTRESOURCE(IDI_BLANK), IMAGE_ICON, 16, 16, 0);
        DrawTextA(hDC, g_CluiData.szNoEvents, lstrlenA(g_CluiData.szNoEvents), rc, DT_VCENTER | DT_SINGLELINE);
        DrawIconEx(hDC, 4, (rc->bottom + rc->top - 16) / 2, hIcon, 16, 16, 0, 0, DI_NORMAL | DI_COMPAT);
        DestroyIcon(hIcon);
    }
}

static BLENDFUNCTION bf = {0, 0, AC_SRC_OVER, 0};
static BOOL avatar_done = FALSE;
static HDC g_HDC;
static BOOL g_RTL;

static int __fastcall DrawAvatar(HDC hdcMem, RECT *rc, struct ClcContact *contact, int y, struct ClcData *dat, WORD cstatus, int rowHeight)
{
    float dScale = 0.;
    float newHeight, newWidth;
    HDC hdcAvatar;
    HBITMAP hbmMem;
    DWORD topoffset = 0;
    LONG bmWidth, bmHeight;
    float dAspect;
    HBITMAP hbm;
    HRGN rgn = 0;
    int avatar_size = g_CluiData.avatarSize;
    DWORD av_saved_left;
    BOOL gdiPlus;
    contact->avatarLeft = -1;
    
	if(!g_CluiData.bAvatarServiceAvail || dat->bisEmbedded)
        return 0;

    if (g_CluiData.bForceRefetchOnPaint)
        contact->ace = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)contact->hContact, 0);

	if(contact->ace != NULL && contact->ace->cbSize == sizeof(struct avatarCacheEntry)) {
        if(contact->ace->dwFlags & AVS_HIDEONCLIST) {
            if (g_CluiData.dwFlags & CLUI_FRAME_ALWAYSALIGNNICK)
                return avatar_size + 2;
            else
                return 0;
        }
        bmHeight = contact->ace->bmHeight;
        bmWidth = contact->ace->bmWidth;
        if(bmWidth != 0)
            dAspect = (float)bmHeight / (float)bmWidth;
        else
            dAspect = 1.0;
        hbm = contact->ace->hbmPic;
        contact->ace->t_lastAccess = time(NULL);
    }
    else if (g_CluiData.dwFlags & CLUI_FRAME_ALWAYSALIGNNICK)
        return avatar_size + 2;
    else
        return 0;

    if(bmHeight == 0 || bmWidth == 0 || hbm == 0)
        return 0;


    gdiPlus = g_gdiPlus && !(contact->ace->dwFlags & AVS_PREMULTIPLIED);
    //gdiPlus = FALSE;
    
    if(!gdiPlus) {
        hdcAvatar = CreateCompatibleDC(g_HDC);
        hbmMem = (HBITMAP)SelectObject(hdcAvatar, hbm);
    }

    if(dAspect >= 1.0) {            // height > width
        dScale = (float)(avatar_size - 2) / (float)bmHeight;
        newHeight = (float)(avatar_size - 2);
        newWidth = (float)bmWidth * dScale;
    }
    else {
        newWidth = (float)(avatar_size - 2);
        dScale = (float)(avatar_size - 2) / (float)bmWidth;
        newHeight = (float)bmHeight * dScale;
    }
    topoffset = rowHeight > (int)newHeight ? (rowHeight - (int)newHeight) / 2 : 0;
    
    // create the region for the avatar border - use the same region for clipping, if needed.

    av_saved_left = rc->left;
    if(g_CluiData.bCenterStatusIcons && newWidth < newHeight)
        rc->left += ((avatar_size - (int)newWidth) / 2);
    
    if(g_CluiData.dwFlags & CLUI_FRAME_ROUNDAVATAR) {
        rgn = CreateRoundRectRgn(rc->left, y + topoffset, rc->left + (int)newWidth + 1, y + topoffset + (int)newHeight + 1, 2 * g_CluiData.avatarRadius, 2 * g_CluiData.avatarRadius);
        SelectClipRgn(hdcMem, rgn);
    }
    else
        rgn = CreateRectRgn(rc->left, y + topoffset, rc->left + (int)newWidth, y + topoffset + (int)newHeight);
        
    if(!gdiPlus) {     // was gdiPlus
        bf.SourceConstantAlpha = (g_CluiData.dwFlags & CLUI_FRAME_TRANSPARENTAVATAR && (UCHAR)saved_alpha > 20) ? (UCHAR)saved_alpha : 255;
        bf.AlphaFormat = contact->ace->dwFlags & AVS_PREMULTIPLIED ? AC_SRC_ALPHA : 0;
        
        if(dat->showIdle && contact->flags & CONTACTF_IDLE)
            bf.SourceConstantAlpha -= (bf.SourceConstantAlpha > 100 ? 50 : 0);

        SetStretchBltMode(hdcMem, HALFTONE);
        if(bf.SourceConstantAlpha == 255 && bf.AlphaFormat == 0) {
            StretchBlt(hdcMem, rc->left - (g_RTL ? 1 : 0), y + topoffset, (int)newWidth, (int)newHeight, hdcAvatar, 0, 0, bmWidth, bmHeight, SRCCOPY);
        }
        else {
            /*
             * get around SUCKY AlphaBlend() rescaling quality...
             */
            HDC hdcTemp = CreateCompatibleDC(g_HDC);
            HBITMAP hbmTemp; // = CreateCompatibleBitmap(hdcAvatar, bmWidth, bmHeight);
            HBITMAP hbmTempOld;//

            hbmTemp = CreateCompatibleBitmap(hdcAvatar, bmWidth, bmHeight);
            hbmTempOld = SelectObject(hdcTemp, hbmTemp);

            //SetBkMode(hdcTemp, TRANSPARENT);
            StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, hdcMem, rc->left, y + topoffset, (int)newWidth, (int)newHeight, SRCCOPY);
            AlphaBlend(hdcTemp, 0, 0, bmWidth, bmHeight, hdcAvatar, 0, 0, bmWidth, bmHeight, bf);
            SetStretchBltMode(hdcTemp, HALFTONE);
            StretchBlt(hdcMem, rc->left - (g_RTL ? 1 : 0), y + topoffset, (int)newWidth, (int)newHeight, hdcTemp, 0, 0, bmWidth, bmHeight, SRCCOPY);
            SelectObject(hdcTemp, hbmTempOld);
            DeleteObject(hbmTemp);
            DeleteDC(hdcTemp);
        }

    }
    else {
        UCHAR alpha = g_CluiData.dwFlags & CLUI_FRAME_TRANSPARENTAVATAR ? (UCHAR)saved_alpha : 255;
        if(dat->showIdle && contact->flags & CONTACTF_IDLE)
            alpha -= (int)((float)alpha / 100.0f * 20.0f);
        DrawWithGDIp(hdcMem, rc->left, y + topoffset, (int)newWidth, (int)newHeight, alpha, contact);
    }
    
    if(g_CluiData.dwFlags & CLUI_FRAME_AVATARBORDER) {
        if(g_RTL)
            OffsetRgn(rgn, -1 , 0);
        FrameRgn(hdcMem, rgn, g_CluiData.hBrushAvatarBorder, 1, 1);
    }
    
    if(g_CluiData.dwFlags & CLUI_FRAME_OVERLAYICONS && cstatus && (int)newHeight >= g_cysmIcon)
        DrawIconEx(hdcMem, rc->left + (int)newWidth - 15, y + topoffset + (int)newHeight - 15, overlayicons[cstatus - ID_STATUS_OFFLINE], g_cxsmIcon, g_cysmIcon, 0, 0, DI_NORMAL | DI_COMPAT);
    
    SelectClipRgn(hdcMem, NULL);
    DeleteObject(rgn);

    if(!gdiPlus) {
        SelectObject(hdcAvatar, hbmMem);        // xxx changed...
        //DeleteObject(hbmMem);
        DeleteDC(hdcAvatar);
    }
    contact->avatarLeft = rc->left;
    avatar_done = TRUE;
    rc->left = av_saved_left;
    return avatar_size + 2;
}

static BOOL pi_avatar = FALSE;
static RECT rcContent;
static BOOL pi_selectiveIcon;

static BOOL av_left, av_right, av_rightwithnick;
static BOOL av_wanted, mirror_rtl, mirror_always, mirror_rtltext;

DWORD savedCORNER = -1;

static void __forceinline PaintItem(HDC hdcMem, struct ClcGroup *group, struct ClcContact *contact, int indent, int y, struct ClcData *dat, int index, HWND hwnd, DWORD style, RECT *clRect, BOOL *bFirstNGdrawn, int groupCountsFontTopShift, int rowHeight)
{
    RECT rc;
    int iImage = -1;
    int selected;
    SIZE textSize, countsSize, spaceSize;
    int width, checkboxWidth;
    char *szCounts;
    int fontHeight;
    BOOL twoRows = FALSE;
    WORD cstatus;
    DWORD leftOffset = 0, rightOffset = 0;
    int iconXSpace = dat->iconXSpace;
    //BOOL xStatusValid = 0;
    HFONT hPreviousFont = 0;
    BYTE type;
    BYTE flags;
    COLORREF oldGroupColor = -1;
    DWORD qLeft = 0;
    int leftX = dat->leftMargin + indent * dat->groupIndent;
    int bg_indent_r = 0;
    int bg_indent_l = 0;
    int rightIcons = 0;
    DWORD dt_nickflags = 0, dt_2ndrowflags = 0;
    struct ExtraCache *cEntry = NULL;
    DWORD dwFlags = g_CluiData.dwFlags;
    int scanIndex;
    
    rowHeight -= g_CluiData.bRowSpacing;
    savedCORNER = 0;
    
    if(group == NULL || contact == NULL)
        return;
    
    g_RTL = FALSE;
    scanIndex = group->scanIndex;
    
    type = contact->type;
    flags = contact->flags;
    selected = index == dat->selection && (dat->showSelAlways || dat->exStyle &CLS_EX_SHOWSELALWAYS || g_focusWnd == hwnd) && type != CLCIT_DIVIDER;
    avatar_done = FALSE;
    if(contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry)
        cEntry = &g_ExtraCache[contact->extraCacheEntry];
    else
        cEntry = g_ExtraCache;
    
    if(type == CLCIT_CONTACT && (cEntry->dwCFlags & ECF_RTLNICK || mirror_always)) {
        if(mirror_rtl || mirror_always) {
            g_RTL = TRUE;
            bg_indent_r = g_CluiData.bApplyIndentToBg ? indent * dat->groupIndent : 0;
        }
        else if(mirror_rtltext) {
            bg_indent_l = g_CluiData.bApplyIndentToBg ? indent * dat->groupIndent : 0;
            dt_nickflags = DT_RTLREADING | DT_RIGHT;
        }
		else
            bg_indent_l = g_CluiData.bApplyIndentToBg ? indent * dat->groupIndent : 0;
    }
    else if((type == CLCIT_GROUP && contact->isRtl) || mirror_always) {
        g_RTL = TRUE;
        bg_indent_r = g_CluiData.bApplyIndentToBg ? indent * dat->groupIndent : 0;
    }
    else
        bg_indent_l = g_CluiData.bApplyIndentToBg ? indent * dat->groupIndent : 0;
    
    g_hottrack = dat->exStyle & CLS_EX_TRACKSELECT && type == CLCIT_CONTACT && dat->iHotTrack == index;
    if (g_hottrack == selected)
        g_hottrack = 0;

    saved_alpha = 0;
    
    //setup
    if (type == CLCIT_GROUP)
        ChangeToFont(hdcMem, dat, FONTID_GROUPS, &fontHeight);
    else if (type == CLCIT_INFO) {
        if (flags & CLCIIF_GROUPFONT)
            ChangeToFont(hdcMem, dat, FONTID_GROUPS, &fontHeight);
        else
            ChangeToFont(hdcMem, dat, FONTID_CONTACTS, &fontHeight);
    } else if (type == CLCIT_DIVIDER)
        ChangeToFont(hdcMem, dat, FONTID_DIVIDERS, &fontHeight);
    else if (type == CLCIT_CONTACT && flags & CONTACTF_NOTONLIST)
        ChangeToFont(hdcMem, dat, FONTID_NOTONLIST, &fontHeight);
    else if (type == CLCIT_CONTACT && ((flags & CONTACTF_INVISTO && GetRealStatus(contact, my_status) != ID_STATUS_INVISIBLE) || (flags & CONTACTF_VISTO && GetRealStatus(contact, my_status) == ID_STATUS_INVISIBLE))) {
    // the contact is in the always visible list and the proto is invisible
    // the contact is in the always invisible and the proto is in any other mode
        ChangeToFont(hdcMem, dat, flags & CONTACTF_ONLINE ? FONTID_INVIS : FONTID_OFFINVIS, &fontHeight);
    } else if (type == CLCIT_CONTACT && !(flags & CONTACTF_ONLINE))
        ChangeToFont(hdcMem, dat, FONTID_OFFLINE, &fontHeight);
    else
        ChangeToFont(hdcMem, dat, FONTID_CONTACTS, &fontHeight);
    GetTextExtentPoint32(hdcMem, contact->szText, lstrlen(contact->szText), &textSize);
    width = textSize.cx;
    if (type == CLCIT_GROUP) {
        szCounts = GetGroupCountsText(dat, contact);
        if (szCounts[0]) {
            GetTextExtentPoint32(hdcMem, _T(" "), 1, &spaceSize);
            ChangeToFont(hdcMem, dat, FONTID_GROUPCOUNTS, &fontHeight);
            GetTextExtentPoint32A(hdcMem, szCounts, lstrlenA(szCounts), &countsSize);
            width += spaceSize.cx + countsSize.cx;
        }
    }
    if ((style & CLS_CHECKBOXES && type == CLCIT_CONTACT) || (style & CLS_GROUPCHECKBOXES && type == CLCIT_GROUP) || (type == CLCIT_INFO && flags & CLCIIF_CHECKBOX))
        checkboxWidth = dat->checkboxSize + 2;
    else
        checkboxWidth = 0;

    rc.left = 0;
    cstatus = contact->wStatus;
    
    /***** BACKGROUND DRAWING *****/
    // contacts
    
    if (type == CLCIT_CONTACT || type == CLCIT_DIVIDER) {
        StatusItems_t *sitem, *sfirstitem, *ssingleitem, *slastitem, *slastitem_NG,
            *sfirstitem_NG, *ssingleitem_NG, *sevencontact_pos, *soddcontact_pos, *pp_item;

        if (cstatus >= ID_STATUS_OFFLINE && cstatus <= ID_STATUS_OUTTOLUNCH) {
            BYTE perstatus_ignored;
            sitem = &StatusItems[cstatus - ID_STATUS_OFFLINE];
            
            pp_item = cEntry->status_item ? cEntry->status_item : cEntry->proto_status_item;
            
            if (!(perstatus_ignored = sitem->IGNORED) && !(flags & CONTACTF_NOTONLIST))
                SetTextColor(hdcMem, sitem->TEXTCOLOR);

            if(g_CluiData.bUsePerProto && pp_item && !pp_item->IGNORED) {
                sitem = pp_item;
                if((perstatus_ignored || g_CluiData.bOverridePerStatusColors) && sitem->TEXTCOLOR != -1)
                    SetTextColor(hdcMem, sitem->TEXTCOLOR);
            }
            
            sevencontact_pos = &StatusItems[ID_EXTBKEVEN_CNTCTPOS - ID_STATUS_OFFLINE];
            soddcontact_pos = &StatusItems[ID_EXTBKODD_CNTCTPOS - ID_STATUS_OFFLINE];
            sfirstitem = &StatusItems[ID_EXTBKFIRSTITEM - ID_STATUS_OFFLINE];
            ssingleitem = &StatusItems[ID_EXTBKSINGLEITEM - ID_STATUS_OFFLINE];
            slastitem = &StatusItems[ID_EXTBKLASTITEM - ID_STATUS_OFFLINE];

            sfirstitem_NG = &StatusItems[ID_EXTBKFIRSTITEM_NG - ID_STATUS_OFFLINE];
            ssingleitem_NG = &StatusItems[ID_EXTBKSINGLEITEM_NG - ID_STATUS_OFFLINE];
            slastitem_NG = &StatusItems[ID_EXTBKLASTITEM_NG - ID_STATUS_OFFLINE];
            
            rc.left = sitem->MARGIN_LEFT + bg_indent_l;
            rc.top = y + sitem->MARGIN_TOP;
            rc.right = clRect->right - sitem->MARGIN_RIGHT - bg_indent_r;
            rc.bottom = y + rowHeight - sitem->MARGIN_BOTTOM;

    // check for special cases (first item, single item, last item)
    // this will only change the shape for this status. Color will be blended over with ALPHA value                                     
            if (!ssingleitem->IGNORED && scanIndex == 0 && group->contactCount == 1 && group->parent != NULL) {
                rc.left = ssingleitem->MARGIN_LEFT + bg_indent_l;
                rc.top = y + ssingleitem->MARGIN_TOP;
                rc.right = clRect->right - ssingleitem->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - ssingleitem->MARGIN_BOTTOM;               

    // draw odd/even contact underlay
                if ((scanIndex == 0 || scanIndex % 2 == 0) && !sevencontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sevencontact_pos->COLOR, sevencontact_pos->ALPHA, sevencontact_pos->COLOR2, sevencontact_pos->COLOR2_TRANSPARENT, sevencontact_pos->GRADIENT, ssingleitem->CORNER, ssingleitem->BORDERSTYLE, sevencontact_pos->imageItem);
                } else if (scanIndex % 2 != 0 && !soddcontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, soddcontact_pos->COLOR, soddcontact_pos->ALPHA, soddcontact_pos->COLOR2, soddcontact_pos->COLOR2_TRANSPARENT, soddcontact_pos->GRADIENT, ssingleitem->CORNER, ssingleitem->BORDERSTYLE, soddcontact_pos->imageItem);
                }

                if (!sitem->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sitem->COLOR, sitem->ALPHA, sitem->COLOR2, sitem->COLOR2_TRANSPARENT, sitem->GRADIENT, ssingleitem->CORNER, sitem->BORDERSTYLE, sitem->imageItem);
                    savedCORNER = ssingleitem->CORNER;
                }
                if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, ssingleitem->COLOR, ssingleitem->ALPHA, ssingleitem->COLOR2, ssingleitem->COLOR2_TRANSPARENT, ssingleitem->GRADIENT, ssingleitem->CORNER, ssingleitem->BORDERSTYLE, ssingleitem->imageItem);
            } else if (scanIndex == 0 && group->contactCount > 1 && !sfirstitem->IGNORED && group->parent != NULL) {
                rc.left = sfirstitem->MARGIN_LEFT + bg_indent_l;
                rc.top = y + sfirstitem->MARGIN_TOP;
                rc.right = clRect->right - sfirstitem->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - sfirstitem->MARGIN_BOTTOM;

    // draw odd/even contact underlay
                if ((scanIndex == 0 || scanIndex % 2 == 0) && !sevencontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sevencontact_pos->COLOR, sevencontact_pos->ALPHA, sevencontact_pos->COLOR2, sevencontact_pos->COLOR2_TRANSPARENT, sevencontact_pos->GRADIENT, sfirstitem->CORNER, sevencontact_pos->BORDERSTYLE, sevencontact_pos->imageItem);
                } else if (scanIndex % 2 != 0 && !soddcontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, soddcontact_pos->COLOR, soddcontact_pos->ALPHA, soddcontact_pos->COLOR2, soddcontact_pos->COLOR2_TRANSPARENT, soddcontact_pos->GRADIENT, sfirstitem->CORNER, soddcontact_pos->BORDERSTYLE, soddcontact_pos->imageItem);
                }

                if (!sitem->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sitem->COLOR, sitem->ALPHA, sitem->COLOR2, sitem->COLOR2_TRANSPARENT, sitem->GRADIENT, sfirstitem->CORNER, sitem->BORDERSTYLE, sitem->imageItem);
                    savedCORNER = sfirstitem->CORNER;
                }
                if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, sfirstitem->COLOR, sfirstitem->ALPHA, sfirstitem->COLOR2, sfirstitem->COLOR2_TRANSPARENT, sfirstitem->GRADIENT, sfirstitem->CORNER, sfirstitem->BORDERSTYLE, sfirstitem->imageItem);
            } else if (scanIndex == group->contactCount - 1 && !slastitem->IGNORED && group->parent != NULL) {
    // last item of group                                                                       
                rc.left = slastitem->MARGIN_LEFT + bg_indent_l;
                rc.top = y + slastitem->MARGIN_TOP;
                rc.right = clRect->right - slastitem->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - slastitem->MARGIN_BOTTOM;

    // draw odd/even contact underlay
                if ((scanIndex == 0 || scanIndex % 2 == 0) && !sevencontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sevencontact_pos->COLOR, sevencontact_pos->ALPHA, sevencontact_pos->COLOR2, sevencontact_pos->COLOR2_TRANSPARENT, sevencontact_pos->GRADIENT, slastitem->CORNER, sevencontact_pos->BORDERSTYLE, sevencontact_pos->imageItem);
                } else if (scanIndex % 2 != 0 && !soddcontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, soddcontact_pos->COLOR, soddcontact_pos->ALPHA, soddcontact_pos->COLOR2, soddcontact_pos->COLOR2_TRANSPARENT, soddcontact_pos->GRADIENT, slastitem->CORNER, soddcontact_pos->BORDERSTYLE, soddcontact_pos->imageItem);
                }

                if (!sitem->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sitem->COLOR, sitem->ALPHA, sitem->COLOR2, sitem->COLOR2_TRANSPARENT, sitem->GRADIENT, slastitem->CORNER, sitem->BORDERSTYLE, sitem->imageItem);
                    savedCORNER = slastitem->CORNER;
                }
                if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, slastitem->COLOR, slastitem->ALPHA, slastitem->COLOR2, slastitem->COLOR2_TRANSPARENT, slastitem->GRADIENT, slastitem->CORNER, slastitem->BORDERSTYLE, slastitem->imageItem);
            } else
                // - - - Non-grouped items - - -                    
                if (type != CLCIT_GROUP // not a group
                    && group->parent == NULL // not grouped
                    && !sfirstitem_NG->IGNORED && scanIndex != group->contactCount - 1 && !(*bFirstNGdrawn)) {
    // first NON-grouped
                *bFirstNGdrawn = TRUE;                       
                rc.left = sfirstitem_NG->MARGIN_LEFT + bg_indent_l;
                rc.top = y + sfirstitem_NG->MARGIN_TOP;
                rc.right = clRect->right - sfirstitem_NG->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - sfirstitem_NG->MARGIN_BOTTOM;

    // draw odd/even contact underlay
                if ((scanIndex == 0 || scanIndex % 2 == 0) && !sevencontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sevencontact_pos->COLOR, sevencontact_pos->ALPHA, sevencontact_pos->COLOR2, sevencontact_pos->COLOR2_TRANSPARENT, sevencontact_pos->GRADIENT, sfirstitem_NG->CORNER, sevencontact_pos->BORDERSTYLE, sevencontact_pos->imageItem);
                } else if (scanIndex % 2 != 0 && !soddcontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, soddcontact_pos->COLOR, soddcontact_pos->ALPHA, soddcontact_pos->COLOR2, soddcontact_pos->COLOR2_TRANSPARENT, soddcontact_pos->GRADIENT, sfirstitem_NG->CORNER, soddcontact_pos->BORDERSTYLE, soddcontact_pos->imageItem);
                }

                if (!sitem->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sitem->COLOR, sitem->ALPHA, sitem->COLOR2, sitem->COLOR2_TRANSPARENT, sitem->GRADIENT, sfirstitem_NG->CORNER, sitem->BORDERSTYLE, sitem->imageItem);
                    savedCORNER = sfirstitem_NG->CORNER;
                }
                if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, sfirstitem_NG->COLOR, sfirstitem_NG->ALPHA, sfirstitem_NG->COLOR2, sfirstitem_NG->COLOR2_TRANSPARENT, sfirstitem_NG->GRADIENT, sfirstitem_NG->CORNER, sfirstitem->BORDERSTYLE, sfirstitem->imageItem);
            } else if (type != CLCIT_GROUP // not a group
                       && group->parent == NULL && !slastitem_NG->IGNORED && scanIndex == group->contactCount - 1 && (*bFirstNGdrawn)) {
    // last item of list (NON-group)
    // last NON-grouped
                rc.left = slastitem_NG->MARGIN_LEFT + bg_indent_l;
                rc.top = y + slastitem_NG->MARGIN_TOP;
                rc.right = clRect->right - slastitem_NG->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - slastitem_NG->MARGIN_BOTTOM;

    // draw odd/even contact underlay
                if ((scanIndex == 0 || scanIndex % 2 == 0) && !sevencontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sevencontact_pos->COLOR, sevencontact_pos->ALPHA, sevencontact_pos->COLOR2, sevencontact_pos->COLOR2_TRANSPARENT, sevencontact_pos->GRADIENT, slastitem_NG->CORNER, sevencontact_pos->BORDERSTYLE, sevencontact_pos->imageItem);
                } else if (scanIndex % 2 != 0 && !soddcontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, soddcontact_pos->COLOR, soddcontact_pos->ALPHA, soddcontact_pos->COLOR2, soddcontact_pos->COLOR2_TRANSPARENT, soddcontact_pos->GRADIENT, slastitem_NG->CORNER, soddcontact_pos->BORDERSTYLE, soddcontact_pos->imageItem);
                }

                if (!sitem->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sitem->COLOR, sitem->ALPHA, sitem->COLOR2, sitem->COLOR2_TRANSPARENT, sitem->GRADIENT, slastitem_NG->CORNER, sitem->BORDERSTYLE, sitem->imageItem);
                    savedCORNER = slastitem_NG->CORNER;
                }
                if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, slastitem_NG->COLOR, slastitem_NG->ALPHA, slastitem_NG->COLOR2, slastitem_NG->COLOR2_TRANSPARENT, slastitem_NG->GRADIENT, slastitem_NG->CORNER, slastitem->BORDERSTYLE, slastitem->imageItem);
            } else if (type != CLCIT_GROUP // not a group
                       && group->parent == NULL && !slastitem_NG->IGNORED && !(*bFirstNGdrawn)) {
    // single item of NON-group
    // single NON-grouped
                rc.left = ssingleitem_NG->MARGIN_LEFT + bg_indent_l;
                rc.top = y + ssingleitem_NG->MARGIN_TOP;
                rc.right = clRect->right - ssingleitem_NG->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - ssingleitem_NG->MARGIN_BOTTOM;

    // draw odd/even contact underlay
                if ((scanIndex == 0 || scanIndex % 2 == 0) && !sevencontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sevencontact_pos->COLOR, sevencontact_pos->ALPHA, sevencontact_pos->COLOR2, sevencontact_pos->COLOR2_TRANSPARENT, sevencontact_pos->GRADIENT, ssingleitem_NG->CORNER, sevencontact_pos->BORDERSTYLE, sevencontact_pos->imageItem);
                } else if (scanIndex % 2 != 0 && !soddcontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, soddcontact_pos->COLOR, soddcontact_pos->ALPHA, soddcontact_pos->COLOR2, soddcontact_pos->COLOR2_TRANSPARENT, soddcontact_pos->GRADIENT, ssingleitem_NG->CORNER, soddcontact_pos->BORDERSTYLE, soddcontact_pos->imageItem);
                }

                if (!sitem->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sitem->COLOR, sitem->ALPHA, sitem->COLOR2, sitem->COLOR2_TRANSPARENT, sitem->GRADIENT, ssingleitem_NG->CORNER, sitem->BORDERSTYLE, sitem->imageItem);
                    savedCORNER = ssingleitem_NG->CORNER;
                }
                if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, ssingleitem_NG->COLOR, ssingleitem_NG->ALPHA, ssingleitem_NG->COLOR2, ssingleitem_NG->COLOR2_TRANSPARENT, ssingleitem_NG->GRADIENT, ssingleitem_NG->CORNER, ssingleitem->BORDERSTYLE, ssingleitem->imageItem);
            } else if (!sitem->IGNORED) {
    // draw default grouped
    // draw odd/even contact underlay
                if ((scanIndex == 0 || scanIndex % 2 == 0) && !sevencontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, sevencontact_pos->COLOR, sevencontact_pos->ALPHA, sevencontact_pos->COLOR2, sevencontact_pos->COLOR2_TRANSPARENT, sevencontact_pos->GRADIENT, sitem->CORNER, sevencontact_pos->BORDERSTYLE, sevencontact_pos->imageItem);
                } else if (scanIndex % 2 != 0 && !soddcontact_pos->IGNORED) {
                    if (!selected || selBlend)
                        DrawAlpha(hdcMem, &rc, soddcontact_pos->COLOR, soddcontact_pos->ALPHA, soddcontact_pos->COLOR2, soddcontact_pos->COLOR2_TRANSPARENT, soddcontact_pos->GRADIENT, sitem->CORNER, soddcontact_pos->BORDERSTYLE, soddcontact_pos->imageItem);
                }

                if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, sitem->COLOR, sitem->ALPHA, sitem->COLOR2, sitem->COLOR2_TRANSPARENT, sitem->GRADIENT, sitem->CORNER, sitem->BORDERSTYLE, sitem->imageItem);
                savedCORNER = sitem->CORNER;
            }
        }
    }
    if (type == CLCIT_GROUP) {
        StatusItems_t *sempty = &StatusItems[ID_EXTBKEMPTYGROUPS - ID_STATUS_OFFLINE];
        StatusItems_t *sexpanded = &StatusItems[ID_EXTBKEXPANDEDGROUP - ID_STATUS_OFFLINE];
        StatusItems_t *scollapsed = &StatusItems[ID_EXTBKCOLLAPSEDDGROUP - ID_STATUS_OFFLINE];
        
        ChangeToFont(hdcMem, dat, FONTID_GROUPS, &fontHeight);
        if (contact->group->contactCount == 0) {
            if (!sempty->IGNORED) {
                rc.left = sempty->MARGIN_LEFT + bg_indent_l;
                rc.top = y + sempty->MARGIN_TOP;
                rc.right = clRect->right - sempty->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - sempty->MARGIN_BOTTOM;
                //if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, sempty->COLOR, sempty->ALPHA, sempty->COLOR2, sempty->COLOR2_TRANSPARENT, sempty->GRADIENT, sempty->CORNER, sempty->BORDERSTYLE, sempty->imageItem);
                savedCORNER = sempty->CORNER;
                oldGroupColor = SetTextColor(hdcMem, sempty->TEXTCOLOR);
            }
        } else if (contact->group->expanded) {
            if (!sexpanded->IGNORED) {
                rc.left = sexpanded->MARGIN_LEFT + bg_indent_l;
                rc.top = y + sexpanded->MARGIN_TOP;
                rc.right = clRect->right - sexpanded->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - (char) sexpanded->MARGIN_BOTTOM;
                //if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, sexpanded->COLOR, sexpanded->ALPHA, sexpanded->COLOR2, sexpanded->COLOR2_TRANSPARENT, sexpanded->GRADIENT, sexpanded->CORNER, sexpanded->BORDERSTYLE, sexpanded->imageItem);
                savedCORNER = sexpanded->CORNER;
                oldGroupColor = SetTextColor(hdcMem, sexpanded->TEXTCOLOR);
            }
        } else {
            if (!scollapsed->IGNORED) {
    // collapsed but not empty
                rc.left = scollapsed->MARGIN_LEFT + bg_indent_l;
                rc.top = y + scollapsed->MARGIN_TOP;
                rc.right = clRect->right - scollapsed->MARGIN_RIGHT - bg_indent_r;
                rc.bottom = y + rowHeight - scollapsed->MARGIN_BOTTOM;
                //if (!selected || selBlend)
                    DrawAlpha(hdcMem, &rc, scollapsed->COLOR, scollapsed->ALPHA, scollapsed->COLOR2, scollapsed->COLOR2_TRANSPARENT, scollapsed->GRADIENT, scollapsed->CORNER, scollapsed->BORDERSTYLE, scollapsed->imageItem);
                savedCORNER = scollapsed->CORNER;
                oldGroupColor = SetTextColor(hdcMem, scollapsed->TEXTCOLOR);
            }
        }
    }
    if (selected) {
        StatusItems_t *sselected = &StatusItems[ID_EXTBKSELECTION - ID_STATUS_OFFLINE];

        if (!g_ignoreselforgroups || type != CLCIT_GROUP) {
            if (!sselected->IGNORED) {
                if (DBGetContactSettingByte(NULL, "CLCExt", "EXBK_EqualSelection", 0) == 1 && savedCORNER != -1) {
                    DrawAlpha(hdcMem, &rc, sselected->COLOR, sselected->ALPHA, sselected->COLOR2, sselected->COLOR2_TRANSPARENT, sselected->GRADIENT, savedCORNER, sselected->BORDERSTYLE, sselected->imageItem);
                } else {
                    rc.left = sselected->MARGIN_LEFT + bg_indent_l;
                    rc.top = y + sselected->MARGIN_TOP;
                    rc.right = clRect->right - sselected->MARGIN_RIGHT - bg_indent_r;
                    rc.bottom = y + rowHeight - sselected->MARGIN_BOTTOM;                     
                    DrawAlpha(hdcMem, &rc, sselected->COLOR, sselected->ALPHA, sselected->COLOR2, sselected->COLOR2_TRANSPARENT, sselected->GRADIENT, sselected->CORNER, sselected->BORDERSTYLE, sselected->imageItem);
                }
            }
            SetTextColor(hdcMem, dat->selTextColour);
        }
    } 
    else if (g_hottrack)
        SetHotTrackColour(hdcMem,dat);


    if(g_RTL)
        SetLayout(hdcMem, LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED);
//bgskipped:

    rcContent.top = y;
    rcContent.bottom = y + rowHeight;
    //rcContent.left = rtl ? dat->rightMargin : leftX;
    //rcContent.right = rtl ? clRect->right - leftX : clRect->right - dat->rightMargin;
    rcContent.left = leftX;
    rcContent.right = clRect->right - dat->rightMargin;
    twoRows = ((dat->fontInfo[FONTID_STATUS].fontHeight + fontHeight <= rowHeight + 2) && (g_CluiData.dualRowMode != MULTIROW_NEVER)) && !dat->bisEmbedded;
    pi_avatar = !dat->bisEmbedded && type == CLCIT_CONTACT && (av_wanted) && contact->ace != 0 && !(contact->ace->dwFlags & AVS_HIDEONCLIST);
    
    //checkboxes
    if (checkboxWidth) {
        RECT rc;
        HANDLE hTheme = NULL;

        if (IsWinVerXPPlus()) {
            if (!themeAPIHandle) {
                InitThemeAPI();
            }
            if (MyOpenThemeData && MyCloseThemeData && MyDrawThemeBackground) {
                hTheme = MyOpenThemeData(hwnd, L"BUTTON");
            }
        }
        rc.left = leftX;
        rc.right = rc.left + dat->checkboxSize;
        rc.top = y + ((rowHeight - dat->checkboxSize) >> 1);
        rc.bottom = rc.top + dat->checkboxSize;
        if (hTheme) {
            MyDrawThemeBackground(hTheme, hdcMem, BP_CHECKBOX, flags & CONTACTF_CHECKED ? (g_hottrack ? CBS_CHECKEDHOT : CBS_CHECKEDNORMAL) : (g_hottrack ? CBS_UNCHECKEDHOT : CBS_UNCHECKEDNORMAL), &rc, &rc);
        } else
            DrawFrameControl(hdcMem, &rc, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_FLAT | (flags & CONTACTF_CHECKED ? DFCS_CHECKED : 0) | (g_hottrack ? DFCS_HOT : 0));
        if (hTheme && MyCloseThemeData) {
            MyCloseThemeData(hTheme);
            hTheme = NULL;
        }
        rcContent.left += checkboxWidth;
        leftX += checkboxWidth;
    }

    if (type == CLCIT_GROUP)
        iImage = contact->group->expanded ? IMAGE_GROUPOPEN : IMAGE_GROUPSHUT;
    else if (type == CLCIT_CONTACT)
        iImage = contact->iImage;


    if(pi_avatar && (av_left || av_right)) {
        RECT rc;
        
        rc.left = rcContent.left;
        rc.right = clRect->right;
        rc.top = y;
        rc.bottom = rc.top + rowHeight;
        
        if(av_left) {
            leftOffset += DrawAvatar(hdcMem, &rc, contact, y, dat, iImage ? cstatus : 0, rowHeight);
            rcContent.left += leftOffset;
            leftX += leftOffset;
        }
        else {
            //rc.left = rc.right - g_CluiData.avatarSize - dat->rightMargin;
            rc.left = rcContent.right - g_CluiData.avatarSize;
            rightOffset += DrawAvatar(hdcMem, &rc, contact, y, dat, iImage ? cstatus : 0, rowHeight);
            rcContent.right -= (rightOffset - 2);
        }
    }
    else if(type == CLCIT_CONTACT && !dat->bisEmbedded && !g_selectiveIcon && (dwFlags & CLUI_FRAME_ALWAYSALIGNNICK) && av_wanted && (av_left || av_right)) {
        if(av_right)
            rcContent.right -= (g_CluiData.avatarSize);
        if(av_left)
            rcContent.left += (g_CluiData.avatarSize + 2);
    }
    //icon
    
    // skip icon for groups if the option is enabled...
    
    if(type == CLCIT_GROUP && dwFlags & CLUI_FRAME_NOGROUPICON) {
        iconXSpace = 0;
        goto text;
    }
    if (iImage != -1) {
    // this doesnt use CLS_CONTACTLIST since the colour prolly wont match anyway
        COLORREF colourFg = dat->selBkColour;
        int clientId = contact->clientId;
        int mode = ILD_NORMAL;
        pi_selectiveIcon = g_selectiveIcon && (type == CLCIT_CONTACT);
        
        if((dwFlags & CLUI_FRAME_STATUSICONS && !pi_selectiveIcon) || type != CLCIT_CONTACT || (pi_selectiveIcon && !avatar_done)) {
            if (g_hottrack) {
                colourFg = dat->hotTextColour;
            } else if (type == CLCIT_CONTACT && flags & CONTACTF_NOTONLIST) {
                colourFg = dat->fontInfo[FONTID_NOTONLIST].colour; 
                mode = ILD_BLEND50;
            }
            if (type == CLCIT_CONTACT && dat->showIdle && (flags & CONTACTF_IDLE) && GetRealStatus(&group->contact[scanIndex], ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE)
                mode = ILD_SELECTED;
            
            if(pi_selectiveIcon && av_right) {
                ImageList_DrawEx(himlCListClc, iImage, hdcMem, rcContent.right - 18, (twoRows && type == CLCIT_CONTACT && !g_CluiData.bCenterStatusIcons) ? y + 2 : y + ((rowHeight - 16) >> 1), 0, 0, CLR_NONE, colourFg, mode);
                rcContent.right -= 18;
            }
            else {
                DWORD offset = 0;
                BOOL centered = FALSE;
                offset +=  (type != CLCIT_CONTACT || avatar_done || !(av_wanted) ? 20 : dwFlags & CLUI_FRAME_ALWAYSALIGNNICK && av_left && g_selectiveIcon ? g_CluiData.avatarSize + 2 : 20);
                centered = (g_CluiData.bCenterStatusIcons && offset == g_CluiData.avatarSize + 2);
                ImageList_DrawEx(himlCListClc, iImage, hdcMem,  centered ? rcContent.left + offset / 2 - 10 : rcContent.left, (twoRows && type == CLCIT_CONTACT && !g_CluiData.bCenterStatusIcons) ? y + 2 : y + ((rowHeight - 16) >> 1), 0, 0, CLR_NONE, colourFg, mode);
                rcContent.left += offset;
            }
        }
        else
            iconXSpace = 0;
        if (type == CLCIT_CONTACT && !dat->bisEmbedded) {
            BYTE bApparentModeDontCare = !((flags & CONTACTF_VISTO) ^ (flags & CONTACTF_INVISTO));
    // client icon
            if (clientId >= 0 && clientId < 35 && dwFlags & CLUI_SHOWCLIENTICONS) {
                //DrawIconEx(hdcMem, rcContent.right - 18, twoRows ? rcContent.bottom - g_cysmIcon - 2 : y + ((rowHeight - 16) >> 1), im_clienthIcons[clientId], g_cxsmIcon, g_cysmIcon, 0, 0, DI_NORMAL | DI_COMPAT);
                DrawIconEx(hdcMem, rcContent.right - g_CluiData.exIconScale, twoRows ? rcContent.bottom - g_exIconSpacing : y + ((rowHeight - g_CluiData.exIconScale) >> 1), im_clienthIcons[clientId], g_CluiData.exIconScale, g_CluiData.exIconScale, 0, 0, DI_NORMAL | DI_COMPAT);
                rcContent.right -= g_exIconSpacing;
                rightIcons++;
            }
            contact->extraIconRightBegin = 0;
            if(contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry && cEntry->iExtraValid) {
                int i;
                for(i = 5; i >= 0; i--) {
                    if(cEntry->iExtraImage[i] != 0xff && ((1 << i) & g_CluiData.dwExtraImageMask)) {
                        if(contact->extraIconRightBegin == 0)
                            contact->extraIconRightBegin = rcContent.right;
                        ImageList_DrawEx(dat->himlExtraColumns, cEntry->iExtraImage[i], hdcMem, rcContent.right - g_CluiData.exIconScale, twoRows ? rcContent.bottom - g_exIconSpacing : y + ((rowHeight - g_CluiData.exIconScale) >> 1), 
                                         0, 0, CLR_NONE, CLR_NONE, ILD_NORMAL);
                        rcContent.right -= g_exIconSpacing;
                        rightIcons++;
                    }
                }
            }
            if (!bApparentModeDontCare && (dwFlags & CLUI_SHOWVISI) && contact->proto) {
                if(strstr(contact->proto, "IRC"))
                    DrawIconEx(hdcMem, rcContent.right - g_CluiData.exIconScale, twoRows ? rcContent.bottom - g_exIconSpacing : y + ((rowHeight - g_CluiData.exIconScale) >> 1), 
                               g_CluiData.hIconChatactive, g_CluiData.exIconScale, g_CluiData.exIconScale, 0, 0, DI_NORMAL | DI_COMPAT);
                else
                    DrawIconEx(hdcMem, rcContent.right - g_CluiData.exIconScale, twoRows ? rcContent.bottom - g_exIconSpacing : y + ((rowHeight - g_CluiData.exIconScale) >> 1), 
                               flags & CONTACTF_VISTO ? g_CluiData.hIconVisible : g_CluiData.hIconInvisible, g_CluiData.exIconScale, g_CluiData.exIconScale, 0, 0, DI_NORMAL | DI_COMPAT);
                rcContent.right -= g_exIconSpacing;
                rightIcons++;
            }
        }
    }
    //text
text:    
    if (type == CLCIT_DIVIDER) {
        RECT rc;
        rc.top = y + ((rowHeight) >> 1); rc.bottom = rc.top + 2;
        rc.left = rcContent.left;
        rc.right = rc.left - dat->rightMargin + ((clRect->right - rc.left - textSize.cx) >> 1) - 3;
        DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT);
        TextOut(hdcMem, rc.right + 3, y + ((rowHeight - fontHeight) >> 1), contact->szText, lstrlen(contact->szText));
        rc.left = rc.right + 6 + textSize.cx;
        rc.right = clRect->right - dat->rightMargin;
        DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT);
    } else if (type == CLCIT_GROUP) {
        RECT rc;
        int leftMargin = 0, countStart = 0, leftLineEnd, rightLineStart;
        fontHeight = dat->fontInfo[FONTID_GROUPS].fontHeight;
        rc.top = y + ((rowHeight - fontHeight) >> 1);
        rc.bottom = rc.top + textSize.cy;
        if (szCounts[0]) {
            int required, labelWidth, offset = 0;
            int height = 0;
            COLORREF clr = GetTextColor(hdcMem);
            
            ChangeToFont(hdcMem, dat, FONTID_GROUPCOUNTS, &height);
            if(oldGroupColor != -1)
                SetTextColor(hdcMem, clr);

            rc.left = dat->leftMargin + indent * dat->groupIndent + checkboxWidth + iconXSpace;
            rc.right = clRect->right - dat->rightMargin;

            if(indent == 0 && iconXSpace == 0)
                rc.left += 2;

            required = textSize.cx + countsSize.cx + spaceSize.cx;

            if(required > rc.right - rc.left)
                textSize.cx = (rc.right - rc.left) - countsSize.cx - spaceSize.cx;

            labelWidth = textSize.cx + countsSize.cx + spaceSize.cx;
            if(g_center)
                offset = ((rc.right - rc.left) - labelWidth) / 2;
            

            TextOutA(hdcMem, rc.left + offset + textSize.cx + spaceSize.cx, rc.top + groupCountsFontTopShift, szCounts, lstrlenA(szCounts));
            rightLineStart = rc.left + offset + textSize.cx + spaceSize.cx + countsSize.cx + 2;
            
            if (selected && !g_ignoreselforgroups)
                SetTextColor(hdcMem, dat->selTextColour);
            else 
                SetTextColor(hdcMem, clr);
            ChangeToFont(hdcMem, dat, FONTID_GROUPS, &height);
            SetTextColor(hdcMem, clr);
            rc.left += offset;
            rc.right = rc.left + textSize.cx;
            leftLineEnd = rc.left - 2;
            qLeft = rc.left;
            DrawText(hdcMem, contact->szText, -1, &rc, DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS);
        } else if (g_center && !szCounts[0]) {
            int offset;
            
            rc.left = rcContent.left;
            rc.right = clRect->right - dat->rightMargin;
            if(textSize.cx >= rc.right - rc.left)
                textSize.cx = rc.right - rc.left;

            offset = ((rc.right - rc.left) - textSize.cx) / 2;
            rc.left += offset;
            rc.right = rc.left + textSize.cx;
            leftLineEnd = rc.left - 2;
            rightLineStart = rc.right + 2;
            DrawText(hdcMem, contact->szText, -1, &rc, DT_CENTER | DT_NOPREFIX | DT_SINGLELINE);
            qLeft = rc.left;
        } else {
            qLeft = rcContent.left + (indent == 0 && iconXSpace == 0 ? 2 : 0);;
            rc.left = qLeft;
            rc.right = min(rc.left + textSize.cx, clRect->right - dat->rightMargin);;
            DrawText(hdcMem, contact->szText, -1, &rc, DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS);
            rightLineStart = qLeft + textSize.cx + 2;
        }

        if (dat->exStyle & CLS_EX_LINEWITHGROUPS) {
            if(!g_center) {
                rc.top = y + ((rowHeight) >> 1); rc.bottom = rc.top + 2;
                rc.left = rightLineStart;
                rc.right = clRect->right - 1 - dat->extraColumnSpacing * dat->extraColumnsCount - dat->rightMargin;
                if (rc.right - rc.left > 1)
                    DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT);
            }
            else {
                rc.top = y + ((rowHeight) >> 1); rc.bottom = rc.top + 2;
                rc.left = dat->leftMargin + indent * dat->groupIndent + checkboxWidth + iconXSpace;
                rc.right = leftLineEnd;
                if (rc.right > rc.left)
                    DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT);
                rc.right = clRect->right - dat->rightMargin;
                rc.left = rightLineStart;
                if (rc.right > rc.left)
                    DrawEdge(hdcMem, &rc, BDR_SUNKENOUTER, BF_RECT);
            }
        }
    } else {
        TCHAR *szText = contact->szText;
        
        rcContent.top = y + ((rowHeight - fontHeight) >> 1);

        // avatar

        if(!dat->bisEmbedded) {
            if(av_wanted && !avatar_done && pi_avatar) {
                if(av_rightwithnick) {
                    RECT rcAvatar = rcContent;

                    rcAvatar.left = rcContent.right - (g_CluiData.avatarSize + 2);
                    DrawAvatar(hdcMem, &rcAvatar, contact, y, dat, iImage ? cstatus : 0, rowHeight);
                    rcContent.right -= (g_CluiData.avatarSize + 2);
                }
                else
                    rcContent.left += DrawAvatar(hdcMem, &rcContent, contact, y, dat, iImage ? cstatus : 0, rowHeight);
            }
            else if(dwFlags & CLUI_FRAME_ALWAYSALIGNNICK && !avatar_done && av_wanted)
                rcContent.left += (dwFlags & (CLUI_FRAME_AVATARSLEFT | CLUI_FRAME_AVATARSRIGHT | CLUI_FRAME_AVATARSRIGHTWITHNICK) ? 0 : g_CluiData.avatarSize + 2);
        }

        // nickname
        if(!twoRows) {
            if(dt_nickflags) {
#if defined(_GDITEXTRENDERING) && defined(_UNICODE)
                if(g_gdiPlusText)
                    DrawTextHQ(hdcMem, dat->fontInfo[dat->currentFontID].hFont, &rcContent, szText, GetTextColor(hdcMem), fontHeight, dt_nickflags);
                else
#endif                
                    DrawText(hdcMem, szText, -1, &rcContent, DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP | DT_WORD_ELLIPSIS | DT_SINGLELINE | dt_nickflags);
            }
            else {
#if defined(_GDITEXTRENDERING) && defined(_UNICODE)
                if(g_gdiPlusText)
                    DrawTextHQ(hdcMem, dat->fontInfo[dat->currentFontID].hFont, &rcContent, szText, GetTextColor(hdcMem), fontHeight, 0);
                else
#endif                
                    DrawText(hdcMem, szText, -1, &rcContent, DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP | DT_WORD_ELLIPSIS | DT_SINGLELINE);
            }
        }
        else {
            int statusFontHeight;
            DWORD dtFlags = DT_WORD_ELLIPSIS | DT_NOPREFIX | DT_NOCLIP | DT_SINGLELINE;
            DWORD saved_right = rcContent.right;
            BOOL verticalfit = FALSE;
            
            rcContent.top = y + g_CluiData.avatarPadding / 2;
            
            if(cEntry->timezone != -1 && g_CluiData.bShowLocalTime) {
                DBTIMETOSTRING dbtts;
                char szResult[80];
                int  idOldFont;
                DWORD final_time;
                DWORD now = time(NULL);
                SIZE szTime;
                RECT rc = rcContent;
                COLORREF oldColor;
                int fHeight = 0;
                
                final_time = now - cEntry->timediff;

                if(final_time == now && g_CluiData.bShowLocalTimeSelective)
                    goto nodisplay;
                
                dbtts.szDest = szResult;
                dbtts.cbDest = 70;
                dbtts.szFormat = "t";
                CallService(MS_DB_TIME_TIMESTAMPTOSTRING, (WPARAM)final_time, (LPARAM)&dbtts);
                oldColor = GetTextColor(hdcMem);
                idOldFont = dat->currentFontID;
                ChangeToFont(hdcMem, dat, FONTID_TIMESTAMP, &fHeight);
                GetTextExtentPoint32A(hdcMem, szResult, lstrlenA(szResult), &szTime);
                verticalfit = (rowHeight - fHeight >= g_CluiData.exIconScale + 1);
				
                if(av_right) {
                    if(verticalfit)
                        rc.left = rcContent.right + (rightIcons * g_exIconSpacing) - szTime.cx - 2;
                    else
                        rc.left = rcContent.right - szTime.cx - 2;
                }
                else if(av_rightwithnick) {
                    if(verticalfit && rightIcons * g_exIconSpacing >= szTime.cx)
                        rc.left = clRect->right - dat->rightMargin - szTime.cx;
                    else if(verticalfit && !avatar_done)
                        rc.left = clRect->right - dat->rightMargin - szTime.cx;
                    else {
                        rc.left = rcContent.right - szTime.cx - 2;
                        rcContent.right = rc.left - 2;
                    }
                }
				else {
					if(verticalfit)
						rc.left = clRect->right - dat->rightMargin - szTime.cx;
					else
						rc.left = rcContent.right - szTime.cx - 2;
				}
				DrawTextA(hdcMem, szResult, -1, &rc, DT_NOPREFIX | DT_NOCLIP | DT_SINGLELINE);
				ChangeToFont(hdcMem, dat, idOldFont, 0);
				SetTextColor(hdcMem, oldColor);
                
                verticalfit = (rowHeight - fontHeight >= g_CluiData.exIconScale + 1);
                if(verticalfit && av_right)
                    rcContent.right = min(clRect->right - g_CluiData.avatarSize - 2, rc.left - 2);
                else if(verticalfit && !av_rightwithnick)
                    rcContent.right = min(clRect->right - dat->rightMargin, rc.left - 3);
            }
            else {
nodisplay:                
                verticalfit = (rowHeight - fontHeight >= g_CluiData.exIconScale + 1);
                if(verticalfit && av_right)
                    rcContent.right = clRect->right - g_CluiData.avatarSize - 2;
                else if(verticalfit && !av_rightwithnick)
                    rcContent.right = clRect->right - dat->rightMargin;
            }

#if defined(_GDITEXTRENDERING) && defined(_UNICODE)
            if(g_gdiPlusText)
                DrawTextHQ(hdcMem, dat->fontInfo[dat->currentFontID].hFont, &rcContent, szText, GetTextColor(hdcMem), fontHeight, dt_nickflags);
            else
#endif                
                DrawText(hdcMem, szText, -1, &rcContent, DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP | DT_WORD_ELLIPSIS | DT_SINGLELINE | dt_nickflags);

            rcContent.right = saved_right;
            
            rcContent.top += (fontHeight - 1);
            hPreviousFont = ChangeToFont(hdcMem, dat, FONTID_STATUS, &statusFontHeight);
            if(selected)
                SetTextColor(hdcMem, dat->selTextColour);
            rcContent.bottom = y + rowHeight;
            
            if(cstatus >= ID_STATUS_OFFLINE && cstatus <= ID_STATUS_OUTTOLUNCH) {
                TCHAR *szText = NULL;
                BYTE smsgValid = cEntry->bStatusMsgValid;
                
                if((dwFlags & CLUI_FRAME_SHOWSTATUSMSG && smsgValid > STATUSMSG_XSTATUSID) || smsgValid == STATUSMSG_XSTATUSNAME)
                    szText = cEntry->statusMsg;
                else
#if defined(_UNICODE)
                szText = &statusNames[cstatus - ID_STATUS_OFFLINE][0];
#else
                szText = statusNames[cstatus - ID_STATUS_OFFLINE];
#endif
                if(cEntry->dwCFlags & ECF_RTLSTATUSMSG && g_CluiData.bUseDCMirroring == 3)
                    dt_2ndrowflags |= (DT_RTLREADING | DT_RIGHT);
                
                if(rightIcons == 0) {
                    if((rcContent.bottom - rcContent.top) >= (2 * statusFontHeight)) {
                        dtFlags &= ~(DT_SINGLELINE | DT_BOTTOM | DT_NOCLIP);
                        dtFlags |= DT_WORDBREAK;
                        rcContent.bottom -= ((rcContent.bottom - rcContent.top) % statusFontHeight);
                    }
#if defined(_GDITEXTRENDERING) && defined(_UNICODE)
                    if(g_gdiPlusText)
                        DrawTextHQ(hdcMem, dat->fontInfo[dat->currentFontID].hFont, &rcContent, szText, GetTextColor(hdcMem), 0, dt_2ndrowflags);
                    else
#endif                    
                        DrawText(hdcMem, szText, -1, &rcContent, dtFlags | dt_2ndrowflags);
                }
                else {
                    if((rcContent.bottom - rcContent.top) < (2 * statusFontHeight) - 2) {
#if defined(_GDITEXTRENDERING) && defined(_UNICODE)
                        if(g_gdiPlusText)
                            DrawTextHQ(hdcMem, dat->fontInfo[dat->currentFontID].hFont, &rcContent, szText, GetTextColor(hdcMem), 0, dt_2ndrowflags);
                        else
#endif                    
                            DrawText(hdcMem, szText, -1, &rcContent, dtFlags | dt_2ndrowflags);
                    }
                    else {
                        DRAWTEXTPARAMS dtp = {0};
                        LONG rightIconsTop = rcContent.bottom - g_exIconSpacing;
                        LONG old_right = rcContent.right;
                        ULONG textCounter = 0;
                        ULONG ulLen = lstrlen(szText);
                        LONG old_bottom = rcContent.bottom;
                        DWORD i_dtFlags = DT_WORDBREAK | DT_NOPREFIX | dt_2ndrowflags;
                        dtp.cbSize = sizeof(dtp);
                        rcContent.right = clRect->right - dat->rightMargin;
                        do {
                            if(rcContent.top + (statusFontHeight - 1) > rightIconsTop + 1)
                                rcContent.right = old_right;
                            dtp.uiLengthDrawn = 0;
                            rcContent.bottom = rcContent.top + statusFontHeight - 1;
                            if(rcContent.bottom + statusFontHeight >= old_bottom)
                                i_dtFlags |= DT_END_ELLIPSIS;
                            DrawTextEx(hdcMem, &szText[textCounter], -1, &rcContent, i_dtFlags, &dtp);
                            rcContent.top += statusFontHeight;
                            textCounter += dtp.uiLengthDrawn;
                        } while (textCounter <= ulLen && dtp.uiLengthDrawn && rcContent.top + statusFontHeight <= old_bottom);
                    }
                }
            }
        }
    }
    if (selected) {
        if (type != CLCIT_DIVIDER) {
            TCHAR *szText = contact->szText;
            RECT rc;
            int qlen = lstrlen(dat->szQuickSearch);
            if(hPreviousFont)
                SelectObject(hdcMem, hPreviousFont);
            SetTextColor(hdcMem, dat->quickSearchColour);                
            if(type == CLCIT_CONTACT) {
                rc.left = rcContent.left;
                rc.top = y + ((rowHeight - fontHeight) >> 1);
                rc.right = clRect->right - rightOffset;
                rc.right = rcContent.right;
                rc.bottom = rc.top;
                if(twoRows)
                    rc.top = y;
            }
            else {
                rc.left = qLeft;
                rc.top = y + ((rowHeight - fontHeight) >> 1);
                rc.right = clRect->right - rightOffset;
                rc.bottom = rc.top;
            }
            if (qlen)
                DrawText(hdcMem, szText, qlen, &rc, DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP | DT_WORD_ELLIPSIS | DT_SINGLELINE);
        }
    }
    //extra icons
    for (iImage = 0; iImage< dat->extraColumnsCount; iImage++) {
        COLORREF colourFg = dat->selBkColour;
        int mode = ILD_NORMAL;
        if (contact->iExtraImage[iImage] == 0xFF)
            continue;
        if (selected)
            mode = ILD_SELECTED;
        else if (g_hottrack) {
            mode = ILD_FOCUS; colourFg = dat->hotTextColour;
        } else if (type == CLCIT_CONTACT && flags & CONTACTF_NOTONLIST) {
            colourFg = dat->fontInfo[FONTID_NOTONLIST].colour; mode = ILD_BLEND50;
        }
        ImageList_DrawEx(dat->himlExtraColumns, contact->iExtraImage[iImage], hdcMem, clRect->right - rightOffset - dat->extraColumnSpacing * (dat->extraColumnsCount - iImage), y + ((rowHeight - 16) >> 1), 0, 0, CLR_NONE, colourFg, mode);
    }
    if(g_RTL)
        SetLayout(hdcMem, 0);
}

void SkinDrawBg(HWND hwnd, HDC hdc)
{
    RECT rcCl;
    POINT ptTest = {0};

    ClientToScreen(hwnd, &ptTest);
    GetClientRect(hwnd, &rcCl);
    
    BitBlt(hdc, 0, 0, rcCl.right - rcCl.left, rcCl.bottom - rcCl.top, g_CluiData.hdcBg, ptTest.x - g_CluiData.ptW.x, ptTest.y - g_CluiData.ptW.y, SRCCOPY);
}

BOOL g_inCLCpaint = FALSE;

void PaintClc(HWND hwnd, struct ClcData *dat, HDC hdc, RECT *rcPaint)
{
    HDC hdcMem;
    RECT clRect;
    int y,indent,index,fontHeight;
    struct ClcGroup *group;
    HBITMAP hBmpOsb, hOldBitmap;
    HFONT hOldFont;
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    int grey = 0,groupCountsFontTopShift;
    BOOL bFirstNGdrawn = FALSE;
    int line_num = -1;
    COLORREF tmpbkcolour = style &CLS_CONTACTLIST ? (dat->useWindowsColours ? GetSysColor(COLOR_3DFACE) : dat->bkColour) : dat->bkColour;
    DWORD done, now = GetTickCount();
    selBlend = DBGetContactSettingByte(NULL, "CLCExt", "EXBK_SelBlend", 1);
    g_inCLCpaint = TRUE;
    g_focusWnd = GetFocus();
    my_status = GetGeneralisedStatus();
    g_HDC = hdc;
    
    g_gdiPlus = (g_CluiData.dwFlags & CLUI_FRAME_GDIPLUS && g_gdiplusToken);
    g_gdiPlusText = DBGetContactSettingByte(NULL, "CLC", "gdiplustext", 0) && g_gdiPlus;
    
    av_left = (g_CluiData.dwFlags & CLUI_FRAME_AVATARSLEFT);
    av_right = (g_CluiData.dwFlags & CLUI_FRAME_AVATARSRIGHT);
    av_rightwithnick = (g_CluiData.dwFlags & CLUI_FRAME_AVATARSRIGHTWITHNICK);
    av_wanted = (g_CluiData.dwFlags & CLUI_FRAME_AVATARS);

    mirror_rtl = (g_CluiData.bUseDCMirroring == 2);
    mirror_always = (g_CluiData.bUseDCMirroring == 1);
    mirror_rtltext = (g_CluiData.bUseDCMirroring == 3);

    g_center = DBGetContactSettingByte(NULL, "CLCExt", "EXBK_CenterGroupnames", 0) && !dat->bisEmbedded;
    g_ignoreselforgroups = DBGetContactSettingByte(NULL, "CLC", "IgnoreSelforGroups", 0);
    g_selectiveIcon = (g_CluiData.dwFlags & CLUI_FRAME_SELECTIVEICONS) && (g_CluiData.dwFlags & CLUI_FRAME_AVATARS) && !dat->bisEmbedded;
    g_exIconSpacing = g_CluiData.exIconScale + 2;
    
    if (dat->greyoutFlags & ClcStatusToPf2(my_status) || style & WS_DISABLED)
        grey = 1;
    else if (GetFocus() != hwnd && dat->greyoutFlags & GREYF_UNFOCUS)
        grey = 1;
    GetClientRect(hwnd, &clRect);
    if (rcPaint == NULL)
        rcPaint = &clRect;
    if (IsRectEmpty(rcPaint))
        return;
    y = -dat->yScroll;
    hdcMem = CreateCompatibleDC(hdc);
    hBmpOsb = CreateBitmap(clRect.right, clRect.bottom, 1, GetDeviceCaps(hdc, BITSPIXEL), NULL);

    hOldBitmap = SelectObject(hdcMem, hBmpOsb); 
    {
        TEXTMETRIC tm;
        hOldFont = SelectObject(hdcMem, dat->fontInfo[FONTID_GROUPS].hFont);
        GetTextMetrics(hdcMem, &tm);
        groupCountsFontTopShift = tm.tmAscent;
        SelectObject(hdcMem, dat->fontInfo[FONTID_GROUPCOUNTS].hFont);
        GetTextMetrics(hdcMem, &tm);
        groupCountsFontTopShift -= tm.tmAscent;
    }
    ChangeToFont(hdcMem, dat, FONTID_CONTACTS, &fontHeight);

    SetBkMode(hdcMem, TRANSPARENT); {
        HBRUSH hBrush, hoBrush;

        hBrush = CreateSolidBrush(tmpbkcolour);
        hoBrush = (HBRUSH) SelectObject(hdcMem, hBrush);
        FillRect(hdcMem, rcPaint, hBrush);

        SelectObject(hdcMem, hoBrush);
        DeleteObject(hBrush);

        if(!g_CluiData.neeedSnap) {
            if(g_CluiData.bWallpaperMode && !dat->bisEmbedded) {
                SkinDrawBg(hwnd, hdcMem);
                goto bgdone;
            }
            if (dat->hBmpBackground && !dat->bisEmbedded) {
                BITMAP bmp;
                HDC hdcBmp;
                int x, y;
                int bitx, bity;
                int maxx, maxy;
                int destw, desth;            
            // XXX: Halftone isnt supported on 9x, however the scretch problems dont happen on 98.
                SetStretchBltMode(hdcMem, HALFTONE);
    
                GetObject(dat->hBmpBackground, sizeof(bmp), &bmp);
                hdcBmp = CreateCompatibleDC(hdcMem);
                SelectObject(hdcBmp, dat->hBmpBackground);
                y = dat->backgroundBmpUse & CLBF_SCROLL ? -dat->yScroll : 0;
                maxx = dat->backgroundBmpUse & CLBF_TILEH ? clRect.right : 1;
                maxy = dat->backgroundBmpUse & CLBF_TILEV ? maxy = rcPaint->bottom : y + 1;
                if (!DBGetContactSettingByte(NULL, "CLCExt", "EXBK_FillWallpaper", 0)) {
                    switch (dat->backgroundBmpUse & CLBM_TYPE) {
                        case CLB_STRETCH:
                            if (dat->backgroundBmpUse & CLBF_PROPORTIONAL) {
                                if (clRect.right * bmp.bmHeight < clRect.bottom * bmp.bmWidth) {
                                    desth = clRect.bottom;
                                    destw = desth * bmp.bmWidth / bmp.bmHeight;
                                } else {
                                    destw = clRect.right;
                                    desth = destw * bmp.bmHeight / bmp.bmWidth;
                                }
                            } else {
                                destw = clRect.right;
                                desth = clRect.bottom;
                            }
                            break;
                        case CLB_STRETCHH:
                            if (dat->backgroundBmpUse & CLBF_PROPORTIONAL) {
                                destw = clRect.right;
                                desth = destw * bmp.bmHeight / bmp.bmWidth;
                            } else {
                                destw = clRect.right;
                                desth = bmp.bmHeight;
                            }
                            break;
                        case CLB_STRETCHV:
                            if (dat->backgroundBmpUse & CLBF_PROPORTIONAL) {
                                desth = clRect.bottom;
                                destw = desth * bmp.bmWidth / bmp.bmHeight;
                            } else {
                                destw = bmp.bmWidth;
                                desth = clRect.bottom;
                            }
                            break;
                        default:
            //clb_topleft
                            destw = bmp.bmWidth;
                            desth = bmp.bmHeight;
                            break;
                    }
                } else {
                    destw = bmp.bmWidth;
                    desth = bmp.bmHeight;
                }
    
                if (DBGetContactSettingByte(NULL, "CLCExt", "EXBK_FillWallpaper", 0)) {
                    RECT mrect;
                    GetWindowRect(hwnd, &mrect);
                    bitx = mrect.left;
                    bity = mrect.top;
                } else {
                    bitx = 0;
                    bity = 0;
                }
    
                for (; y < maxy; y += desth) {
                    if (y< rcPaint->top - desth)
                        continue;
                    for (x = 0; x < maxx; x += destw) {
                        StretchBlt(hdcMem, x, y, destw, desth, g_CluiData.hdcPic, bitx, bity, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
                    }
                }           
                DeleteDC(hdcBmp);
            }
        }
    } 
bgdone:
    group = &dat->list;
    group->scanIndex = 0;
    indent = 0;

    if(g_gdiPlus)
        CreateG(hdcMem);

    for (index = 0; y< rcPaint->bottom;) {
        if (group->scanIndex == group->contactCount) {
            group = group->parent;
            indent--;
            if (group == NULL) {
                break;
            }
            group->scanIndex++;
            continue;
        }

        line_num++;
		if (y > rcPaint->top - dat->row_heights[line_num] && y <= rcPaint->bottom) {
			RowHeights_GetRowHeight(dat, hwnd, &(group->contact[group->scanIndex]), line_num, style);
			PaintItem(hdcMem, group, &group->contact[group->scanIndex], indent, y, dat, index, hwnd, style, &clRect, &bFirstNGdrawn, groupCountsFontTopShift, dat->row_heights[line_num]);
		}
        index++;
        y += dat->row_heights[line_num];
        if (group->contact[group->scanIndex].type == CLCIT_GROUP && group->contact[group->scanIndex].group->expanded) {
            group = group->contact[group->scanIndex].group;
            indent++;
            group->scanIndex = 0;
            continue;
        }
        group->scanIndex++;
    }

    if (dat->iInsertionMark != -1) {
        //insertion mark
        HBRUSH hBrush, hoBrush;
        POINT pts[8];
        HRGN hRgn;

		pts[0].x=dat->leftMargin; pts[0].y= RowHeights_GetItemTopY(dat, dat->iInsertionMark) - dat->yScroll - 4;
        //pts[0]. x = dat->leftMargin; pts[0]. y = dat->iInsertionMark * rowHeight - dat->yScroll - 4;
        pts[1]. x = pts[0].x + 2;      pts[1]. y = pts[0].y + 3;
        pts[2]. x = clRect.right - 4;  pts[2]. y = pts[1].y;
        pts[3]. x = clRect.right - 1;  pts[3]. y = pts[0].y - 1;
        pts[4]. x = pts[3].x;        pts[4]. y = pts[0].y + 7;
        pts[5]. x = pts[2].x + 1;      pts[5]. y = pts[1].y + 2;
        pts[6]. x = pts[1].x;        pts[6]. y = pts[5].y;
        pts[7]. x = pts[0].x;        pts[7]. y = pts[4].y;
        hRgn = CreatePolygonRgn(pts, sizeof(pts) / sizeof(pts[0]), ALTERNATE);
        hBrush = CreateSolidBrush(dat->fontInfo[FONTID_CONTACTS].colour);
        hoBrush = (HBRUSH) SelectObject(hdcMem, hBrush);
        FillRgn(hdcMem, hRgn, hBrush);
        SelectObject(hdcMem, hoBrush);
        DeleteObject(hBrush);
    }
    if (!grey)
        BitBlt(hdc, rcPaint->left, rcPaint->top, rcPaint->right - rcPaint->left, rcPaint->bottom - rcPaint->top, hdcMem, rcPaint->left, rcPaint->top, SRCCOPY);

    if(g_gdiPlus)
        DeleteG();
    
    SelectObject(hdcMem, hOldBitmap);
    SelectObject(hdcMem, hOldFont);

    DeleteDC(hdcMem);
    if (grey) {
        PBYTE bits;
        BITMAPINFOHEADER bmih = {0};
        
        int i;
        int greyRed, greyGreen, greyBlue;
        COLORREF greyColour;
        bmih.biBitCount = 32;
        bmih.biSize = sizeof(bmih);
        bmih.biCompression = BI_RGB;
        bmih.biHeight = -clRect.bottom;
        bmih.biPlanes = 1;
        bmih.biWidth = clRect.right;
        bits = (PBYTE) mir_alloc(4 * bmih.biWidth * -bmih.biHeight);
        GetDIBits(hdc, hBmpOsb, 0, clRect.bottom, bits, (BITMAPINFO *) &bmih, DIB_RGB_COLORS);
        greyColour = GetSysColor(COLOR_3DFACE);
        greyRed = GetRValue(greyColour) * 2;
        greyGreen = GetGValue(greyColour) * 2;
        greyBlue = GetBValue(greyColour) * 2;
        if (divide3[0] == 255) {
            for (i = 0; i < sizeof(divide3) / sizeof(divide3[0]); i++) {
                divide3[i] = (i + 1) / 3;
            }
        }
        for (i = 4 * clRect.right *clRect.bottom - 4; i >= 0; i -= 4) {
            bits[i] = divide3[bits[i] + greyBlue];
            bits[i + 1] = divide3[bits[i + 1] + greyGreen];
            bits[i + 2] = divide3[bits[i + 2] + greyRed];
        }
        SetDIBitsToDevice(hdc, 0, 0, clRect.right, clRect.bottom, 0, 0, 0, clRect.bottom, bits, (BITMAPINFO *) &bmih, DIB_RGB_COLORS);
        mir_free(bits);
    }
    DeleteObject(hBmpOsb);
    g_inCLCpaint = FALSE;
    done = GetTickCount();
    //_DebugPopup(0, "time = %d", done - now);
}


