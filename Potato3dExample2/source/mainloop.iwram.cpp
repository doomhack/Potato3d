#include "../include/mainloop.h"
#include "../include/videosystem.h"


MainLoop::MainLoop()
{
    const P3D::fp halfVFov = 45;

    const P3D::fp halfFrustrumWidth = zFar * std::tan((float)P3D::pD2R(halfVFov));
    const P3D::fp halfFrustrumHeight = zFar * std::tan((float)P3D::pD2R(P3D::pASR(vFov, 1)));

    frustrumPoints[0] = P3D::V3<P3D::fp>(-halfFrustrumWidth, -halfFrustrumHeight, -zFar);
    frustrumPoints[1] = P3D::V3<P3D::fp>(halfFrustrumWidth, halfFrustrumHeight, -zFar);
    frustrumPoints[2] = P3D::V3<P3D::fp>(-halfFrustrumWidth, halfFrustrumHeight, -zFar);
    frustrumPoints[3] = P3D::V3<P3D::fp>(halfFrustrumWidth, -halfFrustrumHeight, -zFar);
}

void MainLoop::Run()
{
    vid.Setup(&keyState);
    vid.SetPalette(model.GetModel()->GetColorMap());

    renderDev.SetRenderFlags<P3D::Fog, P3D::PixelShaderGBA8<P3D::Fog>>();

    renderDev.SetPerspective(vFov, 1.5, zNear, zFar);

    //renderDev.SetFogMode(P3D::FogExponential2);
    //renderDev.SetFogDensity(2);

    renderDev.SetFogMode(P3D::FogLinear);
    renderDev.SetFogDepth(750, 1000);


    renderDev.SetFogLightMap(model.GetModel()->GetFogLightMap());

    const P3D::pixel background = model.GetModel()->GetFogLightMap()[(P3D::FOG_LEVELS-1)*256];

    while(true)
    {
        RunTimeslots();

        renderDev.SetRenderTarget(vid.GetBackBuffer());

        renderDev.ClearColor(background);

        renderDev.PushMatrix();

        renderDev.RotateX(-camera.GetAngle().x);
        renderDev.RotateY(-camera.GetAngle().y);
        renderDev.RotateZ(-camera.GetAngle().z);

        renderDev.Translate(P3D::V3<P3D::fp>(-camera.GetEyePosition().x, -camera.GetEyePosition().y, -camera.GetEyePosition().z));

        UpdateFrustrumBB();

        renderDev.BeginFrame();

        renderDev.BeginDraw(frustrumPlanes);

        RenderModel();

        renderDev.EndDraw();

        renderDev.EndFrame();

        renderDev.PopMatrix();

        vid.PageFlip();
    }
}

void MainLoop::UpdateFrustrumBB()
{
    viewFrustrumBB = P3D::AABB<P3D::fp>();

    P3D::M4<P3D::fp> camMatrix = renderDev.GetMatrix().Inverted();

    P3D::V4<P3D::fp> t1 = camMatrix * frustrumPoints[0];
    P3D::V4<P3D::fp> t2 = camMatrix * frustrumPoints[1];
    P3D::V4<P3D::fp> t3 = camMatrix * frustrumPoints[2];
    P3D::V4<P3D::fp> t4 = camMatrix * frustrumPoints[3];

    viewFrustrumBB.AddPoint(camera.GetEyePosition());

    viewFrustrumBB.AddPoint(P3D::V3<P3D::fp>(t1.x, t1.y, t1.z));
    viewFrustrumBB.AddPoint(P3D::V3<P3D::fp>(t2.x, t2.y, t2.z));
    viewFrustrumBB.AddPoint(P3D::V3<P3D::fp>(t3.x, t3.y, t3.z));
    viewFrustrumBB.AddPoint(P3D::V3<P3D::fp>(t4.x, t4.y, t4.z));
}

void MainLoop::RenderModel()
{
    static std::vector<const P3D::BspModelTriangle*> tris;

    model.GetModel()->SortBackToFront(camera.GetEyePosition(), viewFrustrumBB, tris, true);

    for(unsigned int i = 0; i < tris.size(); i++)
    {
        const P3D::BspModelTriangle* tri = tris[i];

        if(!FrustrumTestTriangle(tri))
            continue;

        const P3D::BspNodeTexture* ntex = model.GetModel()->GetTexture(tri->texture);

        P3D::V3<P3D::fp> verts[3] = {tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos};

        P3D::Material m;

        if(ntex)
        {
            m.type = P3D::Material::Texture;
            m.pixels = model.GetModel()->GetTexturePixels(ntex->texture_pixels_offset);

            const P3D::fp light_levels[3] = {1, 1, 1};

            P3D::V2<P3D::fp> uvs[3] = {tri->tri.verts[0].uv, tri->tri.verts[1].uv, tri->tri.verts[2].uv};

            renderDev.SetMaterial(m);

            renderDev.DrawTriangle(verts, uvs, light_levels);
        }
        else
        {
            m.color = tri->color;

            renderDev.SetMaterial(m);

            renderDev.DrawTriangle(verts);
        }
    }

    tris.clear();
}

bool MainLoop::FrustrumTestTriangle(const P3D::BspModelTriangle* tri) const
{
    if(!frustrumPlanes[P3D::Left].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
        return false;

    if(!frustrumPlanes[P3D::Right].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
        return false;

    if(!frustrumPlanes[P3D::Top].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
        return false;

    if(!frustrumPlanes[P3D::Bottom].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
        return false;

    if(!frustrumPlanes[P3D::Near].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
        return false;

    if(!frustrumPlanes[P3D::Far].TriangleIsFrontside(tri->tri.verts[0].pos, tri->tri.verts[1].pos, tri->tri.verts[2].pos))
        return false;

    return true;
}

void MainLoop::ResolveCollisions()
{
    const int bb_size = 100;
    P3D::AABB<P3D::fp> player_box(camera.GetPosition(), bb_size);

    std::vector<const P3D::BspModelTriangle*> tris;

    model.GetModel()->SortFrontToBack(camera.GetPosition(), player_box, tris, true);

    int collision_count = 0;

    for(unsigned int i = 0; i < tris.size(); i++)
    {
        P3D::V3<P3D::fp> resolutionVector;

        if(collision.CheckCollision(tris.at(i), camera.GetPosition(), 25, resolutionVector))
        {
            camera.MovePosition(resolutionVector);

            i = 0;
            collision_count++;

            if(collision_count > 5)
                return;
        }
    }
}

void MainLoop::RunTimeslots()
{
    static unsigned int gameTime = vid.GetTime();

    unsigned int realTime = vid.GetTime();

    if(realTime < gameTime) //Overflow
        gameTime = realTime;

    while(gameTime <= realTime)
    {
        vid.UpdateKeys();
        camera.HandleInput(keyState);

        ResolveCollisions();

        gameTime += frameTicks;
    }
}
