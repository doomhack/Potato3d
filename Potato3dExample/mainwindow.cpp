#include "mainwindow.h"

#include <QtCore>
#include <QtGui>

const QImage::Format format = QImage::Format_Indexed8;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    this->resize(960, 640);
    this->update();

    fpsTimer.start();

    frameBufferImage = QImage(screenWidth, screenHeight, format);

    object3d = new P3D::Object3d();

    QFile f("C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\Streets\\Streets.bsp");
    f.open(QFile::ReadOnly);
    QByteArray* bf = new QByteArray();
    *bf = f.readAll();
    f.close();

    const P3D::BspModel* bspModel = (P3D::BspModel*)bf->constData();

    object3d->SetModel(bspModel);

    //object3d->Setup(screenWidth, screenHeight, 54, 25, 1500, (P3D::pixel*)frameBufferImage.bits());
    object3d->Setup(screenWidth, screenHeight, 60, 25, 3000, (P3D::pixel*)frameBufferImage.bits());

    object3d->SetBackgroundColor(0x799ED7);

    for(int i = 0; i < 256; i++)
        frameBufferImage.setColor(i, bspModel->GetColorMapColor(i));
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

    p.setPen(Qt::red);

    p.drawText(32,32, QString("FPS: %1").arg(currentFps));
    p.drawText(32,48, QString("Ave Render Time: %1ms").arg(aveRtime));

    p.drawText(32,64, QString("Triangles submitted: %1").arg(rs.triangles_submitted));
    p.drawText(32,80, QString("Triangles drawn: %1").arg(rs.triangles_drawn));
    p.drawText(32,96, QString("Vertexes transformed: %1").arg(rs.vertex_transformed));
    p.drawText(32,112, QString("Scanlines drawn: %1").arg(rs.scanlines_drawn));
    p.drawText(32,128, QString("Spans checked: %1").arg(rs.span_checks));
    p.drawText(32,144, QString("Spans generated: %1").arg(rs.span_count));
    p.drawText(32,160, QString("Triangles clipped: %1").arg(rs.triangles_clipped));

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
