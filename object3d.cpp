#include "common.h"
#include "object3d.h"

#include <string.h>

namespace P3D
{
    Object3d::Object3d()
    {
    }

    bool Object3d::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel *framebuffer)
    {
        this->render = new Render();

        fp aspect = fp((int)screenWidth) / fp((int)screenHeight);
        //fp halfVFov = (aspect * hFov) / 2;

        fp halfVFov = 41;

        fp halfFrustrumWidth = zFar * std::tan((float)pD2R(halfVFov));
        fp halfFrustrumHeight = zFar * std::tan((float)pD2R(pASR(hFov, 1)));

        frustrumPoints[0] = V3<fp>(-halfFrustrumWidth, -halfFrustrumHeight, -zFar);
        frustrumPoints[1] = V3<fp>(halfFrustrumWidth, halfFrustrumHeight, -zFar);
        frustrumPoints[2] = V3<fp>(-halfFrustrumWidth, halfFrustrumHeight, -zFar);
        frustrumPoints[3] = V3<fp>(halfFrustrumWidth, -halfFrustrumHeight, -zFar);

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

        M4<fp> camMatrix = render->GetMatrix(MatrixType::View).Inverted();

        V4<fp> t1 = camMatrix * frustrumPoints[0];
        V4<fp> t2 = camMatrix * frustrumPoints[1];
        V4<fp> t3 = camMatrix * frustrumPoints[2];
        V4<fp> t4 = camMatrix * frustrumPoints[3];

        viewFrustrumBB.AddPoint(cameraPos);

        viewFrustrumBB.AddPoint(V3<fp>(t1.x, t1.y, t1.z));
        viewFrustrumBB.AddPoint(V3<fp>(t2.x, t2.y, t2.z));
        viewFrustrumBB.AddPoint(V3<fp>(t3.x, t3.y, t3.z));
        viewFrustrumBB.AddPoint(V3<fp>(t4.x, t4.y, t4.z));

/*
        Triangle3d t;

        t.verts[0].pos = V3<fp>(t1.x, t1.y, t1.z);
        t.verts[1].pos = V3<fp>(t2.x, t2.y, t2.z);
        t.verts[2].pos = V3<fp>(t3.x, t3.y, t3.z);

        render->DrawTriangle(&t, nullptr, 12345, NoFlags);
*/

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

        if(update_frustrum_bb)
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

        model->SortBackToFront(cameraPos, viewFrustrumBB, tris, true);

        for(unsigned int i = 0; i < tris.size(); i++)
        {            
            const BspModelTriangle* tri = tris[i];

            const BspNodeTexture* ntex = model->GetTexture(tri->texture);

            Texture* tex = nullptr;

            if(ntex)
            {
                tex = textureMap[ntex];

                if(tex == nullptr)
                {
                    tex = new Texture;

                    tex->alpha = ntex->alpha;
                    tex->width = ntex->width;
                    tex->height = ntex->height;
                    tex->u_mask = ntex->u_mask;
                    tex->v_mask = ntex->v_mask;
                    tex->v_shift = ntex ->v_shift;
                    tex->pixels = model->GetTexturePixels(ntex->texture_pixels_offset);
                    textureMap[ntex] = tex;
                }
            }

            render->DrawTriangle(&tri->tri, tex, tri->color, NoFlags);
            //render->DrawTriangle(&tri->tri, nullptr, tri->color, renderFlags);
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

    void Object3d::SetFramebuffer(pixel *framebuffer)
    {
        render->SetFramebuffer(framebuffer);
    }

    Render* Object3d::GetRender()
    {
        return render;
    }

}
