#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include <vector>
#include "Config.h"

#include "RenderCommon.h"
#include "RenderTarget.h"
#include "RenderTriangle.h"
#include "TextureCache.h"
#include "PixelShaderDefault.h"

namespace P3D
{
    class RenderDevice
    {
    public:
        explicit RenderDevice()
        {
            texture_cache = new TextureCacheDefault();

            current_material = new Material();

            //Populate matrix stack with 1 identity matrix.
            model_view_matrix_stack.push_back(M4<fp>());
            LoadIdentity();
        }

        ~RenderDevice() {}

        //Render Target
        void SetRenderTarget(const RenderTarget *target)
        {
            render_target = target;

            SetViewport(0, 0, render_target->GetWidth(), render_target->GetHeight());
        }

        void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
        {
            if(((x + width) > render_target->GetWidth()) || (y + height > render_target->GetHeight()))
            {
                x = 0;
                y = 0;
                width = render_target->GetWidth();
                height = render_target->GetHeight();
            }

            viewport.width = width;
            viewport.height = height;

            viewport.start = &render_target->GetColorBuffer()[(y * render_target->GetColorBufferYPitch()) + x];
            viewport.y_pitch = render_target->GetColorBufferYPitch();

            if(render_target->GetZBuffer())
            {
                viewport.z_start = &render_target->GetZBuffer()[(y * render_target->GetZBufferYPitch()) + x];
                viewport.z_y_pitch = render_target->GetZBufferYPitch();
            }
            else
            {
                viewport.z_start = nullptr;
                viewport.z_y_pitch = 0;
            }

            if(triangle_render)
            {
                triangle_render->SetRenderStateViewport(viewport);
            }
        }

        template<const unsigned int render_flags, class TPixelShader = PixelShaderDefault<render_flags>> void SetRenderFlags()
        {
            if(triangle_render)
                delete triangle_render;

            triangle_render = new P3D::Internal::RenderTriangle<render_flags, TPixelShader>();

            triangle_render->SetRenderStateViewport(viewport);
            triangle_render->SetZPlanes(z_planes);
            triangle_render->SetTextureCache(texture_cache);
            triangle_render->SetFogParams(fog_params);

    #ifdef RENDER_STATS
            triangle_render->SetRenderStats(render_stats);
    #endif
        }

        //Matrix
        void SetPerspective(const fp vertical_fov, const fp aspect_ratio, const fp z_near, const fp z_far)
        {
            projection_matrix.perspective(vertical_fov, aspect_ratio, z_near, z_far);

            z_planes.z_near = z_near;
            z_planes.z_far = z_far;

            z_planes.z_ratio_1 = z_far / (z_far - z_near);

            int zn = z_near;
            int zf = z_far;

            int zi = (-zf * zn);
            int zj = (zf - zn);

            z_planes.z_ratio_2 = fp::fromFPInt(FixedDiv(zi, zj, 16));
            //z_planes.z_ratio_2 = zi / zj;

            z_planes.z_ratio_3 = pReciprocal(z_far - z_near);

            if(triangle_render)
            {
                triangle_render->SetZPlanes(z_planes);
            }
        }

        void SetOrthographic(const fp left, const fp right, const fp bottom, const fp top, const fp z_near, const fp z_far)
        {
            projection_matrix.orthographic(left, right, bottom, top, z_near, z_far);

            z_planes.z_near = z_near;
            z_planes.z_far = z_far;
        }

        unsigned int PushMatrix()
        {
            const M4<fp> m = model_view_matrix_stack.back();

            model_view_matrix_stack.push_back(m);

            return (unsigned int)model_view_matrix_stack.size();
        }

        unsigned int PopMatrix()
        {
            if(model_view_matrix_stack.size() > 1)
            {
                model_view_matrix_stack.pop_back();
            }

            return (unsigned int)model_view_matrix_stack.size();
        }

