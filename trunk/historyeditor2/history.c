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

void setupHistoryList(HWND hwnd2List)
{
	int index = 1;
	LVCOLUMN sLC;
	sLC.fmt = LVCFMT_LEFT;
	ListView_SetExtendedListViewStyle(hwnd2List, 32 | LVS_EX_SUBITEMIMAGES); //LVS_EX_FULLROWSELECT
	sLC.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;

	sLC.pszText = TranslateT("Timestamp");
	sLC.cx = 130;
	ListView_InsertColumn(hwnd2List, 0, &sLC);
	sLC.pszText = TranslateT("Message");
	sLC.cx = 200;
	ListView_InsertColumn(hwnd2List, 1, &sLC);
	sLC.pszText = TranslateT("In/Out");
	sLC.cx = 50;
	ListView_InsertColumn(hwnd2List, 2, &sLC);
	sLC.pszText = TranslateT("Read");
	sLC.cx = 40;
	ListView_InsertColumn(hwnd2List, 3, &sLC);
}

static void cleanText(TCHAR *text)
{
	int i = 0;
	while (text[i] != '\0') {
		if (text[i] < 32)
			text[i] = ' ';
		i++;
	}
}

static int getEventText(DBEVENTINFO *dbei, TCHAR *str, int sizeOfStr)
{
	switch (dbei->eventType) {
		case EVENTTYPE_MESSAGE:
		case EVENTTYPE_URL:
		case EVENTTYPE_LOGGEDSTATUSCHANGE: {
			TCHAR *msg = DbGetEventTextT(dbei, CP_ACP);

			//cleanText(msg);
			lstrcpyn(str, msg, sizeOfStr);
			str[sizeOfStr - 1] = 0;
			mir_free(msg);
			break;
		}
		case EVENTTYPE_AUTHREQUEST: {
			int uin = *((PDWORD)(dbei->pBlob));
			if (uin)
				mir_sntprintf(str, sizeOfStr, TranslateT("%u requests authorization"), uin);
			else
				mir_sntprintf(str, sizeOfStr, TranslateT("(Unknown) requests authorization"));
		}
		break;
		case EVENTTYPE_ADDED: {
			int uin = *((PDWORD)(dbei->pBlob));
			if (uin)
				mir_sntprintf(str, sizeOfStr, TranslateT("%u Added You"), uin);
			else
				mir_sntprintf(str, sizeOfStr, TranslateT("(Unknown) Added You"));
		}
		break;
		case EVENTTYPE_FILE: {
			char *temp = dbei->pBlob + sizeof(DWORD);
			temp[strlen(temp)] = ' ';
#if defined(_UNICODE)
			MultiByteToWideChar(CP_ACP, 0, temp, sizeOfStr, str, sizeOfStr);
			str[sizeOfStr - 1] = 0;
#else
			strncpy(str, temp, sizeOfStr);
			str[sizeOfStr - 1] = 0;
#endif
		}
		break;
		default:
			lstrcpyn(str, TranslateT("Unknown Event"), sizeOfStr);
			return 0;
			break;
	}
	return 1;
}

