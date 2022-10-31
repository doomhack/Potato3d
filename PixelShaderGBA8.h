#ifndef PIXELSHADERGBA8_H
#define PIXELSHADERGBA8_H

#include "RenderCommon.h"
#include "RenderTriangle.h"

namespace P3D
{
    template<const unsigned int render_flags> class PixelShaderGBA8
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

            fp u = pos.u_left;
            fp v = pos.v_left;

            const fp du = delta.u;
            const fp dv = delta.v;


            if((size_t)fb & 1)
            {
                DrawScanlinePixelHighByte(fb, zb, z, texture, u, v); fb++, zb++, z += dz, u += du; v += dv; count--;
            }

            unsigned int l = count >> 4;

            while(l--)
            {
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
            }

            const unsigned int r = ((count & 15) >> 1);

            switch(r)
            {
                case 7: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                case 6: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                case 5: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                case 4: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                case 3: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                case 2: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                case 1: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
            }

            if(count & 1)
                DrawScanlinePixelLowByte(fb, zb, z, texture, u, v);
        }

        static void DrawTriangleScanlineHalfPerspectiveCorrect(const Internal::TriEdgeTrace& pos, const Internal::TriDrawXDeltaZWUV& delta, const pixel* texture)
        {
            const int x_start = (int)pos.x_left;
            const int x_end = (int)pos.x_right;

            unsigned int count = (x_end - x_start);

            pixel* fb = pos.fb_ypos + x_start;
            z_val* zb = pos.zb_ypos + x_start;

            fp z = pos.z_left;
            const fp dz = delta.z;


            fp u = pos.u_left, v = pos.v_left, w = pos.w_left;
            const fp idu = delta.u, idv = delta.v, idw = delta.w;

            if((size_t)fb & 1)
            {
                DrawScanlinePixelHighByte(fb, zb, z, texture, u/w, v/w); fb++; zb++, z += dz, u += idu, v += idv, w += idw, count--;
            }

            unsigned int l = count >> 4;


            while(l--)
            {
                fp u0 = u / w;
                fp v0 = v / w;

                w += (idw * 16);

                const fp w15 = w;

                u += (idu * 16);
                v += (idv * 16);

                const fp u15 = u / w15;
                const fp v15 = v / w15;

                const fp du = (u15 - u0) / 16;
                const fp dv = (v15 - v0) / 16;

                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
            }

            unsigned int r = ((count & 15) >> 1);

            fp u0 = u / w;
            fp v0 = v / w;

            w += (idw * 16);

            const fp w15 = w;

            u += (idu * 16);
            v += (idv * 16);

            const fp u15 = u / w15;
            const fp v15 = v / w15;

            const fp du = (u15 - u0) / 16;
            const fp dv = (v15 - v0) / 16;

            switch(r)
            {
                case 7: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                case 6: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                case 5: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                case 4: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                case 3: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                case 2: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                case 1: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
            }

            if(count & 1)
                DrawScanlinePixelLowByte(fb, zb, z, texture, u0, v0);
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

            if((size_t)fb & 1)
            {
                DrawScanlinePixelHighByte(fb, zb, z, texture, u/w, v/w); fb++, zb++, z += dz, u += du, v += dv, w += dw, count--;
            }

            unsigned int l = count >> 4;

            while(l--)
            {
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
            }

            unsigned int r = ((count & 15) >> 1);

            switch(r)
            {
                case 7: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                case 6: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                case 5: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                case 4: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                case 3: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                case 2: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                case 1: DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
            }

            if(count & 1)
                DrawScanlinePixelLowByte(fb, zb, z, texture, u/w, v/w);
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

            if((size_t)fb & 1)
            {
                DrawScanlinePixelHighByte(fb, zb, z, &color, 0, 0); fb++, zb++, z += dz, count--;
            }

            if constexpr (render_flags & (ZTest | ZWrite))
            {
                unsigned int l = count >> 4;

                while(l--)
                {
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                }

                const unsigned int r = ((count & 15) >> 1);

                switch(r)
                {
                    case 7: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                    case 6: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                    case 5: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                    case 4: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                    case 3: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                    case 2: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                    case 1: DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
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
                DrawScanlinePixelLowByte(fb, zb, z, &color, 0, 0);
        }

        static void DrawScanlinePixelPair(pixel* fb, z_val *zb, const z_val zv1, const z_val zv2, const pixel* texels, const fp u1, const fp v1, const fp u2, const fp v2)
        {
            if constexpr (render_flags & ZTest)
            {
                if(zv1 >= zb[0]) //Reject left?
                {
                    if(zv2 >= zb[1]) //Reject right?
                        return; //Both Z Reject.

                    //Accept right.
                    DrawScanlinePixelHighByte(fb+1, zb+1, zv2, texels, u2, v2);
                    return;
                }
                else //Accept left.
                {
                    if(zv2 >= zb[1]) //Reject right?
                    {
                        DrawScanlinePixelLowByte(fb, zb, zv1, texels, u1, v1);
                        return;
                    }
                }
            }

            const unsigned int tx = (int)u1 & TEX_MASK;
            const unsigned int ty = ((int)v1 & TEX_MASK) << TEX_SHIFT;

            const unsigned int tx2 = (int)u2 & TEX_MASK;
            const unsigned int ty2 = ((int)v2 & TEX_MASK) << TEX_SHIFT;

            *(unsigned short*)fb = ((texels[ty + tx]) | (texels[(ty2 + tx2)] << 8));

            if constexpr (render_flags & ZWrite)
            {
                zb[0] = zv1, zb[1] = zv2;
            }
        }

        static void DrawScanlinePixelHighByte(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v)
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

            const unsigned short texel = (texels[(ty + tx)] << 8) | *p8;

            *p16 = texel;
        }

        static void DrawScanlinePixelLowByte(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v)
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

            const unsigned short texel = texels[(ty + tx)] | (p8[1] << 8);

            *p16 = texel;
        }
    };

};
#endif // PIXELSHADERGBA8_H
