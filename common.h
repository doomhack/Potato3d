#ifndef COMMON_H
#define COMMON_H

#include "3dmaths/f3dmath.h"

namespace P3D
{
#ifndef __arm__
    #define RENDER_STATS
#endif

    #define TEX_SHIFT 6
    #define TEX_SIZE (1u << TEX_SHIFT)
    #define TEX_MASK (TEX_SIZE-1)
    #define TEX_SIZE_PIXELS (TEX_SIZE * TEX_SIZE)
    #define TEX_SIZE_BYTES (TEX_SIZE_PIXELS * sizeof(pixel))

    #define USE_TEXTURE_CACHE

    #define TEX_CACHE_SIZE 16*1024
    #define TEX_CACHE_ENTRIES (TEX_CACHE_SIZE/TEX_SIZE_BYTES)


    #define SPAN_NODES_LINE 16u

    #define USE_VERTEX_CACHE
    #define VERTEX_CACHE_SIZE 2048u


    //#define USE_FLOAT

    //#define FRONT_TO_BACK


    //#define PIXEL_DOUBLE

//#ifndef __arm__
    #define PIXEL_WRITE_BYTE_MIRRORS
//#endif


    typedef unsigned char pixel;
    //const pixel alphaMask = 0x7fff;

    #ifdef USE_FLOAT
        typedef double fp;
    #else
        typedef FP fp;
    #endif

}

#endif // COMMON_H
