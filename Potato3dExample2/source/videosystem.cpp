#include "../include/videosystem.h"

#ifdef GBA
    #include <gba.h>
    #include <gba_input.h>
    #include <gba_timers.h>

    #define DCNT_PAGE 0x0010
    #define VID_PAGE1 VRAM
    #define VID_PAGE2 0x600A000

    #define TM_FREQ_1024 0x0003
    #define TM_ENABLE 0x0080
    #define TM_CASCADE 0x0004
    #define TM_FREQ_1024 0x0003
    #define TM_FREQ_256 0x0002

    #define REG_WAITCNT	*((vu16 *)(0x4000204))
#endif

VideoSystem::VideoSystem()
{
#ifndef GBA
    timer.start();
#endif

}

void VideoSystem::Setup(unsigned short* keyState)
{
#ifdef GBA
    SetMode(MODE_4 | BG2_ENABLE | BIT(5));

    REG_TM2CNT_L= 65535-66;     // 66 = 1ms
    REG_TM2CNT_H = TM_FREQ_256 | TM_ENABLE;       // we're using the 256 cycle timer

    // cascade into tm3
    REG_TM3CNT_H = TM_CASCADE | TM_ENABLE;

    buffers[0] = new P3D::RenderTarget(width, height, (P3D::pixel*)VID_PAGE1);
    buffers[1] = new P3D::RenderTarget(width, height, (P3D::pixel*)VID_PAGE2);

    this->keyState = keyState;
#else
    image[0] = new QImage(width, height, QImage::Format_Indexed8);
    image[1] = new QImage(width, height, QImage::Format_Indexed8);

    image[0]->setColorCount(256);
    image[1]->setColorCount(256);

    buffers[0] = new P3D::RenderTarget(width, height, image[0]->scanLine(0));
    buffers[1] = new P3D::RenderTarget(width, height, image[1]->scanLine(0));

    int z = 0;
    application = new QApplication (z, nullptr);

    window = new GameWindow(keyState);
    window->SetBackbuffer(image[0]);

    window->setAttribute(Qt::WA_PaintOnScreen);
    window->resize(width * 2, height * 2);
    window->show();
#endif
}

const P3D::RenderTarget* VideoSystem::GetBackBuffer()
{
    return buffers[currentBuffer];
}

void VideoSystem::PageFlip()
{
#ifdef GBA
    REG_DISPCNT ^= DCNT_PAGE;
#else
    window->SetBackbuffer(image[currentBuffer]);
    window->repaint();
    application->processEvents();
#endif

    currentBuffer = 1 - currentBuffer;
}

void VideoSystem::SetPalette(const unsigned int pal[256])
{
#ifdef GBA

    unsigned short* pal_ram = (unsigned short*)0x5000000;

    for(int i = 0; i< 256; i++)
    {
        unsigned int r = (pal[i] >> 16) & 0xff;
        unsigned int g = (pal[i] >> 8) & 0xff;
        unsigned int b = (pal[i]) & 0xff;

        pal_ram[i] = RGB8(r, g, b);
    }

#else

    QList<QRgb> colorMap;

    for(int i = 0; i< 256; i++)
    {
        colorMap.append(QRgb(pal[i]));
    }

    image[0]->setColorTable(colorMap);
    image[1]->setColorTable(colorMap);

#endif
}

void VideoSystem::UpdateKeys()
{
#ifdef GBA
    scanKeys();

    u16 key = keysDown();

    if(key & KEY_UP)
        *keyState |= KeyUp;
    else if(key & KEY_DOWN)
        *keyState |= KeyUp;

    if(key & KEY_LEFT)
        *keyState |= KeyLeft;
    else if(key & KEY_RIGHT)
        *keyState |= KeyRight;

    key = keysUp();

    if(key & KEY_UP)
        *keyState &= ~KeyUp;
    else if(key & KEY_DOWN)
        *keyState &= ~KeyUp;

    if(key & KEY_LEFT)
        *keyState &= ~KeyLeft;
    else if(key & KEY_RIGHT)
        *keyState &= ~KeyRight;

#endif
}

unsigned int VideoSystem::GetTime()
{
#ifndef GBA
    return timer.elapsed();
#else
    return *((volatile unsigned short*)(0x400010C));
#endif
}
