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

        for (auto it = models.begin() ; it != models.end(); ++it)
        {
            const Model3d* model = *it;

            DrawModel(model);
        }

        render->EndFrame();
    }

    void Object3d::RenderBsp()
    {
        render->ClearFramebuffer(backgroundColor, true);

        if(bspTree == nullptr)
            return;

        M4<fp>& viewMatrix = render->GetMatrix(MatrixType::View);

        viewMatrix.setToIdentity();
        viewMatrix.rotateX(-cameraAngle.x);
        viewMatrix.rotateY(-cameraAngle.y);
        viewMatrix.rotateZ(-cameraAngle.z);

        viewMatrix.translate(V3<fp>(-cameraPos.x, -cameraPos.y, -cameraPos.z));


        render->BeginFrame();

        render->BeginObject();

        std::vector<BspTriangle*> tris;

        bspTree->SortBackToFront(cameraPos, tris);

        for(unsigned int i = 0; i < tris.size(); i++)
        {
            BspTriangle* tri = tris[i];

            render->DrawTriangle(tri->tri, tri->texture, tri->color, (RenderFlags)(PerspectiveCorrect));
        }

        render->EndObject();

        render->EndFrame();
    }

    void Object3d::DrawModel(const Model3d* model)
    {
        M4<fp>& modelMatrix = render->GetMatrix(MatrixType::Model);

        modelMatrix.setToIdentity();
        modelMatrix.translate(model->pos);

        render->BeginObject();

        for (auto it = model->mesh.begin() ; it != model->mesh.end(); ++it)
        {
            const Mesh3d* mesh = *it;

            DrawMesh(mesh);
        }

        render->EndObject();
    }

    void Object3d::DrawMesh(const Mesh3d* mesh)
    {
        for (auto it = mesh->tris.begin() ; it != mesh->tris.end(); ++it)
        {
            const Triangle3d* triangle = *it;

            render->DrawTriangle(triangle, mesh->texture, mesh->color, (RenderFlags)(0));
        }
    }

    void Object3d::AddModel(const Model3d* model)
    {
        models.push_back(model);
    }

    void Object3d::SetBackgroundColor(pixel color)
    {
        backgroundColor = color;
    }

    void Object3d::SetFramebuffer(pixel* framebuffer)
    {
        render->SetFramebuffer(framebuffer);
    }

    void Object3d::SetBspTree(const BspTree *tree)
    {
        this->bspTree = tree;
    }
}
