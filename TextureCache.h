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

        virtual void AddTexture(const pixel* texture, signed char importance = 0) {}
        virtual void RemoveTexture(const pixel* texture) {}

        virtual const pixel* GetTexture(const pixel* texture)
        {
            return texture;
        }

        virtual void ClearTextureCache() {}
    };

};

#endif // TEXTURECACHE_H
