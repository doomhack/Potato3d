#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <vector>

#include "common.h"
#include "rtypes.h"
#include "render.h"

namespace P3D
{
    class Mesh3d
    {
    public:
        pixel color = 0;
        Texture* texture = nullptr;

        std::vector<Triangle3d*> tris;
    };

    class Model3d
    {
    public:
        V3<fp> pos = V3<fp>(0,0,0);
        std::vector<Mesh3d*> mesh;
    };

    class Object3d
    {
    public:
        Object3d();
        Object3d(Render* render);

        bool Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov = 54, fp zNear = 5, fp zFar = 1024, pixel* framebuffer = nullptr, fp* zBuffer = nullptr);

        V3<fp>& CameraPos();
        V3<fp>& CameraAngle();

        void RenderScene();

        void AddModel(const Model3d* model);

        void SetBackgroundColor(pixel color);

        void SetFramebuffer(pixel* frameBuffer);

    private:

        void DrawModel(const Model3d* model);
        void DrawMesh(const Mesh3d* mesh);

        Render* render = nullptr;

        std::vector<const Model3d*> models;

        V3<fp> cameraPos = V3<fp>(0,0,0);
        V3<fp> cameraAngle = V3<fp>(0,0,0);
        pixel backgroundColor = 0;
    };
}

#endif // OBJECT3D_H
