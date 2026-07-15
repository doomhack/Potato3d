#ifndef MAINLOOP_H
#define MAINLOOP_H

#include "../include/common.h"
#include "../../Config.h"
#include "../../3dmaths/f3dmath.h"

#include "../../RenderDevice.h"
#include "../../PixelShaderGBA8.h"

#include "../include/videosystem.h"
#include "../include/camera.h"
#include "../include/worldmodel.h"
#include "../include/collision.h"

class MainLoop
{
public:
    MainLoop();

    void Run();

private:

    P3D::AABB<short> GetFrustrumBB();
    void RenderModel(const P3D::AABB<short> &viewFrustrumBB);
    bool FrustrumTestTriangle(const P3D::BspModelTriangle* tri) const;
    void ResolveCollisions();
    void RunTimeslots();
    void DrawSkybox();

    static constexpr P3D::fp zNear = METERS(0.25);
    static constexpr P3D::fp zFar = METERS(250);
    static constexpr P3D::fp vFov = 60;
    static constexpr P3D::fp hFov = 90;

    static constexpr unsigned int frameTicks = 50;

    P3D::fp gravity_velocity = 0;


    P3D::V3<P3D::fp> frustrumPoints[4]; //Top left and bottom-right frustrum points.

    P3D::Plane<P3D::fp> frustrumPlanes[6];

    P3D::RenderDevice renderDev;

    Camera camera;
    WorldModel model;
    VideoSystem vid;
    Collision collision;

    unsigned short keyState = 0;

    P3D::List<const P3D::BspModelTriangle*> triBuffer;
};

#endif // MAINLOOP_H
