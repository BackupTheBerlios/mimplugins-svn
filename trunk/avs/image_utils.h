#ifndef __IMAGE_UTILS_H__
# define __IMAGE_UTILS_H__

#include <windows.h>
#include "m_avatars.h"


HBITMAP LoadAnyImage(char *szFilename);
HBITMAP CopyBitmapTo32(HBITMAP hBitmap);


int SaveBMP(HBITMAP hBmp, const char *szFilename);
int SavePNG(HBITMAP hBmp, const char *szFilename);
int SaveJPEG(HBITMAP hBmp, const char *szFilename);
int SaveGIF(HBITMAP hBmp, const char *szFilename);






#endif // __IMAGE_UTILS_H__
