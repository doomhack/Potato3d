#include "../include/videosystem.h"
#include <windows.h>

int z = 0;
QApplication* VideoSystem::application = new QApplication (z, nullptr);

VideoSystem::VideoSystem()
{
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

    timer.start();
}

void VideoSystem::Setup(bool withGui)
{
    image[0] = new QImage(width, height, QImage::Format_Indexed8);
    image[1] = new QImage(width, height, QImage::Format_Indexed8);

    image[0]->setColorCount(256);
    image[1]->setColorCount(256);

    buffers[0] = new P3D::RenderTarget(width, height, image[0]->scanLine(0));
    buffers[1] = new P3D::RenderTarget(width, height, image[1]->scanLine(0));

    buffers[0]->AttachZBuffer();
    buffers[1]->AttachZBuffer();

    if(withGui)
    {
        window = new GameWindow();
        window->SetBackbuffer(image[0]);

        window->setAttribute(Qt::WA_PaintOnScreen);
        window->resize(width * 2, height * 2);
        window->show();
    }
}

const P3D::RenderTarget* VideoSystem::GetBackBuffer()
{
    return buffers[currentBuffer];
}

void VideoSystem::PageFlip()
{
    if(window)
    {
        window->SetBackbuffer(image[currentBuffer]);
        window->repaint();
        application->processEvents();
    }

    currentBuffer = 1 - currentBuffer;
}

void VideoSystem::SetPalette(const unsigned int pal[256])
{
    QList<QRgb> colorMap;

    for(int i = 0; i< 256; i++)
    {
        colorMap.append(QRgb(pal[i]));
    }

    image[0]->setColorTable(colorMap);
    image[1]->setColorTable(colorMap);
}

void VideoSystem::UpdateKeys()
{

}

unsigned int VideoSystem::GetTime()
{
    return timer.elapsed();
}
