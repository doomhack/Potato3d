#include "mainwindow.h"

#include <QtCore>
#include <QtGui>

#include "models/model.h"

#include "../bsp3d.h"

#include "../rtypes.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(1360, 720);
    this->update();

    fpsTimer.start();

#ifdef FB_32BIT
    frameBufferImage = QImage(screenWidth, screenHeight, QImage::Format::Format_RGB32);
#else
    frameBufferImage = QImage(screenWidth, screenHeight, QImage::Format::Format_RGB555);
#endif

    object3d = new P3D::Object3d();

    object3d->Setup(screenWidth, screenHeight, 54, 25, 2048, (P3D::pixel*)frameBufferImage.bits());

    object3d->SetBackgroundColor(qRgb(0,255,255));

    P3D::Model3d* runway = LoadObjFile("://models/temple.obj", "://models/temple.mtl");
    object3d->SetModel(runway);

    SaveModel(runway);

    //P3D::Model3d* runway = LoadM3dData(modeldata);
    //object3d->AddModel(runway);

}

MainWindow::~MainWindow()
{

}

void MainWindow::paintEvent(QPaintEvent *event)
{
    static unsigned int frameCount = 0;
    static unsigned int currentFps = 0;

    static double aveRtime = 0;
    static unsigned int rTime = 0;

    renderTimer.restart();

    object3d->RenderScene();

    rTime += renderTimer.elapsed();

    QPainter p(this);

    p.drawImage(this->rect(), frameBufferImage);

    frameCount++;

    unsigned int elapsed = fpsTimer.elapsed();

    P3D::RenderStats rs = object3d->GetRender()->GetRenderStats();

    if(elapsed > 1000)
    {
        currentFps = qRound((double)frameCount / ((double)elapsed / 1000.0));

        aveRtime = (double)rTime / (double)frameCount;

        rTime = 0;
        frameCount = 0;
        fpsTimer.restart();
    }

    p.setPen(Qt::yellow);

    p.drawText(32,32, QString("FPS: %1").arg(currentFps));
    p.drawText(32,48, QString("Ave Render Time: %1ms").arg(aveRtime));

    p.drawText(32,64, QString("Triangles submitted: %1").arg(rs.triangles_submitted));
    p.drawText(32,80, QString("Triangles drawn: %1").arg(rs.triangles_drawn));
    p.drawText(32,96, QString("Vertexes transformed: %1").arg(rs.vertex_transformed));



    this->update();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Left)
    {
        object3d->CameraAngle().y += 2;
    }
    else if(event->key() == Qt::Key_Right)
    {
        object3d->CameraAngle().y -= 2;
    }
    else if(event->key() == Qt::Key_Up)
    {
        P3D::V3<P3D::fp> camAngle = object3d->CameraAngle();

        float angleYRad = qDegreesToRadians((float)camAngle.y);

        P3D::V3<P3D::fp> d((float)-(qSin(angleYRad) *10), 0, (float)-(qCos(angleYRad) *10));

        object3d->CameraPos() += d;
    }
    else if(event->key() == Qt::Key_Down)
    {
        P3D::V3<P3D::fp> camAngle = object3d->CameraAngle();

        float angleYRad = qDegreesToRadians((float)camAngle.y);

        P3D::V3<P3D::fp> d((float)-(qSin(angleYRad) *10), 0, (float)-(qCos(angleYRad) *10));

        object3d->CameraPos() -= d;
    }
    else if(event->key() == Qt::Key_Z)
    {
        object3d->CameraAngle().x += 1;
    }
    else if(event->key() == Qt::Key_X)
    {
        object3d->CameraAngle().x -= 1;
    }
    else if(event->key() == Qt::Key_Q)
    {
        object3d->CameraPos().y += 1;
    }
    else if(event->key() == Qt::Key_W)
    {
        object3d->CameraPos().y -= 1;
    }

}

