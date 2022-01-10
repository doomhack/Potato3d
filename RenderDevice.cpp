#include "RenderDevice.h"

namespace P3D
{
    RenderDevice::RenderDevice()
    {
        texture_cache = new TextureCacheBase();

        current_material = new Material();

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

    void RenderDevice::SetRenderFlags(RenderFlagsBase* flags_ptr)
    {
        if(render_flags_base != nullptr)
            delete render_flags_base;

        triangle_render = flags_ptr->GetRender();
        render_flags_base = flags_ptr;
    }

    //Matrix
    void RenderDevice::SetPerspective(fp vertical_fov, fp aspect_ratio, fp z_near, fp z_far)
    {
        projection_matrix.setToIdentity();
        projection_matrix.perspective(vertical_fov, aspect_ratio, z_near, z_far);

        z_planes.z_near = z_near;
        z_planes.z_far = z_far;
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
            pixel* p = &render_target->GetColorBuffer()[y * render_target->GetColorBufferYPitch()];

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
            z_val* z = &render_target->GetZBuffer()[y * render_target->GetZBufferYPitch()];

            for(unsigned int x = 0; x < render_target->GetWidth(); x++)
            {
                z[x] = depth;
            }
        }
    }

    //Begin/End Frame.
    void RenderDevice::BeginFrame()
    {
        UpdateTransformMatrix();
    }

    void RenderDevice::EndFrame()
    {

    }

    //Texture cache
    void RenderDevice::SetTextureCache(TextureCacheBase* cache)
    {
        if(texture_cache)
            delete texture_cache;

        texture_cache = cache;
    }

    void RenderDevice::SetMaterial(const Material& material, signed char importance)
    {
        current_material = &material;

        if(material.type == Material::Texture)
        {
            texture_cache->AddTexture(material.pixels, importance);
        }
    }

    //Draw Objects.
    void RenderDevice::DrawTriangle(const V3<fp> vertexes[3], const V2<fp> uvs[3])
    {
        TransformVertexes(vertexes, 3);

        int indexes[3] = {0,1,2};

        DrawTriangle(indexes, uvs);
    }

    void RenderDevice::TransformVertexes(const V3<fp>* vertexes, const unsigned int count)
    {
        if(transformed_vertexes_buffer_count < count)
        {
            delete[] transformed_vertexes;
            transformed_vertexes = new V4<fp>[count]; //+5 is extra vertexs created by clipping.
            transformed_vertexes_buffer_count = count;
        }

        for(unsigned int i = 0; i < count; i++)
        {
            transformed_vertexes[i] = transform_matrix * vertexes[i];
        }
    }

    void RenderDevice::DrawTriangle(const int indexes[3], const V2<fp> uvs[3])
    {
        TransformedTriangle tri;

        tri.verts->pos = transformed_vertexes[indexes[0]];
        tri.verts->pos = transformed_vertexes[indexes[1]];
        tri.verts->pos = transformed_vertexes[indexes[2]];

        if(current_material->type == Material::Texture)
        {
            tri.verts[0].uv =  uvs[0];
            tri.verts[1].uv =  uvs[1];
            tri.verts[2].uv =  uvs[2];
        }

        triangle_render->DrawTriangle(tri, *current_material, *texture_cache, viewport, z_planes);
    }
};
