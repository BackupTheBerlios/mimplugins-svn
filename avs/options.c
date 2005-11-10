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
#include "m_avatars.h"
#include <m_popup.h>
#include "m_metacontacts.h"
#include "resource.h"

#define DM_SETAVATARNAME (WM_USER + 10)
#define DM_REALODAVATAR (WM_USER + 11)

extern int g_protocount;
extern int _DebugPopup(HANDLE hContact, const char *fmt, ...);
extern int SetAvatar(WPARAM wParam, LPARAM lParam);
extern struct protoPicCacheEntry *g_ProtoPictures;
extern HANDLE hEventChanged;
extern HINSTANCE g_hInst;
extern HICON g_hIcon;
extern BOOL g_imgDecoderAvail;
extern CRITICAL_SECTION cachecs;

extern int CreateAvatarInCache(HANDLE hContact, struct avatarCacheEntry *ace, int iIndex, char *szProto);
extern int ProtectAvatar(WPARAM wParam, LPARAM lParam), UpdateAvatar(HANDLE hContact);;
extern int SetAvatarAttribute(HANDLE hContact, DWORD attrib, int mode);
extern int DeleteAvatar(HANDLE hContact);
extern int ChangeAvatar(HANDLE hContact);
extern HBITMAP LoadPNG(struct avatarCacheEntry *ace, char *szFilename);

static BOOL dialoginit = TRUE;

extern pfnImgNewDecoder ImgNewDecoder;
extern pfnImgDeleteDecoder ImgDeleteDecoder;
extern pfnImgNewDIBFromFile ImgNewDIBFromFile;
extern pfnImgDeleteDIBSection ImgDeleteDIBSection;
extern pfnImgGetHandle ImgGetHandle;

static void RemoveProtoPic(const char *szProto)
{
    int i;
    
    DBDeleteContactSetting(NULL, PPICT_MODULE, szProto);
    
    if(szProto) {
        for(i = 0; i < g_protocount; i++) {
            if(g_ProtoPictures[i].szProtoname != NULL && !strcmp(g_ProtoPictures[i].szProtoname, szProto)) {
                if(g_ProtoPictures[i].lpDIBSection != 0 && ImgDeleteDIBSection)
                    ImgDeleteDIBSection(g_ProtoPictures[i].lpDIBSection);
                if(g_ProtoPictures[i].hbmPic != 0)
                    DeleteObject(g_ProtoPictures[i].hbmPic);
                ZeroMemory((void *)&g_ProtoPictures[i], sizeof(struct avatarCacheEntry));
                NotifyEventHooks(hEventChanged, 0, (LPARAM)&g_ProtoPictures[i]);
            }
        }
    }
}

static void SetProtoPic(char *szProto)
{
    char FileName[MAX_PATH];
    OPENFILENAMEA ofn={0};

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    if(g_imgDecoderAvail)
        ofn.lpstrFilter = "Image files\0*.gif;*.jpg;*.bmp;*.png\0\0";
    else
        ofn.lpstrFilter = "Image files\0*.gif;*.jpg;*.bmp\0\0";
    ofn.hwndOwner=0;
    ofn.lpstrFile = FileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.Flags=OFN_HIDEREADONLY;
    ofn.lpstrInitialDir = ".";
    *FileName = '\0';
    ofn.lpstrDefExt="";
    if (GetOpenFileNameA(&ofn)) {
        HANDLE hFile;
        
        if((hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
            return;
        else {
            char szNewPath[MAX_PATH];
            int i;
            
            CloseHandle(hFile);
            CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)FileName, (LPARAM)szNewPath);
            DBWriteContactSettingString(NULL, PPICT_MODULE, szProto, szNewPath);
            for(i = 0; i < g_protocount; i++) {
                if(lstrlenA(g_ProtoPictures[i].szProtoname) == 0)
                    break;
                if(!strcmp(g_ProtoPictures[i].szProtoname, szProto) && lstrlenA(g_ProtoPictures[i].szProtoname) == lstrlenA(szProto)) {
                    if(g_ProtoPictures[i].lpDIBSection && ImgDeleteDIBSection)
                        ImgDeleteDIBSection(g_ProtoPictures[i].lpDIBSection);
                    if(g_ProtoPictures[i].hbmPic != 0) 
                        DeleteObject(g_ProtoPictures[i].hbmPic);
                    ZeroMemory((void *)&g_ProtoPictures[i], sizeof(struct protoPicCacheEntry));
                    CreateAvatarInCache(0, (struct avatarCacheEntry *)&g_ProtoPictures[i], i, (char *)szProto);
                    strncpy(g_ProtoPictures[i].szProtoname, szProto, 100);
                    g_ProtoPictures[i].szProtoname[99] = 0;
                    NotifyEventHooks(hEventChanged, 0, (LPARAM)&g_ProtoPictures[i]);
                    break;
                }
            }
        }
    }
}

