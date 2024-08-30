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
    explicit GameWindow(unsigned short* keyState) : QWidget()
    {
        this->keyState = keyState;
    }

    void SetBackbuffer(QImage* image) { backBuffer = image; }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        if(!backBuffer)
            return;

        QPainter p(this);

        p.drawImage(this->rect(), *backBuffer, backBuffer->rect());
    }

    void closeEvent(QCloseEvent *event) override
    {
        exit(0);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if(event->key() == Qt::Key_Up)
            *keyState |= KeyUp;

        if(event->key() == Qt::Key_Down)
            *keyState |= KeyDown;

        if(event->key() == Qt::Key_Left)
            *keyState |= KeyLeft;

        if(event->key() == Qt::Key_Right)
            *keyState |= KeyRight;
    }

    void keyReleaseEvent(QKeyEvent *event) override
    {
        if(event->key() == Qt::Key_Up)
            *keyState &= ~KeyUp;

        if(event->key() == Qt::Key_Down)
            *keyState &= ~KeyDown;

        if(event->key() == Qt::Key_Left)
            *keyState &= ~KeyLeft;

        if(event->key() == Qt::Key_Right)
            *keyState &= ~KeyRight;
    }

private:
    QImage* backBuffer = nullptr;

    unsigned short* keyState = nullptr;
};
#endif


class VideoSystem
{
public:
    explicit VideoSystem();

    void Setup(unsigned short *keyState);
    const P3D::RenderTarget *GetBackBuffer();
    void PageFlip();
    void SetPalette(const unsigned int pal[]);

    void UpdateKeys();
    unsigned int GetTime();

private:
    P3D::RenderTarget* buffers[2] = {nullptr};
    int currentBuffer = 1;

    const int width = 240;
    const int height = 160;

    unsigned short* keyState = nullptr;

#ifndef GBA
    QImage *image[2];
    QApplication* application = nullptr;
    GameWindow* window = nullptr;

    QElapsedTimer timer;
#endif

};

#endif // VIDEOSYSTEM_H
