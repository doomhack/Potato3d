#ifndef VIDEOSYSTEM_H
#define VIDEOSYSTEM_H

#include "../include/common.h"

#include "../../RenderTarget.h"

#ifndef GBA

#include <QtCore>
#include <QtGui>
#include <QApplication>
#include <QWidget>

class GameWindow : public QWidget
{
public:
    explicit GameWindow() : QWidget()
    {
    }

    void SetBackbuffer(QImage* image) { backBuffer = image; }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        if(!backBuffer)
            return;

        QPainter p(this);

        p.drawImage(this->rect(), *backBuffer, backBuffer->rect());
    }

    void closeEvent(QCloseEvent *event) override
    {
        Q_UNUSED(event)

        exit(0);
    }

private:
    QImage* backBuffer = nullptr;
};
#endif


class VideoSystem
{
public:
    explicit VideoSystem();

    void Setup();
    const P3D::RenderTarget *GetBackBuffer();
    void PageFlip();
    void SetPalette(const unsigned int pal[]);

    void UpdateKeys();
    unsigned int GetTime();

private:
    P3D::RenderTarget* buffers[2] = {nullptr};
    int currentBuffer = 1;

    const int width = 256;
    const int height = 256;

    unsigned short* keyState = nullptr;

#ifndef GBA
    QImage *image[2];
    QApplication* application = nullptr;
    GameWindow* window = nullptr;

    QElapsedTimer timer;
#endif

};

#endif // VIDEOSYSTEM_H
