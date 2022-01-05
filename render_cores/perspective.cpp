#include "../common.h"
#include "../rtypes.h"
#include "pixels.h"

namespace P3D
{

    void DrawTriangleScanlinePerspectiveCorrect(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture)
    {
        int x_start = (int)pos.x_left;
        int x_end = (int)pos.x_right;

        unsigned int count = (x_end - x_start) + 1;

        fp u = pos.u_left, v = pos.v_left, w = pos.w_left;

        pixel* fb = pos.fb_ypos + x_start;

        fp invw_0 = pReciprocal(w);
        fp invw_15 = pReciprocal(w += delta.w);

        fp u0 = u * invw_0;
        fp u15 = (u += delta.u) * invw_15;

        fp v0 = v * invw_0;
        fp v15 = (v += delta.v) * invw_15;

        unsigned int uv = PackUV(u0, v0);
        unsigned int duv = PackUV(pASR(u15-u0, 4), pASR(v15-v0, 4));

        const pixel* t_pxl = texture->pixels;

        if((size_t)fb & 1)
        {
            DrawScanlinePixelLinearHighByte(fb, t_pxl, uv); fb++; uv += duv; count--;
        }

        unsigned int l = count >> 4;

        while(l--)
        {
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2;

            invw_0 = pReciprocal(w);
            invw_15 = pReciprocal(w += delta.w);

            u0 = u * invw_0;
            u15 = (u += delta.u) * invw_15;

            v0 = v * invw_0;
            v15 = (v += delta.v) * invw_15;

            uv = PackUV(u0, v0);
            duv = PackUV(pASR(u15-u0, 4), pASR(v15-v0, 4));
        }

        unsigned int r = ((count & 15) >> 1);

        switch(r)
        {
            case 7: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 6: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 5: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 4: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 3: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 2: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
            case 1: DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
        }

        if(count & 1)
            DrawScanlinePixelLinearLowByte(fb, t_pxl, uv);
    }
};