P3D::Model3d* MainWindow::LoadObjFile(QString objFile, QString mtlFile)
{
    P3D::Model3d* model = new P3D::Model3d();

    model->pos.x = 0;
    model->pos.z = 0;

    QFile f(objFile);

    f.open(QFile::ReadOnly);

    QString objFileText = f.readAll();

    f.close();

    QFile f2(mtlFile);

    f2.open(QFile::ReadOnly);

    QString mtlFileText = f2.readAll();

    f2.close();

    QStringList mtlLines = mtlFileText.split("\n");

    QString currMtlName;

    QMap<QString, P3D::Texture*> textureMap;
    QMap<QString, QRgb> textureColors;


    for(int i = 0; i < mtlLines.length(); i++)
    {
        QString line = mtlLines.at(i);

        QStringList elements = line.split(" ");

        if(elements[0] == "#")
            continue; //Comment

        if(elements[0] == "newmtl")
        {
            currMtlName = elements[1];
        }

        if(elements[0] == "Kd")
        {
            float r = qRound(elements[1].toFloat() * 255);
            float g = qRound(elements[2].toFloat() * 255);
            float b = qRound(elements[3].toFloat() * 255);

            if(currMtlName.length())
            {
                textureColors[currMtlName] = qRgb(r,g,b);
            }
        }

        if(elements[0] == "map_Kd")
        {
            QString fileName = elements[1];

            QStringList parts = fileName.split("\\");
            QString lastBit = parts.at(parts.size()-1);

            if(currMtlName.length())
            {
                P3D::Texture* t = new P3D::Texture();

                QImage* image = new QImage("://models/" + lastBit);

#ifndef FB_32BIT
                *image = image->convertToFormat(QImage::Format_RGB555);
#endif

                if(!image->isNull())
                {
                    textureMap[currMtlName] = t;
                    t->width = image->width();
                    t->height = image->height();

                    t->u_mask = t->width-1;
                    t->v_mask = t->height-1;

                    t->v_shift = 0;

                    while( (1 << t->v_shift) < t->width)
                        t->v_shift++;

                    t->pixels = (const P3D::pixel*)image->constBits();
                }


                currMtlName.clear();
            }
        }
    }

    QStringList lines = objFileText.split("\n");

    QList<P3D::V3<P3D::fp>> vertexes;
    QList<P3D::V2<P3D::fp>> uvs;


    P3D::Mesh3d* currentMesh = new P3D::Mesh3d();
    currentMesh->color = Qt::lightGray;



    for(int i = 0; i < lines.length(); i++)
    {
        QString line = lines.at(i);

        QStringList elements = line.split(" ");

        if(elements[0] == "#")
            continue; //Comment

        //Vertex
        if(elements[0] == "v")
        {
            P3D::fp x = elements[1].toFloat();
            P3D::fp y = elements[2].toFloat();
            P3D::fp z = elements[3].toFloat();

            vertexes.append(P3D::V3<P3D::fp>(x, y, z));
        }

        if(elements[0] == "vt")
        {
            float u = elements[1].toFloat();
            float v = elements[2].toFloat();

            uvs.append(P3D::V2<P3D::fp>(u, v));
        }

        //Face
        if(elements[0] == "f")
        {
            P3D::Triangle3d* t3d = new P3D::Triangle3d();

            for(int t = 0; t < 3; t++)
            {
                QString tri = elements[t+1];

                QStringList vtx_elelments = tri.split("/");

                t3d->verts[t].pos = vertexes.at(vtx_elelments[0].toInt() - 1);
                t3d->verts[t].uv = uvs.at(vtx_elelments[1].toInt() - 1);
            }

            currentMesh->tris.push_back(t3d);
        }

        if(elements[0] == "usemtl")
        {
            if(currentMesh->tris.size())
            {
                //Start new mesh when texture changes.
                model->mesh.push_back(currentMesh);
                currentMesh = new P3D::Mesh3d();
                currentMesh->color = Qt::lightGray;
            }

            currentMesh->texture = textureMap.value(elements[1]);
            currentMesh->color = textureColors.value(elements[1]);
        }
    }

    if(currentMesh->tris.size())
        model->mesh.push_back(currentMesh);

    return model;
}

#pragma pack(push,1)


