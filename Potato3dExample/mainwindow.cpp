#include "mainwindow.h"

#include <QtCore>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(1360, 720);
    this->update();

    fpsTimer.start();

    frameBufferImage = QImage(screenWidth, screenHeight, QImage::Format::Format_RGB32);

    object3d = new P3D::Object3d();

    object3d->Setup(screenWidth, screenHeight, 54, 5, 1024, (P3D::pixel*)frameBufferImage.bits());

    object3d->SetBackgroundColor(qRgb(0,255,255));

    P3D::Model3d* runway = LoadObjFile("://models/runway.obj", "://models/runway.mtl");
    object3d->AddModel(runway);
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

        float angleYRad = qDegreesToRadians(camAngle.y);

        P3D::V3<P3D::fp> d((float)-(qSin(angleYRad) *10), 0, (float)-(qCos(angleYRad) *10));

        object3d->CameraPos() += d;
    }
    else if(event->key() == Qt::Key_Down)
    {
        P3D::V3<P3D::fp> camAngle = object3d->CameraAngle();

        float angleYRad = qDegreesToRadians(camAngle.y);

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

                if(!image->isNull())
                {
                    textureMap[currMtlName] = t;
                    t->width = image->width();
                    t->height = image->height();

                    t->pixels = (const QRgb*)image->constBits();
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

