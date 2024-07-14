#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <QtCore>
#include <QVector3D>

#include "config.h"

namespace Obj2Bsp
{

    class Vertex3d
    {
    public:
        QVector3D pos;
        unsigned int vertex_id = no_vx_id;
        QVector2D uv;

        static const unsigned int no_vx_id = -1;
    };

    class Triangle3d
    {
    public:
        Vertex3d verts[3];
    };

    class Texture
    {
    public:
        QByteArray pixels;
        unsigned short width;
        unsigned short height;
        unsigned short u_mask;
        unsigned short v_mask;
        unsigned short v_shift;
        unsigned short alpha;
    };

    class Mesh3d
    {
    public:
        QRgb color = 0;
        Texture* texture = nullptr;

        QList<Triangle3d*> tris;
    };

    class Model3d
    {
    public:
        QVector3D pos = QVector3D(0,0,0);
        QList<Mesh3d*> mesh;
        unsigned int vertex_id_count = 0;
        unsigned int colormap[256];
        unsigned char foglightmap[256][16][16];
    };


    class ObjLoader
    {
    public:
        ObjLoader();
        bool LoadObjFile(QString filePath);

        Model3d* GetModel();

    private:

        void ParseGeometry(QString objText, QMap<QString, Texture*>& textures, QMap<QString, QRgb>& textureColors);

        void ParseMaterials(QString mtlText, QDir texturePath, QMap<QString, Texture*>& textures, QMap<QString, QRgb>& textureColors);
        QImage GetMegaTexture(QMap<QString, Texture*>& textures);

        QImage QuantizeMegaTexture(QImage megaTexture, QByteArray& fogLightMap);

        QImage nQuantCppImage(QImage imageIn);

        QImage GetImageWithFogAndLightmap(QImage imageIn);

        QColor blendColor(QColor color1, QColor color2, double frac);
        double colorDiff(QColor e1, QColor e2);

        QByteArray GenerateFogAndLightTables(QImage imageIn);

        void CopyTextureDataToTextures(QImage megaTexture, QMap<QString, Texture*>& textures);

        Model3d* model = nullptr;

    };
}

#endif // OBJLOADER_H
