#include "common.h"
#include "object3d.h"

namespace P3D
{
    Object3d::Object3d()
    {

    }

    Object3d::Object3d(Render* render)
    {
        this->render = render;
    }

    bool Object3d::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel* framebuffer, fp* zBuffer)
    {
        this->render = new Render();

        return render->Setup(screenWidth, screenHeight, hFov, zNear, zFar, framebuffer, zBuffer);
    }

    V3<fp>& Object3d::CameraPos()
    {
        return cameraPos;
    }

    V3<fp>& Object3d::CameraAngle()
    {
        return cameraAngle;
    }

    void Object3d::RenderScene()
    {
        render->ClearFramebuffer(backgroundColor, true);

        M4<fp>& viewMatrix = render->GetMatrix(MatrixType::View);

        viewMatrix.setToIdentity();
        viewMatrix.rotateX(-cameraAngle.x);
        viewMatrix.rotateY(-cameraAngle.y);
        viewMatrix.rotateZ(-cameraAngle.z);

        viewMatrix.translate(V3<fp>(-cameraPos.x, -cameraPos.y, -cameraPos.z));


        render->BeginFrame();

        RenderBsp();

        render->EndFrame();
    }

    void Object3d::RenderBsp()
    {
        if(bspTree == nullptr)
            return;

        render->BeginObject();

        std::vector<BspTriangle*> tris;

        bspTree->SortBackToFront(cameraPos, tris);

        for(unsigned int i = 0; i < tris.size(); i++)
        {
            BspTriangle* tri = tris[i];

            render->DrawTriangle(tri->tri, tri->texture, tri->color, (RenderFlags)(PerspectiveCorrect));
        }

        render->EndObject();
    }

    void Object3d::SetModel(const Model3d* model)
    {
        this->model = model;

        P3D::Bsp3d *bsp = new P3D::Bsp3d();

        bspTree = bsp->BuildBspTree(model);
    }

    void Object3d::SetBackgroundColor(pixel color)
    {
        backgroundColor = color;
    }

    void Object3d::SetFramebuffer(pixel* framebuffer)
    {
        render->SetFramebuffer(framebuffer);
    }
}
