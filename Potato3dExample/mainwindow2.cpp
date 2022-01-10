#include "mainwindow2.h"

#include <QtCore>
#include <QtGui>

#include "../RenderDevice.h"
#include "../RenderTarget.h"

MainWindow2::MainWindow2(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(960, 640);
    this->update();

    fpsTimer.start();

    frameBufferImage = QImage(screenWidth, screenHeight, QImage::Format::Format_Indexed8);

    QVector<QRgb> color_table;

    color_table.append(qRgb(0,0,0));
    color_table.append(qRgb(0,255,0));

    frameBufferImage.setColorTable(color_table);

    render_target = new P3D::RenderTarget(screenWidth, screenHeight, frameBufferImage.scanLine(0));

    render_device = new P3D::RenderDevice();

    render_device->SetRenderTarget(render_target);

    float aspectRatio = (float)screenWidth / (float)screenHeight;

    render_device->SetPerspective(60, aspectRatio, 10, 1000);

    render_device->SetRenderFlags(RENDER_FLAGS(P3D::NoFlags));
}

MainWindow2::~MainWindow2()
{

}

void MainWindow2::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    static unsigned int frameCount = 0;
    static unsigned int currentFps = 0;

    static double aveRtime = 0;
    static unsigned int rTime = 0;

    renderTimer.restart();


    P3D::Material mat;
    mat.type = P3D::Material::Color;
    mat.color = 1;

    render_device->SetMaterial(mat);

    render_device->ClearColor(0);

    render_device->BeginFrame();

    P3D::V3<P3D::fp> v[3];
    v[0] = P3D::V3<P3D::fp>(-100,100,-100);
    v[1] = P3D::V3<P3D::fp>(100,100,-100);
    v[2] = P3D::V3<P3D::fp>(100,-100,-100);

    render_device->DrawTriangle(v);

    render_device->EndFrame();







    rTime += renderTimer.elapsed();

    QPainter p(this);

    p.drawImage(this->rect(), frameBufferImage);

    frameCount++;

    unsigned int elapsed = fpsTimer.elapsed();

    //P3D::RenderStats rs = object3d->GetRender()->GetRenderStats();

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
/*
    p.drawText(32,64, QString("Triangles submitted: %1").arg(rs.triangles_submitted));
    p.drawText(32,80, QString("Triangles drawn: %1").arg(rs.triangles_drawn));
    p.drawText(32,96, QString("Vertexes transformed: %1").arg(rs.vertex_transformed));
    p.drawText(32,112, QString("Scanlines drawn: %1").arg(rs.scanlines_drawn));
    p.drawText(32,128, QString("Spans checked: %1").arg(rs.span_checks));
    p.drawText(32,144, QString("Spans generated: %1").arg(rs.span_count));
*/
    this->update();
}

void MainWindow2::keyPressEvent(QKeyEvent *event)
{

}
