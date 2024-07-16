#ifndef PIXELSHADERDEFAULT_H
#define PIXELSHADERDEFAULT_H

#include "RenderCommon.h"
#include "RenderTriangle.h"

namespace P3D
{
    using pixel_pair = double_width_t<pixel>;

    template<const unsigned int render_flags> class PixelShaderDefault
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
                    DrawScanlinePixel(fb+1, zb+1, zv2, texels, u2, v2, f2, l2, fog_color, fog_light_map);
                    return;
                }
                else //Accept left.
                {
                    if(zv2 >= zb[1]) //Reject right?
                    {
                        DrawScanlinePixel(fb, zb, zv1, texels, u1, v1, f1, l1, fog_color, fog_light_map);
                        return;
                    }
                }
            }

            if constexpr (render_flags & ZWrite)
            {
                zb[0] = zv1, zb[1] = zv2;
            }

            const unsigned int tx = (unsigned int)u1 & TEX_MASK;
            const unsigned int ty = ((unsigned int)v1 & TEX_MASK) << TEX_SHIFT;

            const unsigned int tx2 = (unsigned int)u2 & TEX_MASK;
            const unsigned int ty2 = ((unsigned int)v2 & TEX_MASK) << TEX_SHIFT;

            pixel p1 = texels[(ty + tx)], p2 = texels[(ty2 + tx2)];

            if constexpr(render_flags & (Fog | VertexLight))
            {
                p1 = FogLightPixel(p1, f1, l1, fog_color, fog_light_map);
                p2 = FogLightPixel(p2, f2, l2, fog_color, fog_light_map);
            }

            *(pixel_pair*)fb = ( (p1) | ((pixel_pair)p2 << (sizeof(pixel)*8)) );
        }

        static void DrawScanlinePixel(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const fp l, const pixel fog_color, const unsigned char* fog_light_map = nullptr)
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
                p1 = FogLightPixel(p1, f, l, fog_color, fog_light_map);
            }

            *fb = p1;
        }

        static void DrawScanlinePixelHigh(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const fp l, const pixel fog_color, const unsigned char* fog_light_map = nullptr)
        {
            DrawScanlinePixel(fb, zb, zv, texels, u, v, f, l, fog_color, fog_light_map);
        }

        static void DrawScanlinePixelLow(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const fp l, const pixel fog_color, const unsigned char* fog_light_map = nullptr)
        {
            DrawScanlinePixel(fb, zb, zv, texels, u, v, f, l, fog_color, fog_light_map);
        }


        static constexpr pixel BlendPixel(const pixel src_color, const pixel dst_color, const fp f)
        {
            if constexpr (render_flags & Fog)
            {
                if(f == 0)
                    return src_color;
                else if(f == 1)
                    return dst_color;

                const pixelType src(src_color);
                const pixelType dst(dst_color);

                const pixelType out = pixelType(
                            pLerp(fp(src.R()), fp(dst.R()), f),
                            pLerp(fp(src.G()), fp(dst.G()), f),
                            pLerp(fp(src.B()), fp(dst.B()), f)
                        );

                return out;
            }
            else
            {
                return src_color;
            }
        }

        static constexpr pixel FogLightPixel(pixel src_color, fp fog_frac, fp light_frac, pixel fog_color, const unsigned char* fog_light_map)
        {
            if constexpr(sizeof(pixel) == 1)
            {
                //(color×16×16)+(light×16)+fog

                unsigned int light = 0, fog = 0;

                if constexpr(render_flags & VertexLight)
                {
                    light = pASL(light_frac, LIGHT_SHIFT);
                }

                if constexpr(render_flags & Fog)
                {
                    fog = pASL(fog_frac, FOG_SHIFT);
                }

                const unsigned int texel = src_color;

                return fog_light_map[pASL(texel, FOG_SHIFT + LIGHT_SHIFT) + pASL(light, LIGHT_SHIFT) + fog];
            }
            else
            {
                return BlendPixel(src_color, fog_color, fog_frac);
            }
        }
    };

};
#endif // PIXELSHADERDEFAULT_H