        M4<fp>& GetMatrix()
        {
            return model_view_matrix_stack.back();
        }

        void LoadMatrix(const M4<fp> &matrix)
        {
            model_view_matrix_stack.pop_back();

            model_view_matrix_stack.push_back(matrix);
        }

        void LoadIdentity()
        {
            model_view_matrix_stack.back().setToIdentity();
        }

        void Translate(const V3<fp>& v)
        {
            model_view_matrix_stack.back().translate(v);
        }

        void RotateX(const fp angle)
        {
            model_view_matrix_stack.back().rotateX(angle);
        }

        void RotateY(const fp angle)
        {
            model_view_matrix_stack.back().rotateY(angle);
        }

        void RotateZ(const fp angle)
        {
            model_view_matrix_stack.back().rotateZ(angle);
        }


        //Clear
        void ClearColor(const pixel color)
        {
            unsigned int c32 = color;
            unsigned int shift = 0;

            if constexpr(sizeof(pixel) == 1)
            {
                c32 = (c32 << 24) | (c32 << 16) | (c32 << 8) | c32;
                shift = 2;
            }
            else if constexpr(sizeof(pixel) == 2)
            {
                c32 = (c32 << 16) | c32;
                shift = 1;
            }

            for(unsigned int y = 0; y < render_target->GetHeight(); y++)
            {
                pixel* p = &render_target->GetColorBuffer()[y * render_target->GetColorBufferYPitch()];

                FastFill32((unsigned int*)p, c32, render_target->GetWidth() >> shift);
            }
        }

        void ClearDepth(const z_val depth)
        {
            for(unsigned int y = 0; y < render_target->GetHeight(); y++)
            {
                z_val* z = &render_target->GetZBuffer()[y * render_target->GetZBufferYPitch()];

                for(unsigned int x = 0; x < render_target->GetWidth(); x++)
                {
                    z[x] = depth;
                }
            }
        }

        void ClearViewportColor(const pixel color)
        {
            unsigned int c32 = color;
            unsigned int shift = 0;

            if constexpr(sizeof(pixel) == 1)
            {
                c32 = (c32 << 24) | (c32 << 16) | (c32 << 8) | c32;
                shift = 2;
            }
            else if constexpr(sizeof(pixel) == 2)
            {
                c32 = (c32 << 16) | c32;
                shift = 1;
            }

            for(unsigned int y = 0; y < viewport.height; y++)
            {
                unsigned int count = viewport.width;

                pixel* p = &viewport.start[y * viewport.y_pitch];

                while(size_t(p) & 3)
                {
                    *p = color;
                    p++;
                    count--;
                }

                FastFill32((unsigned int*)p, c32, count >> shift);

                while(count & 3)
                {
                    *p++ = color;
                    count--;
                }
            }
        }

        void ClearViewportDepth(const z_val depth)
        {
            for(unsigned int y = 0; y < viewport.height; y++)
            {
                z_val* z = &viewport.z_start[y * viewport.z_y_pitch];

                for(unsigned int x = 0; x < viewport.width; x++)
                {
                    z[x] = depth;
                }
            }
        }

        void SetFogColor(const pixel color)
        {
            fog_params.fog_color = color;
        }

        void SetFogMode(FogMode mode)
        {
            fog_params.mode = mode;
        }

        void SetFogDepth(fp fog_start, fp fog_end)
        {
            fog_params.fog_start = fog_start;
            fog_params.fog_end = fog_end;
        }

        void SetFogDensity(fp density)
        {
            fog_params.fog_density = density;
        }

        //Begin/End Frame.
        void BeginFrame()
        {
#ifdef RENDER_STATS
            render_stats.ResetToZero();
#endif
        }

        void EndFrame() {}

        void BeginDraw(Plane<fp> frustrumPlanes[6] = nullptr)
        {
            UpdateTransformMatrix();

            if(frustrumPlanes)
            {
                transform_matrix.ExtractFrustrumPlanes(frustrumPlanes);
            }
        }

