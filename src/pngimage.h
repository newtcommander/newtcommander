// SPDX-FileCopyrightText: 2026 Newt Commander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//*****************************************************************************
//
// CPngImage
//
// Loads a PNG from an RT_RCDATA resource via WIC (inbox Windows component)
// into a premultiplied 32-bpp DIB and alpha-blends it into a target DC.
// Used for the About/splash artwork (feature 035) so the shipped image is a
// plain hand-swappable PNG with no vector-renderer constraints.

class CPngImage
{
public:
    CPngImage();
    ~CPngImage();

    // discards the bitmap and resets the size
    void Clean();

    // Decodes the PNG resource 'resID' and scales it to fit into
    // 'width' x 'height' points with the aspect ratio preserved (WIC Fant
    // interpolation). If one dimension is -1, it is computed from the other;
    // if both are -1, the native PNG size is used.
    BOOL Load(int resID, int width, int height);

    void GetSize(SIZE* s);

    // 'hDC' is the destination DC; 'x'/'y' the destination coordinates;
    // 'width'/'height' the destination size (-1 = the stored size)
    void AlphaBlend(HDC hDC, int x, int y, int width, int height);

protected:
    int Width; // bitmap dimensions in points
    int Height;
    HBITMAP HBitmap;
};
