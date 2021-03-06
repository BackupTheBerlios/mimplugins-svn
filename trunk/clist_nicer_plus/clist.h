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
*/
void LoadContactTree(void);
int IconFromStatusMode(const char *szProto, int status, HANDLE hContact, HICON *phIcon);
HTREEITEM GetTreeItemByHContact(HANDLE hContact);
void TrayIconUpdateWithImageList(int iImage, const char *szNewTip, char *szPreferredProto, HANDLE hContact);
void SortContacts(void);
void ChangeContactIcon(HANDLE hContact, int iIcon, int add);

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)_alloca(strlen(A)+1),A)

#define CLUIINTM_REDRAW (WM_USER+100)
#define CLUIINTM_STATUSBARUPDATE (WM_USER+101)
#define CLUIINTM_REMOVEFROMTASKBAR (WM_USER+102)

#define CLVM_FILTER_PROTOS 1
#define CLVM_FILTER_GROUPS 2
#define CLVM_FILTER_STATUS 4
#define CLVM_FILTER_VARIABLES 8
#define CLVM_STICKY_CONTACTS 16
#define CLVM_FILTER_STICKYSTATUS 32

#define CLVM_PROTOGROUP_OP 1
#define CLVM_GROUPSTATUS_OP 2
#define CLVM_AUTOCLEAR 4
#define CLVM_INCLUDED_UNGROUPED 8

#if defined(_UNICODE)
    #define CLVM_MODULE "CLVM_W"
#else
    #define CLVM_MODULE "CLVM"
#endif
    

