#include "RenderTarget.h"

namespace P3D
{
    RenderTarget::RenderTarget(unsigned int width, unsigned int height, pixel* buffer, unsigned int y_pitch)
    {
        AttachColorBuffer(width, height, buffer, y_pitch);
    }

    RenderTarget::~RenderTarget()
    {
        RemoveColorBuffer();
        RemoveZBuffer();
    }

    bool RenderTarget::AttachColorBuffer(unsigned int width, unsigned int height, pixel* buffer, unsigned int y_pitch)
    {
        RemoveColorBuffer();

        if(!width || !height)
            return false;

        if(y_pitch == 0)
            y_pitch = width;

        if(y_pitch < width)
            return false;

        color_buffer_y_pitch = y_pitch;
        this->width = width;
        this->height = height;

        if(buffer)
        {
            color_buffer = buffer;
        }
        else
        {
            color_buffer = new pixel[width * height];
            owned_color_buffer = true;
        }

        return true;
    }

    bool RenderTarget::AttachZBuffer(z_val *buffer, unsigned int y_pitch)
    {
        RemoveZBuffer();

        if(!width || !height)
            return false;

        if(y_pitch == 0)
            y_pitch = width;

        if(y_pitch < width)
            return false;

        if(buffer)
        {
            z_buffer = buffer;
            z_buffer_y_pitch = y_pitch;
        }
        else
        {
            z_buffer = new z_val[width * height];
            z_buffer_y_pitch = width;
            owned_z_buffer = true;
        }

        return true;
    }

    void RenderTarget::RemoveColorBuffer()
    {
        if(owned_color_buffer)
            delete[] color_buffer;

        color_buffer = nullptr;
        owned_color_buffer = 0;
    }

    void RenderTarget::RemoveZBuffer()
    {
        if(owned_z_buffer)
            delete[] z_buffer;

        z_buffer = nullptr;
        z_buffer_y_pitch = 0;
    }
};
