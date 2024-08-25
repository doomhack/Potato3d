#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "Config.h"

namespace P3D
{
    class RenderTarget
    {
    public:
        explicit RenderTarget(unsigned int width, unsigned int height, pixel* buffer = nullptr, unsigned int y_pitch = 0)
        {
            AttachColorBuffer(width, height, buffer, y_pitch);
        }

        ~RenderTarget()
        {
            RemoveColorBuffer();
            RemoveZBuffer();
        }

        bool AttachColorBuffer(unsigned int width, unsigned int height, pixel* buffer = nullptr, unsigned int y_pitch = 0)
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

        bool AttachZBuffer(z_val* buffer = nullptr, unsigned int y_pitch = 0)
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

        void RemoveZBuffer()
        {
            if(owned_z_buffer)
                delete[] z_buffer;

            z_buffer = nullptr;
            z_buffer_y_pitch = 0;
        }

        unsigned int GetWidth() const               {return width;}
        unsigned int GetHeight() const              {return height;}
        unsigned int GetColorBufferYPitch() const   {return color_buffer_y_pitch;}
        pixel* GetColorBuffer() const               {return color_buffer;}

        z_val* GetZBuffer() const                   {return z_buffer;}
        unsigned int GetZBufferYPitch() const       {return z_buffer_y_pitch;}
    private:

        void RemoveColorBuffer()
        {
            if(owned_color_buffer)
                delete[] color_buffer;

            color_buffer = nullptr;
            owned_color_buffer = 0;
        }

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
