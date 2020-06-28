#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <vector>

#include "common.h"
#include "rtypes.h"
#include "render.h"
#include "bsp3d.h"

namespace P3D
{
    class Object3d
    {
    public:
        Object3d();
        Object3d(Render* render);

        bool Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov = 54, fp zNear = 5, fp zFar = 1024, pixel* framebuffer = nullptr, fp* zBuffer = nullptr);

        V3<fp>& CameraPos();
        V3<fp>& CameraAngle();

        void RenderScene();

        void SetModel(const Model3d* model);

        void SetBackgroundColor(pixel color);

        void SetFramebuffer(pixel* framebuffer);

    private:

        void RenderBsp();

        Render* render = nullptr;

        const Model3d* model;

        const BspTree* bspTree;

        V3<fp> cameraPos = V3<fp>(0,0,0);
        V3<fp> cameraAngle = V3<fp>(0,0,0);
        pixel backgroundColor = 0;
    };
}

#endif // OBJECT3D_H
