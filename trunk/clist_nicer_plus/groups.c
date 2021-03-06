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

static int RenameGroup(WPARAM wParam, LPARAM lParam);
static int MoveGroupBefore(WPARAM wParam, LPARAM lParam);

static int CountGroups(void)
{
    DBVARIANT dbv;
    int i;
    char str[33];

    for (i = 0; ; i++) {
        _itoa(i, str, 10);
        if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
            break;
        DBFreeVariant(&dbv);
    }
    return i;
}

static int GroupNameExists(const TCHAR *name, int skipGroup)
{
    char idstr[33];
    DBVARIANT dbv;
    int i;

    for (i = 0; ; i++) {
        if (i == skipGroup)
            continue;
        _itoa(i, idstr, 10);
        if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
            break;
        if (!_tcscmp(dbv.ptszVal + 1, name)) {
            DBFreeVariant(&dbv);
            return 1;
        }
        DBFreeVariant(&dbv);
    }
    return 0;
}

static int CreateGroup(WPARAM wParam, LPARAM lParam)
{
    int newId = lParam ? lParam : CountGroups();
    TCHAR newBaseName[127], newName[128];
    char str[33];
    int i;
    DBVARIANT dbv;

    if (wParam) {
        _itoa(wParam - 1, str, 10);
        if (DBGetContactSettingTString(NULL, "CListGroups", str, &dbv))
            return 0;

        _sntprintf(newBaseName, safe_sizeof(newBaseName), _T("%s\\%s"), dbv.ptszVal + 1, TranslateT("New Group"));
        newBaseName[safe_sizeof(newBaseName) - 1] = 0;
        mir_free(dbv.ptszVal);
    } else
        lstrcpyn(newBaseName, TranslateT("New Group"), safe_sizeof(newBaseName));
    _itoa(newId, str, 10);
    i = 1;
    lstrcpyn(newName + 1, newBaseName, safe_sizeof(newName) - 1);
    while (GroupNameExists(newName + 1, -1)) {
        _sntprintf(newName + 1, safe_sizeof(newName) - 1, _T("%s (%d)"), newBaseName, ++i);
        newName[safe_sizeof(newName) - 1] = 0;
    }
    newName[0] = 1 | GROUPF_EXPANDED;  //1 is required so we never get '\0'
    DBWriteContactSettingTString(NULL, "CListGroups", str, newName);
    CallService(MS_CLUI_GROUPADDED, newId + 1, 1);
    return newId + 1;
}

static int GetGroupName2(WPARAM wParam, LPARAM lParam)
{
    char idstr[33];
    DBVARIANT dbv;
    static char name[128];

    _itoa(wParam - 1, idstr, 10);
    if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
        return(int) (char*) NULL;
    lstrcpynA(name, dbv.pszVal + 1, sizeof(name));
    if ((DWORD *) lParam != NULL)
        *(DWORD *) lParam = dbv.pszVal[0];
    DBFreeVariant(&dbv);
    return(int) name;
}

TCHAR * GetGroupNameT(int idx, DWORD *pdwFlags)
{
    char idstr[33];
    DBVARIANT dbv = {0};
    static TCHAR name[128];

	_itoa(idx - 1, idstr, 10);
    if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
        return NULL;

    lstrcpyn(name, dbv.ptszVal + 1, sizeof(name));
    if (pdwFlags != NULL) {                             
        *pdwFlags = dbv.ptszVal[0];
    }
    DBFreeVariant(&dbv);
    return name;
}

static int GetGroupName(WPARAM wParam, LPARAM lParam)
{
    int ret;
    ret = GetGroupName2(wParam, lParam);
    if ((int*) lParam)
        *(int*) lParam = 0 != (*(int*) lParam & GROUPF_EXPANDED);
    return ret;
}

