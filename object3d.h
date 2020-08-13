#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <vector>
#include <map>

#include "common.h"
#include "rtypes.h"
#include "render.h"
#include "bspmodel.h"

namespace P3D
{
    class Object3d
    {
    public:
        Object3d();
        Object3d(Render* render);

        bool Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov = 54, fp zNear = 5, fp zFar = 1024, pixel* framebuffer = nullptr);

        V3<fp>& CameraPos();
        V3<fp>& CameraAngle();

        void RenderScene();

        void SetModel(const BspModel* model);

        void SetBackgroundColor(pixel color);

        void SetFramebuffer(pixel* framebuffer);

        Render* GetRender();

        bool update_frustrum_bb = true;

    private:

        void RenderBsp();

        void UpdateFrustrumAABB();

        Render* render = nullptr;

        const BspModel* model;

        AABB viewFrustrumBB;

        V3<fp> cameraPos = V3<fp>(0,0,0);
        V3<fp> cameraAngle = V3<fp>(0,0,0);
        pixel backgroundColor = 0;

        RenderFlags renderFlags = RenderFlags::NoFlags;

        V3<fp> frustrumPoints[2]; //Top left and bottom-right frustrum points.

        std::map<const BspNodeTexture*, const Texture*> textureMap;
    };
}

#endif // OBJECT3D_H
