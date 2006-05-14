#include "image_utils.h"

#include "commonheaders.h"

#include <gdiplus.h>
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


// GDI+ ///////////////////////////////////////////////////////////////////////////////////////////

using namespace Gdiplus;
DWORD gdiPlusToken;
bool gdiPlusFail = false;

bool gdiPlusLoaded = false;
HMODULE hGdiPlus = NULL;
Status (WINAPI *fGdiplusStartup)(ULONG_PTR*, const GdiplusStartupInput*, GdiplusStartupOutput*);
VOID (WINAPI *fGdiplusShutdown)(ULONG_PTR);
Status (WINAPI *fGetImageEncoders)(UINT, UINT, ImageCodecInfo*);
Status (WINAPI *fGetImageEncodersSize)(UINT*, UINT*);
Status (WINAPI *fGdipCreateBitmapFromHBITMAP)(HBITMAP hbm, HPALETTE hpal, GpBitmap** bitmap);
Status (WINAPI *fGdipSaveImageToFile)(GpImage*, const WCHAR*, const CLSID*, const EncoderParameters*);

void LoadGdiPlus(void)
{
	gdiPlusLoaded = false;
	hGdiPlus = LoadLibraryA("gdiplus.dll");
	if (hGdiPlus != NULL)
	{
		//Look up API's
		(FARPROC&)fGdiplusStartup = GetProcAddress(hGdiPlus, "GdiplusStartup");
		(FARPROC&)fGdiplusShutdown = GetProcAddress(hGdiPlus, "GdiplusShutdown");
		(FARPROC&)fGetImageEncoders = GetProcAddress(hGdiPlus, "GdipGetImageEncoders");
		(FARPROC&)fGetImageEncodersSize = GetProcAddress(hGdiPlus, "GdipGetImageEncodersSize");
		(FARPROC&)fGdipCreateBitmapFromHBITMAP = GetProcAddress(hGdiPlus, "GdipCreateBitmapFromHBITMAP");
		(FARPROC&)fGdipSaveImageToFile = GetProcAddress(hGdiPlus, "GdipSaveImageToFile");

		if (fGdiplusStartup && fGdiplusShutdown && fGetImageEncoders && fGetImageEncodersSize
			&& fGdipCreateBitmapFromHBITMAP && fGdipSaveImageToFile)
		{
			gdiPlusLoaded = true;
		}
		else
		{
			FreeLibrary(hGdiPlus);
			hGdiPlus = NULL;
		}
	}
}

void FreeGdiPlus(void)
{
	if (gdiPlusLoaded)
		FreeLibrary(hGdiPlus);
}

void InitGdiPlus(void)
{
	gdiPlusFail = false;

	if (gdiPlusLoaded)
	{
		__try 
		{
			GdiplusStartupInput gdiPlusStartupInput;
			if (gdiPlusToken == 0)
				fGdiplusStartup(&gdiPlusToken, &gdiPlusStartupInput, NULL);
		}
		__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
			gdiPlusFail = true;
		}
	}
}

void ShutdownGdiPlus(void)
{
	gdiPlusFail = false;

	if (gdiPlusLoaded)
	{
		__try 
		{
			if (gdiPlusToken)
				fGdiplusShutdown(gdiPlusToken);
		}
		__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{
			gdiPlusFail = true;
		}
		gdiPlusToken = 0;
	}
}


// Utils //////////////////////////////////////////////////////////////////////////////////////////


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

// Correct alpha from bitmaps loaded without it (it cames with 0 and should be 255)
void CorrectBitmap32Alpha(HBITMAP hBitmap, BOOL force)
{
	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;
	int x, y;
	BOOL fixIt;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	if (bmp.bmBitsPixel != 32)
		return;

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	fixIt = TRUE;
	for (y = 0; fixIt && y < bmp.bmHeight; ++y) {
        BYTE *px = p + bmp.bmWidth * 4 * y;

        for (x = 0; fixIt && x < bmp.bmWidth; ++x) 
		{
			if (px[3] != 0 && !force) 
			{
				fixIt = FALSE;
			}
			else
			{
				px[3] = 255;
			}

			px += 4;
		}
	}

	if (fixIt)
		SetBitmapBits(hBitmap, dwLen, p);

	free(p);
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
		CorrectBitmap32Alpha(hDirectBitmap, FALSE);
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

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	fGetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	fGetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}


