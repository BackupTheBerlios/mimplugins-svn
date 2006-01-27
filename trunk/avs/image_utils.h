#ifndef __IMAGE_UTILS_H__
# define __IMAGE_UTILS_H__

#include <windows.h>
#include "m_avatars.h"


HBITMAP LoadPNG(struct avatarCacheEntry *ace, char *szFilename);
HBITMAP LoadAnyImage(char *szFilename, LPVOID *lpDIBSection);


int SaveBMP(HBITMAP hBmp, const char *szFilename);
int SavePNG(HBITMAP hBmp, const char *szFilename);
int SaveJPEG(HBITMAP hBmp, const char *szFilename);
int SaveGIF(HBITMAP hBmp, const char *szFilename);






#endif // __IMAGE_UTILS_H__
