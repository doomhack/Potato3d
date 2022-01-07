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
        AffineMapping,
        PerspectiveMapping
    };

    enum RenderFlags
    {
        NoFlags = 0ul,
        BackFaceCulling = 1ul,
        FrontFaceCulling = 2ul,
        ZTest = 4ul,
        ZWrite = 8ul,
        AlphaTest = 16ul,
        CBuffer = 32ul
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
        Y_W_Bottom = 16u
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

    template<const RenderFlags render_flags> class RenderDevice
    {
    public:
        RenderDevice()
        {
            PushMatrix();
        }

        ~RenderDevice() {}

        //Render Target
        void SetRenderTarget(RenderTarget* target)
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
                viewport.y_pitch = render_target->GetZBufferYPitch();
            }
            else
            {
                viewport.z_start = nullptr;
                viewport.z_y_pitch = 0;
            }
        }

        //Matrix
        void SetPerspective(fp vertical_fov, fp aspect_ratio, fp z_near, fp z_far)
        {
            projection_matrix.setToIdentity();
            projection_matrix.perspective(vertical_fov, aspect_ratio, z_near, z_far);
        }

        void SetOrthographic(fp left, fp right, fp bottom, fp top, fp z_near, fp z_far)
        {
            projection_matrix.setToIdentity();
            projection_matrix.orthographic(left, right, bottom, top, z_near, z_far);
        }

        unsigned int PushMatrix()
        {
            M4<fp> m;
            m.setToIdentity();

            model_view_matrix_stack.push_back(m);

            return model_view_matrix_stack.size();
        }

        unsigned int PopMatrix()
        {
            if(model_view_matrix_stack.size() > 1)
            {
                model_view_matrix_stack.pop_back();
            }

            return model_view_matrix_stack.size();
        }

        void UpdateTransformMatrix()
        {
            transform_matrix = projection_matrix * model_view_matrix_stack.back();

            for(int i = model_view_matrix_stack.size() -1; i >= 1; i--)
            {
                transform_matrix *= model_view_matrix_stack[i];
            }
        }

        void LoadIdentity()
        {
            model_view_matrix_stack.back().setToIdentity();
        }

        void Translate(V3<fp> v)
        {
            model_view_matrix_stack.back().translate(v);
        }

        void RotateX(fp angle)
        {
            model_view_matrix_stack.back().rotateX(angle);
        }

        void RotateY(fp angle)
        {
            model_view_matrix_stack.back().rotateY(angle);
        }

        void RotateZ(fp angle)
        {
            model_view_matrix_stack.back().rotateZ(angle);
        }

        //Clear
        void ClearColor(pixel color)
        {
            for(unsigned int y = 0; y < render_target->GetHeight(); y++)
            {
                pixel* p = (y * render_target->GetColorBufferYPitch());

                for(unsigned int x = 0; x < render_target->GetWidth(); x++)
                {
                    p[x] = color;
                }
            }
        }

        void ClearDepth(z_val depth)
        {
            for(unsigned int y = 0; y < render_target->GetHeight(); y++)
            {
                z_val* z = (y * render_target->GetZBufferYPitch());

                for(unsigned int x = 0; x < render_target->GetWidth(); x++)
                {
                    z[x] = depth;
                }
            }
        }

        //Begin/End Frame.
        void BeginFrame()
        {
            UpdateTransformMatrix();
        }

        void EndFrame()
        {

        }

        //Draw Objects.
        void DrawTriangle(const V3<fp> vertexes[3], const V2<fp> uvs[3], const pixel* texture, pixel color)
        {
            DrawPolygon(vertexes, uvs, 3, texture, color);
        }

        void DrawPolygon(const V3<fp>* vertexes, const V2<fp>* uvs, const unsigned int count, const pixel* texture, pixel color)
        {
            TransformVertexes(vertexes, count);

            unsigned int indexes[count];

            for(unsigned int i = 0; i < count; i++)
            {
                indexes[i] = i;
            }

            RenderVertexBuffer rvb;
            rvb.vertex_indexes = indexes;
            rvb.uvs = uvs;
            rvb.count = count;
            rvb.texture = texture;
            rvb.color = color;

            DrawPolygon(rvb);
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
        }

        void DrawPolygon(RenderVertexBuffer& render_buffer)
        {

        }

    private:

        RenderTarget* render_target = nullptr;
        RenderTargetViewport viewport;
        RenderDeviceNearFarPlanes clip_planes;

        //Matrixes.
        M4<fp> projection_matrix;
        M4<fp> transform_matrix; //P*V*Stack
        std::vector<M4<fp>> model_view_matrix_stack;

        V4<fp>* transformed_vertexes = nullptr;
        unsigned int transformed_vertexes_buffer_count = 0;
    };
};
#endif // RENDERDEVICE_H