static char g_selectedProto[100];

BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hwndDlg, IDC_PROTOCOLS);
    HWND hwndChoosePic = GetDlgItem(hwndDlg, IDC_SETPROTOPIC);
    HWND hwndRemovePic = GetDlgItem(hwndDlg, IDC_REMOVEPROTOPIC);
    
    switch (msg) {
        case WM_INITDIALOG: {
            LVITEMA item = {0};
            LVCOLUMN lvc = {0};
            int i = 0, newItem = 0;
            
            dialoginit = TRUE;
            TranslateDialogDefault(hwndDlg);
            ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES);
            lvc.mask = LVCF_FMT;
            lvc.fmt = LVCFMT_IMAGE | LVCFMT_LEFT;
            ListView_InsertColumn(hwndList, 0, &lvc);
            
            item.mask = LVIF_TEXT;
            item.iItem = 1000;
            for(i = 0;;i++) {
                if(lstrlenA(g_ProtoPictures[i].szProtoname) == 0)
                    break;
                item.pszText = g_ProtoPictures[i].szProtoname;
				newItem = SendMessageA(hwndList, LVM_INSERTITEMA, 0, (LPARAM)&item);
                if(newItem >= 0)
                    ListView_SetCheckState(hwndList, newItem, DBGetContactSettingByte(NULL, AVS_MODULE, item.pszText, 1) ? TRUE : FALSE);
            }
            ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
            ListView_Arrange(hwndList, LVA_ALIGNLEFT | LVA_ALIGNTOP);
            EnableWindow(hwndChoosePic, FALSE);
            EnableWindow(hwndRemovePic, FALSE);
            SendDlgItemMessage(hwndDlg, IDC_SIZELIMITSPIN, UDM_SETRANGE, 0, MAKELONG(1024, 0));
            SendDlgItemMessage(hwndDlg, IDC_SIZELIMITSPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingDword(0, AVS_MODULE, "SizeLimit", 70));
            CheckDlgButton(hwndDlg, IDC_SHOWWARNINGS, DBGetContactSettingByte(0, AVS_MODULE, "warnings", 0));
            CheckDlgButton(hwndDlg, IDC_MAKE_TRANSPARENT_BKG, DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));

			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETRANGE, 0, MAKELONG(8, 2));
			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5));

			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));
			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgColorDiff", 10));

			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));

            dialoginit = FALSE;
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_SETPROTOPIC:
                case IDC_REMOVEPROTOPIC:
                {
                    LVITEMA item = {0};
                    int iItem = ListView_GetSelectionMark(hwndList);
                    char szTemp[100];
                    
                    szTemp[0] = 0;
                    
                    item.mask = LVIF_TEXT;
                    item.pszText = szTemp;
                    item.cchTextMax = 100;
                    item.iItem = iItem;
					SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item);
                    if(lstrlenA(szTemp) > 1) {
                        if(LOWORD(wParam) == IDC_SETPROTOPIC)
                            SetProtoPic(szTemp);
                        else
                            RemoveProtoPic(szTemp);
                        SendMessage(hwndList, WM_LBUTTONDOWN, 0, 0);
                    }
                    break;
                }
                case IDC_SIZELIMIT:
                    if(HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())
                        return 0;
                    else {
                        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                        break;
                    }
                default:
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            }
            break;
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

            if(dis->CtlType == ODT_BUTTON && dis->CtlID == IDC_PROTOPIC) {
                AVATARDRAWREQUEST avdrq = {0};
                avdrq.cbSize = sizeof(avdrq);
                avdrq.hTargetDC = dis->hDC;
                avdrq.dwFlags |= AVDRQ_PROTOPICT;
                avdrq.szProto = g_selectedProto;
                GetClientRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), &avdrq.rcDraw);
                CallService(MS_AV_DRAWAVATAR, 0, (LPARAM)&avdrq);
            }
            return TRUE;
        }
        case WM_NOTIFY:
            if(dialoginit)
                break;
            switch (((LPNMHDR) lParam)->idFrom) {
                case IDC_PROTOCOLS:
                    switch (((LPNMHDR) lParam)->code) {
                        case LVN_ITEMCHANGED:
                            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                            break;
                        case NM_CLICK:
                        {
                            int iItem = ListView_GetSelectionMark(hwndList);
                            LVITEMA item = {0};
                            DBVARIANT dbv = {0};
                            g_selectedProto[0] = 0;

                            item.mask = LVIF_TEXT;
                            item.pszText = g_selectedProto;
                            item.cchTextMax = 100;
                            item.iItem = iItem;
							SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item);
                            g_selectedProto[99] = 0;
                            EnableWindow(hwndChoosePic, TRUE);
                            EnableWindow(hwndRemovePic, TRUE);
                            if(!DBGetContactSetting(NULL, PPICT_MODULE, g_selectedProto, &dbv)) {
                                char szFinalPath[MAX_PATH];
                                
                                CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFinalPath);
                                SetWindowTextA(GetDlgItem(hwndDlg, IDC_PROTOAVATARNAME), szFinalPath);
                                InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
                                DBFreeVariant(&dbv);
                            }
                            else {
                                SetWindowTextA(GetDlgItem(hwndDlg, IDC_PROTOAVATARNAME), "");
                                InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
                            }
                            break;
                        }
                    }
                    break;
				case IDC_MAKE_TRANSPARENT_BKG:
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG));
					break;
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                        {
                            LVITEMA item = {0};
                            int i;
                            char szTemp[110];
                            
                            item.mask = LVIF_TEXT;
                            item.pszText = szTemp;
                            item.cchTextMax = 100;
                            for(i = 0; i < ListView_GetItemCount(hwndList); i++) {
                                item.iItem = i;
								SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item);
                                if(ListView_GetCheckState(hwndList, i))
                                    DBWriteContactSettingByte(NULL, AVS_MODULE, item.pszText, 1);
                                else
                                    DBWriteContactSettingByte(NULL, AVS_MODULE, item.pszText, 0);
                            }
                            DBWriteContactSettingDword(NULL, AVS_MODULE, "SizeLimit", SendDlgItemMessage(hwndDlg, IDC_SIZELIMITSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, AVS_MODULE, "warnings", IsDlgButtonChecked(hwndDlg, IDC_SHOWWARNINGS) ? 1 : 0);
                            DBWriteContactSettingByte(NULL, AVS_MODULE, "MakeTransparentBkg", IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG) ? 1 : 0);
							DBWriteContactSettingWord(NULL, AVS_MODULE, "TranspBkgNumPoints", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_GETPOS, 0, 0));
							DBWriteContactSettingWord(NULL, AVS_MODULE, "TranspBkgColorDiff", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_GETPOS, 0, 0));
                        }
                    }
            }
            break;
        case WM_DESTROY:
            break;
    }
    return FALSE;
}

