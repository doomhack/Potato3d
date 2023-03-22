#ifndef TEXTURECACHE_H
#define TEXTURECACHE_H

#include "Config.h"

namespace P3D
{
    class TextureCacheBase
    {
    public:
        TextureCacheBase() = default;
        virtual ~TextureCacheBase() = default;

        virtual void AddTexture(const pixel* texture, const signed char importance = 0) = 0;
        virtual void RemoveTexture(const pixel* texture) = 0;
        virtual const pixel* GetTexture(const pixel* texture) const = 0;
        virtual void ClearTextureCache() = 0;
    };

    class TextureCacheDefault final : public TextureCacheBase
    {
    public:

        TextureCacheDefault() = default;
        ~TextureCacheDefault() = default;

        void AddTexture(const pixel* texture, const signed char importance = 0)
        {
            (void)texture;
            (void)importance;
        }

        void RemoveTexture(const pixel* texture)
        {
            (void)texture;
        }

        const pixel* GetTexture(const pixel* texture) const
        {
            return texture;
        }

        virtual void ClearTextureCache() {}
    };

};

#endif // TEXTURECACHE_H
