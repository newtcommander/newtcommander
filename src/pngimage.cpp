// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <wincodec.h>

#include "pngimage.h"

CPngImage::CPngImage()
{
    Width = 0;
    Height = 0;
    HBitmap = NULL;
}

CPngImage::~CPngImage()
{
    Clean();
}

void CPngImage::Clean()
{
    if (HBitmap != NULL)
    {
        HANDLES(DeleteObject(HBitmap));
        HBitmap = NULL;
    }
    Width = 0;
    Height = 0;
}

BOOL CPngImage::Load(int resID, int width, int height)
{
    Clean();

    HRSRC hRsrc = FindResource(HInstance, MAKEINTRESOURCE(resID), RT_RCDATA);
    if (hRsrc == NULL)
    {
        TRACE_E("CPngImage::Load() Resource not found! resID=" << resID);
        return FALSE;
    }
    void* rawPNG = LoadResource(HInstance, hRsrc);
    DWORD rawSize = SizeofResource(HInstance, hRsrc);
    if (rawPNG == NULL || rawSize == 0)
    {
        TRACE_E("CPngImage::Load() Cannot load resource! resID=" << resID);
        return FALSE;
    }

    // the splash screen decodes from a worker thread (parallel_invoke), so
    // make sure COM is initialized on the calling thread; S_OK/S_FALSE must
    // be balanced, RPC_E_CHANGED_MODE means COM is already up in another mode
    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    BOOL uninitCOM = SUCCEEDED(hrInit);

    BOOL ret = FALSE;
    IWICImagingFactory* factory = NULL;
    IWICStream* stream = NULL;
    IWICBitmapDecoder* decoder = NULL;
    IWICBitmapFrameDecode* frame = NULL;
    IWICBitmapScaler* scaler = NULL;
    IWICFormatConverter* converter = NULL;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&factory));
    if (SUCCEEDED(hr))
        hr = factory->CreateStream(&stream);
    if (SUCCEEDED(hr))
        hr = stream->InitializeFromMemory((BYTE*)rawPNG, rawSize); // resource memory lives for the whole process
    if (SUCCEEDED(hr))
        hr = factory->CreateDecoderFromStream(stream, NULL, WICDecodeMetadataCacheOnDemand, &decoder);
    if (SUCCEEDED(hr))
        hr = decoder->GetFrame(0, &frame);

    UINT srcW = 0;
    UINT srcH = 0;
    if (SUCCEEDED(hr))
        hr = frame->GetSize(&srcW, &srcH);
    if (SUCCEEDED(hr) && (srcW == 0 || srcH == 0))
        hr = E_FAIL;

    if (SUCCEEDED(hr))
    {
        // fit into the requested box with the aspect ratio preserved
        int dstW;
        int dstH;
        if (width == -1 && height == -1)
        {
            dstW = srcW;
            dstH = srcH;
        }
        else if (width == -1)
        {
            dstH = height;
            dstW = max(1, (int)((__int64)srcW * height / srcH));
        }
        else if (height == -1)
        {
            dstW = width;
            dstH = max(1, (int)((__int64)srcH * width / srcW));
        }
        else
        {
            double scale = min((double)width / srcW, (double)height / srcH);
            dstW = max(1, (int)(srcW * scale));
            dstH = max(1, (int)(srcH * scale));
        }

        hr = factory->CreateBitmapScaler(&scaler);
        if (SUCCEEDED(hr))
            hr = scaler->Initialize(frame, dstW, dstH, WICBitmapInterpolationModeFant);
        if (SUCCEEDED(hr))
            hr = factory->CreateFormatConverter(&converter);
        if (SUCCEEDED(hr))
        {
            // AlphaBlend(AC_SRC_ALPHA) needs premultiplied BGRA
            hr = converter->Initialize(scaler, GUID_WICPixelFormat32bppPBGRA,
                                       WICBitmapDitherTypeNone, NULL, 0.0,
                                       WICBitmapPaletteTypeCustom);
        }
        if (SUCCEEDED(hr))
        {
            HDC hMemDC = HANDLES(CreateCompatibleDC(NULL));
            BITMAPINFOHEADER bmhdr;
            memset(&bmhdr, 0, sizeof(bmhdr));
            bmhdr.biSize = sizeof(bmhdr);
            bmhdr.biWidth = dstW;
            bmhdr.biHeight = -dstH; // top-down, matches WIC CopyPixels row order
            bmhdr.biPlanes = 1;
            bmhdr.biBitCount = 32;
            bmhdr.biCompression = BI_RGB;
            void* bits = NULL;
            HBITMAP hBmp = HANDLES(CreateDIBSection(hMemDC, (CONST BITMAPINFO*)&bmhdr,
                                                    DIB_RGB_COLORS, &bits, NULL, 0));
            HANDLES(DeleteDC(hMemDC));
            if (hBmp != NULL)
            {
                UINT stride = dstW * 4;
                hr = converter->CopyPixels(NULL, stride, stride * dstH, (BYTE*)bits);
                if (SUCCEEDED(hr))
                {
                    HBitmap = hBmp;
                    Width = dstW;
                    Height = dstH;
                    ret = TRUE;
                }
                else
                    HANDLES(DeleteObject(hBmp));
            }
        }
    }

    if (!ret)
        TRACE_E("CPngImage::Load() PNG decode failed! resID=" << resID << " hr=0x" << std::hex << hr);

    if (converter != NULL)
        converter->Release();
    if (scaler != NULL)
        scaler->Release();
    if (frame != NULL)
        frame->Release();
    if (decoder != NULL)
        decoder->Release();
    if (stream != NULL)
        stream->Release();
    if (factory != NULL)
        factory->Release();
    if (uninitCOM)
        CoUninitialize();
    return ret;
}

void CPngImage::GetSize(SIZE* s)
{
    s->cx = Width;
    s->cy = Height;
}

void CPngImage::AlphaBlend(HDC hDC, int x, int y, int width, int height)
{
    if (HBitmap == NULL)
        return;

    HDC hMemTmpDC = HANDLES(CreateCompatibleDC(hDC));
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemTmpDC, HBitmap);

    if (width == -1)
        width = Width;
    if (height == -1)
        height = Height;

    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 0xff; // use the per-pixel alpha values
    bf.AlphaFormat = AC_SRC_ALPHA;
    ::AlphaBlend(hDC, x, y, width, height, hMemTmpDC, 0, 0, Width, Height, bf);

    SelectObject(hMemTmpDC, hOldBitmap);
    HANDLES(DeleteDC(hMemTmpDC));
}
