#ifndef CONFIGUSER_H
#define CONFIGUSER_H

#include "3dmaths/f3dmath.h"
#include "Pixel.h"

namespace P3D
{
    #ifndef __arm__
        #define RENDER_STATS
    #endif

    //Type of a texture and framebuffer pixel.

    typedef uint8_t pixel;
    typedef Pixel<3,2,3, pixel> pixelType;

    inline constexpr int LIGHT_LEVELS = 16;
    inline constexpr int FOG_LEVELS = 16;


    //Width & height of a texture in pixels.
    inline constexpr int TEX_SIZE = 64;

    //Maximum UV tiling. Increasing this will reduce bits available for perspective correct texture mapper.
    inline constexpr int TEX_MAX_TILE = 4;

    inline constexpr int CLIP_GUARD_BAND_SHIFT = 1;


    //#define USE_FLOAT
    #ifdef USE_FLOAT
        typedef double fp;
    #else
        typedef FP16 fp;
    #endif

    typedef fp z_val;

    inline constexpr int SUBDIVIDE_SPAN_LEN = 16;
    //inline constexpr fp SUBDIVIDE_Z_THREASHOLD = fp(11);
    inline constexpr fp SUBDIVIDE_Z_THREASHOLD = fp(2);
}

#endif // CONFIGUSER_H
