#include <stdarg.h>
#include <stdio.h>
#include <cstring>


#ifdef __arm__
    #include <gba.h>
    #include <gba_input.h>
    #include <gba_timers.h>
#else
    #include <QElapsedTimer>
#endif


#include "../../RenderDevice.h"
#include "../../RenderTarget.h"
#include "../../PixelShaderGBA8.h"

#define DCNT_PAGE 0x0010

#define VID_PAGE1 VRAM
#define VID_PAGE2 0x600A000

#define TM_FREQ_1024 0x0003
#define TM_ENABLE 0x0080
#define TM_CASCADE 0x0004
#define TM_FREQ_1024 0x0003
#define TM_FREQ_256 0x0002

#define REG_WAITCNT	*((vu16 *)(0x4000204))

#ifndef __arm__
    P3D::pixel frontbuffer[240*160];
    P3D::pixel backbuffer[240*160];
#endif

P3D::pixel* I_GetBackBuffer()
{    
#ifdef __arm__
    if(REG_DISPCNT & DCNT_PAGE)
        return (P3D::pixel*)VID_PAGE1;

    return (P3D::pixel*)VID_PAGE2;
#else
    return backbuffer;
#endif
}

void I_FinishUpdate_e32()
{
#ifdef __arm__
    REG_DISPCNT ^= DCNT_PAGE;
#else

#endif
}

inline uint32_t rng32()
{
    static uint32_t seed = 12345;

    return seed = (seed*1664525U + 1013904223U);
}

inline int8_t r8()
{
    return (int8_t)rng32();
}

int main()
{
#ifdef __arm__

    irqInit();

    //Set gamepak wait states and prefetch.
    REG_WAITCNT = 0x46DA;

    SetMode(MODE_4 | BG2_ENABLE | BIT(5));

    unsigned short* pal_ram = (unsigned short*)0x5000000;

    for(int i = 0; i< 256; i++)
    {
        pal_ram[i] = RGB8(i, i, i);
    }
#endif

    P3D::RenderTarget* render_target = new P3D::RenderTarget(240, 160, I_GetBackBuffer());
    render_target->AttachZBuffer();

    P3D::RenderDevice* render_device = new P3D::RenderDevice();
    render_device->SetRenderFlags<P3D::NoFlags, P3D::PixelShaderGBA8<P3D::NoFlags>>();

    render_device->SetRenderTarget(render_target);


    P3D::pixel* tex = new P3D::pixel[P3D::TEX_SIZE*P3D::TEX_SIZE];
    for(int i = 0; i < P3D::TEX_SIZE*P3D::TEX_SIZE; i++)
    {
        tex[i] = rand();
    }

    P3D::Material m;
#if 0
    m.type = P3D::Material::Texture;
    m.pixels = tex;
#else
    m.type = P3D::Material::Color;
    m.color = 100;
#endif
    render_device->SetMaterial(m);


    const P3D::fp aspectRatio = (float)240 / (float)160;

    render_device->SetPerspective(60, aspectRatio, 10, 1000);

    render_device->PushMatrix();
    render_device->LoadIdentity();
    render_device->Translate(P3D::V3<P3D::fp>(0,0,-200));

    render_device->BeginFrame();

    render_device->BeginDraw();
    render_device->ClearColor(0);


    P3D::V2<P3D::fp> uv[3];
    uv[0] = P3D::V2<P3D::fp>(0,0);
    uv[1] = P3D::V2<P3D::fp>(64,0);
    uv[2] = P3D::V2<P3D::fp>(64,64);

    unsigned int vi[3] = {2,1,0};

#ifndef __arm__
    QElapsedTimer t;
    t.start();
#else
    REG_TM2CNT_L= 65535-65;     // 65 ticks = 1/1000 secs
    REG_TM2CNT_H = TM_FREQ_256 | TM_ENABLE;       // we're using the 256 cycle timer

    // cascade into tm3
    REG_TM3CNT_H = TM_CASCADE | TM_ENABLE;
#endif

#ifdef __arm__
    const int runs = 1000;
#else
    const int runs = 1000000;
#endif

    I_FinishUpdate_e32();

    for(int i = 0; i < runs; i++)
    {        
        P3D::V3<P3D::fp> v[3];
        v[0] = P3D::V3<P3D::fp>(-100 + r8(),100+ r8(),0+ r8());
        v[1] = P3D::V3<P3D::fp>(100+ r8(),100+ r8(),0+ r8());
        v[2] = P3D::V3<P3D::fp>(100+ r8(),-100+ r8(),0+ r8());

        render_device->TransformVertexes(v, 3);

        render_device->DrawTriangle(vi, uv);
    }



#ifndef __arm__
    uint64_t x = t.elapsed();
#else

    uint64_t x = REG_TM3CNT_L;
    consoleDemoInit();

#endif

    render_device->EndDraw();

    render_device->EndFrame();

    render_device->PopMatrix();
    render_device->PopMatrix();

    double s = x / 1000.0;
    double p = (double)runs / s;

    printf("%d polys rendered in %f s\n", runs, s);
    printf("Polys/second %d\n", (int)p);


    while(true)
    {
    }
}



