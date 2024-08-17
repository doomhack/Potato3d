#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <vector>
#include <map>

#include "Config.h"
#include "bspmodel.h"

#include "RenderDevice.h"

namespace P3D
{
    class Object3d
    {
    public:
        Object3d();

        bool Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov = 54, fp zNear = 5, fp zFar = 1024, pixel *framebuffer = nullptr);

        V3<fp>& CameraPos();
        V3<fp>& CameraAngle();

        void DoCollisions();
        bool CheckCollision(const BspModelTriangle* tri, V3<fp>& point, const fp radius);


        void RenderScene();

        void SetModel(const BspModel* model);

        void SetBackgroundColor(pixel color);

        void SetFrameBuffer(pixel* buffer);

#ifdef RENDER_STATS
        const RenderStats& GetRenderStats();
#endif

        bool update_frustrum_bb = true;
        bool do_collisions = true;
        fp light_level = 1.0;

    private:

        void RenderBsp();

        void UpdateFrustrumAABB();

        RenderDevice* render_device = nullptr;

        RenderTarget* render_target = nullptr;

        const BspModel* model;

        AABB<fp> viewFrustrumBB;

        V3<fp> eyePos;
        V3<fp> cameraPos = V3<fp>(0,50,0);
        //V3<fp> cameraPos = V3<fp>(8508,13,8563);
        V3<fp> cameraAngle = V3<fp>(0,-236,0);
        pixel backgroundColor = 0x799ED7;

        V3<fp> frustrumPoints[4]; //Top left and bottom-right frustrum points.

        Plane<fp> frustrumPlanes[6];
    };
}

#endif // OBJECT3D_H
