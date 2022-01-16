#ifndef TEXTURECACHE_H
#define TEXTURECACHE_H

#include "common.h"

namespace P3D
{

    class TextureCacheBase
    {
    public:
        TextureCacheBase() = default;
        virtual ~TextureCacheBase() = default;

        virtual void AddTexture(const pixel* texture, signed char importance = 0)
        {
            (void)texture;
            (void)importance;
        }

        virtual void RemoveTexture(const pixel* texture)
        {
            (void)texture;
        }

        virtual const pixel* GetTexture(const pixel* texture) const
        {
            return texture;
        }

        virtual void ClearTextureCache() {}
    };

};

#endif // TEXTURECACHE_H