        void EndDraw() {}

        //
        void SetTextureCache(TextureCacheBase* cache)
        {
            if(texture_cache)
                delete texture_cache;

            texture_cache = cache;

            triangle_render->SetTextureCache(texture_cache);
        }

        void SetMaterial(const Material& material, const signed char importance = 0)
        {
            current_material = &material;

            if(material.type == Material::Texture)
            {
                texture_cache->AddTexture(material.pixels, importance);
            }
        }

        void SetFogLightMap(const unsigned char* colorMap)
        {
            triangle_render->SetFogLightMap(colorMap);
        }

        //Draw Objects.
        void DrawTriangle(const V3<fp> vertexes[3], const V2<fp> uvs[3] = nullptr, const fp light_levels[3] = nullptr)
        {
            TransformVertexes(vertexes, 3);

            const unsigned int indexes[3] = {0,1,2};

            DrawTriangle(indexes, uvs, light_levels);
        }

        void TransformVertexes(const V3<fp>* vertexes, const unsigned int count)
        {
            if(transformed_vertexes_buffer_count < count)
            {
                delete[] transformed_vertexes;
                transformed_vertexes = new V4<fp>[count];
                transformed_vertexes_buffer_count = count;
            }

            for(unsigned int i = 0; i < count; i++)
            {
                transformed_vertexes[i] = transform_matrix * vertexes[i];
            }

#ifdef RENDER_STATS
            render_stats.vertex_transformed += count;
#endif
        }

        void DrawTriangle(const unsigned int indexes[3], const V2<fp> uvs[3] = nullptr, const fp light_levels[3] = nullptr) const
        {
            P3D::Internal::TransformedTriangle tri;

            tri.verts[0].pos = transformed_vertexes[indexes[0]];
            tri.verts[1].pos = transformed_vertexes[indexes[1]];
            tri.verts[2].pos = transformed_vertexes[indexes[2]];

            if(current_material->type == Material::Texture)
            {
                tri.verts[0].uv =  uvs[0];
                tri.verts[1].uv =  uvs[1];
                tri.verts[2].uv =  uvs[2];
            }

            if(light_levels)
            {
                tri.verts[0].light_factor = pClamp(fp(0), fp(1) - light_levels[0], LIGHT_MAX);
                tri.verts[1].light_factor = pClamp(fp(0), fp(1) - light_levels[1], LIGHT_MAX);
                tri.verts[2].light_factor = pClamp(fp(0), fp(1) - light_levels[2], LIGHT_MAX);
            }

            triangle_render->DrawTriangle(tri, *current_material);
        }

#ifdef RENDER_STATS
        const RenderStats& GetRenderStats() const
        {
            return render_stats;
        }
#endif

    private:

        void UpdateTransformMatrix()
        {
            transform_matrix = projection_matrix * model_view_matrix_stack.back();

            for(int i = model_view_matrix_stack.size() -2; i >= 0; i--)
            {
                transform_matrix *= model_view_matrix_stack[i];
            }
        }

        const RenderTarget* render_target = nullptr;
        P3D::Internal::RenderTargetViewport viewport;
        P3D::Internal::RenderDeviceNearFarPlanes z_planes;
        P3D::Internal::RenderDeviceFogParameters fog_params;

        //Matrixes.
        M4<fp> projection_matrix;
        M4<fp> transform_matrix; //P*V*Stack
        std::vector<M4<fp>> model_view_matrix_stack;

        V4<fp>* transformed_vertexes = nullptr;
        unsigned int transformed_vertexes_buffer_count = 0;

        TextureCacheBase* texture_cache = nullptr;
        const Material* current_material = nullptr;

        P3D::Internal::RenderTriangleBase* triangle_render = nullptr;

#ifdef RENDER_STATS
        RenderStats render_stats;
#endif
    };
};
#endif // RENDERDEVICE_H
