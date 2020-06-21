#ifndef RTYPES_H
#define RTYPES_H

#include "common.h"
#include "3dmaths/f3dmath.h"

namespace P3D
{
    class Vertex3d
    {
    public:
        V3<fp> pos;
        V2<fp> uv;
    };

    class Vertex2d
    {
    public:
        V4<fp> pos;
        V2<fp> uv;

        static const int uv_scale = 128;

        void toPerspectiveCorrect()
        {
            pos.w = fp(uv_scale) / pos.w;

            uv.x = uv.x * pos.w;
            uv.y = uv.y * pos.w;
        }
    };

    class Triangle3d
    {
    public:
        Vertex3d verts[3];
    };

    class Triangle2d
    {
    public:
        Vertex2d verts[3];
    };

    class Texture
    {
    public:
        const pixel* pixels;
        unsigned int width;
        unsigned int height;
    };
}

#endif // RTYPES_H
