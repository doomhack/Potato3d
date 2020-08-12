#include "common.h"
#include "object3d.h"

namespace P3D
{
    Object3d::Object3d()
    {
        renderFlags = (RenderFlags)(0);
    }

    Object3d::Object3d(Render* render)
    {
        this->render = render;
    }

    bool Object3d::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel* framebuffer)
    {
        this->render = new Render();

        fp halfFrustrumWidth = zFar * std::tan((float)pD2R(fp(45)));
        fp halfFrustrumHeight = zFar * std::tan((float)pD2R(pASR(hFov, 1)));

        frustrumPoints[0] = V3<fp>(-halfFrustrumWidth, -halfFrustrumHeight, -zFar);
        frustrumPoints[1] = V3<fp>(halfFrustrumWidth, halfFrustrumHeight, -zFar);

        return render->Setup(screenWidth, screenHeight, hFov, zNear, zFar, framebuffer);
    }

    V3<fp>& Object3d::CameraPos()
    {
        return cameraPos;
    }

    V3<fp>& Object3d::CameraAngle()
    {
        return cameraAngle;
    }

    void Object3d::UpdateFrustrumAABB()
    {
        viewFrustrumBB = AABB();

        M4<fp> camMatrix;
        camMatrix.setToIdentity();
        camMatrix.translate(V3<fp>(cameraPos.x,cameraPos.y,cameraPos.z));

        camMatrix.rotateX(cameraAngle.x);
        camMatrix.rotateY(cameraAngle.y);
        camMatrix.rotateZ(cameraAngle.z);

        viewFrustrumBB.AddPoint(cameraPos);

        V4<fp> t1 = camMatrix * frustrumPoints[0];
        V4<fp> t2 = camMatrix * frustrumPoints[1];

        viewFrustrumBB.AddPoint(V3<fp>(t1.x, t1.y, t1.z));
        viewFrustrumBB.AddPoint(V3<fp>(t2.x, t2.y, t2.z));
    }

    void Object3d::RenderScene()
    {
        render->ClearFramebuffer(backgroundColor);

        M4<fp>& viewMatrix = render->GetMatrix(MatrixType::View);

        viewMatrix.setToIdentity();
        viewMatrix.rotateX(-cameraAngle.x);
        viewMatrix.rotateY(-cameraAngle.y);
        viewMatrix.rotateZ(-cameraAngle.z);

        viewMatrix.translate(V3<fp>(-cameraPos.x, -cameraPos.y, -cameraPos.z));

        UpdateFrustrumAABB();

        render->BeginFrame();

        RenderBsp();

        render->EndFrame();
    }

    void Object3d::RenderBsp()
    {
        if(model == nullptr)
            return;

        render->BeginObject();

        static std::vector<const BspModelTriangle*> tris;

        bool backface_cull = !(renderFlags & NoBackfaceCull);

        model->SortFrontToBack(cameraPos, viewFrustrumBB, tris, backface_cull);

        for(unsigned int i = 0; i < tris.size(); i++)
        {
            const BspModelTriangle* tri = tris[i];

            const BspNodeTexture* ntex = model->GetTexture(tri->texture);

            const Texture* tex = nullptr;

            if(ntex)
            {
                if(textureMap.count(ntex) == 0)
                {
                    Texture* t = new Texture;

                    t->alpha = ntex->alpha;
                    t->width = ntex->width;
                    t->height = ntex->height;
                    t->u_mask = ntex->u_mask;
                    t->v_mask = ntex->v_mask;
                    t->v_shift = ntex ->v_shift;
                    t->pixels = model->GetTexturePixels(ntex->texture_pixels_offset);

                    tex = t;

                    textureMap[ntex] = tex;
                }
                else
                {
                    tex = textureMap[ntex];
                }
            }

            render->DrawTriangle(&tri->tri, tex, tri->color, renderFlags);
        }

        tris.clear();

        render->EndObject();
    }

    void Object3d::SetModel(const BspModel *model)
    {
        this->model = model;
    }

    void Object3d::SetBackgroundColor(pixel color)
    {
        backgroundColor = color;
    }

    void Object3d::SetFramebuffer(pixel* framebuffer)
    {
        render->SetFramebuffer(framebuffer);
    }

    Render* Object3d::GetRender()
    {
        return render;
    }

}
