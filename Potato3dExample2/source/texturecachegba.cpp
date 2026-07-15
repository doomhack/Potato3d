#include "../include/texturecachegba.h"
#include <cstdlib>

void TextureCacheGBA::AddTexture(const P3D::pixel* texture, const signed char importance)
{
    for(unsigned int i = 0; i < CACHE_SLOT_COUNT; i++)
    {
        if(textureSlots[i] == texture)
            return;
    }

    for(unsigned int i = 0; i < CACHE_SLOT_COUNT; i++)
    {
        if(textureSlots[i] == nullptr)
        {
            StoreTexture(i, texture);
            return;
        }
    }

    unsigned int evictSlot = rand() & (CACHE_SLOT_COUNT - 1);

    StoreTexture(evictSlot, texture);
}

void TextureCacheGBA::RemoveTexture(const P3D::pixel* texture)
{
    for(unsigned int i = 0; i < CACHE_SLOT_COUNT; i++)
    {
        if(textureSlots[i] == texture)
        {
            textureSlots[i] = nullptr;
            break;
        }
    }
}

const P3D::pixel* TextureCacheGBA::GetTexture(const P3D::pixel* texture) const
{
    for(unsigned int i = 0; i < CACHE_SLOT_COUNT; i++)
    {
        if(textureSlots[i] == texture)
            return &cacheMemory[i * P3D::TEX_SIZE_PIXELS];
    }

    return texture;
}

void TextureCacheGBA::ClearTextureCache()
{
    for(unsigned int i = 0; i < CACHE_SLOT_COUNT; i++)
    {
        textureSlots[i] = nullptr;
    }
}

void TextureCacheGBA::StoreTexture(unsigned int slot, const P3D::pixel* texture)
{
    textureSlots[slot] = texture;
    P3D::FastCopy32(&cacheMemory[slot * P3D::TEX_SIZE_PIXELS], texture, P3D::TEX_SIZE_PIXELS);
}