#include "../include/mainloop.h"
#include "../include/videosystem.h"


MainLoop::MainLoop()
{
    const P3D::fp halfHFov = hFov / 2;
    const P3D::fp halfVFov = vFov / 2;

    const P3D::fp halfFrustrumWidth = zFar * std::tan((float)P3D::pD2R(halfHFov));
    const P3D::fp halfFrustrumHeight = zFar * std::tan((float)P3D::pD2R(halfVFov));

    frustrumPoints[0] = P3D::V3<P3D::fp>(-halfFrustrumWidth, -halfFrustrumHeight, -zFar);
    frustrumPoints[1] = P3D::V3<P3D::fp>(halfFrustrumWidth, halfFrustrumHeight, -zFar);
    frustrumPoints[2] = P3D::V3<P3D::fp>(-halfFrustrumWidth, halfFrustrumHeight, -zFar);
    frustrumPoints[3] = P3D::V3<P3D::fp>(halfFrustrumWidth, -halfFrustrumHeight, -zFar);
}

void MainLoop::Run()
{
    vid.Setup(&keyState);
    vid.SetPalette(model.GetModel()->GetColorMap());

    //constexpr unsigned int flags = P3D::ZWrite | P3D::ZTest;

    constexpr unsigned int flags = P3D::NoFlags;
    //constexpr unsigned int flags = P3D::SubdividePerspectiveMapping;
    //constexpr unsigned int flags = P3D::Fog;
    //constexpr unsigned int flags = P3D::SubdividePerspectiveMapping | P3D::Fog;
    //constexpr unsigned int flags = P3D::SubdividePerspectiveMapping | P3D::VertexLight | P3D::Fog;

    renderDev.SetRenderFlags<flags, P3D::PixelShaderGBA8<flags>>();


    renderDev.SetPerspective(vFov, 1.5, zNear, zFar);

    renderDev.SetFogMode(P3D::FogExponential2);
    renderDev.SetFogDensity(1.33);

    //renderDev.SetFogMode(P3D::FogLinear);
    //renderDev.SetFogDepth(750, 1000);


    renderDev.SetFogLightMap(model.GetModel()->GetFogLightMap());

    const P3D::pixel background = model.GetModel()->GetFogLightMap()[(P3D::FOG_LEVELS-1)*256];

    while(true)
    {
        RunTimeslots();

        renderDev.SetRenderTarget(vid.GetBackBuffer());

        renderDev.ClearColor(background);
        //renderDev.ClearDepth(std::numeric_limits<P3D::fp>::max());


        renderDev.PushMatrix();

        renderDev.RotateX(-camera.GetAngle().x);
        renderDev.RotateY(-camera.GetAngle().y);
        renderDev.RotateZ(-camera.GetAngle().z);

        renderDev.Translate(P3D::V3<P3D::fp>(-camera.GetEyePosition().x, -camera.GetEyePosition().y, -camera.GetEyePosition().z));

        P3D::AABB<short> viewFrustrumBB = GetFrustrumBB();

        renderDev.BeginFrame();

        renderDev.BeginDraw(frustrumPlanes);

        RenderModel(viewFrustrumBB);

        renderDev.EndDraw();

        renderDev.EndFrame();

        renderDev.PopMatrix();

        vid.PageFlip();        
    }
}

P3D::AABB<short> MainLoop::GetFrustrumBB()
{
    P3D::AABB<short> viewFrustrumBB = P3D::AABB<short>();

    P3D::M4<P3D::fp> camMatrix = renderDev.GetMatrix().Inverted();

    P3D::V4<P3D::fp> t1 = camMatrix * frustrumPoints[0];
    P3D::V4<P3D::fp> t2 = camMatrix * frustrumPoints[1];
    P3D::V4<P3D::fp> t3 = camMatrix * frustrumPoints[2];
    P3D::V4<P3D::fp> t4 = camMatrix * frustrumPoints[3];

    P3D::V3<short> c = P3D::V3<short>((int)camera.GetEyePosition().x, (int)camera.GetEyePosition().y, (int)camera.GetEyePosition().z);

    viewFrustrumBB.AddPoint(c);

    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t1.x, (int)t1.y, (int)t1.z));
    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t2.x, (int)t2.y, (int)t2.z));
    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t3.x, (int)t3.y, (int)t3.z));
    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t4.x, (int)t4.y, (int)t4.z));

    return viewFrustrumBB;
}

