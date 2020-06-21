#ifndef COMMON_H
#define COMMON_H

#include "3dmaths/f3dmath.h"

//#define USE_FLOAT
#define FB_32BIT //RGBX888 framebuffer format.

#define PERSPECTIVE_CORRECT

#ifdef FB_32BIT
    typedef unsigned int pixel;
#else
    typedef unsigned short pixel;
#endif


#ifdef USE_FLOAT
    typedef float fp;
#else
    typedef P3D::FP fp;
#endif

#endif // COMMON_H
