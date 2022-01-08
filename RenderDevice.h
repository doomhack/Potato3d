#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include <vector>
#include "common.h"
#include "RenderTarget.h"

namespace P3D
{
    enum RenderType
    {
        Points,
        Lines,
        Flat,
        Texture,
    };

    enum RenderFlags
    {
        NoFlags = 0ul,
        BackFaceCulling = 1ul,
        FrontFaceCulling = 2ul,
        ZTest = 4ul,
        ZWrite = 8ul,
        AlphaTest = 16ul,
        CBuffer = 32ul,
        PerspectiveMapping = 64ul
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
        Reject = 32u
    } ClipPlane;

    class RenderVertexBuffer
    {
    public:
        const unsigned int* vertex_indexes;
        const V2<fp>* uvs;
        unsigned int count;
        const pixel* texture;
        pixel color;
    };

    class RenderDevice
    {
    public:
        RenderDevice();

        ~RenderDevice();

        //Render Target
        void SetRenderTarget(RenderTarget* target);
        void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

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

        void SetRenderType(RenderType type);

        //Begin/End Frame.
        void BeginFrame();
        void EndFrame();

        //Draw Objects.
        void DrawTriangle(const V3<fp> vertexes[3], const V2<fp> uvs[3], const pixel* texture, pixel color);

        void DrawPolygon(const V3<fp>* vertexes, const V2<fp>* uvs, const unsigned int count, const pixel* texture, pixel color);
        void TransformVertexes(const V3<fp>* vertexes, const unsigned int count);
        void DrawPolygon(const RenderVertexBuffer& render_buffer);

    private:

        unsigned int GetClipPlanesForPolygon(const unsigned int *vertex_indexes, unsigned int count);
        void ClipPolygon(unsigned int clip_planes, unsigned int* in, unsigned int* out);

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

        RenderType render_type;
    };
};
#endif // RENDERDEVICE_H
