#include "image_utils.h"

#include "commonheaders.h"

#include <ocidl.h>
#include <olectl.h>

// This is needed to use mEncoderQuality
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUID(mEncoderQuality, 0x1d5be4b5,0xfa4a,0x452d,0x9c,0xdd,0x5d,0xb3,0x51,0x05,0xe7,0xeb);
//

/*
Theese are the ones needed
#include <win2k.h>
#include <newpluginapi.h>
#include <m_langpack.h>
#include <m_utils.h>
#include <m_png.h>
#include <m_protocols.h>
*/

extern int _DebugTrace(const char *fmt, ...);
extern int _DebugTrace(HANDLE hContact, const char *fmt, ...);


#define GET_PIXEL(__P__, __X__, __Y__) ( __P__ + width * 4 * (__Y__) + 4 * (__X__) )


// GDI+ ///////////////////////////////////////////////////////////////////////////////////////////

extern int AVS_pathToRelative(const char *sPrc, char *pOut);
extern int AVS_pathToAbsolute(const char *pSrc, char *pOut);
extern FI_INTERFACE *fei;

// Make a bitmap all transparent, but only if it is a 32bpp
void MakeBmpTransparent(HBITMAP hBitmap)
{
	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	if (bmp.bmBitsPixel != 32)
		return;

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return;

	memset(p, 0, dwLen);
	SetBitmapBits(hBitmap, dwLen, p);

	free(p);
}

// Resize /////////////////////////////////////////////////////////////////////////////////////////


// Returns a copy of the bitmap with the size especified
// wParam = ResizeBitmap *
// lParam = NULL
int BmpFilterResizeBitmap(WPARAM wParam,LPARAM lParam)
{
	BITMAP bminfo;
	int width, height;
	int xOrig, yOrig, widthOrig, heightOrig;
	ResizeBitmap *info = (ResizeBitmap *) wParam;

	if (info == NULL || info->size != sizeof(ResizeBitmap)
			|| info->hBmp == NULL || info->max_width <= 0 
			|| info->max_height <= 0 
			|| (info->fit & ~RESIZEBITMAP_FLAG_DONT_GROW) < RESIZEBITMAP_STRETCH 
			|| (info->fit & ~RESIZEBITMAP_FLAG_DONT_GROW) > RESIZEBITMAP_MAKE_SQUARE)
		return 0;

	// Well, lets do it

	// Calc final size
	GetObject(info->hBmp, sizeof(bminfo), &bminfo);

	width = info->max_width;
	height = info->max_height;

	xOrig = 0;
	yOrig = 0;
	widthOrig = bminfo.bmWidth;
	heightOrig = bminfo.bmHeight;

	if (widthOrig == 0 || heightOrig == 0)
		return 0;

	switch(info->fit & ~RESIZEBITMAP_FLAG_DONT_GROW)
	{
		case RESIZEBITMAP_STRETCH:
		{
			// Do nothing
			break;
		}
		case RESIZEBITMAP_KEEP_PROPORTIONS:
		{
			if (height * widthOrig / heightOrig <= width)
				width = height * widthOrig / heightOrig;
			else
				height = width * heightOrig / widthOrig;

			break;
		}
		case RESIZEBITMAP_MAKE_SQUARE:
		{
			if (info->fit & RESIZEBITMAP_FLAG_DONT_GROW)
			{
				width = min(width, bminfo.bmWidth);
				height = min(height, bminfo.bmHeight);
			}

			width = height = min(width, height);
			// Do not break. Use crop calcs to make size
		}
		case RESIZEBITMAP_CROP:
		{
			if (heightOrig * width / height >= widthOrig)
			{
				heightOrig = widthOrig * height / width;
				yOrig = (bminfo.bmHeight - heightOrig) / 2;
			}
			else
			{
				widthOrig = heightOrig * width / height;
				xOrig = (bminfo.bmWidth - widthOrig) / 2;
			}

			break;
		}
	}

	if ((width == bminfo.bmWidth && height == bminfo.bmHeight) 
		|| ((info->fit & RESIZEBITMAP_FLAG_DONT_GROW) 
			&& width > bminfo.bmWidth && height > bminfo.bmHeight))
	{
		// Do nothing
		return (int) info->hBmp;
	}
	else
	{
        FIBITMAP *dib;

        if(fei == NULL)
            return (int)info->hBmp;

        dib = fei->FI_CreateDIBFromHBITMAP(info->hBmp);
        if(dib) { 
            fei->FI_Rescale(dib, width, height, FILTER_LANCZOS3);
            HBITMAP hbmReturn = fei->FI_CreateHBITMAPFromDIB(dib);
            fei->FI_Unload(dib);
            return (int)hbmReturn;
        }
        return (int)info->hBmp;
	}
}

