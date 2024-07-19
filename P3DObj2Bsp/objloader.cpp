#include "objloader.h"

namespace Obj2Bsp
{
    ObjLoader::ObjLoader()
    {

    }

    bool ObjLoader::LoadObjFile(QString filePath)
    {
        QFile objFile(filePath);

        QString mtlPath = filePath.chopped(3).append("mtl");

        QFile mtlFile(mtlPath);

        if(!objFile.open(QFile::ReadOnly))
        {
            qDebug() << "Failed to open obj file:" << filePath;
            return false;
        }

        if(!mtlFile.open(QFile::ReadOnly))
        {
            qDebug() << "Failed to open mtl file:" << filePath;
            return false;
        }

        QString objFileText = objFile.readAll();
        QString mtlFileText = mtlFile.readAll();

        objFile.close();
        mtlFile.close();

        QMap<QString, Texture*> textureMap;
        QMap<QString, QRgb> textureColors;

        ParseMaterials(mtlFileText, QDir(QFileInfo(filePath).absolutePath()), textureMap, textureColors);

        QImage megaTexture = GetMegaTexture(textureMap);

        QByteArray fogLightMap;

        model = new Model3d();

        if(textureFormat == QImage::Format_Indexed8)
        {
            //8 bit palette format.
            //Need to convert to 8 bit and generate fog and lightmaps.

            qDebug() << "Quantizing textures...";

            megaTexture = QuantizeMegaTexture(megaTexture, fogLightMap);

            for(int i = 0; i < megaTexture.colorTable().length(); i++)
                model->colormap[i] = megaTexture.colorTable().at(i);

            memcpy(model->foglightmap, fogLightMap.constData(), FOG_LEVELS * LIGHT_LEVELS * 256);
        }
        else
        {
            megaTexture = megaTexture.convertToFormat(textureFormat);
        }

        CopyTextureDataToTextures(megaTexture, textureMap);

        ParseGeometry(objFileText, textureMap, textureColors);

        return true;
    }


    Model3d* ObjLoader::GetModel()
    {
        return model;
    }

    void ObjLoader::ParseGeometry(QString objText, QMap<QString, Texture *> &textures, QMap<QString, QRgb> &textureColors)
    {
        qDebug() << "Parsing geometry...";

        QStringList lines = objText.split("\n");

        QList<QVector3D> vertexes;
        QList<QVector2D> uvs;

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
                float x = elements[1].toFloat();
                float y = elements[2].toFloat();
                float z = elements[3].toFloat();

                vertexes.append(QVector3D(x, y, z));
            }

            if(elements[0] == "vt")
            {
                float u = (elements[1].toFloat() * TEX_SIZE);
                float v = (elements[2].toFloat() * TEX_SIZE);

                if(u > (TEX_MAX_TILE * TEX_SIZE))
                {
                    qDebug() << "Clamp u" << u << "to" << TEX_MAX_TILE * TEX_SIZE;
                    u = TEX_MAX_TILE * TEX_SIZE;
                }

                if(v > (TEX_MAX_TILE * TEX_SIZE))
                {
                    qDebug() << "Clamp v" << v << "to" << TEX_MAX_TILE * TEX_SIZE;
                    v = TEX_MAX_TILE * TEX_SIZE;
                }

                uvs.append(QVector2D(u, v));
            }

            //Face
            if(elements[0] == "f")
            {
                Triangle3d* t3d = new Triangle3d();

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
                    currentMesh = new Mesh3d();
                    currentMesh->color = Qt::lightGray;
                }

                currentMesh->texture = textures.value(elements[1]);

