/* 
Copyright (C) 2006 Ricardo Pescuma Domenecci

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "commonheaders.h"


/*
It has 2 queues:

1. A queue to request items. One request is done at a time, REQUEST_WAIT_TIME miliseconts after it has beeing fired
   ACKRESULT_STATUS. This thread only requests the avatar (and maybe add it to the cache queue)

2. A queue to cache items. It will cache each item CACHE_WAIT_TIME miliseconds after it has beeing added to the list.
   This is done because some protos change the value of the "File" db setting some times before the final one.
   This is fired by any change in the db key ContactPhoto\File
*/

#define REQUEST_WAIT_TIME 3000
#define CACHE_WAIT_TIME 2000

// Number of mileseconds the threads wait until take a look if it is time to request another item
#define POOL_DELAY 1000

// Number of mileseconds the threads wait after a GAIR_WAITFOR is returned
#define REQUEST_DELAY 6000


// Prototypes ///////////////////////////////////////////////////////////////////////////

ThreadQueue requestQueue = {0};
ThreadQueue cacheQueue = {0};

DWORD WINAPI RequestThread(LPVOID vParam);
DWORD WINAPI CacheThread(LPVOID vParam);


extern char *g_szMetaName;
extern int ChangeAvatar(HANDLE hContact);
extern CacheNode *GetContactNode(HANDLE hContact, int needToLock);

#ifdef _DEBUG
int _DebugTrace(const char *fmt, ...);
int _DebugTrace(HANDLE hContact, const char *fmt, ...);
#endif


// Functions ////////////////////////////////////////////////////////////////////////////


// Itens with higher priority at end
int QueueSortItems(void *i1, void *i2)
{
	return ((QueueItem*)i2)->check_time - ((QueueItem*)i1)->check_time;
}


void InitPolls() 
{
	// Init request queue
	ZeroMemory(&requestQueue, sizeof(requestQueue));
	requestQueue.queue = List_Create(0, 20);
	requestQueue.queue->sortFunc = QueueSortItems;
	requestQueue.bThreadRunning = TRUE;
    InitializeCriticalSection(&requestQueue.cs);
	requestQueue.hThread = CreateThread(NULL, 16000, RequestThread, NULL, 0, &requestQueue.dwThreadID);
	mir_sntprintf(requestQueue.eventName, 128, _T("evt_avscache_request_%d"), requestQueue.dwThreadID);
	requestQueue.hEvent = CreateEvent(NULL, TRUE, FALSE, requestQueue.eventName);
	requestQueue.waitTime = REQUEST_WAIT_TIME;

	// Init cache queue
	ZeroMemory(&cacheQueue, sizeof(cacheQueue));
	cacheQueue.queue = List_Create(0, 20);
	cacheQueue.queue->sortFunc = QueueSortItems;
	cacheQueue.bThreadRunning = TRUE;
	InitializeCriticalSection(&cacheQueue.cs);
	cacheQueue.hThread = CreateThread(NULL, 16000, CacheThread, NULL, 0, &cacheQueue.dwThreadID);
	mir_sntprintf(cacheQueue.eventName, 128, _T("evt_avscache_cache_%d"), cacheQueue.dwThreadID);
	cacheQueue.hEvent = CreateEvent(NULL, TRUE, FALSE, cacheQueue.eventName);
	cacheQueue.waitTime = CACHE_WAIT_TIME;
}


void FreePolls()
{
	// Stop request queue
    requestQueue.bThreadRunning = FALSE;
    SetEvent(requestQueue.hEvent);
	ResumeThread(requestQueue.hThread);
    WaitForSingleObject(requestQueue.hThread, 3000);
    CloseHandle(requestQueue.hThread);
    CloseHandle(requestQueue.hEvent);
	DeleteCriticalSection(&requestQueue.cs);
	List_DestroyFreeContents(requestQueue.queue);
	mir_free(requestQueue.queue);

	// Stop cache queue
    cacheQueue.bThreadRunning = FALSE;
    SetEvent(cacheQueue.hEvent);
	ResumeThread(cacheQueue.hThread);
    WaitForSingleObject(cacheQueue.hThread, 3000);
    CloseHandle(cacheQueue.hThread);
    CloseHandle(cacheQueue.hEvent);
	DeleteCriticalSection(&cacheQueue.cs);
	List_DestroyFreeContents(cacheQueue.queue);
	mir_free(cacheQueue.queue);
}


// Return true if this protocol has to be checked
BOOL PollCheckProtocol(const char *szProto)
{
	int pCaps = CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_4, 0);
	int status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
	return (pCaps & PF4_AVATARS)
		&& (g_szMetaName == NULL || strcmp(g_szMetaName, szProto))
		&& status != ID_STATUS_OFFLINE && status != ID_STATUS_INVISIBLE
		&& DBGetContactSettingByte(NULL, AVS_MODULE, szProto, 1);
}

// Return true if this protocol has to be checked
BOOL PollCacheProtocol(const char *szProto)
{
	return DBGetContactSettingByte(NULL, AVS_MODULE, szProto, 1);
}

// Return true if this contact has to be checked
BOOL PollCheckContact(HANDLE hContact, const char *szProto)
{
	int status = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
	return status != ID_STATUS_OFFLINE
		&& !DBGetContactSettingByte(hContact,"CList","NotOnList",0)
		&& DBGetContactSettingByte(hContact,"CList","ApparentMode",0) != ID_STATUS_OFFLINE
		&& !DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0)
		&& GetContactNode(hContact, 1) != NULL;
}