static int DeleteGroup(WPARAM wParam, LPARAM lParam)
{
	int i;
	char str[33];
	DBVARIANT dbv;
	HANDLE hContact;
	TCHAR name[256], szNewParent[256], *pszLastBackslash;

	//get the name
	_itoa(wParam - 1, str, 10);
	if (DBGetContactSettingTString(NULL, "CListGroups", str, &dbv))
		return 1;
	if (DBGetContactSettingByte(NULL, "CList", "ConfirmDelete", SETTING_CONFIRMDELETE_DEFAULT))
		if (MessageBox((HWND)CallService(MS_CLUI_GETHWND, 0, 0), TranslateT("Are you sure you want to delete this group?  This operation can not be undone."), TranslateT("Delete Group"), MB_YESNO|MB_ICONQUESTION)==IDNO)
			return 1;
	lstrcpyn(name, dbv.ptszVal + 1, safe_sizeof(name));
	DBFreeVariant(&dbv);
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	//must remove setting from all child contacts too
	//children are demoted to the next group up, not deleted.
	lstrcpy(szNewParent, name);
	pszLastBackslash = _tcsrchr(szNewParent, '\\');
	if (pszLastBackslash)
		pszLastBackslash[0] = '\0';
	else
		szNewParent[0] = '\0';
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	do {
		if (DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
			continue;
		if (_tcscmp(dbv.ptszVal, name)) {
			DBFreeVariant(&dbv);
			continue;
		}
		DBFreeVariant(&dbv);
		if (szNewParent[0])
			DBWriteContactSettingTString(hContact, "CList", "Group", szNewParent);
		else
			DBDeleteContactSetting(hContact, "CList", "Group");
	} while ((hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)) != NULL);
	//shuffle list of groups up to fill gap
	for (i = wParam - 1;; i++) {
		_itoa(i + 1, str, 10);
		if (DBGetContactSettingStringUtf(NULL, "CListGroups", str, &dbv))
			break;
		_itoa(i, str, 10);
		DBWriteContactSettingStringUtf(NULL, "CListGroups", str, dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	_itoa(i, str, 10);
	DBDeleteContactSetting(NULL, "CListGroups", str);
	//rename subgroups
	{
		TCHAR szNewName[256];
		int len;

		len = lstrlen(name);
		for (i = 0;; i++) {
			_itoa(i, str, 10);
			if (DBGetContactSettingTString(NULL, "CListGroups", str, &dbv))
				break;
			if (!_tcsncmp(dbv.ptszVal + 1, name, len) && dbv.ptszVal[len + 1] == '\\' && _tcschr(dbv.ptszVal + len + 2, '\\') == NULL) {
				if (szNewParent[0])
					mir_sntprintf(szNewName, safe_sizeof(szNewName), _T("%s\\%s"), szNewParent, dbv.ptszVal + len + 2);
				else
					lstrcpyn(szNewName, dbv.ptszVal + len + 2, safe_sizeof(szNewName));
				RenameGroupT(i + 1, szNewName);
			}
			DBFreeVariant(&dbv);
		}
	}
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	LoadContactTree();
	return 0;
}

static int RenameGroupWithMove(int groupId, const TCHAR *szName, int move)
{
    char idstr[33];
    TCHAR str[256], oldName[256]; 
    DBVARIANT dbv;
    HANDLE hContact;

    if (GroupNameExists(szName, groupId)) {
        MessageBox(NULL, TranslateT("You already have a group with that name. Please enter a unique name for the group."), 
                   TranslateT("Rename Group"), MB_OK);
        return 1;
    }

    //do the change
    _itoa(groupId, idstr, 10);
    if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
        return 1;
    str[0] = dbv.pszVal[0];
    lstrcpyn(oldName, dbv.ptszVal + 1, safe_sizeof(oldName));
    DBFreeVariant(&dbv);
    lstrcpyn(str + 1, szName, safe_sizeof(str) - 1);
    DBWriteContactSettingTString(NULL, "CListGroups", idstr, str);

    //must rename setting in all child contacts too
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    do {
        if (DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
            continue;
        if (_tcscmp(dbv.ptszVal, oldName))
            continue;
        DBWriteContactSettingTString(hContact, "CList", "Group", szName);
    } while ((hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)) != NULL);

    //rename subgroups
    {
        TCHAR szNewName[256];
        int len, i;

        len = lstrlen(oldName);
        for (i = 0; ; i++) {
            if (i == groupId)
                continue;
            _itoa(i, idstr, 10);
            if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
                break;
            if (!_tcsncmp(dbv.ptszVal + 1, oldName, len) && dbv.ptszVal[len + 1] == '\\' && _tcschr(dbv.ptszVal + len + 2, '\\') == NULL) {
                _sntprintf(szNewName, safe_sizeof(szNewName), _T("%s\\%s"), szName, dbv.ptszVal + len + 2);
                RenameGroupWithMove(i, szNewName, 0);   //luckily, child groups will never need reordering
            }
            DBFreeVariant(&dbv);
        }
    }

    //finally must make sure it's after any parent items
    if (move) {
        TCHAR *pszLastBackslash;
        int i;

        lstrcpyn(str, szName, safe_sizeof(str));
        pszLastBackslash = _tcsrchr(str, '\\');
        if (pszLastBackslash == NULL)
            return 0;
        *pszLastBackslash = '\0';
        for (i = 0; ; i++) {
            _itoa(i, idstr, 10);
            if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
                break;
            if (!lstrcmp(dbv.ptszVal + 1, str)) {
                if (i < groupId)
                    break;      //is OK
                MoveGroupBefore(groupId + 1, i + 2);
                break;
            }
            DBFreeVariant(&dbv);
        }
    }
    return 0;
}

int RenameGroupT( int groupID, TCHAR* newName )
{
	return -1 != RenameGroupWithMove( groupID-1, newName, 1);
}

static int RenameGroup(WPARAM wParam, LPARAM lParam)
{
	#if defined( _UNICODE )
		WCHAR* temp = a2u(( char* )lParam );
		int result = ( -1 != RenameGroupWithMove(wParam - 1, temp, 1));
		free( temp );
		return result;
	#else
		return -1 != RenameGroupWithMove(wParam - 1, (TCHAR*) lParam, 1);
	#endif
}

static int SetGroupExpandedState(WPARAM wParam, LPARAM lParam)
{
	char idstr[33];
	DBVARIANT dbv;

	_itoa(wParam - 1, idstr, 10);
	if (DBGetContactSettingStringUtf(NULL, "CListGroups", idstr, &dbv))
		return 1;
	if (lParam)
		dbv.pszVal[0] |= GROUPF_EXPANDED;
	else
		dbv.pszVal[0] = dbv.pszVal[0] & ~GROUPF_EXPANDED;
	DBWriteContactSettingStringUtf(NULL, "CListGroups", idstr, dbv.pszVal);
	DBFreeVariant(&dbv);
	return 0;
}

static int SetGroupFlags(WPARAM wParam, LPARAM lParam)
{
	char idstr[33];
	DBVARIANT dbv;
	int flags, oldval, newval;

	_itoa(wParam - 1, idstr, 10);
	if (DBGetContactSettingStringUtf(NULL, "CListGroups", idstr, &dbv))
		return 1;
	flags = LOWORD(lParam) & HIWORD(lParam);
	oldval = dbv.pszVal[0];
	newval = dbv.pszVal[0] = (dbv.pszVal[0] & ~HIWORD(lParam)) | flags;
	DBWriteContactSettingStringUtf(NULL, "CListGroups", idstr, dbv.pszVal);
	DBFreeVariant(&dbv);
	if ((oldval & GROUPF_HIDEOFFLINE) != (newval & GROUPF_HIDEOFFLINE))
		LoadContactTree();
	return 0;
}

static int MoveGroupBefore(WPARAM wParam, LPARAM lParam)
{
	int i, shuffleFrom, shuffleTo, shuffleDir;
	char str[33];
	TCHAR *szMoveName;
	DBVARIANT dbv;

	if (wParam == 0 || (LPARAM) wParam == lParam)
		return 0;
	_itoa(wParam - 1, str, 10);
	if (DBGetContactSettingTString(NULL, "CListGroups", str, &dbv))
		return 0;
	szMoveName = dbv.ptszVal;
	//shuffle list of groups up to fill gap
	if (lParam == 0) {
		shuffleFrom = wParam - 1;
		shuffleTo = -1;
		shuffleDir = -1;
	}
	else {
		if ((LPARAM) wParam < lParam) {
			shuffleFrom = wParam - 1;
			shuffleTo = lParam - 2;
			shuffleDir = -1;
		}
		else {
			shuffleFrom = wParam - 1;
			shuffleTo = lParam - 1;
			shuffleDir = 1;
		}
	}
	if (shuffleDir == -1) {
		for (i = shuffleFrom; i != shuffleTo; i++) {
			_itoa(i + 1, str, 10);
			if (DBGetContactSettingStringUtf(NULL, "CListGroups", str, &dbv)) {
				shuffleTo = i;
				break;
			}
			_itoa(i, str, 10);
			DBWriteContactSettingStringUtf(NULL, "CListGroups", str, dbv.pszVal);
			DBFreeVariant(&dbv);
		}
	}
	else {
		for (i = shuffleFrom; i != shuffleTo; i--) {
			_itoa(i - 1, str, 10);
			if (DBGetContactSettingStringUtf(NULL, "CListGroups", str, &dbv)) {
				mir_free(szMoveName);
				return 1;
			}                   //never happens
			_itoa(i, str, 10);
			DBWriteContactSettingStringUtf(NULL, "CListGroups", str, dbv.pszVal);
			DBFreeVariant(&dbv);
		}
	}
	_itoa(shuffleTo, str, 10);
	DBWriteContactSettingTString(NULL, "CListGroups", str, szMoveName);
	mir_free(szMoveName);
	return shuffleTo + 1;
}

static int BuildGroupMenu(WPARAM wParam, LPARAM lParam)
{
	char idstr[33];
	DBVARIANT dbv;
	int groupId;
	HMENU hRootMenu, hThisMenu;
	int nextMenuId = 100;
	TCHAR *pBackslash, *pNextField, szThisField[128], szThisMenuItem[128];
	int menuId, compareResult, menuItemCount;
	MENUITEMINFO mii = { 0 };

	if (DBGetContactSettingStringUtf(NULL, "CListGroups", "0", &dbv))
		return (int) (HMENU) NULL;
	DBFreeVariant(&dbv);
	hRootMenu = CreateMenu();
	for (groupId = 0;; groupId++) {
		_itoa(groupId, idstr, 10);
		if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
			break;

		pNextField = dbv.ptszVal + 1;
		hThisMenu = hRootMenu;
		mii.cbSize = MENUITEMINFO_V4_SIZE;
		do {
			pBackslash = _tcschr(pNextField, '\\');
			if (pBackslash == NULL) {
				lstrcpyn(szThisField, pNextField, safe_sizeof(szThisField));
				pNextField = NULL;
			}
			else {
				lstrcpyn(szThisField, pNextField, min(safe_sizeof(szThisField), pBackslash - pNextField + 1));
				pNextField = pBackslash + 1;
			}
			compareResult = 1;
			menuItemCount = GetMenuItemCount(hThisMenu);
			for (menuId = 0; menuId < menuItemCount; menuId++) {
				mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_DATA;
				mii.cch = safe_sizeof(szThisMenuItem);
				mii.dwTypeData = szThisMenuItem;
				GetMenuItemInfo(hThisMenu, menuId, TRUE, &mii);
				compareResult = lstrcmp(szThisField, szThisMenuItem);
				if (compareResult == 0) {
					if (pNextField == NULL) {
						mii.fMask = MIIM_DATA;
						mii.dwItemData = groupId + 1;
						SetMenuItemInfo(hThisMenu, menuId, TRUE, &mii);
					}
					else {
						if (mii.hSubMenu == NULL) {
							mii.fMask = MIIM_SUBMENU;
							mii.hSubMenu = CreateMenu();
							SetMenuItemInfo(hThisMenu, menuId, TRUE, &mii);
							mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
							//dwItemData doesn't change
							mii.fType = MFT_STRING;
							mii.dwTypeData = TranslateT("This group");
							mii.wID = nextMenuId++;
							InsertMenuItem(mii.hSubMenu, 0, TRUE, &mii);
							mii.fMask = MIIM_TYPE;
							mii.fType = MFT_SEPARATOR;
							InsertMenuItem(mii.hSubMenu, 1, TRUE, &mii);
						}
						hThisMenu = mii.hSubMenu;
					}
					break;
				}
				if ((int) mii.dwItemData - 1 > groupId)
					break;
			}
			if (compareResult) {
				mii.fMask = MIIM_TYPE | MIIM_ID;
				mii.wID = nextMenuId++;
				mii.dwTypeData = szThisField;
				mii.fType = MFT_STRING;
				if (pNextField) {
					mii.fMask |= MIIM_SUBMENU;
					mii.hSubMenu = CreateMenu();
				}
				else {
					mii.fMask |= MIIM_DATA;
					mii.dwItemData = groupId + 1;
				}
				InsertMenuItem(hThisMenu, menuId, TRUE, &mii);
				if (pNextField) {
					hThisMenu = mii.hSubMenu;
				}
			}
		} while (pNextField);

		DBFreeVariant(&dbv);
	}
	return (int) hRootMenu;
}

int InitGroupServices(void)
{
    CreateServiceFunction(MS_CLIST_GROUPCREATE, CreateGroup);
    CreateServiceFunction(MS_CLIST_GROUPDELETE, DeleteGroup);
    CreateServiceFunction(MS_CLIST_GROUPRENAME, RenameGroup);
    CreateServiceFunction(MS_CLIST_GROUPGETNAME, GetGroupName);
    CreateServiceFunction(MS_CLIST_GROUPGETNAME2, GetGroupName2);
    CreateServiceFunction(MS_CLIST_GROUPSETEXPANDED, SetGroupExpandedState);
    CreateServiceFunction(MS_CLIST_GROUPSETFLAGS, SetGroupFlags);
    CreateServiceFunction(MS_CLIST_GROUPMOVEBEFORE, MoveGroupBefore);
    CreateServiceFunction(MS_CLIST_GROUPBUILDMENU, BuildGroupMenu);
    return 0;
}