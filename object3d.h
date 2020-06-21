#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <vector>

#include "common.h"
#include "rtypes.h"
#include "render.h"


class Mesh
{
public:
    pixel color;
    Texture* texture;

    std::vector<Triangle3d> tris;
};

class Model
{
public:
    V3<fp> pos;
    std::vector<Mesh*> mesh;
};

class Object3d
{
    public:
    Object3d();
    Object3d(Render* render);


private:
    Render* render;
};


#endif // OBJECT3D_H
