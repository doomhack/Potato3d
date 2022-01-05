#ifndef PIXELS_H
#define PIXELS_H

#include "../common.h"

namespace P3D
{
    inline void DrawScanlinePixelLinearPair(pixel* fb, const pixel* texels, const unsigned int uv1, const unsigned int uv2)
    {
        unsigned int tx = (uv1 >> 26);
        unsigned int ty = ((uv1 >> 4) & (TEX_MASK << TEX_SHIFT));

        unsigned int tx2 = (uv2 >> 26);
        unsigned int ty2 = ((uv2 >> 4) & (TEX_MASK << TEX_SHIFT));

        *(unsigned short*)fb = ((texels[ty + tx]) | (texels[(ty2 + tx2)] << 8));
    }

    inline void DrawScanlinePixelLinearLowByte(pixel *fb, const pixel* texels, const unsigned int uv)
    {
        unsigned int tx = (uv >> 26);
        unsigned int ty = ((uv >> 4) & (TEX_MASK << TEX_SHIFT));

        unsigned short* p16 = (unsigned short*)(fb);
        pixel* p8 = (pixel*)p16;

        unsigned short texel = texels[(ty + tx)] | (p8[1] << 8);

        *p16 = texel;
    }

    inline void DrawScanlinePixelLinearHighByte(pixel *fb, const pixel* texels, const unsigned int uv)
    {
        unsigned int tx = (uv >> 26);
        unsigned int ty = ((uv >> 4) & (TEX_MASK << TEX_SHIFT));

        unsigned short* p16 = (unsigned short*)(fb-1);
        pixel* p8 = (pixel*)p16;

        unsigned short texel = (texels[(ty + tx)] << 8) | *p8;

        *p16 = texel;
    }

    inline unsigned int PackUV(fp u, fp v)
    {
#ifndef USE_FLOAT
        unsigned int du = u.toFPInt();
        unsigned int dv = v.toFPInt();
#else
        unsigned int du = (unsigned int)pASL(u, 16);
        unsigned int dv = (unsigned int)pASL(v, 16);
#endif

        return ((du << 10) & 0xffff0000) | ((dv >> 6) & 0x0000ffff);
    }

};

#endif // PIXELS_H
