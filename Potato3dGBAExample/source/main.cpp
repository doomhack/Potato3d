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

unsigned short* I_GetBackBuffer()
{
    if(REG_DISPCNT & DCNT_PAGE)
        return (unsigned short*)VID_PAGE1;

    return (unsigned short*)VID_PAGE2;
}

void I_FinishUpdate_e32()
{
    REG_DISPCNT ^= DCNT_PAGE;
}

#pragma pack(push,1)


typedef struct FileModel
{
    unsigned int mesh_count;
    P3D::V3<P3D::fp> pos;
} FileModel;

typedef struct FileTexture
{
    unsigned int width;
    unsigned int height;
    //unsigned short pixels[width * height];
} FileTexture;

typedef struct FileMesh
{
    unsigned int color;
    unsigned int has_texture;
    unsigned int triangle_count;
    //P3D::Triangle3d triangles[triangle_count];
} FileMesh;

#pragma pack(pop)

P3D::Model3d* LoadM3dData(const unsigned char* data)
{
    P3D::Model3d* model = new P3D::Model3d();


    FileModel* fm = (FileModel*)data;

    model->pos = fm->pos;

    FileMesh* fms = (FileMesh*)&fm[1];

    for(unsigned int i = 0; i < fm->mesh_count; i++)
    {
        P3D::Mesh3d* mesh = new P3D::Mesh3d();
        mesh->color = fms->color;

        P3D::Triangle3d* t = (P3D::Triangle3d*)&fms[1];

        for(unsigned int j = 0; j < fms->triangle_count; j++)
        {
            mesh->tris.push_back(&t[j]);
        }

        if(fms->has_texture)
        {
            FileTexture* ft = (FileTexture*)&t[fms->triangle_count];

            mesh->texture = new P3D::Texture;
            mesh->texture->width = ft->width;
            mesh->texture->height = ft->height;

            mesh->texture->pixels = (P3D::pixel*)&ft[1];

            fms = (FileMesh*)&mesh->texture->pixels[ft->width * ft->height];
        }
        else
        {
            fms = (FileMesh*)&t[fms->triangle_count];
        }

        model->mesh.push_back(mesh);
    }

    return model;
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
            obj3d->CameraAngle().y += 2;
        }

        if(key_down & KEY_RIGHT)
        {
            obj3d->CameraAngle().y -= 2;
        }
    }
}

#define INI_X 160
#define INI_Y 190
#define INI_DX 500
#define INI_DY 960

int main()
{    
    irqInit();

    //Set gamepak wait states and prefetch.
    REG_WAITCNT = 0x46DA;

    //Bit5 = unlocked vram at h-blank.
    SetMode(MODE_5 | BG2_ENABLE | BIT(5));

    REG_BG2PA=INI_X;	//escalado horizontal;
    REG_BG2PD=INI_Y;	//escalado vertical;
    REG_BG2X=INI_DX;	//desp horizontal
    REG_BG2Y=10;		//desp vert

    obj3d = new P3D::Object3d();

    obj3d->Setup(160, 128, 54, 5, 1024, I_GetBackBuffer());

    P3D::Model3d* runway = LoadM3dData(modeldata);
    obj3d->AddModel(runway);

    while(true)
    {
        PollKeys();

        obj3d->RenderScene();
        I_FinishUpdate_e32();

        obj3d->SetFramebuffer(I_GetBackBuffer());
    }
}
