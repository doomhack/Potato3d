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


    #define USE_VERTEX_CACHE
    #define VERTEX_CACHE_SIZE 4096u

    #define PERSPECTIVE_CORRECT_Z_DELTA_THREASHOLD (fp(0.03f))

    #define USE_FLOAT

    typedef unsigned char pixel;

    #ifdef USE_FLOAT
        typedef double fp;
        typedef fp z_val;
    #else
        typedef FP fp;
        typedef unsigned short z_val;
    #endif

}

#endif // COMMON_H