HBITMAP CopyBitmapTo32(HBITMAP hBitmap)
{
	BITMAPINFO RGB32BitsBITMAPINFO; 
	BYTE * ptPixels;
	HBITMAP hDirectBitmap;

	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	dwLen = bmp.bmWidth * bmp.bmHeight * 4;
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return NULL;

	// Create bitmap
	ZeroMemory(&RGB32BitsBITMAPINFO, sizeof(BITMAPINFO));
	RGB32BitsBITMAPINFO.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RGB32BitsBITMAPINFO.bmiHeader.biWidth = bmp.bmWidth;
	RGB32BitsBITMAPINFO.bmiHeader.biHeight = bmp.bmHeight;
	RGB32BitsBITMAPINFO.bmiHeader.biPlanes = 1;
	RGB32BitsBITMAPINFO.bmiHeader.biBitCount = 32;

	hDirectBitmap = CreateDIBSection(NULL, 
									(BITMAPINFO *)&RGB32BitsBITMAPINFO, 
									DIB_RGB_COLORS,
									(void **)&ptPixels, 
									NULL, 0);

	// Copy data
	if (bmp.bmBitsPixel != 32)
	{
		HDC hdcOrig, hdcDest;
		HBITMAP oldOrig, oldDest;
		
		hdcOrig = CreateCompatibleDC(NULL);
		oldOrig = (HBITMAP) SelectObject(hdcOrig, hBitmap);

		hdcDest = CreateCompatibleDC(NULL);
		oldDest = (HBITMAP) SelectObject(hdcDest, hDirectBitmap);

		BitBlt(hdcDest, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcOrig, 0, 0, SRCCOPY);

		SelectObject(hdcDest, oldDest);
		DeleteObject(hdcDest);
		SelectObject(hdcOrig, oldOrig);
		DeleteObject(hdcOrig);

		// Set alpha
		fei->FI_CorrectBitmap32Alpha(hDirectBitmap, FALSE);
	}
	else
	{
		GetBitmapBits(hBitmap, dwLen, p);
		SetBitmapBits(hDirectBitmap, dwLen, p);
	}

	free(p);

	return hDirectBitmap;
}

HBITMAP CreateBitmap32(int cx, int cy)
{
	BITMAPINFO RGB32BitsBITMAPINFO; 
	UINT * ptPixels;
	HBITMAP DirectBitmap;

	ZeroMemory(&RGB32BitsBITMAPINFO,sizeof(BITMAPINFO));
	RGB32BitsBITMAPINFO.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	RGB32BitsBITMAPINFO.bmiHeader.biWidth=cx;//bm.bmWidth;
	RGB32BitsBITMAPINFO.bmiHeader.biHeight=cy;//bm.bmHeight;
	RGB32BitsBITMAPINFO.bmiHeader.biPlanes=1;
	RGB32BitsBITMAPINFO.bmiHeader.biBitCount=32;

	DirectBitmap = CreateDIBSection(NULL, 
									(BITMAPINFO *)&RGB32BitsBITMAPINFO, 
									DIB_RGB_COLORS,
									(void **)&ptPixels, 
									NULL, 0);
	return DirectBitmap;
}

// Set the color of points that are transparent
void SetTranspBkgColor(HBITMAP hBitmap, COLORREF color)
{
	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;
	int x, y;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	if (bmp.bmBitsPixel != 32)
		return;

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	bool changed = false;
	for (y = 0; y < bmp.bmHeight; ++y) {
        BYTE *px = p + bmp.bmWidth * 4 * y;

        for (x = 0; x < bmp.bmWidth; ++x) 
		{
			if (px[3] == 0) 
			{
				px[0] = GetBValue(color);
				px[1] = GetGValue(color);
				px[2] = GetRValue(color);
				changed = true;
			}
			px += 4;
		}
	}

	if (changed)
		SetBitmapBits(hBitmap, dwLen, p);

	free(p);
}


#define HIMETRIC_INCH   2540    // HIMETRIC units per inch