// Load ///////////////////////////////////////////////////////////////////////////////////////////


// Load an image
// wParam = NULL
// lParam = filename
int BmpFilterLoadBitmap32(WPARAM wParam,LPARAM lParam)
{
	IPicture *pic;
	HBITMAP hBmp,hBmpCopy;
	HBITMAP hOldBitmap, hOldBitmap2;
	BITMAP bmpInfo;
	WCHAR pszwFilename[MAX_PATH];
	HDC hdc,hdcMem1,hdcMem2;
	short picType;
	const char *szFile=(const char *)lParam;
	char szFilename[MAX_PATH];
	int filenameLen;
    
	if (!CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)szFile, (LPARAM)szFilename))
		mir_snprintf(szFilename, SIZEOF(szFilename), "%s", szFile);
	filenameLen=lstrlenA(szFilename);
	if(filenameLen>4) {
		char* pszExt = szFilename+filenameLen-4;
		if ( !lstrcmpiA( pszExt,".bmp" ) || !lstrcmpiA( pszExt, ".rle" )) {
			// LoadImage can do this much faster
			hBmpCopy = (HBITMAP) LoadImageA( GetModuleHandle(NULL), szFilename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
			CorrectBitmap32Alpha(hBmpCopy, TRUE);
			return (int) hBmpCopy;
		}

		if ( !lstrcmpiA( pszExt, ".png" )) {
			HANDLE hFile, hMap = NULL;
			BYTE* ppMap = NULL;
			long  cbFileSize = 0;
			BITMAPINFOHEADER* pDib;
			BYTE* pDibBits;

			if ( !ServiceExists( MS_PNG2DIB )) {
				MessageBox( NULL, TranslateT( "You need the png2dib plugin v. 0.1.3.x or later to process PNG images" ), TranslateT( "Error" ), MB_OK );
				return (int)(HBITMAP)NULL;
			}

			if (( hFile = CreateFileA( szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE )
				if (( hMap = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL )) != NULL )
					if (( ppMap = ( BYTE* )MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 )) != NULL )
						cbFileSize = GetFileSize( hFile, NULL );

			if ( cbFileSize != 0 ) {
				PNG2DIB param;
				param.pSource = ppMap;
				param.cbSourceSize = cbFileSize;
				param.pResult = &pDib;
				if ( CallService( MS_PNG2DIB, 0, ( LPARAM )&param ))
					pDibBits = ( BYTE* )( pDib+1 );
				else
					cbFileSize = 0;
			}

			if ( ppMap != NULL )	UnmapViewOfFile( ppMap );
			if ( hMap  != NULL )	CloseHandle( hMap );
			if ( hFile != NULL ) CloseHandle( hFile );

			if ( cbFileSize == 0 )
				return (int)(HBITMAP)NULL;

			{	HDC sDC = GetDC( NULL );
				HBITMAP hBitmap = CreateDIBitmap( sDC, pDib, CBM_INIT, pDibBits, ( BITMAPINFO* )pDib, DIB_PAL_COLORS );
				SelectObject( sDC, hBitmap );
				ReleaseDC( NULL, sDC );
				GlobalFree( pDib );

				// Should not be here
				CorrectBitmap32Alpha(hBitmap, FALSE);
				return (int)hBitmap;
	}	}	}

	OleInitialize(NULL);
	MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);
#ifndef __WINE__
 	MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);