// Return true if this contact has to be checked
BOOL PollCacheContact(HANDLE hContact, const char *szProto)
{
	return !DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0)
		&& GetContactNode(hContact, 1) != NULL;
}

void QueueRemove(ThreadQueue &queue, HANDLE hContact)
{
	//EnterCriticalSection(&queue.cs);   // may not be needed (called only from within the same crit sect)
    
	if (queue.queue->items != NULL)
	{
		for (int i = queue.queue->realCount - 1 ; i >= 0 ; i-- )
		{
			QueueItem *item = (QueueItem*) queue.queue->items[i];
			
			if (item->hContact == hContact)
			{
				mir_free(item);
				List_Remove(queue.queue, i);
			}
		}
	}

	//LeaveCriticalSection(&queue.cs);
}

// Add an contact to a queue
void QueueAdd(ThreadQueue &queue, HANDLE hContact)
{
	EnterCriticalSection(&queue.cs);

	// Only add if not exists yeat
	int i;
	for (i = queue.queue->realCount - 1; i >= 0; i--)
	{
		if (((QueueItem *) queue.queue->items[i])->hContact == hContact)
			break;
	}

	if (i < 0)
	{
		QueueItem *item = (QueueItem *) mir_alloc0(sizeof(QueueItem));

		if (item == NULL) 
		{
			LeaveCriticalSection(&queue.cs);
			return;
		}

		item->hContact = hContact;
		item->check_time = GetTickCount() + queue.waitTime;

		List_InsertOrdered(queue.queue, item);
		LeaveCriticalSection(&queue.cs);
		ResumeThread(queue.hThread);
	}
	else
		LeaveCriticalSection(&queue.cs);
}


DWORD WINAPI RequestThread(LPVOID vParam)
{
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, requestQueue.eventName);

	while (requestQueue.bThreadRunning)
	{
		EnterCriticalSection(&requestQueue.cs);

		if (!List_HasItens(requestQueue.queue))
		{
			// No items, so supend thread
			LeaveCriticalSection(&requestQueue.cs);

			if (requestQueue.bThreadRunning)
				SuspendThread(requestQueue.hThread);
		}
		else
		{
			// Take a look at first item
			QueueItem *qi = (QueueItem *) List_Peek(requestQueue.queue);

			if (qi->check_time > GetTickCount()) 
			{
				// Not time to request yeat, wait...
				LeaveCriticalSection(&requestQueue.cs);

				if (requestQueue.bThreadRunning)
					WaitForSingleObject(hEvent, POOL_DELAY);
			}
			else
			{
				// Will request this item
				qi = (QueueItem *) List_Pop(requestQueue.queue);
				HANDLE hContact = qi->hContact;
				mir_free(qi);

				QueueRemove(requestQueue, hContact);

                LeaveCriticalSection(&requestQueue.cs);

				if (!requestQueue.bThreadRunning)
					break;

				char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
				if (szProto != NULL && PollCheckProtocol(szProto) && PollCheckContact(hContact, szProto))
				{
					// Request it
					PROTO_AVATAR_INFORMATION pai_s;
					pai_s.cbSize = sizeof(pai_s);
					pai_s.hContact = hContact;
					pai_s.format = PA_FORMAT_UNKNOWN;
					pai_s.filename[0] = '\0';

					int result = CallProtoService(szProto, PS_GETAVATARINFO, GAIF_FORCE, (LPARAM)&pai_s);
					if (result == GAIR_SUCCESS) 
					{
						DBDeleteContactSetting(hContact, "ContactPhoto", "NeedUpdate");
						DBWriteContactSettingString(hContact, "ContactPhoto", "File", "");
						DBWriteContactSettingString(hContact, "ContactPhoto", "File", pai_s.filename);
					}
					else if (result == GAIR_NOAVATAR) 
					{
						DBDeleteContactSetting(hContact, "ContactPhoto", "NeedUpdate");
						DBDeleteContactSetting(hContact, "ContactPhoto", "File");
					}
					else if (result == GAIR_WAITFOR) 
					{
						// Wait a little until requesting again
						if (requestQueue.bThreadRunning)
							WaitForSingleObject(hEvent, REQUEST_DELAY);
					}
				}
			}
		}
	}

	return 0;
}

DWORD WINAPI CacheThread(LPVOID vParam)
{
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, requestQueue.eventName);

	while (cacheQueue.bThreadRunning)
	{
		EnterCriticalSection(&cacheQueue.cs);

		if (!List_HasItens(cacheQueue.queue))
		{
			// No items, so supend thread
			LeaveCriticalSection(&cacheQueue.cs);

			if (cacheQueue.bThreadRunning)
				SuspendThread(cacheQueue.hThread);
		}
		else
		{
			// Take a look at first item
			QueueItem *qi = (QueueItem *) List_Peek(cacheQueue.queue);

			if (qi->check_time > GetTickCount()) 
			{
				// Not time to request yeat, wait...
				LeaveCriticalSection(&cacheQueue.cs);

				if (cacheQueue.bThreadRunning)
					WaitForSingleObject(hEvent, POOL_DELAY);
			}
			else
			{
				// Will cache this item
				qi = (QueueItem *) List_Pop(cacheQueue.queue);
				HANDLE hContact = qi->hContact;
				mir_free(qi);

				QueueRemove(cacheQueue, hContact);

				LeaveCriticalSection(&cacheQueue.cs);

				if (!cacheQueue.bThreadRunning)
					break;

				char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
				if (szProto != NULL && PollCacheProtocol(szProto) && PollCacheContact(hContact, szProto))
				{
					ChangeAvatar(hContact);
				}
			}
		}
	}

	return 0;
}
