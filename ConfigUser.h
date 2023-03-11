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
    //typedef unsigned char pixel;
    typedef uint8_t pixel;
    typedef Pixel<3,2,3, pixel> pixelType;

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

    inline constexpr int MIN_SPLIT_SPAN_LEN = 32;
    inline constexpr fp MIN_SPLIT_SPAN_Z_DELTA = 0.01;

}

#endif // CONFIGUSER_H
