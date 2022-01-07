#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "common.h"

namespace P3D
{
    class RenderTarget
    {
    public:
        explicit RenderTarget(unsigned int width, unsigned int height, pixel* buffer = nullptr, unsigned int y_pitch = 0);
        ~RenderTarget();

        bool AttachColorBuffer(unsigned int width, unsigned int height, pixel* buffer = nullptr, unsigned int y_pitch = 0);
        bool AttachZBuffer(z_val* buffer = nullptr, unsigned int y_pitch = 0);

        bool RemoveZBuffer();

        unsigned int GetWidth()     {return width;}
        unsigned int GetHeight()    {return height;}
        unsigned int GetColorBufferYPitch()    {return color_buffer_y_pitch;}
        pixel* GetColorBuffer()     {return color_buffer;}

        z_val* GetZBuffer()         {return z_buffer;}
        unsigned int GetZBufferYPitch() {return z_buffer_y_pitch;}
    private:

        bool RemoveColorBuffer();

        pixel* color_buffer = nullptr;
        unsigned int width = 0;
        unsigned int height = 0;
        unsigned int color_buffer_y_pitch = 0;

        z_val* z_buffer = nullptr;
        unsigned int z_buffer_y_pitch = 0;

        bool owned_color_buffer = false;
        bool owned_z_buffer = false;
    };
};
#endif // RENDERTARGET_H
