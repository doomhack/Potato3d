#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <QtCore>
#include <QtMath>
#include <QtGui>

#include "common.h"

#include "3dmaths/f3dmath.h"

class Vertex3d
{
public:
    P3D::V3<fp> pos;
    P3D::V2<fp> uv;
};

class Vertex2d
{
public:
    P3D::V4<fp> pos;
    P3D::V2<fp> uv;

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
    QImage* texture;

    const QRgb* pixels;
    unsigned int width;
    unsigned int height;
};

class Mesh3d
{
public:
    QRgb color;
    Texture* texture;

    QVector<Triangle3d> tris;
};

class Object3d
{
public:
    P3D::V3<fp> pos;
    QVector<Mesh3d*> mesh;

    bool LoadFromObjFile(QString objFile, QString mtlFile);
};

#endif // OBJECT3D_H
