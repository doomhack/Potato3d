#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
}

MainWindow::~MainWindow()
{

}

bool Object3d::LoadFromObjFile(QString objFile, QString mtlFile)
{
    this->pos.x = 1024;
    this->pos.z = 1024;

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

    QMap<QString, Texture*> textureMap;
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
                Texture* t = new Texture();

                t->texture = new QImage(":/models/VRML/" + lastBit);

                if(!t->texture->isNull())
                {
                    textureMap[currMtlName] = t;
                    t->width = t->texture->width();
                    t->height = t->texture->height();

                    t->pixels = (const QRgb*)t->texture->constBits();
                }


                currMtlName.clear();
            }
        }
    }






    QStringList lines = objFileText.split("\n");

    QList<P3D::V3<fp>> vertexes;
    QList<P3D::V2<fp>> uvs;


    Mesh3d* currentMesh = new Mesh3d();
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
            fp x = elements[1].toFloat();
            fp y = elements[2].toFloat();
            fp z = elements[3].toFloat();

            vertexes.append(P3D::V3<fp>(x, y, z));
        }

        if(elements[0] == "vt")
        {
            float u = elements[1].toFloat();
            float v = elements[2].toFloat();

            uvs.append(P3D::V2<fp>(u, v));
        }

        //Face
        if(elements[0] == "f")
        {
            Triangle3d t3d;

            for(int t = 0; t < 3; t++)
            {
                QString tri = elements[t+1];

                QStringList vtx_elelments = tri.split("/");

                t3d.verts[t].pos = vertexes.at(vtx_elelments[0].toInt() - 1);
                t3d.verts[t].uv = uvs.at(vtx_elelments[1].toInt() - 1);
            }

            currentMesh->tris.append(t3d);
        }

        if(elements[0] == "usemtl")
        {
            if(currentMesh->tris.length())
            {
                //Start new mesh when texture changes.
                this->mesh.append(currentMesh);
                currentMesh = new Mesh3d();
                currentMesh->color = Qt::lightGray;
            }

            currentMesh->texture = textureMap.value(elements[1]);
            currentMesh->color = textureColors.value(elements[1]);
        }
    }

    if(currentMesh->tris.length())
        this->mesh.append(currentMesh);

    return true;
}