void MainLoop::RenderModel(const P3D::AABB<short>& viewFrustrumBB)
{
    model.GetModel()->Sort(camera.GetEyePosition(), viewFrustrumBB, triBuffer, true, true);

    //for(int i = triBuffer.size()-1; i >= 0; i--)
    for(unsigned int i = 0; i < triBuffer.Size(); i++)
    {
        const P3D::BspModelTriangle* tri = triBuffer.At(i);

        if(!FrustrumTestTriangle(tri))
            continue;

        const P3D::BspNodeTexture* ntex = model.GetModel()->GetTexture(tri->texture);

        P3D::V3<P3D::fp> verts[3] = {tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos};

        P3D::Material m;

        if(ntex)
        {
            m.type = P3D::Material::Texture;
            m.pixels = model.GetModel()->GetTexturePixels(ntex->texture_pixels_offset);

            const P3D::V2<P3D::fp> uvs[3] = {tri->tri.verts[0].uv, tri->tri.verts[1].uv, tri->tri.verts[2].uv};
            renderDev.SetMaterial(m);
/*
            const P3D::V3<P3D::fp> lightVector(0.66,0.66,0.33);

            const P3D::fp lightLevel = P3D::fp(0.5) + P3D::pASR(P3D::fp(1) + tri->normal_plane.Normal().DotProduct(lightVector), 2);

            const P3D::fp light_levels[3] = {lightLevel, lightLevel, lightLevel};

            renderDev.DrawTriangle(verts, uvs, light_levels);
            */
            renderDev.DrawTriangle(verts, uvs);
        }
        else
        {
            m.color = tri->color;

            renderDev.SetMaterial(m);

            renderDev.DrawTriangle(verts);
        }
    }
}

bool MainLoop::FrustrumTestTriangle(const P3D::BspModelTriangle* tri) const
{
    for(unsigned int i = P3D::Left; i <= P3D::Near; i++)
    {
        if(!frustrumPlanes[i].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
        {
            return false;
        }
    }

    return true;
}

void MainLoop::ResolveCollisions()
{
    const int bb_size = 100;
    P3D::AABB<short> player_box(P3D::V3<short>((int)camera.GetPosition().x, (int)camera.GetPosition().y, (int)camera.GetPosition().z), bb_size);

    model.GetModel()->Sort(camera.GetPosition(), player_box, triBuffer, false, false);

    int collision_count = 0;

    for(int i = triBuffer.Size() - 1; i >= 0; i--)
    {
        P3D::V3<P3D::fp> resolutionVector;

        if(collision.CheckCollision(triBuffer.At(i), camera.GetPosition(), 50, resolutionVector))
        {
            if( (pAbs(resolutionVector.x) + pAbs(resolutionVector.z)) > (pASR(resolutionVector.y,1)))
            {
                resolutionVector.y = 0;
            }

            camera.MovePosition(resolutionVector);

            i = triBuffer.Size() -1;
            collision_count++;

            if(collision_count > 5)
                break;
        }
    }

    if(collision_count > 0)
        gravity_velocity = 0;
    else
        gravity_velocity = P3D::pMin(gravity_velocity + 2, P3D::fp(25));
}

void MainLoop::RunTimeslots()
{
    static unsigned int gameTime = vid.GetTime();

    unsigned int realTime = vid.GetTime();

    while(gameTime < realTime)
    {
        vid.UpdateKeys();
        camera.HandleInput(keyState, gravity_velocity);

        ResolveCollisions();

        gameTime += frameTicks;
    }
}
