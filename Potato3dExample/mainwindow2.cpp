
#include "mainwindow2.h"

#include <QtCore>
#include <QtGui>

#include "../RenderDevice.h"
#include "../RenderTarget.h"
#include "../PixelShaderGBA8.h"

const QImage::Format format = QImage::Format::Format_Indexed8;

MainWindow2::MainWindow2(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(screenWidth * 3, screenHeight * 3);
    this->update();

    fpsTimer.start();

    frameBufferImage = QImage(screenWidth, screenHeight, format);

    QVector<QRgb> color_table;

    color_table.append(qRgb(0,0,0));
    color_table.append(qRgb(0,255,0));
    color_table.append(qRgb(255,0,0));


    frameBufferImage.setColorTable(color_table);

    render_target = new P3D::RenderTarget(screenWidth, screenHeight, (P3D::pixel*)frameBufferImage.scanLine(0));
    render_target->AttachZBuffer();

    render_device = new P3D::RenderDevice();

    render_device->SetRenderTarget(render_target);

    const float aspectRatio = (float)screenWidth / (float)screenHeight;

    render_device->SetPerspective(60, aspectRatio, 10, 1000);

    //render_device->SetRenderFlags(RENDER_FLAGS(P3D::ZTest | P3D::ZWrite));
    //render_device->SetRenderFlags<P3D::NoFlags>();
    render_device->SetRenderFlags<P3D::NoFlags, P3D::PixelShaderGBA8<P3D::NoFlags>>();

    render_device->SetFogColor(0);

#if 1
    render_device->SetFogMode(P3D::FogLinear);
    render_device->SetFogDepth(200, 400);
#else
    render_device->SetFogMode(P3D::FogExponential2);
    render_device->SetFogDensity(0.0025);
#endif
}

MainWindow2::~MainWindow2()
{

}
static P3D::fp rotateY = 0;
static P3D::fp translate = -200;

void MainWindow2::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)


    static unsigned int frameCount = 0;
    static unsigned int currentFps = 0;

    static double aveRtime = 0;
    static unsigned int rTime = 0;

    renderTimer.restart();


    QImage texture = QImage(":/models/test_text.png");
    texture.convertTo(format);
    frameBufferImage.setColorTable(texture.colorTable());

    P3D::Material mat1;
    mat1.type = P3D::Material::Texture;
    mat1.pixels = (P3D::pixel*)texture.constBits();

    render_device->SetMaterial(mat1);

    render_device->ClearColor(0);
    render_device->ClearDepth(1);

    render_device->PushMatrix();
    render_device->RotateY(rotateY);

    render_device->PushMatrix();
    render_device->LoadIdentity();
    render_device->Translate(P3D::V3<P3D::fp>(0,0,translate));

/*

    render_device->BeginFrame();

    P3D::V3<P3D::fp> v[3];
    v[0] = P3D::V3<P3D::fp>(-100,100,0);
    v[1] = P3D::V3<P3D::fp>(100,100,0);
    v[2] = P3D::V3<P3D::fp>(100,-100,0);

    P3D::V2<P3D::fp> uv[3];
    uv[0] = P3D::V2<P3D::fp>(0,0);
    uv[1] = P3D::V2<P3D::fp>(64,0);
    uv[2] = P3D::V2<P3D::fp>(64,64);


    render_device->DrawTriangle(v, uv);

    v[0] = P3D::V3<P3D::fp>(-100,100,0);
    v[1] = P3D::V3<P3D::fp>(100,-100,0);
    v[2] = P3D::V3<P3D::fp>(-100,-100,0);

    uv[0] = P3D::V2<P3D::fp>(0,0);
    uv[1] = P3D::V2<P3D::fp>(64,64);
    uv[2] = P3D::V2<P3D::fp>(0,64);

    render_device->DrawTriangle(v, uv);

    render_device->EndFrame();
*/

    render_device->BeginFrame();

    render_device->BeginDraw();

    P3D::V3<P3D::fp> v[4];
    v[0] = P3D::V3<P3D::fp>(-100,100,0);
    v[1] = P3D::V3<P3D::fp>(100,100,0);
    v[2] = P3D::V3<P3D::fp>(100,-100,0);
    v[3] = P3D::V3<P3D::fp>(-100,-100,0);

    render_device->TransformVertexes(v, 4);

    P3D::V2<P3D::fp> uv[3];
    uv[0] = P3D::V2<P3D::fp>(0,0);
    uv[1] = P3D::V2<P3D::fp>(64,0);
    uv[2] = P3D::V2<P3D::fp>(64,64);

    unsigned int vi[3] = {2,1,0};

    render_device->DrawTriangle(vi, uv);

    uv[0] = P3D::V2<P3D::fp>(0,0);
    uv[1] = P3D::V2<P3D::fp>(64,64);
    uv[2] = P3D::V2<P3D::fp>(0,64);

    unsigned int vi2[3] = {0,2,3};

    render_device->DrawTriangle(vi2, uv);


    render_device->EndDraw();

    render_device->EndFrame();

    render_device->PopMatrix();
    render_device->PopMatrix();


    rTime += renderTimer.elapsed();

    QPainter p(this);

    p.drawImage(this->rect(), frameBufferImage);

    frameCount++;

    unsigned int elapsed = fpsTimer.elapsed();

    P3D::RenderStats rs = render_device->GetRenderStats();

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

void MainWindow2::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Left)
    {
        rotateY -= P3D::fp(1);
    }
    else if(event->key() == Qt::Key_Right)
    {
        rotateY += P3D::fp(1);
    }

    if(event->key() == Qt::Key_Up)
    {
        translate += P3D::fp(1);
    }
    else if(event->key() == Qt::Key_Down)
    {
        translate -= P3D::fp(1);
    }

    if(rotateY >= 360)
        rotateY -= 360;

    if(rotateY < 0)
        rotateY += 360;
}