void SetHIMETRICtoDP(HDC hdc, SIZE* sz)
{
    POINT pt;
    int nMapMode = GetMapMode(hdc);
    if ( nMapMode < MM_ISOTROPIC && nMapMode != MM_TEXT )
    {
        // when using a constrained map mode, map against physical inch
        SetMapMode(hdc,MM_HIMETRIC);
        pt.x = sz->cx;
        pt.y = sz->cy;
        LPtoDP(hdc,&pt,1);
        sz->cx = pt.x;
        sz->cy = pt.y;
        SetMapMode(hdc, nMapMode);
    }
    else
    {
        // map against logical inch for non-constrained mapping modes
        int cxPerInch, cyPerInch;
        cxPerInch = GetDeviceCaps(hdc,LOGPIXELSX);
        cyPerInch = GetDeviceCaps(hdc,LOGPIXELSY);
        sz->cx = MulDiv(sz->cx, cxPerInch, HIMETRIC_INCH);
        sz->cy = MulDiv(sz->cy, cyPerInch, HIMETRIC_INCH);
    }

    pt.x = sz->cx;
    pt.y = sz->cy;
    DPtoLP(hdc,&pt,1);
    sz->cx = pt.x;
    sz->cy = pt.y;
}

int BmpFilterLoadBitmap32(WPARAM wParam,LPARAM lParam)
{
    FIBITMAP *dib32 = NULL;

	if(fei == NULL)
        return 0;

    FIBITMAP *dib = (FIBITMAP *)CallService(MS_IMG_LOAD, lParam, IMGL_RETURNDIB);
	
	if(dib == NULL)
		return 0;

    if(fei->FI_GetBPP(dib) != 32) {
        dib32 = fei->FI_ConvertTo32Bits(dib);
	    fei->FI_Unload(dib);
    }
    else
        dib32 = dib;

    if(dib32) {
        if(fei->FI_IsTransparent(dib32)) {
            if(wParam) {
                DWORD *dwTrans = (DWORD *)wParam;
				*dwTrans = 1;
			}
        }
		if(fei->FI_GetWidth(dib32) > 100 || fei->FI_GetHeight(dib32) > 100) {
			FIBITMAP *dib_new = fei->FI_MakeThumbnail(dib32, 100, FALSE);
            fei->FI_Unload(dib32);
            if(dib_new == NULL)
				return (int)0;
            dib32 = dib_new;
        }

        HBITMAP bitmap = fei->FI_CreateHBITMAPFromDIB(dib32);

        fei->FI_Unload(dib32);
        fei->FI_CorrectBitmap32Alpha(bitmap, FALSE);
        return (int)bitmap;
	}
    return 0;
}

static HWND hwndClui = 0;

// 
// Save ///////////////////////////////////////////////////////////////////////////////////////////
// PNG and BMP will be saved as 32bit images, jpg as 24bit with default quality (75)
// returns 1 on success, 0 on failure

int SaveIMG(HBITMAP hBmp, const char *szFilename)
{
    FIBITMAP *dib = NULL;
    if(hBmp) {
        dib = fei->FI_CreateDIBFromHBITMAP(hBmp);
        if(dib == NULL)
            return 0;

        FREE_IMAGE_FORMAT fif = fei->FI_GetFIFFromFilename(szFilename);
        //FreeImage_Save(fif, dib, szFilename, 0);
        if(fif != FIF_UNKNOWN) {
            if(fif == FIF_PNG || fif == FIF_BMP || fif == FIF_JNG)
                fei->FI_Save(fif, dib, szFilename, 0);
            else {
                FIBITMAP *dib_new = fei->FI_ConvertTo24Bits(dib);

                fei->FI_Save(fif, dib_new, szFilename, JPEG_DEFAULT);
                fei->FI_Unload(dib_new);
            }
        }
        fei->FI_Unload(dib);
        return 0;
    }
    return 1;
}

// Save an HBITMAP to an image
// wParam = HBITMAP
// lParam = filename
int BmpFilterSaveBitmap(WPARAM wParam,LPARAM lParam)
{
	HBITMAP hBmp = (HBITMAP) wParam;
	const char *szFile=(const char *)lParam;
	char szFilename[MAX_PATH];
	int filenameLen;

    if(fei == NULL)
        return -1;

    if (!AVS_pathToAbsolute(szFile, szFilename))
		mir_snprintf(szFilename, SIZEOF(szFilename), "%s", szFile);
	filenameLen=lstrlenA(szFilename);
	if(filenameLen>4) 
	{
        return SaveIMG(hBmp, szFilename);
	}

	return -1;
}


// Returns != 0 if can save that type of image, = 0 if cant
// wParam = 0
// lParam = PA_FORMAT_*   // image format
// kept for compatibilty - with freeimage we can save all common formats

int BmpFilterCanSaveBitmap(WPARAM wParam,LPARAM lParam)
{
    return 1;
}


// Other utilities ////////////////////////////////////////////////////////////////////////////////


