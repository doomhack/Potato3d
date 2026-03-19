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

    triBuffer.SetSize(32768);
}

void MainLoop::Run(bool withGui, unsigned int slice, unsigned int totalSlices)
{
    vid.Setup(withGui);
    vid.SetPalette(model.GetModel()->GetColorMap());

    constexpr unsigned int flags = P3D::ZWrite | P3D::ZTest;
    //constexpr unsigned int flags = P3D::SubdividePerspectiveMapping;
    //constexpr unsigned int flags = P3D::Fog;
    //constexpr unsigned int flags = P3D::SubdividePerspectiveMapping | P3D::Fog;
    //constexpr unsigned int flags = P3D::SubdividePerspectiveMapping | P3D::VertexLight | P3D::Fog;

    renderDev.SetRenderFlags<flags, P3D::PixelShaderGBA8<flags>>();

    renderDev.SetPerspective(vFov, 1.0, zNear, zFar);

    const P3D::pixel background = model.GetModel()->GetFogLightMap()[(P3D::FOG_LEVELS-1)*256];

    constexpr P3D::fp step = 50;


    const P3D::AABB<short>& model_aabb = model.GetModel()->GetModelAABB();

    P3D::fp x_start = model_aabb.GetX1();
    P3D::fp x_end = model_aabb.GetX2();

    P3D::fp y_start = model_aabb.GetY1();
    P3D::fp y_end = model_aabb.GetY2();

    P3D::fp z_start = model_aabb.GetZ1();
    P3D::fp z_end = model_aabb.GetZ2();

    x_start = int(x_start / step) * int(step);
    y_start = int(y_start / step) * int(step);
    z_start = int(z_start / step) * int(step);

    x_start += step / 2;
    y_start += step / 2;
    z_start += step / 2;

    x_end = int(((x_end + (step-1)) / step)) * int(step);
    y_end = int(((y_end + (step-1)) / step)) * int(step);
    z_end = int(((z_end + (step-1)) / step)) * int(step);

    P3D::fp sx = (x_end - x_start) / P3D::fp(totalSlices);
    x_start += sx * P3D::fp(slice);
    x_end = std::min(x_end, x_start + sx);

    unsigned int frames = ((unsigned int)((x_end - x_start) / step) * (unsigned int)((y_end - y_start) / step) * (unsigned int)((z_end - z_start) / step));

    unsigned int start_time = vid.GetTime();

    unsigned long long f = 0;
    unsigned long long c = 0;


    for(P3D::fp x = x_start; x < x_end; x += step)
    {
        for(P3D::fp y = y_start; y < y_end; y += step)
        {
            for(P3D::fp z = z_start; z < z_end; z += step)
            {
                position = P3D::V3<P3D::fp>(x,y,z);

                f++;

                if(CheckCollisions(position))
                {
                    c++;
                    continue;
                }

                if((f % 16) == 0)
                {
                    vid.PageFlip();
                }

                for(int i = 0; i < 4; i++)
                {
                    renderDev.SetRenderTarget(vid.GetBackBuffer());

                    renderDev.ClearColor(background);
                    renderDev.ClearDepth(std::numeric_limits<P3D::fp>::max());

                    renderDev.PushMatrix();

                    renderDev.RotateY(-i * 90);

                    renderDev.Translate(P3D::V3<P3D::fp>(-x, -y, -z));

                    UpdateFrustrumBB();

                    renderDev.BeginFrame();

                    renderDev.BeginDraw(frustrumPlanes);

                    RenderModel();

                    renderDev.EndDraw();

                    renderDev.EndFrame();

                    renderDev.PopMatrix();

                    if( ((f % 1000) == 0) && i == 0)
                    {
                        unsigned int now = vid.GetTime();

                        unsigned int fps = (f * 1000) / (now - start_time);

                        unsigned int time_left = (frames - f) / fps;

                        const char* suffix = nullptr;

                        if(time_left > 2*60*60*24)
                        {
                            time_left /= (2*60*60*24);
                            suffix = "days";
                        }
                        else if(time_left > 2*60*60)
                        {
                            time_left /= 2*60*60;
                            suffix = "hours";
                        }
                        else if(time_left > 2*60)
                        {
                            time_left /= 2*60;
                            suffix = "minutes";
                        }
                        else
                        {
                            suffix = "seconds";
                        }

                        qDebug() << "Frame" << f << "/" << frames << "Skipped:" << c << "FPS:" << fps << "Time left:" << time_left << suffix;
                    }

                    //_sleep(1000);

                }
            }
        }
    }
}

bool MainLoop::CheckCollisions(P3D::V3<P3D::fp> point)
{
    const int bb_size = 100;

    P3D::AABB<short> player_box(P3D::V3<short>((int)point.x, (int)point.y, (int)point.z), bb_size);

    model.GetModel()->Sort(point, player_box, triBuffer, true, false);

    for(int i = triBuffer.Size() - 1; i >= 0; i--)
    {
        P3D::V3<P3D::fp> resolutionVector;

        if(collision.CheckCollision(triBuffer.At(i), point, 50, resolutionVector))
            return true;
    }

    return false;
}

void MainLoop::UpdateFrustrumBB()
{
    viewFrustrumBB = P3D::AABB<short>();

    P3D::M4<P3D::fp> camMatrix = renderDev.GetMatrix().Inverted();

    P3D::V4<P3D::fp> t1 = camMatrix * frustrumPoints[0];
    P3D::V4<P3D::fp> t2 = camMatrix * frustrumPoints[1];
    P3D::V4<P3D::fp> t3 = camMatrix * frustrumPoints[2];
    P3D::V4<P3D::fp> t4 = camMatrix * frustrumPoints[3];

    P3D::V3<short> c = P3D::V3<short>((int)position.x, (int)position.y, (int)position.z);

    viewFrustrumBB.AddPoint(c);

    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t1.x, (int)t1.y, (int)t1.z));
    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t2.x, (int)t2.y, (int)t2.z));
    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t3.x, (int)t3.y, (int)t3.z));
    viewFrustrumBB.AddPoint(P3D::V3<short>((int)t4.x, (int)t4.y, (int)t4.z));
}

void MainLoop::RenderModel()
{
    const unsigned int current_node = model.GetModel()->GetLeafNodeId(position);

    model.GetModel()->Sort(position, viewFrustrumBB, triBuffer, true, false);

    for(int i = triBuffer.Size()-1; i >= 0; i--)
    {
        const unsigned int p1 = renderDev.GetRenderStats().pixels_drawn;

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

            renderDev.DrawTriangle(verts, uvs);
        }
        else
        {
            m.color = tri->color;

            renderDev.SetMaterial(m);

            renderDev.DrawTriangle(verts);
        }

        const unsigned int p2 = renderDev.GetRenderStats().pixels_drawn;

        if(p2 >= 65536)
            return;

        if((p2 - p1) > 3)
        {
            unsigned int node = *((unsigned int*)&tri->color);

            visData[current_node].insert(node);
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

const std::map<unsigned int, std::unordered_set<unsigned int>>& MainLoop::GetPVSData() const
{
    return visData;
}