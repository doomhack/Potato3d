#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QMainWindow>

#include "../potato3d.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:

    P3D::Model3d* LoadObjFile(QString objFile, QString mtlFile);
    void SaveBytesAsCFile(QByteArray *bytes, QString file);


    QElapsedTimer fpsTimer;
    QElapsedTimer renderTimer;


    P3D::Object3d* object3d;

    QImage frameBufferImage;

    //static const int screenWidth = 1920;
    //static const int screenHeight = 1080;

    //static const int screenWidth = 1280;
    //static const int screenHeight = 1024;

    //static const int screenWidth = 640;
    //static const int screenHeight = 512;

    //static const int screenWidth = 160;
    //static const int screenHeight = 128;

    static const int screenWidth = 240;
    static const int screenHeight = 160;

    //static const int screenWidth = 256;
    //static const int screenHeight = 256;
};
#endif // MAINWINDOW_H
