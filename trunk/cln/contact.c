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

extern struct CluiData g_CluiData;
extern int g_nextExtraCacheEntry;
extern struct ExtraCache *g_ExtraCache;
extern struct ClcData *g_clcData;

struct {
    int status, order;
} statusModeOrder[] = {
    {ID_STATUS_OFFLINE,500}, {ID_STATUS_ONLINE,0}, {ID_STATUS_AWAY,200}, {ID_STATUS_DND,400}, {ID_STATUS_NA,450}, {ID_STATUS_OCCUPIED,100}, {ID_STATUS_FREECHAT,50}, {ID_STATUS_INVISIBLE,20}, {ID_STATUS_ONTHEPHONE,150}, {ID_STATUS_OUTTOLUNCH,425}
};

static int GetContactStatus(HANDLE hContact)
{
    char *szProto;

    szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto == NULL)
        return ID_STATUS_OFFLINE;
    return DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
}

static int __forceinline GetStatusModeOrdering(int statusMode)
{
    int i;
    for (i = 0; i < sizeof(statusModeOrder) / sizeof(statusModeOrder[0]); i++) {
        if (statusModeOrder[i].status == statusMode)
            return statusModeOrder[i].order;
    }
    return 1000;
}

int mf_updatethread_running = TRUE;
HANDLE hThreadMFUpdate = 0;

static void MF_CalcFrequency(HANDLE hContact, DWORD dwCutoffDays, int doSleep)
{
    DWORD     curTime = time(NULL);
    DWORD     frequency, eventCount;
    DBEVENTINFO dbei = {0};
    HANDLE hEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0);
    DWORD firstEventTime = 0, lastEventTime = 0;

    eventCount = 0;
    dbei.cbSize = sizeof(dbei);
    dbei.timestamp = 0;

    while(hEvent) {
        dbei.cbBlob = 0;
        dbei.pBlob = NULL;
        CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbei);

        if(dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_SENT)) { // record time of last event
            eventCount++;
        }
        if(eventCount >= 100 || dbei.timestamp < curTime - (dwCutoffDays * 86400))
            break;
        hEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hEvent, 0);
        if(doSleep && mf_updatethread_running == FALSE)
            return;
        if(doSleep)
            Sleep(100);
    }

    if(eventCount == 0) {
        frequency = 0x7fffffff;
        DBWriteContactSettingDword(hContact, "CList", "mf_firstEvent", curTime - (dwCutoffDays * 86400));
    }
    else {
        frequency = (curTime - dbei.timestamp) / eventCount;
        DBWriteContactSettingDword(hContact, "CList", "mf_firstEvent", dbei.timestamp);
    }

    DBWriteContactSettingDword(hContact, "CList", "mf_freq", frequency);
    DBWriteContactSettingDword(hContact, "CList", "mf_count", eventCount);
}

DWORD WINAPI MF_UpdateThread(LPVOID p)
{
    HANDLE hContact;
    HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("mf_update_evt"));

    WaitForSingleObject(hEvent, 20000);

    while(mf_updatethread_running) {
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        while (hContact != NULL && mf_updatethread_running) {
            MF_CalcFrequency(hContact, 50, 1);
            if(mf_updatethread_running)
                WaitForSingleObject(hEvent, 5000);
            hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
        }
        if(mf_updatethread_running)
            WaitForSingleObject(hEvent, 1000000);
    }
    return 0;
}

static BOOL mc_hgh_removed = FALSE;

void LoadContactTree(void)
{
    HANDLE hContact;
    int i, status, hideOffline;
    BOOL mc_disablehgh = ServiceExists(MS_MC_DISABLEHIDDENGROUP);
    DBVARIANT dbv = {0};
    BYTE      bMsgFrequency = DBGetContactSettingByte(NULL, "CList", "fhistdata", 0);

    CallService(MS_CLUI_LISTBEGINREBUILD, 0, 0);
    for (i = 1; ; i++) {
        if (pcli->pfnGetGroupName(i, NULL) == NULL)
            break;
        CallService(MS_CLUI_GROUPADDED, i, 0);
    }

    hideOffline = DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT);
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact != NULL) {
        status = GetContactStatus(hContact);
        if ((!hideOffline || status != ID_STATUS_OFFLINE) && !CLVM_GetContactHiddenStatus(hContact, NULL, NULL))
            pcli->pfnChangeContactIcon(hContact, IconFromStatusMode((char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0), status, hContact, NULL), 1);

        if(mc_disablehgh && !mc_hgh_removed) {
            if(!DBGetContactSetting(hContact, "CList", "Group", &dbv)) {
                if(!strcmp(dbv.pszVal, "MetaContacts Hidden Group"))
                   DBDeleteContactSetting(hContact, "CList", "Group");
                mir_free(dbv.pszVal);
            }
        }

        // build initial data for message frequency
        if(!bMsgFrequency)
            MF_CalcFrequency(hContact, 100, 0);

        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
    DBWriteContactSettingByte(NULL, "CList", "fhistdata", 1);
    mc_hgh_removed = TRUE;
    CallService(MS_CLUI_SORTLIST, 0, 0);
    CallService(MS_CLUI_LISTENDREBUILD, 0, 0);
}

static int __forceinline GetProtoIndex(char * szName)
{
    DWORD i;
    char buf[11];
    char * name;
    DWORD pc;
    
    if (!szName) 
        return -1;
    
    pc=DBGetContactSettingDword(NULL,"Protocols","ProtoCount",-1);
    for (i=0; i<pc; i++) {
        _itoa(i,buf,10);
        name=DBGetString(NULL,"Protocols",buf);
        if (name) {
            if (!strcmp(name,szName)) {
                mir_free(name);
                return i;
            }
            mir_free(name);
        }
    }
    return -1;
}

