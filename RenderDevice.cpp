#include "RenderDevice.h"

namespace P3D
{
    RenderDevice::RenderDevice()
    {
        PushMatrix();
    }

    RenderDevice::~RenderDevice()
    {

    }

    //Render Target
    void RenderDevice::SetRenderTarget(RenderTarget* target)
    {
        render_target = target;

        SetViewport(0, 0, render_target->GetWidth(), render_target->GetHeight());
    }

    void RenderDevice::SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
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
    void RenderDevice::SetPerspective(fp vertical_fov, fp aspect_ratio, fp z_near, fp z_far)
    {
        projection_matrix.setToIdentity();
        projection_matrix.perspective(vertical_fov, aspect_ratio, z_near, z_far);
    }

    void RenderDevice::SetOrthographic(fp left, fp right, fp bottom, fp top, fp z_near, fp z_far)
    {
        projection_matrix.setToIdentity();
        projection_matrix.orthographic(left, right, bottom, top, z_near, z_far);
    }

    unsigned int RenderDevice::PushMatrix()
    {
        M4<fp> m;
        m.setToIdentity();

        model_view_matrix_stack.push_back(m);

        return model_view_matrix_stack.size();
    }

    unsigned int RenderDevice::PopMatrix()
    {
        if(model_view_matrix_stack.size() > 1)
        {
            model_view_matrix_stack.pop_back();
        }

        return model_view_matrix_stack.size();
    }

    void RenderDevice::UpdateTransformMatrix()
    {
        transform_matrix = projection_matrix * model_view_matrix_stack.back();

        for(int i = model_view_matrix_stack.size() -1; i >= 1; i--)
        {
            transform_matrix *= model_view_matrix_stack[i];
        }
    }

    void RenderDevice::LoadIdentity()
    {
        model_view_matrix_stack.back().setToIdentity();
    }

    void RenderDevice::Translate(V3<fp> v)
    {
        model_view_matrix_stack.back().translate(v);
    }

    void RenderDevice::RotateX(fp angle)
    {
        model_view_matrix_stack.back().rotateX(angle);
    }

    void RenderDevice::RotateY(fp angle)
    {
        model_view_matrix_stack.back().rotateY(angle);
    }

    void RenderDevice::RotateZ(fp angle)
    {
        model_view_matrix_stack.back().rotateZ(angle);
    }

    //Clear
    void RenderDevice::ClearColor(pixel color)
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

    void RenderDevice::ClearDepth(z_val depth)
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

    void RenderDevice::SetRenderType(RenderType type)
    {
        render_type = type;
    }

    //Begin/End Frame.
    void RenderDevice::BeginFrame()
    {
        UpdateTransformMatrix();
    }

    void RenderDevice::EndFrame()
    {

    }

    //Draw Objects.
    void RenderDevice::DrawTriangle(const V3<fp> vertexes[3], const V2<fp> uvs[3], const pixel* texture, pixel color)
    {
        DrawPolygon(vertexes, uvs, 3, texture, color);
    }

    void RenderDevice::DrawPolygon(const V3<fp>* vertexes, const V2<fp>* uvs, const unsigned int count, const pixel* texture, pixel color)
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

    void RenderDevice::TransformVertexes(const V3<fp>* vertexes, const unsigned int count)
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

    void RenderDevice::DrawPolygon(const RenderVertexBuffer& render_buffer)
    {
        unsigned int clip_planes = GetClipPlanesForPolygon(render_buffer.vertex_indexes);

        if(clip_planes == Reject)
            return;

        if(clip_planes == NoClip)
        {

        }
        else
        {

        }
    }

    unsigned int RenderDevice::GetClipPlanesForPolygon(unsigned int* vertex_indexes, unsigned int count)
    {
        unsigned int clip = NoClip;

        bool reject = true;
        for(unsigned int i = 0; i < count; i++)
        {
            if(transformed_vertexes[vertex_indexes[i]].w > z_planes.z_near)
            {
                reject = false;
                break;
            }
        }

        if(reject)
            return Reject;

        reject = true;
        for(unsigned int i = 0; i < count; i++)
        {
            if(transformed_vertexes[vertex_indexes[i]].w < z_planes.z_far)
            {
                reject = false;
                break;
            }
        }

        if(reject)
            return Reject;

        for(unsigned int i = 0; i < count; i++)
        {
            if(transformed_vertexes[vertex_indexes[i]].w < z_planes.z_near)
            {
                clip |= W_Near;
                break;
            }
        }


        reject = true;
        for(unsigned int i = 0; i < count; i++)
        {
            if(transformed_vertexes[vertex_indexes[i]].x < transformed_vertexes[vertex_indexes[i]].w)
            {
                reject = false;
                break;
            }
        }

        if(reject)
            return Reject;

        reject = true;
        for(unsigned int i = 0; i < count; i++)
        {
            if(-transformed_vertexes[vertex_indexes[i]].x < transformed_vertexes[vertex_indexes[i]].w)
            {
                reject = false;
                break;
            }
        }

        if(reject)
            return Reject;


        for(unsigned int i = 0; i < count; i++)
        {
            if(transformed_vertexes[vertex_indexes[i]].x > transformed_vertexes[vertex_indexes[i]].w)
            {
                clip |= X_W_Right;
                break;
            }
        }

        for(unsigned int i = 0; i < count; i++)
        {
            if(-transformed_vertexes[vertex_indexes[i]].x > transformed_vertexes[vertex_indexes[i]].w)
            {
                clip |= X_W_Left;
                break;
            }
        }

        reject = true;
        for(unsigned int i = 0; i < count; i++)
        {
            if(transformed_vertexes[vertex_indexes[i]].y < transformed_vertexes[vertex_indexes[i]].w)
            {
                reject = false;
                break;
            }
        }

        if(reject)
            return Reject;

        reject = true;
        for(unsigned int i = 0; i < count; i++)
        {
            if(-transformed_vertexes[vertex_indexes[i]].y < transformed_vertexes[vertex_indexes[i]].w)
            {
                reject = false;
                break;
            }
        }

        if(reject)
            return Reject;


        for(unsigned int i = 0; i < count; i++)
        {
            if(transformed_vertexes[vertex_indexes[i]].y > transformed_vertexes[vertex_indexes[i]].w)
            {
                clip |= Y_W_Top;
                break;
            }
        }

        for(unsigned int i = 0; i < count; i++)
        {
            if(-transformed_vertexes[vertex_indexes[i]].y > transformed_vertexes[vertex_indexes[i]].w)
            {
                clip |= Y_W_Bottom;
                break;
            }
        }

        return clip;
    }

    void RenderDevice::ClipPolygon(unsigned int clip_planes, unsigned int* in, unsigned int* out)
    {

    }
};