void populateHistory(HWND hwnd2List, HANDLE hContact)
{
	HIMAGELIST	himl;
	HICON		hIcon;
	LVITEM		lvItem;
	int			index;
	TCHAR		time[64], text[4096];
	HANDLE		hDbEvent;
	DBEVENTINFO dbei = { 0 };
	DBTIMETOSTRINGT dbtts;
	int			newBlobSize, oldBlobSize;

	// clear any settings that may be there...
	ListView_DeleteAllItems(hwnd2List);

	SetWindowLong(hwnd2List, GWL_USERDATA, (LONG)hContact);

	//image list
	if ((himl = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 5, 0)) == NULL) {
		msg(TranslateT("Couldnt create the image list..."), _T(modFullname));
		return;
	}
	hIcon = LoadIconEx("core_main_10");
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIconEx("core_main_1");
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIconEx("core_main_3");
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIconEx("core-main_2");
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIconEx("core_main_19");
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIconEx("core_main_20");
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIconEx("core_main_4");
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_AUTH)); // 7
	ImageList_AddIcon(himl, hIcon);

	hIcon = LoadIconEx("core_main_8");
	ImageList_AddIcon(himl, hIcon);

	ListView_SetImageList(hwnd2List, himl, LVSIL_SMALL);

	for (hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0);
			hDbEvent;
			hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hDbEvent, 0), free(dbei.pBlob)) {
		lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvItem.iItem = 0;
		lvItem.iSubItem = 0;
		lvItem.lParam = (LPARAM)hDbEvent;
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
		// timestamp
		dbtts.cbDest = SIZEOF(time);
		dbtts.szDest = time;
		dbtts.szFormat = _T("d t");
		CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dbei.timestamp, (LPARAM)&dbtts);
		lvItem.pszText = time;

		switch (dbei.eventType) {
			case EVENTTYPE_MESSAGE:
				lvItem.iImage = 1;
				break;
			case EVENTTYPE_URL:
				lvItem.iImage = 3;
				break;
			case EVENTTYPE_LOGGEDSTATUSCHANGE:
				lvItem.iImage = 6;
				break;
			case EVENTTYPE_AUTHREQUEST:
				lvItem.iImage = 7;
				break;
			case EVENTTYPE_ADDED:
				lvItem.iImage = 8;
				break;
			case EVENTTYPE_FILE:
				lvItem.iImage = 2;
				break;
			default:
				continue;
		}
		getEventText(&dbei, text, SIZEOF(text));
		index = ListView_InsertItem(hwnd2List, &lvItem);
		ListView_SetItemText(hwnd2List, index, 1, text);

		// un/read
		lvItem.mask = LVIF_IMAGE;
		if (dbei.flags & (DBEF_READ | DBEF_SENT)) 
			lvItem.iImage = 4;
		else 
			lvItem.iImage = 5;
		lvItem.iItem = index;
		lvItem.iSubItem = 3;
		ListView_SetItem(hwnd2List, &lvItem);
		// in/out
		ListView_SetItemText(hwnd2List, index, 2, (dbei.flags & DBEF_SENT) ? TranslateT("Out") : TranslateT("In"));

	}
}

BOOL CALLBACK EditHistoryDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG: {
			EditWindowStruct *ews = (EditWindowStruct*)lParam;
			DBEVENTINFO dbei = { 0 };
			TCHAR		*msg;
			int			iBlobSize;

			TranslateDialogDefault(hwnd);
			SetWindowLong(hwnd, GWL_USERDATA, (LONG)ews);
			// get the event
			ZeroMemory(&dbei, sizeof(dbei));
			dbei.cbSize = sizeof(dbei);
	
			iBlobSize = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)ews->hDbEvent, 0);
			dbei.pBlob = malloc(iBlobSize);
			dbei.cbBlob = iBlobSize;
			if (CallService(MS_DB_EVENT_GET, (WPARAM)ews->hDbEvent, (LPARAM)&dbei)) {
				free(dbei.pBlob);
				return FALSE;
			}
			msg = DbGetEventTextT(&dbei, CP_ACP);
			SetDlgItemText(hwnd, IDC_EDIT1, msg);
			mir_free(msg);
			CheckDlgButton(hwnd, IDC_CHECK1, dbei.flags&(DBEF_READ | DBEF_SENT));
			if (dbei.flags & DBEF_SENT)
				EnableWindow(GetDlgItem(hwnd, IDC_CHECK1), 0);
			free(dbei.pBlob);
		}
		return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					EditWindowStruct *ews = (EditWindowStruct*)GetWindowLong(hwnd, GWL_USERDATA);
					DBEVENTINFO dbei = { 0 };
					int newBlobSize, oldBlobSize;
					TCHAR *msg, *oldMsg;

					newBlobSize = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT1));
					if (newBlobSize) {
						msg = (TCHAR *)malloc(sizeof(TCHAR) * (newBlobSize + 1));
						GetDlgItemText(hwnd, IDC_EDIT1, msg, newBlobSize + 1);
						// get the event
						ZeroMemory(&dbei, sizeof(dbei));
						dbei.cbSize = sizeof(dbei);
						dbei.pBlob = NULL;
						oldBlobSize = 0;

						oldBlobSize = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)ews->hDbEvent, 0);
						dbei.pBlob = (PBYTE)malloc(oldBlobSize);
						dbei.cbBlob = oldBlobSize;

						if (CallService(MS_DB_EVENT_GET, (WPARAM)ews->hDbEvent, (LPARAM)&dbei)) {
							free(dbei.pBlob);
							free(msg);
							return FALSE;
						}
						oldMsg = DbGetEventTextT(&dbei, CP_ACP);

						if (lstrcmp(msg, oldMsg)) { // message changed so delete and reinsert the message
#if defined(_UNICODE)
							char *szUtf = mir_utf8encodeW(msg);
							dbei.pBlob = szUtf;
							dbei.cbBlob = lstrlenA(szUtf) + 1;
#else
							dbei.pBlob = msg;
							dbei.cbBlob = lstrlen(msg) + 1;
#endif
							free(dbei.pBlob);
							free(oldMsg);
							if (!(dbei.flags & (DBEF_READ | DBEF_SENT)))
								CallService(MS_CLIST_REMOVEEVENT, (WPARAM)ews->hContact, (LPARAM)ews->hDbEvent);
							if (IsDlgButtonChecked(hwnd, IDC_CHECK1)) 
								dbei.flags |= DBEF_READ;
							else 
								dbei.flags &= (DBEF_SENT | DBEF_FIRST);
#if defined(_UNICODE)
							dbei.flags |= DBEF_UTF;
#endif
							CallService(MS_DB_EVENT_DELETE, (WPARAM)ews->hContact, (LPARAM)ews->hDbEvent);
#if defined(_UNICODE)
							mir_free(szUtf);
#endif
							ews->hDbEvent = (HANDLE)CallService(MS_DB_EVENT_ADD, (WPARAM)ews->hContact, (LPARAM) & dbei);
							if (ews->hwndOwner && ((HANDLE)GetWindowLong(GetDlgItem(ews->hwndOwner, IDC_HISTORY), GWL_USERDATA) == ews->hContact))
								populateHistory(GetDlgItem(ews->hwndOwner, IDC_HISTORY), ews->hContact);
						}
						else {
							free(dbei.pBlob);
							free(oldMsg);
						}
						free(msg);
					}
				}
				case IDCANCEL: {
					free((EditWindowStruct*)GetWindowLong(hwnd, GWL_USERDATA));
					DestroyWindow(hwnd);
				}
				break;
			}
			break;
	}
	return 0;
}

