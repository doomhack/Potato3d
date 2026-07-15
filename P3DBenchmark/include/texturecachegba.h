#ifndef TEXTURECACHEGBA_H
#define TEXTURECACHEGBA_H

#include "../../TextureCache.h"

class TextureCacheGBA final : public P3D::TextureCacheBase
{
public:
    TextureCacheGBA();
    virtual ~TextureCacheGBA();

    void AddTexture(const P3D::pixel* texture, const signed char importance = 0);
    void RemoveTexture(const P3D::pixel* texture);
    const P3D::pixel* GetTexture(const P3D::pixel* texture) const;
    void ClearTextureCache();

private:
#ifdef GBA
    P3D::pixel* const cacheMemory = (P3D::pixel*)0x6014000;
#else
    P3D::pixel cacheMemory[16384];
#endif    
    static constexpr unsigned int slotCount = (16384 / P3D::TEX_SIZE_BYTES);

    const P3D::pixel* textureSlots[slotCount] = { nullptr };
};

#endif // TEXTURECACHEGBA_H
