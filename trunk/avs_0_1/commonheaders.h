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

#include <tchar.h>

#ifdef _DEBUG
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#else
#	include <malloc.h>
#endif

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <math.h>
#include <win2k.h>
#include <newpluginapi.h>
#include <m_clist.h>
#include <m_clc.h>
#include <m_clui.h>
#include <m_plugins.h>
#include <m_system.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_utils.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_file.h>
#include <m_addcontact.h>
#include <m_png.h>
#include "m_folders.h"

#include "m_avatars.h"
#include <m_popup.h>
#include "m_metacontacts.h"
#include "resource.h"
#include "m_updater.h"
#include "image_utils.h"


// shared vars
extern HINSTANCE g_hInst;

/* most free()'s are invalid when the code is executed from a dll, so this changes
 all the bad free()'s to good ones, however it's still incorrect code. The reasons for not
 changing them include:

  * DBFreeVariant has a CallService() lookup
  * free() is executed in some large loops to do with clist creation of group data
  * easy search and replace

*/

extern struct MM_INTERFACE memoryManagerInterface;

#define mir_alloc(n) memoryManagerInterface.mmi_malloc(n)
#define mir_free(ptr) memoryManagerInterface.mmi_free(ptr)
#define mir_realloc(ptr,size) memoryManagerInterface.mmi_realloc(ptr,size)

__inline char * mir_strdup(const char *src)
{
    char *p = 0;
    if (src == NULL)
        return NULL;
    p = (char *)mir_alloc(strlen(src) + 1);
    strcpy(p, src);
    return p;
}

struct protoPicCacheEntry {
    DWORD cbSize;                   // set to sizeof(struct)
    HANDLE hContact;                // contacts handle, 0, if it is a protocol avatar
    HBITMAP hbmPic;                 // bitmap handle of the picutre itself
    DWORD dwFlags;                  // see above for flag values
    LONG bmHeight, bmWidth;         // bitmap dimensions
    time_t t_lastAccess;            // last access time (currently unused, but plugins should still
                                    // use it whenever they access the avatar. may be used in the future
                                    // to implement cache expiration
    char szFilename[MAX_PATH];      // filename of the avatar (absolute path)
    char szProtoname[100];
};

int SetAvatarAttribute(HANDLE hContact, DWORD attrib, int mode);
static BOOL ColorsAreTheSame(int colorDiff, BYTE *px1, BYTE *px2);
void AddToStack(int *stack, int *topPos, int x, int y);
