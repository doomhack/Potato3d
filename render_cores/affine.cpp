#include "../common.h"
#include "../rtypes.h"
#include "pixels.h"

namespace P3D
{
    void DrawTriangleScanlineAffine(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const Texture* texture)
    {
        int x_start = (int)pos.x_left;
        int x_end = (int)pos.x_right;

        unsigned int count = (x_end - x_start) + 1;

        pixel* fb = pos.fb_ypos + x_start;

        unsigned int uv = PackUV(pos.u_left, pos.v_left);
        unsigned int duv = PackUV(pASR(delta.u, 4), pASR(delta.v, 4));

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
            DrawScanlinePixelLinearPair(fb, t_pxl, uv, uv+duv); fb+=2; uv += (duv << 1);
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
