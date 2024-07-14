#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore>
#include <QtGui>

#include "../3dmaths/f3dmath.h"

namespace Obj2Bsp
{
    inline constexpr int TEX_SIZE = 64;
    inline constexpr int TEX_MAX_TILE = 4;

    inline constexpr QImage::Format textureFormat = QImage::Format_Indexed8;


    inline constexpr int LIGHT_LEVELS = 16;
    inline constexpr int FOG_LEVELS = 16;

    inline constexpr int FOG_COLOR = 0x799ED7;
}

namespace P3D
{
    typedef quint8 pixel;

    typedef P3D::FP16 fp;
}

#endif // CONFIG_H
