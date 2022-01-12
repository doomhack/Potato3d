#ifndef RENDERCOMMON_H
#define RENDERCOMMON_H

#include "common.h"

namespace P3D
{
    enum RenderFlags
    {
        NoFlags = 0ul,
        ZTest = 1ul,
        ZWrite = 2ul,
        AlphaTest = 4ul,
        CBuffer = 8ul,
        PerspectiveMapping = 16ul,
        BackFaceCulling = 32ul,
        FrontFaceCulling = 64ul,
        WireFrame = 128ul
    };


    class RenderTargetViewport
    {
    public:
        pixel* start = nullptr;
        unsigned int width = 0;
        unsigned int height = 0;
        unsigned int y_pitch = 0;

        z_val* z_start = nullptr;
        unsigned int z_y_pitch;
    };

    class RenderDeviceNearFarPlanes
    {
    public:
        fp z_near = 0;
        fp z_far = 0;
    };

    typedef enum ClipPlane
    {
        NoClip = 0u,
        W_Near = 1u,
        X_W_Left = 2u,
        X_W_Right = 4u,
        Y_W_Top = 8u,
        Y_W_Bottom = 16u,
    } ClipPlane;


    class Vertex4d
    {
    public:
        V4<fp> pos;
        V2<fp> uv;

        void toPerspectiveCorrect()
        {
            uv.x = uv.x / pos.w;
            uv.y = uv.y / pos.w;

            pos.w = fp(1) / pos.w;
        }
    };

    class TransformedTriangle
    {
    public:
        Vertex4d verts[8];
    };


    class Material
    {
    public:

        Material() {color = 0;};

        enum MaterialType
        {
            Color = 0,
            Texture = 1
        };

        MaterialType type = Color;

        union
        {
            struct
            {
                const pixel* pixels;
                pixel alpha_mask; //If pixel & alpha_mask > 0 -> draw pixel.
            };
            struct
            {
                pixel color;
            };
        };

        static constexpr unsigned int width = TEX_SIZE;
        static constexpr unsigned int height = TEX_SIZE;
    };
};

#endif // RENDERCOMMON_H
