#ifndef EXPERIENCE2K_IMAGE_H
#define EXPERIENCE2K_IMAGE_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Decode locally using XP's GDI+. The source is never modified. Output is an
   opaque, uncompressed 24-bit BMP fitted to the requested display dimensions. */
int e2k_convert_logon_image(const WCHAR *source, const WCHAR *destination,
                           UINT width, UINT height);
int e2k_write_solid_background(const WCHAR *destination, BYTE red, BYTE green, BYTE blue);

#ifdef __cplusplus
}
#endif
#endif
