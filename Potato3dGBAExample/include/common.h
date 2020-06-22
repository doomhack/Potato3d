#ifndef COMMON_H
#define COMMON_H

#include "f3dmath.h"

namespace P3D
{

    //#define USE_FLOAT
    //#define FB_32BIT //RGBX888 framebuffer format.

    #define PERSPECTIVE_CORRECT

    #ifdef FB_32BIT
        typedef unsigned int pixel;
    #else
        typedef unsigned short pixel;
    #endif


    #ifdef USE_FLOAT
        typedef float fp;
    #else
        typedef FP fp;
    #endif

}

#endif // COMMON_H
