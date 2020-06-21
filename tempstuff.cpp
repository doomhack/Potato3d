#include <limits>

#include <QtMath>
#include "voxelterrain.h"
#include "object3d.h"

#include "3dmaths/f3dmath.h"



VoxelTerrain::VoxelTerrain(QObject *parent) : QObject(parent)
{
    cameraHeight = 50;

    cameraPos = P3D::V3<fp>(mapSize/2, cameraHeight, mapSize/2);
    cameraAngle = 0;

    frameBufferImage = QImage(screenWidth, screenHeight, QImage::Format::Format_RGB32);
    frameBuffer = (QRgb*)frameBufferImage.bits();

    heightMapImage.load(":/images/D1.png");
    heightMapImage = heightMapImage.convertToFormat(QImage::Format::Format_Grayscale8);

    heightMap = heightMapImage.bits();

    colorMapImage.load(":/images/C1W.png");
    colorMapImage = colorMapImage.convertToFormat(QImage::Format::Format_ARGB32);

    colorMap = (const QRgb*)colorMapImage.bits();

    waterImage.load(":/images/water.bmp");
    waterImage = waterImage.convertToFormat(QImage::Format::Format_RGB32);
    water = (const QRgb*)waterImage.bits();


    zBuffer = new fp[screenWidth * screenHeight];

    //55.7954
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(54, 1.888888888f, zNear, zFar);

/*
    Object3d* airport = new Object3d();
    airport->LoadFromFile(":/models/VRML/airport.obj", ":/models/VRML/airport.mtl");
    objects.append(airport);

    Object3d* church = new Object3d();
    church->LoadFromFile(":/models/VRML/church.obj", ":/models/VRML/church.mtl");
    objects.append(church);

    Object3d* hotel = new Object3d();
    hotel->LoadFromFile(":/models/VRML/hotel.obj", ":/models/VRML/hotel.mtl");
    objects.append(hotel);

    Object3d* vilage = new Object3d();
    vilage->LoadFromFile(":/models/VRML/vilage.obj", ":/models/VRML/vilage.mtl");
    objects.append(vilage);

    Object3d* funfair = new Object3d();
    funfair->LoadFromFile(":/models/VRML/funfair.obj", ":/models/VRML/funfair.mtl");
    objects.append(funfair);

    Object3d* rock = new Object3d();
    rock->LoadFromFile(":/models/VRML/rock.obj", ":/models/VRML/rock.mtl");
    objects.append(rock);


    Object3d* world = new Object3d();
    world->LoadFromFile(":/models/VRML/world.obj", ":/models/VRML/world.mtl");
    objects.append(world);
*/

    Object3d* runway = new Object3d();
    runway->LoadFromFile(":/models/VRML/runway.obj", ":/models/VRML/runway.mtl");
    objects.append(runway);

/*
    Object3d* plane = new Object3d();
    plane->LoadFromFile(":/models/VRML/plane.obj", ":/models/VRML/plane.mtl");
    objects.append(plane);
    */
}

void VoxelTerrain::BeginFrame()
{
    cameraPos.y = cameraHeight;

    int xpos = cameraPos.x;
    int zpos = cameraPos.z;

    if(xpos < mapSize && zpos < mapSize && xpos >= 0 && zpos >= 0)
    {
        cameraPos.y = heightMap[zpos * mapSize + xpos] + cameraHeight;
    }

    frameBufferImage.fill(Qt::cyan);

    std::fill_n(zBuffer, screenWidth*screenHeight, 1);
    std::fill_n(yBuffer, screenWidth, screenHeight);


    viewMatrix.setToIdentity();
    viewMatrix.rotateX(-zAngle);
    viewMatrix.rotateY(qRadiansToDegrees(-cameraAngle));
    viewMatrix.translate(P3D::V3<fp>(-cameraPos.x, -cameraPos.y, -cameraPos.z));

    viewProjectionMatrix = projectionMatrix * viewMatrix;
}

void VoxelTerrain::Render()
{
    BeginFrame();

    Draw3d();
    return;


    fp sinphi = (float)qSin(cameraAngle);
    fp cosphi = (float)qCos(cameraAngle);

    fp cZstep = zStep;

    for(fp z = zNear; z < zFar; z += cZstep, cZstep += zStepD)
    {
        P3D::V2<fp> pleft = P3D::V2<fp>((-cosphi*z - sinphi*z) + cameraPos.x, ( sinphi*z - cosphi*z) + cameraPos.z);
        P3D::V2<fp> pright = P3D::V2<fp>(( cosphi*z - sinphi*z) + cameraPos.x, (-sinphi*z - cosphi*z) + cameraPos.z);

        P3D::V2<fp> delta((pright.x - pleft.x) / screenWidth, (pright.y - pleft.y) / screenWidth);

        fp invz = (fp(1) / z);

        fp zDepth = fp(1)-invz;

        fp invh = invz * heightScale;
        fp invy = invz * yScale;

#ifdef USE_FLOAT
        int yPos = (invy * cameraPos.y) + (screenHeight / 2);
#else
        int yPos = (invy.intMul(cameraPos.y)) + (screenHeight / 2);
#endif

        for(int i = 0; i < screenWidth; i++)
        {
            int pointHeight = 0;
            QRgb lineColor;

            if((pleft.x < mapSize && pleft.x >= 0) && (pleft.y < mapSize && pleft.y >= 0))
            {
                const int pixelOffs = (int)pleft.y*mapSize + (int)pleft.x;

                QRgb color = colorMap[pixelOffs];

                if(qAlpha(color) == 255)
                {
                    pointHeight = heightMap[pixelOffs];

                    lineColor = color;
                }
                else
                {
                    lineColor = water[((int)pleft.y & 63) * 64 + ((int)pleft.x & 63)];
                }
            }
            else
            {
                lineColor = water[((int)pleft.y & 63) * 64 + ((int)pleft.x & 63)];
            }

#ifdef USE_FLOAT
            int hDiff = invh * pointHeight;
#else
            int hDiff = invh.intMul(pointHeight);
#endif


            int lineTop = yPos - hDiff;

            if(lineTop < 0)
                lineTop = 0;

            for(int y = lineTop; y < yBuffer[i]; y++)
            {
                frameBuffer[y*screenWidth + i] = lineColor;
                zBuffer[(y * screenWidth) + i] = zDepth;
            }

            if (lineTop < yBuffer[i])
                yBuffer[i] = lineTop;

            pleft += delta;
        }
    }

    Draw3d();
}

void VoxelTerrain::Draw3d()
{
    for(int i = 0; i < objects.length(); i++)
    {
        const Object3d* obj = objects.at(i);

        this->DrawObject(obj);
    }
}

void VoxelTerrain::DrawObject(const Object3d* obj)
{
    modelMatrix.setToIdentity();
    modelMatrix.translate(obj->pos);

    transformMatrix = viewProjectionMatrix * modelMatrix;

    for(int i = 0; i < obj->mesh.length(); i++)
    {
        const Mesh3d* mesh = obj->mesh.at(i);

        this->DrawMesh(mesh);
    }
}

void VoxelTerrain::DrawMesh(const Mesh3d* mesh)
{
    for(int i = 0; i < mesh->tris.count(); i++)
    {
        const Triangle3d* tri = &mesh->tris.at(i);

        this->DrawTriangle(tri, mesh->texture, mesh->color);
    }
}