BOOL CALLBACK DlgProcAvatarOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact = (HANDLE)GetWindowLong(hwndDlg, (LONG)GWL_USERDATA);

    switch(msg) {
        case WM_INITDIALOG:
        {
            TCHAR szTitle[512];
            TCHAR *szNick = NULL;
            
            SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
            hContact = (HANDLE)lParam;
            TranslateDialogDefault(hwndDlg);
            if(hContact) {
#if defined(_UNICODE)
                szNick = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_UNICODE);
#else
                szNick = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0);
#endif
                _sntprintf(szTitle, 500, TranslateT("Set avatar options for %s"), szNick);
                SetWindowText(hwndDlg, szTitle);
            }
            SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, FALSE);
            CheckDlgButton(hwndDlg, IDC_PROTECTAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0) ? TRUE : FALSE);
            CheckDlgButton(hwndDlg, IDC_HIDEAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Hidden", 0) ? TRUE : FALSE);
			CheckDlgButton(hwndDlg, IDC_MAKETRANSPBKG, DBGetContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", 1));
			EnableWindow(GetDlgItem(hwndDlg, IDC_MAKETRANSPBKG), DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));

			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETRANGE, 0, MAKELONG(8, 2));
			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", 5));

			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));
			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgColorDiff", 10));

			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));

            SendMessage(hwndDlg, WM_SETICON, IMAGE_ICON, (LPARAM)g_hIcon);
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    if(LOWORD(wParam) == IDOK) {
                        int hidden = IsDlgButtonChecked(hwndDlg, IDC_HIDEAVATAR) ? 1 : 0;

                        ProtectAvatar((WPARAM)hContact, IsDlgButtonChecked(hwndDlg, IDC_PROTECTAVATAR) ? 1 : 0);
                        SetAvatarAttribute(hContact, AVS_HIDEONCLIST, hidden);
                        if(hidden != DBGetContactSettingByte(hContact, "ContactPhoto", "Hidden", 0))
                            DBWriteContactSettingByte(hContact, "ContactPhoto", "Hidden", hidden);

						DBWriteContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG));
						DBWriteContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_GETPOS, 0, 0));
						DBWriteContactSettingWord(hContact, "ContactPhoto", "TranspBkgColorDiff", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_GETPOS, 0, 0));
						if(IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(NULL, AVS_MODULE, "MakeTransparentBkg", 0))
							SendMessage(hwndDlg, DM_REALODAVATAR, 0, 0);
                    }
                    DestroyWindow(hwndDlg);
                    break;
                case IDC_CHANGE:
				{
                    SetAvatar((WPARAM)hContact, 0);
                    SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
                    CheckDlgButton(hwndDlg, IDC_PROTECTAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0) ? TRUE : FALSE);
                    break;
				}
				case IDC_BKG_NUM_POINTS:
				case IDC_BKG_COLOR_DIFFERENCE:
					if (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())
						break;
				case IDC_MAKETRANSPBKG:
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG) && DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
                    SendMessage(hwndDlg, DM_REALODAVATAR, 0, 0);
                    break;
                case IDC_RESET:
                {
                    char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					DBVARIANT dbv = {0};

					EnterCriticalSection(&cachecs);
                    ProtectAvatar((WPARAM)hContact, 0);
					if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
						DeleteFileA(dbv.pszVal);
						DBFreeVariant(&dbv);
					}
                    DBDeleteContactSetting(hContact, "ContactPhoto", "File");
                    DBDeleteContactSetting(hContact, szProto, "AvatarHash");
                    DBDeleteContactSetting(hContact, szProto, "AvatarSaved");
					DeleteAvatar(hContact);
                    UpdateAvatar(hContact);
					LeaveCriticalSection(&cachecs);
                    DestroyWindow(hwndDlg);
                    break;
                }
                case IDC_DELETE:
					EnterCriticalSection(&cachecs);
                    ProtectAvatar((WPARAM)hContact, 0);
                    DBWriteContactSettingByte(hContact, "ContactPhoto", "Locked", 0);
                    DBDeleteContactSetting(hContact, "ContactPhoto", "File");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "RFile");
                    SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
                    DeleteAvatar(hContact);
					LeaveCriticalSection(&cachecs);
                    break;
            }
            break;
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

            if(dis->CtlType == ODT_BUTTON && dis->CtlID == IDC_PROTOPIC) {
                AVATARDRAWREQUEST avdrq = {0};
                avdrq.hContact = hContact;
                avdrq.cbSize = sizeof(avdrq);
                avdrq.hTargetDC = dis->hDC;
                avdrq.dwFlags |= AVDRQ_DRAWBORDER;
                avdrq.clrBorder = GetSysColor(COLOR_BTNTEXT);
                avdrq.radius = 6;
                GetClientRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), &avdrq.rcDraw);
                CallService(MS_AV_DRAWAVATAR, 0, (LPARAM)&avdrq);
            }
            return TRUE;
        }
        case DM_SETAVATARNAME:
        {
            char szFinalName[MAX_PATH];
            DBVARIANT dbv = {0};
            BYTE is_locked = DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0);

            szFinalName[0] = 0;
            
            if(is_locked && !DBGetContactSetting(hContact, "ContactPhoto", "Backup", &dbv)) {
                CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFinalName);
                DBFreeVariant(&dbv);
            }
            else if(!DBGetContactSetting(hContact, "ContactPhoto", "RFile", &dbv)) {
                CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFinalName);
                DBFreeVariant(&dbv);
            }
            else if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
                strncpy(szFinalName, dbv.pszVal, MAX_PATH);
                DBFreeVariant(&dbv);
            }
            szFinalName[MAX_PATH - 1] = 0;
            SetDlgItemTextA(hwndDlg, IDC_AVATARNAME, szFinalName);
            break;
        }
        case DM_REALODAVATAR:
        {
			DBWriteContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG));
			DBWriteContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_GETPOS, 0, 0));
			DBWriteContactSettingWord(hContact, "ContactPhoto", "TranspBkgColorDiff", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_GETPOS, 0, 0));

            ChangeAvatar(hContact);
			InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
            break;
        }
    }
    return FALSE;
}
