#include "../include/collision.h"

Collision::Collision()
{

}

bool Collision::CheckCollision(const P3D::BspModelTriangle* tri, const P3D::V3<P3D::fp>& point, const P3D::fp radius, P3D::V3<P3D::fp>& resolutionVector)
{
    P3D::fp distance = tri->normal_plane.DistanceToPoint(point);

    //No collision
    if(distance < 0 || distance >= radius)
        return false;

    P3D::fp margin = -P3D::fp(radius >> 4);

    if(tri->edge_plane_0_1.DistanceToPoint(point) < margin)
        return false;

    if(tri->edge_plane_1_2.DistanceToPoint(point) < margin)
        return false;

    if(tri->edge_plane_2_0.DistanceToPoint(point) < margin)
        return false;

    //Move in direction of normal.
    P3D::fp penetrationDepth = radius - distance;

    resolutionVector = tri->normal_plane.Normal() * (penetrationDepth + P3D::fp(0.01));

    return true;
}
