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

    V3<fp>& Object3d::GetCameraPos()
    {
        return cameraPos;
    }

    void P3D::Object3d::SetCameraPos(const V3<fp> pos)
    {
        cameraPos = pos;
    }

    V3<fp>& Object3d::GetCameraAngle()
    {
        return cameraAngle;
    }

    void Object3d::SetCameraAngle(const V3<fp> rot)
    {
        cameraAngle = rot;
    }

    void Object3d::RenderScene()
    {
        render->ClearFramebuffer(0, true);

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

            render->DrawTriangle(triangle, mesh->texture, mesh->color, PerspectiveCorrect);
        }
    }

    void Object3d::AddModel(const Model3d* model)
    {
        models.push_back(model);
    }
}
