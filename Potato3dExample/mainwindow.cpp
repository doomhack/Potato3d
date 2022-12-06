#include "mainwindow.h"

#include <QtCore>
#include <QtGui>

#include "models/model.h"

#include "bsp3d.h"

#include "../rtypes.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    this->resize(960, 640);
    this->update();

    fpsTimer.start();

    frameBufferImage = QImage(screenWidth, screenHeight, QImage::Format::Format_Indexed8);

    object3d = new P3D::Object3d();

    //object3d->Setup(screenWidth, screenHeight, 54, 25, 1500, (P3D::pixel*)frameBufferImage.bits());
    object3d->Setup(screenWidth, screenHeight, 60, 25, 5000, (P3D::pixel*)frameBufferImage.bits());

    object3d->SetBackgroundColor(0);

    //P3D::Model3d* runway = LoadObjFile(":/models/temple.obj", ":/models/temple.mtl");
    //P3D::Model3d* runway = LoadObjFile(":/models/Mk64Beach/Mk64Kb.obj", ":/models/Mk64Beach/Mk64Kb.mtl");
    P3D::Model3d* runway = LoadObjFile(":/models/Streets/Streets.obj", ":/models/Streets/Streets.mtl");
    //P3D::Model3d* runway = LoadObjFile(":/models/gymclass/gymclass2.obj", ":/models/gymclass/gymclass2.mtl");

    P3D::Bsp3d* bsp = new P3D::Bsp3d;

    P3D::BspTree* bspTree = bsp->BuildBspTree(runway);

    QByteArray* ba = new QByteArray;
    bspTree->SaveBspTree(ba);

    SaveBytesAsCFile(ba, "C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\model.cpp");

    const P3D::BspModel* bspModel = (P3D::BspModel*)ba->constData();

    object3d->SetModel(bspModel);
}

MainWindow::~MainWindow()
{

}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

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

    P3D::RenderStats rs = object3d->GetRenderStats();

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
    p.drawText(32,112, QString("Scanlines drawn: %1").arg(rs.scanlines_drawn));
    p.drawText(32,128, QString("Spans checked: %1").arg(rs.span_checks));
    p.drawText(32,144, QString("Spans generated: %1").arg(rs.span_count));

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
    else if(event->key() == Qt::Key_Space)
    {
        object3d->update_frustrum_bb = !object3d->update_frustrum_bb;
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


    QStringList texParts = mtlFile.split("/");
    texParts.pop_back();
    QString texBase = texParts.join("/");


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

                QImage* image = new QImage(texBase + "/" + lastBit);

                *image = image->scaled(P3D::TEX_SIZE, P3D::TEX_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).mirrored();

                textureColors[currMtlName] = image->scaled(1,1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).pixel(0,0);

                *image = image->convertToFormat(QImage::Format_RGB32);

                if(!image->isNull())
                {
                    textureMap[currMtlName] = t;
                    t->width = image->width();
                    t->height = image->height();

                    t->u_mask = t->width-1;


                    t->v_shift = 0;

                    if(lastBit.left(2) == "A_")
                        t->alpha = 1;

                    while( (1 << t->v_shift) < t->width)
                        t->v_shift++;

                    t->v_mask = (t->height-1) << t->v_shift;

                    t->pixels = image->constScanLine(0);
                }

                currMtlName.clear();
            }
        }
    }

    //
    unsigned int numTextures = textureMap.keys().count();
    QImage allTexImage = QImage(P3D::TEX_SIZE, P3D::TEX_SIZE * numTextures, QImage::Format_RGB32);

    for(int i =0; i < numTextures; i++)
    {
        unsigned int* scanline = (unsigned int*)allTexImage.scanLine(i * P3D::TEX_SIZE);

        memcpy(scanline, textureMap.value(textureMap.keys().at(i))->pixels, P3D::TEX_SIZE * P3D::TEX_SIZE * 4);
    }



    //Quantise to 256 colors.
    allTexImage.save("C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\allTex.png");

    QDir d = QDir::current();
    d.cdUp();
    d.cd("Potato3dExample");
    d.cd("models");


    QProcess p;
    p.setWorkingDirectory(d.path());

    QStringList params;
    params.append("allTex.png");
    params.append("/m");
    params.append("256");

    p.start("nQuantCpp.exe", params);
    p.waitForFinished();

    qDebug() << p.readAll();

    QImage* allTex256 = new QImage("C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\allTex-WUquant256.png");
    //QImage* allTex256 = new QImage("C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\allTex-WUquant128.png");
    //QImage* allTex256 = new QImage("C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\allTex.png");


    for(int i =0; i < numTextures; i++)
    {
        unsigned char* scanline = (unsigned char*)allTex256->scanLine(i * P3D::TEX_SIZE);

        textureMap.value(textureMap.keys().at(i))->pixels = scanline;
    }

    for(int i = 0; i < allTex256->colorTable().length(); i++)
        model->colormap[i] = allTex256->colorTable().at(i);

    frameBufferImage.setColorTable(allTex256->colorTable());

    //

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
            float u = (elements[1].toFloat() * P3D::TEX_SIZE);
            float v = (elements[2].toFloat() * P3D::TEX_SIZE);

            if(u > (P3D::TEX_MAX_TILE * P3D::TEX_SIZE))
            {
                qDebug() << "Clamp u" << u << "to" << P3D::TEX_MAX_TILE * P3D::TEX_SIZE;
                u = P3D::TEX_MAX_TILE * P3D::TEX_SIZE;
            }

            if(v > (P3D::TEX_MAX_TILE * P3D::TEX_SIZE))
            {
                qDebug() << "Clamp v" << v << "to" << P3D::TEX_MAX_TILE * P3D::TEX_SIZE;
                v = P3D::TEX_MAX_TILE * P3D::TEX_SIZE;
            }

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
                t3d->verts[t].vertex_id = vtx_elelments[0].toInt() - 1;
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

            QRgb cx = textureColors.value(elements[1]);

            currentMesh->color = ((qRed(cx) >> 3) << 10) | ((qGreen(cx) >> 3) << 5) | ((qBlue(cx) >> 3));
        }
    }

    model->vertex_id_count = vertexes.length();

    if(currentMesh->tris.size())
        model->mesh.push_back(currentMesh);

    return model;
}

void MainWindow::SaveBytesAsCFile(QByteArray* bytes, QString file)
{
    QFile f(file);

    if(!f.open(QIODevice::Truncate | QIODevice::ReadWrite))
        return;

    QString decl = QString("const extern unsigned char modeldata[%1UL] = {\n").arg(bytes->size());

    f.write(decl.toLatin1());

    for(int i = 0; i < bytes->size(); i++)
    {
        QString element = QString("0x%1,").arg((quint8)bytes->at(i),2, 16, QChar('0'));

        if(( (i+1) % 40) == 0)
            element += "\n";

        f.write(element.toLatin1());
    }

    QString close = QString("\n};");
    f.write(close.toLatin1());

    f.close();
}
