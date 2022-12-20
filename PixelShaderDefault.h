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

        static void DrawTriangleScanlineAffine(const Internal::TriEdgeTrace& pos, const Internal::TriDrawXDeltaZWUV& delta, const pixel* texture)
        {
            const int x_start = (int)pos.x_left;
            const int x_end = (int)pos.x_right;

            unsigned int count = (x_end - x_start);

            pixel* fb = pos.fb_ypos + x_start;
            z_val* zb = pos.zb_ypos + x_start;

            fp z = pos.z_left;
            const fp dz = delta.z;

            fp f = pos.f_left;
            const fp df = delta.f;
            const pixel fog_color = pos.fog_color;

            fp u = pos.u_left;
            fp v = pos.v_left;

            const fp du = delta.u;
            const fp dv = delta.v;


            if((size_t)fb & 1)
            {
                DrawScanlinePixel(fb, zb, z, texture, u, v, f, fog_color); fb++, zb++, z += dz, u += du, v += dv, f += df, count--;
            }

            unsigned int l = count >> 4;

            while(l--)
            {
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
            }

            const unsigned int r = ((count & 15) >> 1);

            switch(r)
            {
                case 7: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 6: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 5: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 4: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 3: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 2: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 1: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
            }

            if(count & 1)
                DrawScanlinePixel(fb, zb, z, texture, u, v, f, fog_color);
        }

        static void DrawTriangleScanlinePerspectiveCorrect(const Internal::TriEdgeTrace& pos, const Internal::TriDrawXDeltaZWUV& delta, const pixel* texture)
        {
            const int x_start = (int)pos.x_left;
            const int x_end = (int)pos.x_right;

            unsigned int count = (x_end - x_start);

            fp u = pos.u_left, v = pos.v_left, w = pos.w_left;
            const fp du = delta.u, dv = delta.v, dw = delta.w;

            pixel* fb = pos.fb_ypos + x_start;
            z_val* zb = pos.zb_ypos + x_start;

            fp z = pos.z_left;
            const fp dz = delta.z;

            fp f = pos.f_left;
            const fp df = delta.f;
            const pixel fog_color = pos.fog_color;

            if((size_t)fb & 1)
            {
                DrawScanlinePixel(fb, zb, z, texture, u/w, v/w, f, fog_color); fb++, zb++, z += dz, u += du, v += dv, w += dw, f += df, count--;
            }

            unsigned int l = count >> 4;

            while(l--)
            {
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
            }

            unsigned int r = ((count & 15) >> 1);

            switch(r)
            {
                case 7: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                case 6: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                case 5: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                case 4: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                case 3: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                case 2: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                case 1: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
            }

            if(count & 1)
                DrawScanlinePixel(fb, zb, z, texture, u/w, v/w, f, fog_color);
        }

        static void DrawTriangleScanlineFlat(const Internal::TriEdgeTrace& pos, const Internal::TriDrawXDeltaZWUV& delta,  const pixel color)
        {
            const int x_start = (int)pos.x_left;
            const int x_end = (int)pos.x_right;

            unsigned int count = (x_end - x_start);

            pixel* fb = pos.fb_ypos + x_start;
            z_val* zb = pos.zb_ypos + x_start;

            fp z = pos.z_left;
            const fp dz = delta.z;

            fp f = pos.f_left;
            const fp df = delta.f;
            const pixel fog_color = pos.fog_color;

            if((size_t)fb & 1)
            {
                DrawScanlinePixel(fb, zb, z, &color, 0, 0, f, fog_color); fb++, zb++, z += dz, f += df, count--;
            }

            if constexpr (render_flags & (ZTest | ZWrite | Fog))
            {
                unsigned int l = count >> 4;

                while(l--)
                {
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                }

                const unsigned int r = ((count & 15) >> 1);

                switch(r)
                {
                    case 7: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                    case 6: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                    case 5: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                    case 4: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                    case 3: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                    case 2: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                    case 1: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                }
            }
            else
            {
                if(count >> 1)
                {
                    FastFill16((unsigned short*)fb, color | color << 8, count >> 1); fb+=count-1;
                }
            }

            if(count & 1)
                DrawScanlinePixel(fb, zb, z, &color, 0, 0, f, fog_color);
        }

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
