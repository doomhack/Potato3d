#ifndef COLLISION_H
#define COLLISION_H

#include "../include/common.h"
#include "../../Config.h"
#include "../../3dmaths/f3dmath.h"
#include "../../bspmodel.h"

class Collision
{
public:
    Collision();
    bool CheckCollision(const P3D::BspModelTriangle* tri, const P3D::V3<P3D::fp>& point, const P3D::fp radius, P3D::V3<P3D::fp>& resolutionVector);
};

#endif // COLLISION_H