typedef struct FileModel
{
    unsigned int mesh_count;
    P3D::V3<P3D::fp> pos;
} FileModel;

typedef struct FileTexture
{
    unsigned int width;
    unsigned int height;
    unsigned int u_mask;
    unsigned int v_mask;
    unsigned int v_shift;
    //unsigned short pixels[width * height];
} FileTexture;

typedef struct FileMesh
{
    unsigned int color;
    unsigned int has_texture;
    unsigned int triangle_count;
    //P3D::Triangle3d triangles[triangle_count];
} FileMesh;

#pragma pack(pop)

#define RGB8(r,g,b)	( (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3) )

void MainWindow::SaveModel(P3D::Model3d* model)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    FileModel fm;
    fm.pos = model->pos;
    fm.mesh_count = model->mesh.size();

    buffer.write((const char*)&fm, sizeof(fm));

    for(unsigned int i = 0; i < model->mesh.size(); i++)
    {
        P3D::Mesh3d* mesh = model->mesh[i];

        FileMesh fms;
        fms.color = mesh->color;
        fms.has_texture = mesh->texture != nullptr;
        fms.triangle_count = mesh->tris.size();

        buffer.write((const char*)&fms, sizeof(fms));

        for(unsigned int j = 0; j < mesh->tris.size(); j++)
        {
            buffer.write((const char*)mesh->tris[j], sizeof(P3D::Triangle3d));
        }

        if(fms.has_texture)
        {
            FileTexture ft;

            ft.width = mesh->texture->width;
            ft.height = mesh->texture->height;
            ft.u_mask = mesh->texture->u_mask;
            ft.v_mask = mesh->texture->v_mask;
            ft.v_shift = mesh->texture->v_shift;

            buffer.write((const char*)&ft, sizeof(ft));

            for(unsigned int k = 0; k < ft.width * ft.height; k++)
            {
                QRgb p8 = mesh->texture->pixels[k];

                unsigned int r = qRed(p8);
                unsigned int g = qGreen(p8);
                unsigned int b = qBlue(p8);

                P3D::pixel p5 = RGB8(r,g,b);

                buffer.write((const char*)&p5, sizeof(unsigned short));
            }
        }
    }

    buffer.close();

    SaveBytesAsCFile(byteArray, "C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\model.cpp");
}

P3D::Model3d* MainWindow::LoadM3dData(const unsigned char* data)
{
    P3D::Model3d* model = new P3D::Model3d();


    FileModel* fm = (FileModel*)data;

    model->pos = fm->pos;

    FileMesh* fms = (FileMesh*)&fm[1];

    for(unsigned int i = 0; i < fm->mesh_count; i++)
    {
        P3D::Mesh3d* mesh = new P3D::Mesh3d();
        mesh->color = fms->color;

        P3D::Triangle3d* t = (P3D::Triangle3d*)&fms[1];

        for(unsigned int j = 0; j < fms->triangle_count; j++)
        {
            mesh->tris.push_back(&t[j]);
        }

        if(fms->has_texture)
        {
            FileTexture* ft = (FileTexture*)&t[fms->triangle_count];

            mesh->texture = new P3D::Texture;
            mesh->texture->width = ft->width;
            mesh->texture->height = ft->height;

            mesh->texture->u_mask = ft->u_mask;
            mesh->texture->v_mask = ft->v_mask;
            mesh->texture->v_shift = ft->v_shift;

            mesh->texture->pixels = (P3D::pixel*)&ft[1];

            fms = (FileMesh*)&mesh->texture->pixels[ft->width * ft->height];
        }
        else
        {
            fms = (FileMesh*)&t[fms->triangle_count];
        }

        model->mesh.push_back(mesh);
    }

    return model;
}

void MainWindow::SaveBytesAsCFile(QByteArray& bytes, QString file)
{
    QFile f(file);

    if(!f.open(QIODevice::Truncate | QIODevice::ReadWrite))
        return;

    QString decl = QString("const extern unsigned char modeldata[%1UL] = {\n").arg(bytes.size());

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
