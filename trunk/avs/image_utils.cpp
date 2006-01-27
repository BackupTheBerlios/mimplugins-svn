#include <windows.h>
#include <stdio.h>

#include "commonheaders.h"

using namespace Gdiplus;

INT GetEncoderClsid(const WCHAR* format, CLSID* pClsid);  // helper function



HBITMAP LoadPNG(struct avatarCacheEntry *ace, char *szFilename)
{
    LPVOID imgDecoder = NULL;
    LPVOID pImg = NULL;
    HBITMAP hBitmap = 0;
    LPVOID pBitmapBits = NULL;
    LPVOID m_pImgDecoder = NULL;

    if(!g_imgDecoderAvail)
        return 0;
    
    ImgNewDecoder(&m_pImgDecoder);
	if (!ImgNewDIBFromFile(m_pImgDecoder, szFilename, &pImg)) {
		if(pImg)
			ImgGetHandle(pImg, &hBitmap, (LPVOID *)&pBitmapBits);
	}
    ImgDeleteDecoder(m_pImgDecoder);
    if(hBitmap == 0)
        return 0;
    ace->hbmPic = hBitmap;
    ace->lpDIBSection = pImg;
    return hBitmap;
}


HBITMAP LoadAnyImage(char *szFilename, LPVOID *lpDIBSection)
{
	HBITMAP hBmp;
    char *szExt = &szFilename[lstrlenA(szFilename) - 4];

    if(!(!_stricmp(szExt, ".jpg") || (!_stricmp(szExt, ".gif") && !g_imgDecoderAvail) || !_stricmp(szExt, ".bmp") || !_stricmp(szExt, ".dat"))) {
		// Gif too can have transparency... image decoder today cant load it, but a gif with tranpsarency load by imagedecoder seens better than
		// one loaded from MS_UTILS_LOADBITMAP
        if(!_stricmp(szExt, ".png") || !_stricmp(szExt, ".gif")) { 
            struct avatarCacheEntry ace_private = {0};
            
            hBmp = LoadPNG(&ace_private, szFilename);
            if(hBmp == 0)
                return 0;
            *lpDIBSection = ace_private.lpDIBSection;
        }
        else
            return 0;
    }
    else {
        hBmp = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM)szFilename);
        *lpDIBSection = NULL;
    }

	return hBmp;
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


int SavePNG(HBITMAP hBmp, const char *szFilename)
{
	HDC hdc = CreateCompatibleDC( NULL );
	HBITMAP hOldBitmap = ( HBITMAP )SelectObject( hdc, hBmp );

	BITMAPINFO* bmi = ( BITMAPINFO* )malloc( sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
	memset( bmi, 0, sizeof BITMAPINFO );
	bmi->bmiHeader.biSize = 0x28;
	if ( GetDIBits( hdc, hBmp, 0, 96, NULL, bmi, DIB_RGB_COLORS ) == 0 ) {
		return -1;
	}

	BITMAPINFOHEADER* pDib;
	BYTE* pDibBits;
	pDib = ( BITMAPINFOHEADER* )GlobalAlloc( LPTR, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 + bmi->bmiHeader.biSizeImage );
	if ( pDib == NULL )
		return -2;

	memcpy( pDib, bmi, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
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
		return -3;
	}

	BYTE* pPngMemBuffer = new BYTE[ dwPngSize ];
	d2p.pResult = pPngMemBuffer;
	CallService(MS_DIB2PNG, 0, (LPARAM) &d2p);
	GlobalFree( pDib );

	FILE* out = fopen( szFilename, "wb" );
	if ( out != NULL ) {
		fwrite( pPngMemBuffer, dwPngSize, 1, out );
		fclose( out );
	}
	delete pPngMemBuffer;

	return 0;
}

int SaveJPEG(HBITMAP hBmp, const char *szFilename)
{
	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	CLSID             encoderClsid;
	Status            stat;

	// Get a JPEG image from the disk.
	Bitmap *image = new Bitmap(hBmp, NULL);

	// Get the CLSID of the JPEG encoder.
	if (GetEncoderClsid(L"image/jpeg", &encoderClsid) < 0)
		return -2;

	WCHAR pszwFilename[MAX_PATH];
	MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);
	stat = image->Save(pszwFilename, &encoderClsid, NULL);

	delete image;
	GdiplusShutdown(gdiplusToken);
	
	if(stat == Ok)
		return 0;
	else
		return -1;
}

int SaveGIF(HBITMAP hBmp, const char *szFilename)
{
	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	CLSID             encoderClsid;
	Status            stat;

	// Get a JPEG image from the disk.
	Bitmap *image = new Bitmap(hBmp, NULL);

	// Get the CLSID of the GIF encoder.
	if (GetEncoderClsid(L"image/gif", &encoderClsid) < 0)
		return -2;

	WCHAR pszwFilename[MAX_PATH];
	MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);
	stat = image->Save(pszwFilename, &encoderClsid, NULL);

	delete image;
	GdiplusShutdown(gdiplusToken);
	
	if(stat == Ok)
		return 0;
	else
		return -1;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

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



#include <ocidl.h>
#include <olectl.h>

int SaveImage(HBITMAP hBmp, char *szFilename)
{
	IPicture *pic = NULL;
	
	OleInitialize(NULL);

	// Do the conversion from a HBITMAP to IPicture
	PICTDESC picture_desc;
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
            TRUE, // save mem copy
            NULL);
        pStream->Release();
    }
    pStg->Release();

	pic->Release();
	OleUninitialize();

	return 0;
}