void editHistory(HANDLE hContact, HANDLE hDbEvent, HWND hwndParent)
{
	EditWindowStruct *ews = (EditWindowStruct*)malloc(sizeof(EditWindowStruct));
	if (!ews) return;
	ews->hContact = hContact;
	ews->hDbEvent = hDbEvent;
	ews->hwndOwner = hwndParent;
	CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_HISTORYEDIT), 0, EditHistoryDlgProc, (LPARAM)ews);
}

void deleteAllHistory(HANDLE hContact)
{
	HANDLE hDbEvent;
	hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0);
	while (hDbEvent) {
		CallService(MS_DB_EVENT_DELETE, (WPARAM)hContact, (LPARAM)hDbEvent);
		hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0);
	}
	msg(TranslateT("Remember to run DBTool to shrink your profile as it will have wasted space now!"), _T(modFullname));
}
static int Openfile(TCHAR *outputFile, int xml)
{
	OPENFILENAME ofn;
	TCHAR filename[MAX_PATH] = _T("");
	TCHAR *filter = xml ? _T("XML Files\0*.xml\0") : _T("Text Files\0*.txt\0");
	int r;
	TCHAR *title = TranslateT("Export to file");

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = filename;
	ofn.lpstrFilter = filter;
	ofn.Flags = OFN_HIDEREADONLY | OFN_SHAREAWARE | OFN_PATHMUSTEXIST;
	ofn.lpstrTitle = title;
	ofn.nMaxFile = MAX_PATH;

	r = GetSaveFileName((LPOPENFILENAME) & ofn);
	if (!r)
		return 0;
	lstrcpy(outputFile, filename);
	return 1;
}

void copyHistory(HANDLE hContactFrom, HANDLE hContactTo)
{
	HANDLE hDbEvent;
	DBEVENTINFO dbei = { 0 };
	int newBlobSize, oldBlobSize;

	if (!hContactFrom || !hContactTo) {
		msg(_T("error!"), _T(modFullname));
		return;
	}
	for (hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDFIRST, (WPARAM)hContactFrom, 0);
			hDbEvent;
			hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hDbEvent, 0), free(dbei.pBlob)) {
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
		CallService(MS_DB_EVENT_ADD, (WPARAM)hContactTo, (LPARAM)&dbei);
	}
}