static BOOL ColorsAreTheSame(int colorDiff, BYTE *px1, BYTE *px2)
{
	return abs(px1[0] - px2[0]) <= colorDiff 
			&& abs(px1[1] - px2[1]) <= colorDiff 
			&& abs(px1[2] - px2[2])  <= colorDiff;
}


void AddToStack(int *stack, int *topPos, int x, int y)
{
	int i;

	// Already is in stack?
	for(i = 0 ; i < *topPos ; i += 2)
	{
		if (stack[i] == x && stack[i+1] == y)
			return;
	}

	stack[*topPos] = x; 
	(*topPos)++;

	stack[*topPos] = y; 
	(*topPos)++;
}


BOOL GetColorForPoint(int colorDiff, BYTE *p, int width, int height, 
					  int x0, int y0, int x1, int y1, int x2, int y2, BOOL *foundBkg, BYTE colors[][3])
{
	BYTE *px1, *px2, *px3; 

	px1 = GET_PIXEL(p, x0,y0);
	px2 = GET_PIXEL(p, x1,y1);
	px3 = GET_PIXEL(p, x2,y2);

	// If any of the corners have transparency, forget about it
	// Not using != 255 because some MSN bmps have 254 in some positions
	if (px1[3] < 253 || px2[3] < 253 || px3[3] < 253)
		return FALSE;

	// See if is the same color
	if (ColorsAreTheSame(colorDiff, px1, px2) && ColorsAreTheSame(colorDiff, px3, px2))
	{
		*foundBkg = TRUE;
		memmove(colors, px1, 3);
	}
	else
	{
		*foundBkg = FALSE;
	}

	return TRUE;
}


DWORD GetImgHash(HBITMAP hBitmap)
{
	BITMAP bmp;
	DWORD dwLen;
	WORD *p;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (WORD *)malloc(dwLen);
	if (p == NULL)
		return 0;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	DWORD ret = 0;
	for (DWORD i = 0 ; i < dwLen/2 ; i++)
		ret += p[i];

	free(p);

	return ret;
}

/*
 * Changes the handle to a grayscale image
 */
HBITMAP MakeGrayscale(HANDLE hContact, HBITMAP hBitmap)
{
    if(hBitmap) {
        FIBITMAP *dib = fei->FI_CreateDIBFromHBITMAP(hBitmap);

        if(dib) {
            FIBITMAP *dib_new = fei->FI_ConvertToGreyscale(dib);
            fei->FI_Unload(dib);
            if(dib_new) {
                DeleteObject(hBitmap);
                HBITMAP hbm_new = fei->FI_CreateHBITMAPFromDIB(dib_new);
                fei->FI_Unload(dib_new);
                return hbm_new;
            }
        }
    }
    return hBitmap;
}

/*
 * See if finds a transparent background in image, and set its transparency
 * Return TRUE if found a transparent background
 */
