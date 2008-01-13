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

#define MAXMODS 128

static	WNDPROC HistoryListSubClass;
extern	HANDLE	hWindowList;

static LRESULT CALLBACK HistoryListSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_KEYDOWN:
			if (wParam == VK_F2) {
				LVITEM lvi;
				HANDLE hContact = (HANDLE)GetWindowLong(hwnd, GWL_USERDATA);
				lvi.mask = LVIF_PARAM;
				lvi.iItem = ListView_GetSelectionMark(hwnd);
				lvi.iSubItem = 0;
				if (ListView_GetItem(hwnd, &lvi))
					editHistory(hContact, (HANDLE)lvi.lParam, hwnd);
			}
			break;

		default:
			break;
	}
	return CallWindowProc(HistoryListSubClass, hwnd, msg, wParam, lParam);
}

static int DialogResize(HWND hwnd, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch (urc->wId) {
		case IDC_HISTORY:
			urc->rcItem.top = 0;
			urc->rcItem.bottom = urc->dlgNewSize.cy;
			urc->rcItem.left = lParam;
			urc->rcItem.right = urc->dlgNewSize.cx;
			return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;
	}
	return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;

}
BOOL CALLBACK MainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HistoryWindowStruct *dat = (HistoryWindowStruct *)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg) {
		case WM_INITDIALOG: {
			HistoryWindowStruct *dat;
			int					i, iWidth;
			char				szTemp[255];
			TCHAR				tszTitle[256];

			WindowList_Add(hWindowList, hwnd, (HANDLE)lParam);
			
			dat = malloc(sizeof(HistoryWindowStruct));
			dat->hContact = (HANDLE)lParam;
			dat->hFirstEvent = dat->hLastEvent = 0;

			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconEx("core_main_10"));

			HistoryListSubClass = (WNDPROC)SetWindowLong(GetDlgItem(hwnd, IDC_HISTORY), GWL_WNDPROC, (LONG)HistoryListSubClassProc);
			setupHistoryList(GetDlgItem(hwnd, IDC_HISTORY));

			for(i = 0; i < COLUMN_COUNT; i++) {
				mir_snprintf(szTemp, sizeof(szTemp), "col%dWidth", i);
				iWidth = DBGetContactSettingDword(0, MODULE_NAME, szTemp, -1);
				if(iWidth != -1)
					ListView_SetColumnWidth(GetDlgItem(hwnd, IDC_HISTORY), i, iWidth);
			}
			TranslateDialogDefault(hwnd);
			SetWindowLong(hwnd, GWL_USERDATA, (LONG)dat);
			populateHistory(GetDlgItem(hwnd, IDC_HISTORY), dat->hContact);

			if (Utils_RestoreWindowPosition(hwnd, 0, MODULE_NAME, "hEdit")) {
				if (Utils_RestoreWindowPositionNoMove(hwnd, 0, MODULE_NAME, "hEdit"))
					if (Utils_RestoreWindowPosition(hwnd, 0, MODULE_NAME, "hEdit"))
						if (Utils_RestoreWindowPositionNoMove(hwnd, 0, MODULE_NAME, "hEdit"))
							SetWindowPos(hwnd, 0, 50, 50, 450, 300, SWP_NOZORDER | SWP_NOACTIVATE);
			}
			mir_sntprintf(tszTitle, 256, _T("Edit history for %s"), (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)dat->hContact, GCDNF_TCHAR));
			SetWindowText(hwnd, tszTitle);
			return TRUE;
		}

		case WM_GETMINMAXINFO: {
			MINMAXINFO *mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.y = 150;
			return 0;
		}

		case WM_SIZE: {
			UTILRESIZEDIALOG urd;
			ZeroMemory(&urd, sizeof(urd));
			urd.cbSize = sizeof(urd);
			urd.hInstance = hInst;
			urd.hwndDlg = hwnd;
			urd.lParam = 0;
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_MAIN);
			urd.pfnResizer = DialogResize;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
			break;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCANCEL:
				case MENU_EXIT:
					DestroyWindow(hwnd);
					break;
					///// menus
				case MENU_REFRESH_HISTORY:
					populateHistory(GetDlgItem(hwnd, IDC_HISTORY), (HANDLE)GetWindowLong(GetDlgItem(hwnd, IDC_HISTORY), GWL_USERDATA));
					break;
			}
			return TRUE;

		case WM_NOTIFY:
			switch (LOWORD(wParam)) {
				case IDC_HISTORY: {
					LVITEM lvi;
					LVHITTESTINFO hti;
					HANDLE hContact = (HANDLE)GetWindowLong(GetDlgItem(hwnd, IDC_HISTORY), GWL_USERDATA),  hDbEvent;
					DBEVENTINFO dbei = { 0 };
					int newBlobSize, oldBlobSize;

					hti.pt = ((NMLISTVIEW*)lParam)->ptAction;
					ListView_SubItemHitTest(GetDlgItem(hwnd, IDC_HISTORY), &hti);
					lvi.mask = LVIF_PARAM;
					lvi.iItem = hti.iItem;
					lvi.iSubItem = 0;
					if (ListView_GetItem(GetDlgItem(hwnd, IDC_HISTORY), &lvi)) {
						hDbEvent = (HANDLE)lvi.lParam;
						// get the event
						ZeroMemory(&dbei, sizeof(dbei));
						dbei.cbSize = sizeof(dbei);
						dbei.pBlob = NULL;
						oldBlobSize = 0;

						newBlobSize = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0);
						if (newBlobSize > oldBlobSize) {
							dbei.pBlob = (PBYTE)realloc(dbei.pBlob, newBlobSize);
							oldBlobSize = newBlobSize;
						}
						dbei.cbBlob = oldBlobSize;
						if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei)) break;
						switch (((NMHDR*)lParam)->code) {
							case NM_DBLCLK:
								if (dbei.eventType == EVENTTYPE_MESSAGE || dbei.eventType == EVENTTYPE_URL || dbei.eventType == EVENTTYPE_LOGGEDSTATUSCHANGE)
									editHistory(hContact, hDbEvent, GetDlgItem(hwnd, IDC_HISTORY));
								break;
							case NM_RCLICK: {
								HMENU hMenu = GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXTMENU)), 0);
								GetCursorPos(&(hti.pt));
								if (dbei.eventType != EVENTTYPE_MESSAGE && dbei.eventType != EVENTTYPE_URL && dbei.eventType != EVENTTYPE_LOGGEDSTATUSCHANGE)
									RemoveMenu(hMenu, 0, MF_BYPOSITION);
								switch (TrackPopupMenu(hMenu, TPM_RETURNCMD, hti.pt.x, hti.pt.y, 0, hwnd, NULL)) {
									case MENU_EDIT_ITEM:
										editHistory(hContact, hDbEvent, GetDlgItem(hwnd, IDC_HISTORY));
										break;
									case MENU_DELETE_ITEM:
										CallService(MS_DB_EVENT_DELETE, (WPARAM)hContact, (LPARAM)hDbEvent);
										ListView_DeleteItem(GetDlgItem(hwnd, IDC_HISTORY), lvi.iItem);
										break;
								}
							}
							break;

							case NM_CLICK: {
								if (hti.iSubItem == 3) { // un/read column
									if (dbei.flags & DBEF_READ) { // read
										CallService(MS_DB_EVENT_DELETE, (WPARAM)hContact, (LPARAM)hDbEvent);
										dbei.flags -= DBEF_READ;
										hDbEvent = (HANDLE)CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM) & dbei);
										if (hDbEvent) {
											lvi.mask = LVIF_IMAGE;
											lvi.iImage = 5;
											lvi.iItem = hti.iItem;
											lvi.iSubItem = 3;
											ListView_SetItem(GetDlgItem(hwnd, IDC_HISTORY), &lvi);
											lvi.mask = LVIF_PARAM;
											lvi.iItem = hti.iItem;
											lvi.iSubItem = 0;
											lvi.lParam = (LPARAM)hDbEvent;
											ListView_SetItem(GetDlgItem(hwnd, IDC_HISTORY), &lvi);
										}

									}
									else if (dbei.flags & DBEF_SENT)// was outgoing event
										msg(TranslateT("Cannot mark a outgoing event as Unread!"), _T(modFullname));
									else { // unread
										CallService(MS_DB_EVENT_MARKREAD, (WPARAM)hContact, (LPARAM)hDbEvent);
										CallService(MS_CLIST_REMOVEEVENT, (WPARAM)hContact, (LPARAM)hDbEvent);
										lvi.mask = LVIF_IMAGE;
										lvi.iImage = 4;
										lvi.iItem = hti.iItem;
										lvi.iSubItem = 3;
										ListView_SetItem(GetDlgItem(hwnd, IDC_HISTORY), &lvi);
									}

								}
							}
							break;
						}
						if (dbei.pBlob) free(dbei.pBlob);
					}
				}
				break;
			}
			return TRUE;

		case WM_DESTROY: {
			WINDOWPLACEMENT wp = {0};
			int				i;
			char			szTemp[255];

			wp.length = sizeof(wp);
			if (GetWindowPlacement(hwnd, &wp)) {
				DBWriteContactSettingDword(0, MODULE_NAME, "hEditx", wp.rcNormalPosition.left);
				DBWriteContactSettingDword(0, MODULE_NAME, "hEdity", wp.rcNormalPosition.top);
				DBWriteContactSettingDword(0, MODULE_NAME, "hEditwidth", wp.rcNormalPosition.right - wp.rcNormalPosition.left);
				DBWriteContactSettingDword(0, MODULE_NAME, "hEditheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
			}
			WindowList_Remove(hWindowList, hwnd);
			for(i = 0; i < COLUMN_COUNT; i++) {
				mir_snprintf(szTemp, 255, "col%dWidth", i);
				DBWriteContactSettingDword(0, MODULE_NAME, szTemp, ListView_GetColumnWidth(GetDlgItem(hwnd, IDC_HISTORY), i));
			}
			break;
		}

		case WM_NCDESTROY:
			if(dat)
				free(dat);
			break;
	}
	return 0;
}