#ifndef COMMON_H
#define COMMON_H

#include "3dmaths/f3dmath.h"

namespace P3D
{
    #define RENDER_STATS

    #define USE_FLOAT

#ifndef __arm__
    //#define FB_32BIT //RGBX888 framebuffer format.
#endif

    #define FAST_LERP

    #ifdef FB_32BIT
        typedef unsigned int pixel;
        const pixel alphaMask = 0xffffff;
    #else
        typedef unsigned short pixel;
        const pixel alphaMask = 0x7fff;
    #endif


    #ifdef USE_FLOAT
        typedef double fp;
    #else
        typedef FP fp;
    #endif

}

#endif // COMMON_H
