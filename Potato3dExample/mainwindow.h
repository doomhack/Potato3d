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

    QElapsedTimer fpsTimer;

    P3D::Object3d* object3d;

    QImage frameBufferImage;

    static const int screenWidth = 720;
    static const int screenHeight = 360;
};
#endif // MAINWINDOW_H
