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

    triBuffer.reserve(8192);
}

void MainLoop::Run()
{
    vid.Setup();
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

    unsigned int f = 0;

    const P3D::AABB<P3D::fp>& model_aabb = model.GetModel()->GetModelAABB();

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

    unsigned int frames = ((unsigned int)((x_end - x_start) / step) * (unsigned int)((y_end - y_start) / step) * (unsigned int)((z_end - z_start) / step) * 4);

    unsigned int start_time = vid.GetTime();

    for(P3D::fp x = x_start; x < x_end; x += step)
    {
        for(P3D::fp y = y_start; y < y_end; y += step)
        {
            for(P3D::fp z = z_start; z < z_end; z += step)
            {
                camera.SetPosition(P3D::V3<P3D::fp>(x,y,z));

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

                    f++;

                    if((f % 16) == 0)
                    {
                        vid.PageFlip();
                    }



                    if( (f % 10000) == 0)
                    {
                        unsigned int now = vid.GetTime();

                        unsigned int elapsed = now - start_time;

                        unsigned int fps = (f * 1000) / elapsed;

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

                        qDebug() << "Frame" << f << "/" << frames << "FPS:" << fps << "Time left:" << time_left << suffix;
                    }

                    //_sleep(1000);

                }
            }
        }
    }

    StorePVS();
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
    const unsigned int current_node = model.GetModel()->GetLeafNodeId(camera.GetEyePosition());

    model.GetModel()->Sort(camera.GetEyePosition(), viewFrustrumBB, triBuffer, true, false);

    for(int i = triBuffer.size()-1; i >= 0; i--)
    {
        const unsigned int p1 = renderDev.GetRenderStats().pixels_drawn;

        const P3D::BspModelTriangle* tri = triBuffer[i];

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

        if(p2 > (p1+8))
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

void SaveBytesAsCFile(QByteArray bytes, QString file);

void MainLoop::StorePVS()
{
    const unsigned int node_count = model.GetModel()->header.node_count;
    const unsigned int leaf_count = (node_count * 2) + 1; //We can be front or back of each leaf.

    const unsigned int node_bitmap_len_bytes = (node_count + 7) / 8;

    unsigned char** vis_bitmaps = new unsigned char*[leaf_count];
    unsigned int* vis_offsets = new unsigned int[leaf_count];

    for(int i = 0; i < leaf_count; i++)
    {
        if(visData.contains(i))
        {
            vis_bitmaps[i] = new unsigned char[node_bitmap_len_bytes];
            std::memset(vis_bitmaps[i], 0, node_bitmap_len_bytes);

            std::unordered_set<unsigned int> visSet = visData[i];

            for(int j = 0; j < node_count; j++)
            {
                if(visSet.contains(j))
                {
                    vis_bitmaps[i][j / 8] |= (1 << (j % 8));
                }
            }
        }
        else
            vis_bitmaps[i] = nullptr;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    P3D::VisDataHeader vdh;

    vdh.leaf_count = leaf_count;
    vdh.leaf_index_offset = sizeof(P3D::VisDataHeader);

    buffer.write((const char*)&vdh, sizeof(vdh));

    unsigned int wb = 0;

    for(int i = 0; i < leaf_count; i++)
    {
        if(vis_bitmaps[i])
        {
            vis_offsets[i] = buffer.pos() + (sizeof(unsigned int) * leaf_count) + wb;
            wb+= node_bitmap_len_bytes;
        }
        else
            vis_offsets[i] = 0;
    }

    buffer.write((const char*)vis_offsets, sizeof(unsigned int) * leaf_count);

    for(int i = 0; i < leaf_count; i++)
    {
        if(vis_bitmaps[i])
        {
            buffer.write((const char*)vis_bitmaps[i], node_bitmap_len_bytes);
        }
    }

    QString objPath = "C:\\Users\\Zak\\Downloads\\Facility\\Facility.obj";

    QDir workDir = QDir(QFileInfo(objPath).absolutePath());
    QString baseName = QFileInfo(objPath).fileName().chopped(3);

    QFile bspFile(workDir.filePath(baseName + "pvs"));
    bspFile.open(QFile::Truncate | QFile::ReadWrite);
    bspFile.write(bytes);
    bspFile.close();

    SaveBytesAsCFile(bytes, workDir.filePath(baseName + "pvs.cpp"));
}

void SaveBytesAsCFile(QByteArray bytes, QString file)
{
    QFile f(file);

    if(!f.open(QIODevice::Truncate | QIODevice::ReadWrite))
        return;

    QString decl = QString("const extern unsigned char pvsdata[%1UL] = {\n").arg(bytes.size());

    f.write(decl.toLatin1());

    for(int i = 0; i < bytes.size(); i++)
    {
        QString element = QString("0x%1,").arg((quint8)bytes.at(i),2, 16, QChar('0'));

        if(( (i+1) % 40) == 0)
            element += "\n";

        f.write(element.toLatin1());
    }

    QString close = QString("\n};");
    f.write(close.toLatin1());

    f.close();
}
