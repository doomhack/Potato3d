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

        static void DrawScanlinePixelPair(pixel* fb, z_val *zb, const z_val zv1, const z_val zv2, const pixel* texels, const fp u1, const fp v1, const fp u2, const fp v2, const fp f1, const fp f2, const pixel fog_color)
        {
            if constexpr (render_flags & ZTest)
            {
                if(zv1 >= zb[0]) //Reject left?
                {
                    if(zv2 >= zb[1]) //Reject right?
                        return; //Both Z Reject.

                    //Accept right.
                    DrawScanlinePixel(fb+1, zb+1, zv2, texels, u2, v2, f2, fog_color);
                    return;
                }
                else //Accept left.
                {
                    if(zv2 >= zb[1]) //Reject right?
                    {
                        DrawScanlinePixel(fb, zb, zv1, texels, u1, v1, f1, fog_color);
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

            pixel p1 = BlendPixel(texels[(ty + tx)], fog_color, f1);
            pixel p2 = BlendPixel(texels[(ty2 + tx2)], fog_color, f2);

            *(pixel_pair*)fb = ( (p1) | ((pixel_pair)p2 << (sizeof(pixel)*8)) );
        }

        static void DrawScanlinePixel(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v, const fp f, const pixel fog_color)
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

            *fb = BlendPixel(texels[(ty + tx)], fog_color, f);
        }

        static pixel BlendPixel(const pixel src_color, const pixel dst_color, const fp f)
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
    };

};
#endif // PIXELSHADERDEFAULT_H
