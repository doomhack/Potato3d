#include <gba.h>
#include <gba_input.h>
#include <gba_timers.h>

#include "../../potato3d.h"

#include "model.h"

#define DCNT_PAGE 0x0010

#define VID_PAGE1 VRAM
#define VID_PAGE2 0x600A000

#define TM_FREQ_1024 0x0003
#define TM_ENABLE 0x0080
#define TM_CASCADE 0x0004
#define TM_FREQ_1024 0x0003
#define TM_FREQ_256 0x0002

#define REG_WAITCNT	*((vu16 *)(0x4000204))

unsigned char* I_GetBackBuffer()
{
    if(REG_DISPCNT & DCNT_PAGE)
        return (unsigned char*)VID_PAGE1;

    return (unsigned char*)VID_PAGE2;
}

void I_FinishUpdate_e32()
{
    REG_DISPCNT ^= DCNT_PAGE;
}

P3D::Object3d* obj3d = nullptr;

void PollKeys()
{
    scanKeys();

    u16 key_down = keysHeld();

    if(key_down)
    {
        if(key_down & KEY_LEFT)
        {
            obj3d->CameraAngle().y += 4;
        }

        if(key_down & KEY_RIGHT)
        {
            obj3d->CameraAngle().y -= 4;
        }

        if(key_down & KEY_UP)
        {
            P3D::V3<P3D::fp> camAngle = obj3d->CameraAngle();

            P3D::fp angleYRad = P3D::pD2R(camAngle.y);

            P3D::V3<P3D::fp> d(-(std::sin(angleYRad.f()) *25), 0, -(std::cos(angleYRad.f()) *25));

            obj3d->CameraPos() += d;
        }

        if(key_down & KEY_DOWN)
        {
            P3D::V3<P3D::fp> camAngle = obj3d->CameraAngle();

            P3D::fp angleYRad = P3D::pD2R(camAngle.y);

            P3D::V3<P3D::fp> d(-(std::sin(angleYRad.f()) *25), 0, -(std::cos(angleYRad.f()) *25));

            obj3d->CameraPos() -= d;
        }

        if(key_down & KEY_L)
        {
            obj3d->CameraPos().y -= 10;
        }

        if(key_down & KEY_R)
        {
            obj3d->CameraPos().y += 10;
        }
    }
}

int main()
{    
    irqInit();

    //Set gamepak wait states and prefetch.
    REG_WAITCNT = 0x46DA;

    SetMode(MODE_4 | BG2_ENABLE | BIT(5));

    obj3d = new P3D::Object3d();

    obj3d->Setup(240, 160, 60, 25, 2500, I_GetBackBuffer());


    const P3D::BspModel* runway = (const P3D::BspModel*)modeldata;
    obj3d->SetModel(runway);

    unsigned short* pal_ram = (unsigned short*)0x5000000;

    for(int i = 0; i< 256; i++)
    {
        unsigned int c = runway->GetColorMapColor(i);

        unsigned int r = (c >> 16) & 0xff;
        unsigned int g = (c >> 8) & 0xff;
        unsigned int b = (c) & 0xff;

        pal_ram[i] = RGB8(r, g, b);
    }


    obj3d->SetBackgroundColor(RGB8(0x4d, 0xc9, 0xff));

    while(true)
    {
        PollKeys();

        obj3d->RenderScene();
        I_FinishUpdate_e32();

        obj3d->SetFrameBuffer(I_GetBackBuffer());
    }
}
