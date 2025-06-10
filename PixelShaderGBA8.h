#ifndef PIXELSHADERGBA8_H
#define PIXELSHADERGBA8_H

#include "RenderCommon.h"

namespace P3D
{
    template<const unsigned int render_flags> class PixelShaderGBA8
    {
        public:

        static void DrawScanlinePixelPair(pixel* fb, z_val *zb, const z_val zv1, const z_val zv2, const pixel* texels, const fp u1, const fp v1, const fp u2, const fp v2, const fp f1, const fp f2, const fp l1, const fp l2, const pixel fog_color, const unsigned char* fog_light_map = nullptr)
        {
            if constexpr (render_flags & ZTest)
            {
                if(zv1 >= zb[0]) //Reject left?
                {
                    if(zv2 >= zb[1]) //Reject right?
                        return; //Both Z Reject.

                    //Accept right.
                    DrawScanlinePixelHigh(fb+1, zb+1, zv2, texels, u2, v2, f2, l2, fog_color);
                    return;
                }
                else //Accept left.
                {
                    if(zv2 >= zb[1]) //Reject right?
                    {
                        DrawScanlinePixelLow(fb, zb, zv1, texels, u1, v1, f1, l1, fog_color);
                        return;
                    }
                }
            }

            const unsigned int tx = (int)u1 & TEX_MASK;
            const unsigned int ty = ((int)v1 & TEX_MASK) << TEX_SHIFT;

            const unsigned int tx2 = (int)u2 & TEX_MASK;
            const unsigned int ty2 = ((int)v2 & TEX_MASK) << TEX_SHIFT;

            pixel p1 = texels[(ty + tx)], p2 = texels[(ty2 + tx2)];

            if constexpr(render_flags & (Fog | VertexLight))
            {
                p1 = FogLightPixel(p1, f1, l1, fog_light_map);
                p2 = FogLightPixel(p2, f2, l2, fog_light_map);
            }

            *(unsigned short*)fb = (p1 | (p2 << 8));

            if constexpr (render_flags & ZWrite)
            {
                zb[0] = zv1, zb[1] = zv2;
            }
        }

        static void DrawScanlinePixelHigh(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const fp l, const pixel, const unsigned char* fog_light_map = nullptr)
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

            pixel p1 = texels[(ty + tx)];

            if constexpr(render_flags & (Fog | VertexLight))
            {
                p1 = FogLightPixel(p1, f, l, fog_light_map);
            }

            unsigned short* p16 = (unsigned short*)(fb-1);
            const pixel* p8 = (pixel*)p16;

            const unsigned short texel = (p1 << 8) | *p8;

            *p16 = texel;
        }

        static void DrawScanlinePixelLow(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const fp l, const pixel, const unsigned char* fog_light_map = nullptr)
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

            pixel p1 = texels[(ty + tx)];

            if constexpr(render_flags & (Fog | VertexLight))
            {
                p1 = FogLightPixel(p1, f, l, fog_light_map);
            }

            unsigned short* p16 = (unsigned short*)(fb);
            const pixel* p8 = (pixel*)p16;

            const unsigned short texel = p1 | (p8[1] << 8);

            *p16 = texel;
        }

        static constexpr pixel FogLightPixel(pixel src_color, fp fog_frac, fp light_frac, const unsigned char* fog_light_map)
        {
            unsigned int light = 0, fog = 0;

            if constexpr(render_flags & VertexLight)
            {
                light = pASL(pClamp(fp(0), light_frac, LIGHT_MAX), LIGHT_SHIFT);
            }

            if constexpr(render_flags & Fog)
            {
                fog = pASL(pClamp(fp(0), fog_frac, FOG_MAX), FOG_SHIFT);
            }

            const unsigned int texel = src_color;

            return fog_light_map[FogLightIndex(texel, fog, light)];
        }

        static constexpr unsigned int FogLightIndex(const unsigned int color, const unsigned int fog, const unsigned int light)
        {
            return (light * (FOG_LEVELS * 256)) + (fog * 256) + color;
        }
    };
};
#endif // PIXELSHADERGBA8_H
