#ifndef PIXELSHADERGBA8_H
#define PIXELSHADERGBA8_H

#include "RenderCommon.h"
#include "RenderTriangle.h"

namespace P3D
{
    template<const unsigned int render_flags> class PixelShaderGBA8
    {
        public:

        static void DrawScanlinePixelPair(pixel* fb, z_val *zb, const z_val zv1, const z_val zv2, const pixel* texels, const fp u1, const fp v1, const fp u2, const fp v2, const fp f1, const fp f2, const pixel fog_color)
        {
            if constexpr (render_flags & ZTest)
            {
                if(zv1 >= zb[0]) //Reject left?
                {
                    if(zv2 >= zb[1]) //Reject right?
                        return; //Both Z Reject.

                    //Accept right.
                    DrawScanlinePixelHigh(fb+1, zb+1, zv2, texels, u2, v2, f2, fog_color);
                    return;
                }
                else //Accept left.
                {
                    if(zv2 >= zb[1]) //Reject right?
                    {
                        DrawScanlinePixelLow(fb, zb, zv1, texels, u1, v1, f1, fog_color);
                        return;
                    }
                }
            }

            const unsigned int tx = (int)u1 & TEX_MASK;
            const unsigned int ty = ((int)v1 & TEX_MASK) << TEX_SHIFT;

            const unsigned int tx2 = (int)u2 & TEX_MASK;
            const unsigned int ty2 = ((int)v2 & TEX_MASK) << TEX_SHIFT;

            *(unsigned short*)fb = ((texels[ty | tx]) | (texels[(ty2 | tx2)] << 8));

            if constexpr (render_flags & ZWrite)
            {
                zb[0] = zv1, zb[1] = zv2;
            }
        }

        static void DrawScanlinePixelHigh(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const pixel fog_color)
        {
            if constexpr (render_flags & ZTest)
            {
                if(*zb <= zv)
                    return;
            }

            if constexpr (render_flags & ZWrite)
            {
                *zb = zv;
            }

            const unsigned int tx = (int)u & TEX_MASK;
            const unsigned int ty = ((int)v & TEX_MASK) << TEX_SHIFT;

            unsigned short* p16 = (unsigned short*)(fb-1);
            const pixel* p8 = (pixel*)p16;

            const unsigned short texel = (texels[(ty | tx)] << 8) | *p8;

            *p16 = texel;
        }

        static void DrawScanlinePixelLow(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const pixel fog_color)
        {
            if constexpr (render_flags & ZTest)
            {
                if(*zb <= zv)
                    return;
            }

            if constexpr (render_flags & ZWrite)
            {
                *zb = zv;
            }

            const unsigned int tx = (int)u & TEX_MASK;
            const unsigned int ty = ((int)v & TEX_MASK) << TEX_SHIFT;

            unsigned short* p16 = (unsigned short*)(fb);
            const pixel* p8 = (pixel*)p16;

            const unsigned short texel = texels[(ty | tx)] | (p8[1] << 8);

            *p16 = texel;
        }
    };
};
#endif // PIXELSHADERGBA8_H
