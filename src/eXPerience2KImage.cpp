#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include "eXPerience2KImage.h"

using namespace Gdiplus;

static int save_bitmap(Bitmap &bitmap, const WCHAR *destination)
{
    /* The BMP encoder is part of GDI+ on both supported XP architectures. */
    const CLSID bmp_encoder = {
        0x557cf400, 0x1a04, 0x11d3,
        {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
    };
    return bitmap.Save(destination, &bmp_encoder, NULL) == Ok;
}

static int convert_image(const WCHAR *source, const WCHAR *destination,
                         UINT width, UINT height)
{
    Bitmap input(source, FALSE);
    GUID format;
    if (input.GetLastStatus() != Ok || input.GetRawFormat(&format) != Ok ||
        (!IsEqualGUID(format, ImageFormatPNG) &&
         !IsEqualGUID(format, ImageFormatJPEG) &&
         !IsEqualGUID(format, ImageFormatBMP))) return 0;
    UINT image_width = input.GetWidth(), image_height = input.GetHeight();
    if (!image_width || !image_height || image_width > 16384 || image_height > 16384 ||
        (ULONGLONG)image_width * image_height > 32000000) return 0;

    /* Honour the camera orientation metadata common in JPEG photographs. */
    UINT property_size = input.GetPropertyItemSize(0x0112);
    if (property_size >= sizeof(PropertyItem) && property_size <= 4096) {
        PropertyItem *property = static_cast<PropertyItem *>(
            HeapAlloc(GetProcessHeap(), 0, property_size));
        if (!property) return 0;
        if (input.GetPropertyItem(0x0112, property_size, property) == Ok &&
            property->type == 3 && property->length >= sizeof(WORD) && property->value) {
            WORD orientation = *static_cast<WORD *>(property->value);
            const RotateFlipType transforms[] = {
                RotateNoneFlipNone, RotateNoneFlipNone, RotateNoneFlipX,
                Rotate180FlipNone, Rotate180FlipX, Rotate90FlipX,
                Rotate90FlipNone, Rotate270FlipX, Rotate270FlipNone
            };
            if (orientation >= 2 && orientation <= 8 &&
                input.RotateFlip(transforms[orientation]) != Ok) {
                HeapFree(GetProcessHeap(), 0, property);
                return 0;
            }
        }
        HeapFree(GetProcessHeap(), 0, property);
    }
    image_width = input.GetWidth();
    image_height = input.GetHeight();
    Bitmap output(width, height, PixelFormat24bppRGB);
    if (output.GetLastStatus() != Ok) return 0;
    {
        Graphics graphics(&output);
        if (graphics.GetLastStatus() != Ok ||
            graphics.Clear(Color(255, 58, 110, 165)) != Ok ||
            graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic) != Ok ||
            graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality) != Ok) return 0;
        double scale = static_cast<double>(width) / image_width;
        double vertical_scale = static_cast<double>(height) / image_height;
        if (vertical_scale < scale) scale = vertical_scale;
        INT fitted_width = static_cast<INT>(image_width * scale + 0.5);
        INT fitted_height = static_cast<INT>(image_height * scale + 0.5);
        if (fitted_width < 1) fitted_width = 1;
        if (fitted_height < 1) fitted_height = 1;
        Rect destination_rect((static_cast<INT>(width) - fitted_width) / 2,
                              (static_cast<INT>(height) - fitted_height) / 2,
                              fitted_width, fitted_height);
        ImageAttributes attributes;
        attributes.SetWrapMode(WrapModeTileFlipXY);
        if (graphics.DrawImage(&input, destination_rect, 0, 0,
                               image_width, image_height, UnitPixel,
                               &attributes) != Ok) return 0;
    }
    return save_bitmap(output, destination);
}

extern "C" int e2k_convert_logon_image(const WCHAR *source, const WCHAR *destination,
                                       UINT width, UINT height)
{
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!source || !destination || lstrcmpiW(source, destination) == 0 ||
        !width || !height || width > 8192 || height > 8192 ||
        (ULONGLONG)width * height > 32000000 ||
        !GetFileAttributesExW(source, GetFileExInfoStandard, &info) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || info.nFileSizeHigh ||
        info.nFileSizeLow > 64 * 1024 * 1024) return 0;
    GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &startup, NULL) != Ok) return 0;
    int result = convert_image(source, destination, width, height);
    GdiplusShutdown(token);
    if (!result) DeleteFileW(destination);
    return result;
}

extern "C" int e2k_write_solid_background(const WCHAR *destination,
                                          BYTE red, BYTE green, BYTE blue)
{
    GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (!destination || GdiplusStartup(&token, &startup, NULL) != Ok) return 0;
    int result = 0;
    {
        Bitmap output(8, 8, PixelFormat24bppRGB);
        {
            Graphics graphics(&output);
            result = output.GetLastStatus() == Ok && graphics.GetLastStatus() == Ok &&
                     graphics.Clear(Color(255, red, green, blue)) == Ok;
        }
        if (result) result = save_bitmap(output, destination);
    }
    GdiplusShutdown(token);
    if (!result) DeleteFileW(destination);
    return result;
}
