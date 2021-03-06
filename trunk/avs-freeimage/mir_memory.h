/* 
Copyright (C) 2005 Ricardo Pescuma Domenecci

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


#ifndef __MIR_MEMORY_H__
# define __MIR_MEMORY_H__

#include <windows.h>

#ifdef __cplusplus
extern "C" 
{
#endif


// Need to be called on ME_SYSTEM_MODULESLOADED
void init_mir_malloc();


void * mir_alloc0(size_t size);
char * mir_dup(const char *ptr);
WCHAR * mir_dupW(const wchar_t *ptr);
char *mir_dupToAscii(WCHAR *ptr);
WCHAR *mir_dupToUnicode(char *ptr);

#ifdef _UNICODE 
# define mir_dupT mir_dupW
#else
# define mir_dupT mir_dup
#endif



#ifdef __cplusplus
}
#endif

#endif // __MIR_MEMORY_H__