DWORD __forceinline INTSORT_GetLastMsgTime(HANDLE hContact)
{
	HANDLE hDbEvent;
	DBEVENTINFO dbei = {0};

	if(g_CluiData.sortOrder[0] == SORTBY_LASTMSG || g_CluiData.sortOrder[1] == SORTBY_LASTMSG || g_CluiData.sortOrder[2] == SORTBY_LASTMSG) {
		hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0);

		dbei.cbSize = sizeof(dbei);
		dbei.pBlob = 0;
		dbei.cbBlob = 0;
		CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei);
		
		return dbei.timestamp;
	}
	return 0;
}

static int __forceinline INTSORT_CompareContacts(const struct ClcContact* c1, const struct ClcContact* c2, UINT bywhat)
{
    TCHAR *namea, *nameb;
    int statusa, statusb;
    char *szProto1, *szProto2;
    int rc;

    if (c1 == 0 || c2 == 0)
        return 0;

    szProto1 = c1->proto;
    szProto2 = c2->proto;
    statusa = c1->wStatus;
    statusb = c2->wStatus;
    // make sure, sticky contacts are always at the beginning of the group/list

    if ((c1->flags & CONTACTF_STICKY) != (c2->flags & CONTACTF_STICKY))
        return 2 * (c2->flags & CONTACTF_STICKY) - 1;


    if(bywhat == SORTBY_PRIOCONTACTS) {
        if((g_clcData->exStyle & CLS_EX_DIVIDERONOFF) && ((c1->flags & CONTACTF_ONLINE) != (c2->flags & CONTACTF_ONLINE)))
            return 0;
	    if ((c1->flags & CONTACTF_PRIORITY) != (c2->flags & CONTACTF_PRIORITY))
		    return 2 * (c2->flags & CONTACTF_PRIORITY) - 1;
		else
			return 0;
	}

	if (bywhat == SORTBY_STATUS) {
        int ordera, orderb;

        ordera = GetStatusModeOrdering(statusa);
        orderb = GetStatusModeOrdering(statusb);
        if (ordera != orderb)
            return ordera - orderb;
		else
			return 0;
    }

    // separate contacts treated as "offline"
	if(!g_CluiData.bDontSeparateOffline && ((statusa == ID_STATUS_OFFLINE) != (statusb == ID_STATUS_OFFLINE)))
	    return 2 * (statusa == ID_STATUS_OFFLINE) - 1;

	if(bywhat == SORTBY_NAME) {
		namea = (TCHAR *)c1->szText;
		nameb = (TCHAR *)c2->szText;
		return CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, namea, -1, nameb, -1) - 2;
	} else if(bywhat == SORTBY_LASTMSG) {
		if(c1->extraCacheEntry >= 0 && c1->extraCacheEntry < g_nextExtraCacheEntry && 
		   c2->extraCacheEntry >= 0 && c2->extraCacheEntry < g_nextExtraCacheEntry)
			return(g_ExtraCache[c2->extraCacheEntry].dwLastMsgTime - g_ExtraCache[c1->extraCacheEntry].dwLastMsgTime);
		else {
			DWORD timestamp1 = INTSORT_GetLastMsgTime(c1->hContact);
			DWORD timestamp2 = INTSORT_GetLastMsgTime(c2->hContact);
			return timestamp2 - timestamp1;
		}
    } else if(bywhat == SORTBY_FREQUENCY) {
        if(c1->extraCacheEntry >= 0 && c1->extraCacheEntry < g_nextExtraCacheEntry && 
           c2->extraCacheEntry >= 0 && c2->extraCacheEntry < g_nextExtraCacheEntry)
            return(g_ExtraCache[c1->extraCacheEntry].msgFrequency - g_ExtraCache[c2->extraCacheEntry].msgFrequency);
    }  else if(bywhat == SORTBY_PROTO) {
        if(c1->bIsMeta)
            szProto1 = c1->metaProto ? c1->metaProto : c1->proto;
        if(c2->bIsMeta)
            szProto2 = c2->metaProto ? c2->metaProto : c2->proto;

        rc = GetProtoIndex(szProto1) - GetProtoIndex(szProto2);

        if (rc != 0 && (szProto1 != NULL && szProto2 != NULL))
            return rc;
	}
	return 0;
}

int CompareContacts(const struct ClcContact* c1, const struct ClcContact* c2)
{
	int i, result;

	result = INTSORT_CompareContacts(c1, c2, SORTBY_PRIOCONTACTS);
	if(result)
		return result;

	for(i = 0; i <= 2; i++) {
		if(g_CluiData.sortOrder[i]) {
			result = INTSORT_CompareContacts(c1, c2, g_CluiData.sortOrder[i]);
			if(result != 0)
				return result;
		}
	}
	return 0;
}

#undef SAFESTRING

static int resortTimerId = 0;
static VOID CALLBACK SortContactsTimer(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
    KillTimer(NULL, resortTimerId);
    resortTimerId = 0;
    CallService(MS_CLUI_SORTLIST, 0, 0);
}

int SetHideOffline(WPARAM wParam, LPARAM lParam)
{
    switch ((int) wParam) {
        case 0:
            DBWriteContactSettingByte(NULL, "CList", "HideOffline", 0); break;
        case 1:
            DBWriteContactSettingByte(NULL, "CList", "HideOffline", 1); break;
        case -1:
            DBWriteContactSettingByte(NULL, "CList", "HideOffline", (BYTE) ! DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT)); break;
    }
    SetButtonStates(pcli->hwndContactList);
    LoadContactTree();
    return 0;
}
