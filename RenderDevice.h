#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include <vector>
#include "common.h"

#include "RenderCommon.h"
#include "RenderTarget.h"
#include "RenderTriangle.h"
#include "TextureCache.h"

namespace P3D
{
    class RenderDevice
    {
    public:
        RenderDevice();

        ~RenderDevice();

        //Render Target
        void SetRenderTarget(const RenderTarget *target);
        void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

        template<const unsigned int render_flags> void SetRenderFlags()
        {
            if(triangle_render)
                delete triangle_render;

            triangle_render = new P3D::Internal::RenderTriangle<render_flags>();

            triangle_render->SetRenderStateViewport(viewport, z_planes);
            triangle_render->SetTextureCache(texture_cache);

    #ifdef RENDER_STATS
            triangle_render->SetRenderStats(render_stats);
    #endif
        }

        //Matrix
        void SetPerspective(const fp vertical_fov, const fp aspect_ratio, const fp z_near, const fp z_far);
        void SetOrthographic(const fp left, const fp right, const fp bottom, const fp top, const fp z_near, const fp z_far);

        unsigned int PushMatrix();
        unsigned int PopMatrix();
        M4<fp>& GetMatrix();
        void LoadMatrix(const M4<fp> &matrix);

        void LoadIdentity();

        void Translate(const V3<fp>& v);

        void RotateX(const fp angle);
        void RotateY(const fp angle);
        void RotateZ(const fp angle);


        //Clear
        void ClearColor(const pixel color);
        void ClearDepth(const z_val depth);

        void ClearViewportColor(const pixel color);
        void ClearViewportDepth(const z_val depth);

        //Begin/End Frame.
        void BeginFrame();
        void EndFrame();

        void BeginDraw();
        void EndDraw();

        //
        void SetTextureCache(TextureCacheBase* cache);
        void SetMaterial(const Material& material, const signed char importance = 0);

        //Draw Objects.
        void DrawTriangle(const V3<fp> vertexes[3], const V2<fp> uvs[3] = nullptr);

        void TransformVertexes(const V3<fp>* vertexes, const unsigned int count);

        void DrawTriangle(const unsigned int vx_indexes[], const V2<fp> uvs[3] = nullptr) const;

#ifdef RENDER_STATS
        const RenderStats& GetRenderStats() const;
#endif

    private:

        void UpdateTransformMatrix();

        const RenderTarget* render_target = nullptr;
        P3D::Internal::RenderTargetViewport viewport;
        P3D::Internal::RenderDeviceNearFarPlanes z_planes;

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
