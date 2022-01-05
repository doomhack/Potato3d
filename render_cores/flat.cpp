#include "../common.h"
#include "../rtypes.h"
#include "pixels.h"

namespace P3D
{
    void DrawTriangleScanlineFlat(const TriEdgeTrace& pos, const pixel color)
    {
        int x_start = (int)pos.x_left;
        int x_end = (int)pos.x_right;

        unsigned int count = (x_end - x_start) + 1;

        pixel* fb = pos.fb_ypos + x_start;

        if((size_t)fb & 1)
        {
            DrawScanlinePixelLinearHighByte(fb, &color, 0); fb++; count--;
        }

        if(count >> 1)
        {
            FastFill16((unsigned short*)fb, color | color << 8, count >> 1);
        }

        if(count & 1)
        {
            DrawScanlinePixelLinearLowByte(&fb[count-1], &color, 0);
        }
    }
}
