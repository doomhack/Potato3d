#ifndef CONFIGUSER_H
#define CONFIGUSER_H

#include "3dmaths/f3dmath.h"

namespace P3D
{
    #ifndef __arm__
        #define RENDER_STATS
    #endif

    //Type of a texture and framebuffer pixel.
    typedef unsigned char pixel;

    //Width & height of a texture in pixels.
    inline constexpr int TEX_SIZE = 64;

    //Maximum UV tiling. Increasing this will reduce bits available for perspective correct texture mapper.
    inline constexpr int TEX_MAX_TILE = 4;

    //#define USE_FLOAT
    #ifdef USE_FLOAT
        typedef double fp;
    #else
        typedef FP16 fp;
    #endif

    typedef fp z_val;
}

#endif // CONFIGUSER_H
