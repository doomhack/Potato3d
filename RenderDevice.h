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
        void SetRenderTarget(RenderTarget* target);
        void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

        //Flags
        #define RENDER_FLAGS(x) (new P3D::Internal::TriangleRenderFlags<x>())
        void SetRenderFlags(P3D::Internal::RenderFlagsBase* flags_ptr);

        //Matrix
        void SetPerspective(fp vertical_fov, fp aspect_ratio, fp z_near, fp z_far);
        void SetOrthographic(fp left, fp right, fp bottom, fp top, fp z_near, fp z_far);

        unsigned int PushMatrix();
        unsigned int PopMatrix();

        void UpdateTransformMatrix();

        void LoadIdentity();

        void Translate(V3<fp> v);

        void RotateX(fp angle);
        void RotateY(fp angle);
        void RotateZ(fp angle);

        //Clear
        void ClearColor(pixel color);
        void ClearDepth(z_val depth);

        //Begin/End Frame.
        void BeginFrame();
        void EndFrame();

        //
        void SetTextureCache(TextureCacheBase* cache);
        void SetMaterial(const Material& material, signed char importance = 0);

        //Draw Objects.
        void DrawTriangle(const V3<fp> vertexes[3], const V2<fp> uvs[3] = nullptr);

        void TransformVertexes(const V3<fp>* vertexes, const unsigned int count);

        void DrawTriangle(const unsigned int vx_indexes[], const V2<fp> uvs[3] = nullptr);

    private:

        RenderTarget* render_target = nullptr;
        RenderTargetViewport viewport;
        RenderDeviceNearFarPlanes z_planes;

        //Matrixes.
        M4<fp> projection_matrix;
        M4<fp> transform_matrix; //P*V*Stack
        std::vector<M4<fp>> model_view_matrix_stack;

        V4<fp>* transformed_vertexes = nullptr;
        unsigned int transformed_vertexes_buffer_count = 0;

        TextureCacheBase* texture_cache = nullptr;
        const Material* current_material = nullptr;

        P3D::Internal::RenderTriangleBase* triangle_render = nullptr;
        P3D::Internal::RenderFlagsBase* render_flags_base = nullptr;
    };
};
#endif // RENDERDEVICE_H