#else
	char szFilenameAux[MAX_PATH];
	mir_snprintf(szFilenameAux, MAX_PATH, "%s%s", "file://", szFilename);
	MultiByteToWideChar(CP_ACP,0,szFilenameAux,-1,pszwFilename,MAX_PATH);
#endif

	if(S_OK!=OleLoadPicturePath(pszwFilename,NULL,0,0,IID_IPicture,(PVOID*)&pic)) {
		OleUninitialize();
		return (int)(HBITMAP)NULL;
	}
	pic->get_Type(&picType);
	if(picType!=PICTYPE_BITMAP) {
		pic->Release();
		OleUninitialize();
		return (int)(HBITMAP)NULL;
	}
	pic->get_Handle((OLE_HANDLE*)&hBmp);
	GetObject(hBmp,sizeof(bmpInfo),&bmpInfo);

	//need to copy bitmap so we can free the IPicture
	hdc=GetDC(NULL);
	hdcMem1=CreateCompatibleDC(hdc);
	hdcMem2=CreateCompatibleDC(hdc);
	hOldBitmap=(HBITMAP)SelectObject(hdcMem1,hBmp);
	hBmpCopy=CreateCompatibleBitmap(hdcMem1,bmpInfo.bmWidth,bmpInfo.bmHeight);
	hOldBitmap2=(HBITMAP)SelectObject(hdcMem2,hBmpCopy);
	BitBlt(hdcMem2,0,0,bmpInfo.bmWidth,bmpInfo.bmHeight,hdcMem1,0,0,SRCCOPY);
	SelectObject(hdcMem1,hOldBitmap);
	SelectObject(hdcMem2,hOldBitmap2);
	DeleteDC(hdcMem2);
	DeleteDC(hdcMem1);
	ReleaseDC(NULL,hdc);

	DeleteObject(hBmp);
	pic->Release();
	OleUninitialize();

	CorrectBitmap32Alpha(hBmpCopy, FALSE);
	return (int)hBmpCopy;
}


// Resize /////////////////////////////////////////////////////////////////////////////////////////


// Returns a copy of the bitmap with the size especified
// wParam = ResizeBitmap *
// lParam = NULL
int BmpFilterResizeBitmap(WPARAM wParam,LPARAM lParam)
{
	HBITMAP hStretchedBitmap;
	HDC hdc, hdcMem;
	HBITMAP hOldBitmap, hOldBitmap2;
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
		// Resize it, baby
		hdcMem = CreateCompatibleDC(NULL);
		hStretchedBitmap = CreateBitmap32(width, height);
		MakeBmpTransparent(hStretchedBitmap);
		hOldBitmap = (HBITMAP) SelectObject(hdcMem, hStretchedBitmap);

		hdc = CreateCompatibleDC(hdcMem);
		hOldBitmap2 = (HBITMAP) SelectObject(hdc, info->hBmp);

		SetStretchBltMode(hdcMem, HALFTONE);
		StretchBlt(hdcMem, 0, 0, width, height, hdc, xOrig, yOrig, widthOrig, heightOrig, SRCCOPY);

		SelectObject(hdcMem, hOldBitmap);
		DeleteDC(hdcMem);
		SelectObject(hdc, hOldBitmap2);
		DeleteDC(hdc);
		CorrectBitmap32Alpha(hStretchedBitmap, FALSE);
		return (int) hStretchedBitmap;
	}
}


// Save ///////////////////////////////////////////////////////////////////////////////////////////


int SaveGIF(HBITMAP hBmp, const char *szFilename)
{
	// Initialize GDI+.
	InitGdiPlus();

	if (!gdiPlusFail)
	{
		CLSID             encoderClsid;
		Status            stat;

		SetTranspBkgColor(hBmp, RGB(255, 255, 255));

		GpBitmap *bitmap = NULL;
		fGdipCreateBitmapFromHBITMAP(hBmp, NULL, &bitmap);

		// Get the CLSID of the GIF encoder.
		if (GetEncoderClsid(L"image/gif", &encoderClsid) < 0)
			return -2;

		WCHAR pszwFilename[MAX_PATH];
		MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);
		stat = fGdipSaveImageToFile(bitmap, pszwFilename, &encoderClsid, NULL);

		//delete image;
		ShutdownGdiPlus();
		
		if(stat == Ok)
			return 0;
		else
			return -1;
	}
	else
	{
		return -1000;
	}
}