BOOL MakeTransparentBkg(HANDLE hContact, HBITMAP *hBitmap)
{
	BYTE *p = NULL;
	DWORD dwLen;
    int width, height, i, j;
    BITMAP bmp;
	BYTE colors[8][3];
	BOOL foundBkg[8];
	BYTE *px1; 
	int count, maxCount, selectedColor;
	HBITMAP hBmpTmp;
	int colorDiff;

	GetObject(*hBitmap, sizeof(bmp), &bmp);
    width = bmp.bmWidth;
	height = bmp.bmHeight;
	colorDiff = DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgColorDiff", 
					DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgColorDiff", 10));

	// Min 5x5 to easy things in loop
	if (width <= 4 || height <= 4)
		return FALSE;

	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
    if (p == NULL) 
	{
		return FALSE;
	}

	if (bmp.bmBitsPixel == 32)
	{
		hBmpTmp = *hBitmap;
	}
	else
	{
		// Convert to 32 bpp
		hBmpTmp = CopyBitmapTo32(*hBitmap);
	}

    GetBitmapBits(hBmpTmp, dwLen, p);

	// **** Get corner colors

	// Top left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  0, 0, 0, 1, 1, 0, &foundBkg[0], &colors[0]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top center
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width/2, 0, width/2-1, 0, width/2+1, 0, &foundBkg[1], &colors[1]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top Right
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width-1, 0, width-1, 1, width-2, 0, &foundBkg[2], &colors[2]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  0, height/2, 0, height/2-1, 0, height/2+1, &foundBkg[3], &colors[3]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width-1, height/2, width-1, height/2-1, width-1, height/2+1, &foundBkg[4], &colors[4]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom left
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  0, height-1, 0, height-2, 1, height-1, &foundBkg[5], &colors[5]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom center
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width/2, height-1, width/2-1, height-1, width/2+1, height-1, &foundBkg[6], &colors[6]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom Right
	if (!GetColorForPoint(colorDiff, p, width, height, 
						  width-1, height-1, width-1, height-2, width-2, height-1, &foundBkg[7], &colors[7]))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}


	// **** X corners have to have the same color

	count = 0;
	for (i = 0 ; i < 8 ; i++)
	{
		if (foundBkg[i])
			count++;
	}

	if (count < DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", 
						DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5)))
	{
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Ok, X corners at least have a color, lets compare then
	maxCount = 0;
	for (i = 0 ; i < 8 ; i++)
	{
		if (foundBkg[i])
		{
			count = 0;

			for (j = 0 ; j < 8 ; j++)
			{
				if (foundBkg[j] && ColorsAreTheSame(colorDiff, (BYTE *) &colors[i], (BYTE *) &colors[j]))
					count++;
			}

			if (count > maxCount)
			{
				maxCount = count;
				selectedColor = i;
			}
		}
	}

	if (maxCount < DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", 
						DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5)))
	{
		// Not enought corners with the same color
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Get bkg color as mean of colors
	{
		int bkgColor[3];

		bkgColor[0] = 0;
		bkgColor[1] = 0;
		bkgColor[2] = 0;
		for (i = 0 ; i < 8 ; i++)
		{
			if (foundBkg[i] && ColorsAreTheSame(colorDiff, (BYTE *) &colors[i], (BYTE *) &colors[selectedColor]))
			{
				bkgColor[0] += colors[i][0];
				bkgColor[1] += colors[i][1];
				bkgColor[2] += colors[i][2];
			}
		}
		bkgColor[0] /= maxCount;
		bkgColor[1] /= maxCount;
		bkgColor[2] /= maxCount;

		colors[selectedColor][0] = bkgColor[0];
		colors[selectedColor][1] = bkgColor[1];
		colors[selectedColor][2] = bkgColor[2];
	}

	// **** Set alpha for the background color, from the borders

	if (hBmpTmp != *hBitmap)
	{
		DeleteObject(*hBitmap);

		*hBitmap = hBmpTmp;

		GetObject(*hBitmap, sizeof(bmp), &bmp);
		GetBitmapBits(*hBitmap, dwLen, p);
	}

	{
		// Set alpha from borders
		int x, y;
		int topPos = 0;
		int curPos = 0;
		int *stack = (int *)malloc(width * height * 2 * sizeof(int));
		bool transpProportional = (DBGetContactSettingByte(NULL, AVS_MODULE, "MakeTransparencyProportionalToColorDiff", 0) != 0);

		if (stack == NULL)
		{
			free(p);
			return FALSE;
		}

		// Put four corners
		AddToStack(stack, &topPos, 0, 0);
		AddToStack(stack, &topPos, width/2, 0);
		AddToStack(stack, &topPos, width-1, 0);
		AddToStack(stack, &topPos, 0, height/2);
		AddToStack(stack, &topPos, width-1, height/2);
		AddToStack(stack, &topPos, 0, height-1);
		AddToStack(stack, &topPos, width/2, height-1);
		AddToStack(stack, &topPos, width-1, height-1);

		while(curPos < topPos)
		{
			// Get pos
			x = stack[curPos]; curPos++;
			y = stack[curPos]; curPos++;

			// Get pixel
			px1 = GET_PIXEL(p, x, y);

			// It won't change the transparency if one exists
			// (This avoid an endless loop too)
			// Not using == 255 because some MSN bmps have 254 in some positions
			if (px1[3] >= 253)
			{
				if (ColorsAreTheSame(colorDiff, px1, (BYTE *) &colors[selectedColor]))
				{
					if (transpProportional)
					{
						px1[3] = min(252, 
								(abs(px1[0] - colors[selectedColor][0]) 
								+ abs(px1[1] - colors[selectedColor][1]) 
								+ abs(px1[2] - colors[selectedColor][2])) / 3);
					}
					else
					{
						px1[3] = 0;
					}

					// Add 4 neighbours

					if (x + 1 < width)
						AddToStack(stack, &topPos, x + 1, y);

					if (x - 1 >= 0)
						AddToStack(stack, &topPos, x - 1, y);

					if (y + 1 < height)
						AddToStack(stack, &topPos, x, y + 1);

					if (y - 1 >= 0)
						AddToStack(stack, &topPos, x, y - 1);
				}
			}
		}

		free(stack);
	}

    dwLen = SetBitmapBits(*hBitmap, dwLen, p);
    free(p);

	return TRUE;
}


