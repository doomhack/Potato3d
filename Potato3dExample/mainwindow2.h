#ifndef MAINWINDOW2_H
#define MAINWINDOW2_H

#include <QtCore>
#include <QMainWindow>

#include "../RenderDevice.h"
#include "../RenderTarget.h"

class MainWindow2 : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow2(QWidget *parent = nullptr);
    ~MainWindow2();

protected:
    void paintEvent(QPaintEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:

    QElapsedTimer fpsTimer;
    QElapsedTimer renderTimer;

    QImage frameBufferImage;

    P3D::RenderTarget* render_target = nullptr;
    P3D::RenderDevice* render_device = nullptr;

    //static const int screenWidth = 1280;
    //static const int screenHeight = 1024;

    //static const int screenWidth = 640;
    //static const int screenHeight = 512;

    //static const int screenWidth = 160;
    //static const int screenHeight = 128;

    //static const int screenWidth = 240;
    //static const int screenHeight = 160;

    static const int screenWidth = 256;
    static const int screenHeight = 256;
};

#endif // MAINWINDOW2_H
