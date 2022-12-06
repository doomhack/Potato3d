#ifndef RENDERCOMMON_H
#define RENDERCOMMON_H

#include "Config.h"

namespace P3D
{
    enum RenderFlags
    {
        NoFlags = 0ul,
        ZTest = 1ul,
        ZWrite = 2ul,
        AlphaTest = 4ul,
        CBuffer = 8ul,
        FullPerspectiveMapping = 16ul,
        HalfPerspectiveMapping = 32ul,
        BackFaceCulling = 64ul,
        FrontFaceCulling = 128ul,
        WireFrame = 256ul,
        Fog = 512ul
    };

    typedef enum FogMode
    {
        FogLinear = 0u, //Linear fog
        FogExponential = 1u, //Exponential Fog
        FogExponential2 = 2u, //Exponential Squared Fog
    } FogMode;

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

    class RenderStats
    {
    public:
        unsigned int vertex_transformed;
        unsigned int triangles_submitted;
        unsigned int triangles_drawn;
        unsigned int scanlines_drawn;
        unsigned int span_checks;
        unsigned int span_count;

        void ResetToZero()
        {
            vertex_transformed = 0;
            triangles_submitted = 0;
            triangles_drawn = 0;
            scanlines_drawn = 0;
            span_checks = 0;
            span_count = 0;
        }
    };

    namespace Internal
    {
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

        class RenderDeviceFogParameters
        {
        public:

            union
            {
                struct
                {
                    fp fog_start = 0;
                    fp fog_end = 0;
                };
                struct
                {
                    fp fog_density;
                };
            };



            enum FogType
            {
                BlendFog = 0,
                TableFog = 1
            };

            FogType type = BlendFog;
            FogMode mode = FogLinear;

            union
            {
                struct
                {
                    pixel fog_color;
                };
                struct
                {
                    const pixel* fog_table = nullptr;
                    unsigned int fog_table_count = 0;
                };
            };
        };

        typedef enum ClipPlane
        {
            NoClip = 0u,
            W_Near = 1u,
            X_W_Left = 2u,
            X_W_Right = 4u,
            Y_W_Top = 8u,
            Y_W_Bottom = 16u,
            W_Far = 32u,
        } ClipPlane;

        typedef enum ClipOperation
        {
            Accept = 0u, //No clip required.
            Clip = 1u, //Clip required.
            Reject = 2u //All out. Reject polygon.
        } ClipOperation;

        class Vertex4d
        {
        public:
            V4<fp> pos;
            V2<fp> uv;

            void toPerspectiveCorrect(const fp scale = fp(1))
            {
                pos.w = fp(scale) / pos.w;
                uv.x = uv.x * pos.w;
                uv.y = uv.y * pos.w;
            }
        };

        class TransformedTriangle
        {
        public:
            Vertex4d verts[8];
        };
    };
};

#endif // RENDERCOMMON_H
