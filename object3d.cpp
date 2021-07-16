#include "common.h"
#include "object3d.h"

#include <string.h>

namespace P3D
{

#ifdef USE_TEXTURE_CACHE
    static unsigned short texCacheEntries[TEX_CACHE_ENTRIES] = {65535};
    #ifndef __arm__
        static pixel texCache[TEX_CACHE_SIZE/sizeof(pixel)];
    #else
        static pixel* texCache = ((pixel*)0x6014000);
    #endif
#endif

    Object3d::Object3d()
    {
        renderFlags = (RenderFlags)(0);
    }

    Object3d::Object3d(Render* render)
    {
        this->render = render;
    }

    bool Object3d::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel *framebuffer)
    {
        this->render = new Render();

        fp halfFrustrumWidth = zFar * std::tan((float)pD2R(fp(30)));
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

        bool backface_cull = !(renderFlags & NoBackfaceCull);

#ifdef FRONT_TO_BACK
        model->SortFrontToBack(cameraPos, viewFrustrumBB, tris, backface_cull);
#else
        model->SortBackToFront(cameraPos, viewFrustrumBB, tris, backface_cull);
#endif

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
#ifndef USE_TEXTURE_CACHE
                    tex->pixels = model->GetTexturePixels(ntex->texture_pixels_offset);
#endif
                    textureMap[ntex] = tex;
                }


#ifdef USE_TEXTURE_CACHE
                const unsigned int cacheKey = tri->texture & (TEX_CACHE_ENTRIES - 1);

                pixel* texCacheSlot = &texCache[TEX_SIZE_PIXELS * cacheKey];

                if(texCacheEntries[cacheKey] != tri->texture)
                {
                    FastCopy32((unsigned int*)texCacheSlot, (unsigned int*)model->GetTexturePixels(ntex->texture_pixels_offset), TEX_SIZE_BYTES);
                    texCacheEntries[cacheKey] = tri->texture;
                }

                tex->pixels = texCacheSlot;
#endif
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

    void Object3d::SetFramebuffer(pixel *framebuffer)
    {
        render->SetFramebuffer(framebuffer);
    }

    Render* Object3d::GetRender()
    {
        return render;
    }

}
