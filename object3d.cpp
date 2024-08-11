#include "Config.h"
#include "object3d.h"

#include "PixelShaderDefault.h"
#include "PixelShaderGBA8.h"
#include "qlogging.h"

#include <string.h>

namespace P3D
{
    Object3d::Object3d()
    {
    }

    bool Object3d::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel *framebuffer)
    {
        //fp halfVFov = (aspect * hFov) / 2;

        fp halfVFov = 45;

        fp halfFrustrumWidth = zFar * std::tan((float)pD2R(halfVFov));
        fp halfFrustrumHeight = zFar * std::tan((float)pD2R(pASR(hFov, 1)));

        frustrumPoints[0] = V3<fp>(-halfFrustrumWidth, -halfFrustrumHeight, -zFar);
        frustrumPoints[1] = V3<fp>(halfFrustrumWidth, halfFrustrumHeight, -zFar);
        frustrumPoints[2] = V3<fp>(-halfFrustrumWidth, halfFrustrumHeight, -zFar);
        frustrumPoints[3] = V3<fp>(halfFrustrumWidth, -halfFrustrumHeight, -zFar);

        P3D::RenderTarget* render_target = new P3D::RenderTarget(screenWidth, screenHeight, framebuffer);
        render_target->AttachZBuffer();


        render_device = new P3D::RenderDevice();

        render_device->SetRenderTarget(render_target);

        float aspectRatio = (float)screenWidth / (float)screenHeight;

        render_device->SetPerspective(hFov, aspectRatio, zNear, zFar);

        //render_device->SetRenderFlags<P3D::NoFlags, P3D::PixelShaderGBA8<P3D::NoFlags>>();
        //render_device->SetRenderFlags<P3D::SubdividePerspectiveMapping, P3D::PixelShaderGBA8<P3D::SubdividePerspectiveMapping>>();
        //render_device->SetRenderFlags<P3D::RenderFlags::SubdividePerspectiveMapping | P3D::RenderFlags::Fog | P3D::RenderFlags::VertexLight>();
        render_device->SetRenderFlags<P3D::RenderFlags::SubdividePerspectiveMapping | P3D::RenderFlags::Fog | P3D::RenderFlags::VertexLight, P3D::PixelShaderGBA8<P3D::RenderFlags::SubdividePerspectiveMapping | P3D::RenderFlags::Fog | P3D::RenderFlags::VertexLight>>();
        //render_device->SetRenderFlags<P3D::RenderFlags::Fog>();

#if 0
        render_device->SetFogMode(FogLinear);
        render_device->SetFogColor(0x799ED7);
        render_device->SetFogDepth(2000, 3000);
#else
        render_device->SetFogMode(FogExponential2);
        render_device->SetFogColor(0x799ED7);
        render_device->SetFogDensity(2);
#endif

        if(sizeof(P3D::pixel) == 1)
        {
            render_device->SetFogLightMap(model->GetFogLightMap());
        }

        //render_device->SetRenderFlags<P3D::ZWrite | P3D::ZTest, PixelShaderGBA8<P3D::ZWrite | P3D::ZTest>>();

        return true;
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

        M4<fp> camMatrix = render_device->GetMatrix().Inverted();

        V4<fp> t1 = camMatrix * frustrumPoints[0];
        V4<fp> t2 = camMatrix * frustrumPoints[1];
        V4<fp> t3 = camMatrix * frustrumPoints[2];
        V4<fp> t4 = camMatrix * frustrumPoints[3];
#if 1
        viewFrustrumBB.AddPoint(cameraPos);

        viewFrustrumBB.AddPoint(V3<fp>(t1.x, t1.y, t1.z));
        viewFrustrumBB.AddPoint(V3<fp>(t2.x, t2.y, t2.z));
        viewFrustrumBB.AddPoint(V3<fp>(t3.x, t3.y, t3.z));
        viewFrustrumBB.AddPoint(V3<fp>(t4.x, t4.y, t4.z));
#else
        fp bb_size = 100;
        viewFrustrumBB.AddPoint(cameraPos);
        viewFrustrumBB.x1 -= bb_size;
        viewFrustrumBB.x2 += bb_size;

        viewFrustrumBB.y1 -= bb_size;
        viewFrustrumBB.y2 += bb_size;

        viewFrustrumBB.z1 -= bb_size;
        viewFrustrumBB.z2 += bb_size;
#endif

        V3<fp> t[3];

        t[0] = V3<fp>(t1.x, t1.y, t1.z);
        t[1] = V3<fp>(t2.x, t2.y, t2.z);
        t[2] = V3<fp>(t3.x, t3.y, t3.z);

        //render_device->DrawTriangle(t, nullptr);
    }

    void Object3d::DoCollisions()
    {
        const int bb_size = 100;
        AABB player_box;

        player_box.AddPoint(cameraPos);
        player_box.x1 -= bb_size;
        player_box.x2 += bb_size;

        player_box.y1 -= bb_size;
        player_box.y2 += bb_size;

        player_box.z1 -= bb_size;
        player_box.z2 += bb_size;

        std::vector<const BspModelTriangle*> tris;
/*
        BspModelTriangle t;
        t.tri.verts[0].pos = V3<fp>(0,0,0);
        t.tri.verts[1].pos = V3<fp>(-100,0,0);
        t.tri.verts[2].pos = V3<fp>(0,0,-100);

        V3<fp> p(-25, 10, -25);

        CheckCollision2(&t, p, 50);
*/
        model->SortFrontToBack(cameraPos, player_box, tris, true);

        for(int i = 0; i < tris.size(); i++)
        {
            if(CheckCollision(tris.at(i), cameraPos, 50))
            {
                i = 0;
                continue;
            }
        }

    }

    bool Object3d::CheckCollision(const BspModelTriangle* tri, V3<fp>& point, const fp radius)
    {
        //Compute Normal
        V3<fp> side1 = tri->tri.verts[1].pos - tri->tri.verts[0].pos;
        V3<fp> side2 = tri->tri.verts[2].pos - tri->tri.verts[0].pos;

        V3<fp> normal = side2.CrossProductNormalised(side1);

        //Compute plane.
        float d = -normal.DotProduct(tri->tri.verts[0].pos);

        Plane<fp> plane(normal, d);

        fp distance = plane.DistanceToPoint(point);

        //No collision
        if(distance < 0 || distance >= radius)
            return false;

        fp margin = 0;

        V3<fp> edge01 = tri->tri.verts[1].pos - tri->tri.verts[0].pos;
        V3<fp> edge01_normal = edge01.CrossProductNormalised(normal);
        fp d01 = -edge01_normal.DotProduct(tri->tri.verts[0].pos);

        Plane<fp> plane01(edge01_normal, d01);
        fp pd01 = plane01.DistanceToPoint(point);
        if(pd01 < margin)
            return false;

        V3<fp> edge12 = tri->tri.verts[2].pos - tri->tri.verts[1].pos;
        V3<fp> edge12_normal = edge12.CrossProductNormalised(normal);
        fp d12 = -edge12_normal.DotProduct(tri->tri.verts[1].pos);

        Plane<fp> plane12(edge12_normal, d12);
        fp pd12 = plane12.DistanceToPoint(point);
        if(pd12 < margin)
            return false;

        V3<fp> edge20 = tri->tri.verts[0].pos - tri->tri.verts[2].pos;
        V3<fp> edge20_normal = edge20.CrossProductNormalised(normal);
        fp d20 = -edge20_normal.DotProduct(tri->tri.verts[2].pos);

        Plane<fp> plane20(edge20_normal, d20);
        fp pd20 = plane20.DistanceToPoint(point);
        if(pd20 < margin)
            return false;

        //Move in direction of normal.
        fp penetrationDepth = radius - distance;
        point += normal * (penetrationDepth + fp(0.1));

        return true;
    }

    void Object3d::RenderScene()
    {        
        render_device->ClearColor(backgroundColor);
        render_device->ClearDepth(1);

        render_device->PushMatrix();

        render_device->RotateX(-cameraAngle.x);
        render_device->RotateY(-cameraAngle.y);
        render_device->RotateZ(-cameraAngle.z);

        render_device->Translate(V3<fp>(-cameraPos.x, -cameraPos.y, -cameraPos.z));

        if(update_frustrum_bb)
            UpdateFrustrumAABB();

        render_device->BeginFrame();

        render_device->BeginDraw(update_frustrum_bb ? frustrumPlanes : nullptr);

        RenderBsp();

        render_device->EndDraw();

        render_device->EndFrame();

        render_device->PopMatrix();
    }

    void Object3d::RenderBsp()
    {
        if(model == nullptr)
            return;

        static std::vector<const BspModelTriangle*> tris;

        //model->SortBackToFront(cameraPos, viewFrustrumBB, tris, true);
        model->SortBackToFront(cameraPos, viewFrustrumBB, tris, true);

        for(unsigned int i = 0; i < tris.size(); i++)
        {            
            const BspModelTriangle* tri = tris[i];
#if 1
            if(!frustrumPlanes[Left].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
                continue;

            if(!frustrumPlanes[Right].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
                continue;

            if(!frustrumPlanes[Top].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
                continue;

            if(!frustrumPlanes[Bottom].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
                continue;

            if(!frustrumPlanes[Near].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
                continue;

            if(!frustrumPlanes[Far].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
                continue;
#endif
            const BspNodeTexture* ntex = model->GetTexture(tri->texture);

            V3<fp> verts[3] = {tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos};

            Material m;

            if(ntex)
            {
                m.type = Material::Texture;
                m.pixels = model->GetTexturePixels(ntex->texture_pixels_offset);

                //m.type = Material::Color;
                //m.color = *model->GetTexturePixels(ntex->texture_pixels_offset);

                fp light_levels[3] = {light_level, light_level, light_level};

                V2<fp> uvs[3] = {tri->tri.verts[0].uv, tri->tri.verts[1].uv, tri->tri.verts[2].uv};

                render_device->SetMaterial(m);

                render_device->DrawTriangle(verts, uvs, light_levels);
            }
            else
            {
                m.color = tri->color;

                render_device->SetMaterial(m);

                render_device->DrawTriangle(verts);
            }
        }

        tris.clear();
    }

    void Object3d::SetModel(const BspModel *model)
    {
        this->model = model;
    }

    void Object3d::SetBackgroundColor(pixel color)
    {
        backgroundColor = color;
    }

    void Object3d::SetFrameBuffer(pixel* buffer)
    {
        int w = render_target->GetWidth();
        int h = render_target->GetHeight();

        render_target->AttachColorBuffer(w, h, buffer);

        render_device->SetRenderTarget(render_target);
    }

#ifdef RENDER_STATS
    const RenderStats& Object3d::GetRenderStats()
    {
        return render_device->GetRenderStats();
    }
#endif
}