int SaveJPEG(HBITMAP hBmp, const char *szFilename)
{
	// Initialize GDI+.
	InitGdiPlus();

	if (!gdiPlusFail)
	{
		CLSID             encoderClsid;
		EncoderParameters encoderParameters;
		Status            stat;

		SetTranspBkgColor(hBmp, RGB(255, 255, 255));

		GpBitmap *bitmap = NULL;
		fGdipCreateBitmapFromHBITMAP(hBmp, NULL, &bitmap);

		// Get the CLSID of the JPEG encoder.
		if (GetEncoderClsid(L"image/jpeg", &encoderClsid) < 0)
			return -2;

		WCHAR pszwFilename[MAX_PATH];
		MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);

		// Setting the jpeg quality
		int Quality = 90;
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = mEncoderQuality;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		encoderParameters.Parameter[0].Value = &Quality;

		stat = fGdipSaveImageToFile(bitmap, pszwFilename, &encoderClsid, &encoderParameters);

		//delete image;
		ShutdownGdiPlus();
		
		if(stat == Ok)
			return 0;
		else
			return -1;
	}
	else
	{
		return -1000;
	}
}

int SavePNG(HBITMAP hBmp, const char *szFilename)
{
	if (ServiceExists(MS_DIB2PNG))
	{
		SetTranspBkgColor(hBmp, RGB(255, 255, 255));

		HDC hdc = CreateCompatibleDC( NULL );
		HBITMAP hOldBitmap = ( HBITMAP )SelectObject( hdc, hBmp );

		BITMAPINFO* bmi = ( BITMAPINFO* )malloc( sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
		memset( bmi, 0, sizeof BITMAPINFO );
		bmi->bmiHeader.biSize = 0x28;
		if ( GetDIBits( hdc, hBmp, 0, 96, NULL, bmi, DIB_RGB_COLORS ) == 0 ) {
			return -11;
		}

		BITMAPINFOHEADER* pDib;
		BYTE* pDibBits;
		pDib = ( BITMAPINFOHEADER* )GlobalAlloc( LPTR, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 + bmi->bmiHeader.biSizeImage );
		if ( pDib == NULL )
			return -12;

		memcpy( pDib, bmi, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
		free(bmi);
		pDibBits = (( BYTE* )pDib ) + sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256;

		GetDIBits( hdc, hBmp, 0, pDib->biHeight, pDibBits, ( BITMAPINFO* )pDib, DIB_RGB_COLORS );
		SelectObject( hdc, hOldBitmap );
		DeleteDC( hdc );

		long dwPngSize = 0;
		DIB2PNG d2p;
		d2p.pbmi = (BITMAPINFO *) pDib;
		d2p.pDiData = pDibBits;
		d2p.pResult = NULL;
		d2p.pResultLen = &dwPngSize;

		if ( !CallService(MS_DIB2PNG, 0, (LPARAM) &d2p) ) {
			GlobalFree( pDib );
			return -13;
		}

		BYTE* pPngMemBuffer = new BYTE[ dwPngSize ];
		d2p.pResult = pPngMemBuffer;
		CallService(MS_DIB2PNG, 0, (LPARAM) &d2p);
		GlobalFree( pDib );

		FILE* out = fopen( szFilename, "wb" );
		if ( out != NULL ) {
			fwrite( pPngMemBuffer, dwPngSize, 1, out );
			fclose( out );
			delete pPngMemBuffer;
			return 0;
		}
		else
		{
			delete pPngMemBuffer;
			return -15;
		}
	}
	else
	{
		return -1;
	}
}

#include "savebmp.c"
int SaveBMP(HBITMAP hBmp, const char *szFilename)
{
	HDC hdc;
	HBITMAP orig;

	hdc = CreateCompatibleDC(NULL);
	orig = (HBITMAP) SelectObject(hdc, hBmp);

	int ret = SaveBitmapFile(hdc, hBmp, szFilename);

	SelectObject(hdc, orig);
	DeleteObject(hdc);

	return ret;
}

/*
int SaveImage(HBITMAP hBmp, char *szFilename)
{
	IPicture *pic = NULL;
	
	OleInitialize(NULL);

	// Do the conversion from a HBITMAP to IPicture
	PICTDESC picture_desc = {0};
	picture_desc.cbSizeofstruct = sizeof(picture_desc);
	picture_desc.picType = PICTYPE_BITMAP;
	picture_desc.bmp.hbitmap = hBmp;
	picture_desc.bmp.hpal = NULL;
	HRESULT hr = OleCreatePictureIndirect(&picture_desc, IID_IPicture, FALSE, (void**)&pic);
	if (FAILED(hr)) 
	{
		OleUninitialize();
		return -1;
	}

	// Save it
	IStorage *pStg = NULL;
	WCHAR pszwFilename[MAX_PATH];
	MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);
    hr = StgCreateDocfile(pszwFilename, STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, 0, &pStg);
	if (FAILED(hr)) 
	{
		pic->Release();
		OleUninitialize();
		return -1;
	}

    IStream* pStream = NULL;
    hr = pStg->CreateStream(L"PICTURE", STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, 0, 0, &pStream);
    if(SUCCEEDED(hr))
    {
        hr = pic->SaveAsFile(pStream, 
            FALSE, // save mem copy
            NULL);
        pStream->Release();
    }
    pStg->Release();

	pic->Release();
	OleUninitialize();

	return 0;
}
*/

// Save an HBITMAP to an image
// wParam = HBITMAP
// lParam = filename
int BmpFilterSaveBitmap(WPARAM wParam,LPARAM lParam)
{
	HBITMAP hBmp = (HBITMAP) wParam;
	const char *szFile=(const char *)lParam;
	char szFilename[MAX_PATH];
	int filenameLen;
    
	if (!CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)szFile, (LPARAM)szFilename))
		mir_snprintf(szFilename, SIZEOF(szFilename), "%s", szFile);
	filenameLen=lstrlenA(szFilename);
	if(filenameLen>4) 
	{
		char* pszExt = szFilename+filenameLen-4;
		if (!lstrcmpiA( pszExt,".bmp" ) /*|| !lstrcmpiA( pszExt, ".rle" ) 
			|| !lstrcmpiA( pszExt, ".wmf" ) || !lstrcmpiA( pszExt, ".ico" ) */) 
		{
			return SaveBMP(hBmp, szFilename);
		}
		else if (!lstrcmpiA( pszExt, ".png" )) 
		{
			return SavePNG(hBmp, szFilename);
		}
		else if (!lstrcmpiA( pszExt, ".gif" )) 
		{
			return SaveGIF(hBmp, szFilename);
		}
		else if (!lstrcmpiA( pszExt, ".jpg" ) || !lstrcmpiA( pszExt, ".jpeg" )) 
		{
			return SaveJPEG(hBmp, szFilename);
		}
	}

	return -1;
}


// Returns != 0 if can save that type of image, = 0 if cant
// wParam = 0
// lParam = PA_FORMAT_*   // image format
int BmpFilterCanSaveBitmap(WPARAM wParam,LPARAM lParam)
{
	switch(lParam)
	{
		case PA_FORMAT_PNG: 
			return ServiceExists(MS_DIB2PNG);
		
		case PA_FORMAT_JPEG:
		case PA_FORMAT_GIF:
			return gdiPlusLoaded;

		case PA_FORMAT_BMP:
			return 1;

		default: 
			return 0;
	}
}
