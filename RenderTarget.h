#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "Config.h"

namespace P3D
{
    class RenderTarget
    {
    public:
        explicit RenderTarget(unsigned int width, unsigned int height, pixel* buffer = nullptr, unsigned int y_pitch = 0);
        ~RenderTarget();

        bool AttachColorBuffer(unsigned int width, unsigned int height, pixel* buffer = nullptr, unsigned int y_pitch = 0);
        bool AttachZBuffer(z_val* buffer = nullptr, unsigned int y_pitch = 0);

        void RemoveZBuffer();

        unsigned int GetWidth() const               {return width;}
        unsigned int GetHeight() const              {return height;}
        unsigned int GetColorBufferYPitch() const   {return color_buffer_y_pitch;}
        pixel* GetColorBuffer() const               {return color_buffer;}

        z_val* GetZBuffer() const                   {return z_buffer;}
        unsigned int GetZBufferYPitch() const       {return z_buffer_y_pitch;}
    private:

        void RemoveColorBuffer();

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