                currentMesh->color = textureColors.value(elements[1]);
            }
        }

        model->vertex_id_count = vertexes.length();

        if(currentMesh->tris.size())
            model->mesh.push_back(currentMesh);
    }

    void ObjLoader::ParseMaterials(QString mtlText, QDir texturePath, QMap<QString, Texture*>& textures, QMap<QString, QRgb>& textureColors)
    {
        qDebug() << "Parsing materials...";

        QStringList mtlLines = mtlText.split("\n");

        QString currMtlName;

        for(int i = 0; i < mtlLines.length(); i++)
        {
            QString line = mtlLines.at(i);

            QStringList elements = line.split(" ");

            if(elements[0] == "#")
                continue; //Comment

            if(elements[0] == "newmtl")
            {
                currMtlName = elements[1];
                qDebug() << "Material:" << currMtlName;
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

                    QImage image = QImage(texturePath.filePath(lastBit));

                    image = image.scaled(TEX_SIZE, TEX_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).mirrored();

                    textureColors[currMtlName] = image.scaled(1,1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).pixel(0,0);

                    image = image.convertToFormat(QImage::Format_RGB32);

                    if(!image.isNull())
                    {
                        textures[currMtlName] = t;
                        t->width = image.width();
                        t->height = image.height();

                        t->u_mask = t->width-1;


                        t->v_shift = 0;

                        if(lastBit.left(2) == "A_")
                            t->alpha = 1;

                        while( (1 << t->v_shift) < t->width)
                            t->v_shift++;

                        t->v_mask = (t->height-1) << t->v_shift;

                        t->pixels = QByteArray((const char*)image.constScanLine(0), TEX_SIZE * TEX_SIZE * 4);
                    }

                    currMtlName.clear();
                }
            }
        }
    }

    QImage ObjLoader::GetMegaTexture(QMap<QString, Texture*>& textures)
    {
        qDebug() << "Bulding mega texture with" << textures.keys().count() << "images...";

        int numTextures = textures.keys().count();
        QImage allTexImage = QImage(TEX_SIZE, TEX_SIZE * numTextures, QImage::Format_RGB32);

        for(int i =0; i < numTextures; i++)
        {
            unsigned int* scanline = (unsigned int*)allTexImage.scanLine(i * TEX_SIZE);

            memcpy(scanline, textures.value(textures.keys().at(i))->pixels.constData(), TEX_SIZE * TEX_SIZE * 4);
        }

        return allTexImage;
    }

    QImage ObjLoader::QuantizeMegaTexture(QImage megaTexture, QByteArray &fogLightMap)
    {
        if(FOG_LEVELS > 0 || LIGHT_LEVELS > 0)
        {
            QImage flTexture = GetImageWithFogAndLightmap(megaTexture);

            fogLightMap = GenerateFogAndLightTables(flTexture);

            return flTexture.copy(0, 0, megaTexture.width(), megaTexture.height());
        }

        return nQuantCppImage(megaTexture);
    }

    QByteArray ObjLoader::GenerateFogAndLightTables(QImage imageIn)
    {
        QList<QRgb> colorTable = imageIn.colorTable();

        quint8 fogLightMap[LIGHT_LEVELS][FOG_LEVELS][256];

        for(int i = 0; i < 256; i++)
        {
            QColor c = colorTable.at(i);

            for(int l = 0; l < LIGHT_LEVELS; l++)
            {
                double lFrac = double(l) / double(LIGHT_LEVELS-1);

                //Blend C with black.
                QColor cl = blendColor(c, QColor(Qt::black), lFrac);

                for(int f = 0; f < FOG_LEVELS; f++)
                {
                    double fFrac = double(f) / double(FOG_LEVELS-1);
                    //Blend C with fog.
                    QColor clf = blendColor(cl, QColor::fromRgb(0x799ED7), fFrac);

                    int bestColor = 0;
                    double bestMatch = std::numeric_limits<double>::max();
                    //Find nearest color in map.
                    for(int x = 0; x < 256; x++)
                    {
                        double diff = colorDiff(colorTable.at(x), clf);

                        if(diff < bestMatch)
                        {
                            bestMatch = diff;
                            bestColor = x;
                        }
                    }

                    fogLightMap[l][f][i] = bestColor;
                }
            }
        }

        return QByteArray((const char*)fogLightMap, 256 * LIGHT_LEVELS * FOG_LEVELS);
    }

    QImage ObjLoader::GetImageWithFogAndLightmap(QImage imageIn)
    {
        qDebug() << "Building fog and lightmap...";

        const int w = imageIn.width();
        const int h = imageIn.height();

        QImage flTex = QImage(w * LIGHT_LEVELS, h * FOG_LEVELS, QImage::Format_RGB32);
        QPainter painter(&flTex);
        painter.drawImage(0,0,imageIn);

        for(int i = 0; i < LIGHT_LEVELS; i++)
        {
            int ll = i;
            double lFrac = double(i) / double(LIGHT_LEVELS-1);

            painter.setOpacity(1);


            painter.drawImage(w*ll,0,imageIn);

            painter.setOpacity(lFrac);
            painter.fillRect(w*ll,0,w,h,QBrush(Qt::black));

            for(int f = 0; f < FOG_LEVELS; f++)
            {
                double fFrac = double(f) / double(FOG_LEVELS-1);

                painter.setOpacity(1);

                painter.drawImage(w*ll,h*f,flTex,w*ll,0,w,h);

                painter.setOpacity(fFrac);
                painter.fillRect(w*ll,h*f,w,h,QBrush(QColor::fromRgb(FOG_COLOR)));
            }
        }

        QImage quantizedImage = nQuantCppImage(flTex);

        return quantizedImage;
    }

    void ObjLoader::CopyTextureDataToTextures(QImage megaTexture, QMap<QString, Texture*>& textures)
    {
        //Copy the quantized texture into the texture struct.
        int numTextures = textures.keys().count();

        for(int i =0; i < numTextures; i++)
        {
            P3D::pixel* scanline = (P3D::pixel*)megaTexture.scanLine(i * TEX_SIZE);

            textures.value(textures.keys().at(i))->pixels = QByteArray((const char*)scanline, TEX_SIZE * TEX_SIZE * sizeof(P3D::pixel));
        }
    }

    QImage ObjLoader::nQuantCppImage(QImage imageIn)
    {
        qDebug() << "Running nQuantCpp...";

        QDir temp = QDir::temp();

        QFile nQuantResource(":/exe/nQuantCpp.exe");
        nQuantResource.open(QFile::ReadOnly);

        QFile nQuantTemp(temp.filePath("nQuantCpp.exe"));
        nQuantTemp.open(QFile::ReadWrite | QFile::Truncate);
        nQuantTemp.write(nQuantResource.readAll());

        nQuantTemp.close();
        nQuantResource.close();

        imageIn.save(temp.filePath("megaTex.png"));

        QProcess p;
        p.setProgram(temp.filePath("nQuantCpp.exe"));
        p.setWorkingDirectory(temp.path());

        p.setArguments(QStringList() << "megaTex.png" << "/a" << "wu" << "/m" << "256");

        p.start();
        p.waitForFinished(10 * 60 * 1000); //Wait for up to 10 mins.

        QImage quantizedImage = QImage(temp.filePath("megaTex-WUquant256.png")).convertToFormat(QImage::Format_Indexed8);

        return quantizedImage;
    }

    QColor ObjLoader::blendColor(QColor color1, QColor color2, double frac)
    {
        double r1 = color1.redF();
        double g1 = color1.greenF();
        double b1 = color1.blueF();

        double r2 = color2.redF();
        double g2 = color2.greenF();
        double b2 = color2.blueF();

        double r3 = std::lerp(r1,r2,frac);
        double g3 = std::lerp(g1,g2,frac);
        double b3 = std::lerp(b1,b2,frac);

        return QColor::fromRgbF(r3, g3, b3);
    }

    double ObjLoader::colorDiff(QColor e1, QColor e2)
    {
        long rmean = ( (long)e1.red() + (long)e2.red() ) / 2;
        long r = (long)e1.red() - (long)e2.red();
        long g = (long)e1.green() - (long)e2.green();
        long b = (long)e1.blue() - (long)e2.blue();
        return sqrt((((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8));
    }

}
