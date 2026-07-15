#include "../include/texturecachegba.h"

TextureCacheGBA::TextureCacheGBA()
{

}

TextureCacheGBA::~TextureCacheGBA()
{

}

void TextureCacheGBA::AddTexture(const P3D::pixel* texture, const signed char importance)
{
    for(unsigned int i = 0; i < slotCount; i++)
    {
        if(textureSlots[i] == texture)
            return;
    }

    for(unsigned int i = 0; i < slotCount; i++)
    {
        if(textureSlots[i] == nullptr)
        {
            textureSlots[i] = texture;
            P3D::FastCopy32(&cacheMemory[i * P3D::TEX_SIZE_PIXELS], texture, P3D::TEX_SIZE_PIXELS);
            return;
        }
    }

    unsigned int evictSlot = rand() & (slotCount - 1);

    textureSlots[evictSlot] = texture;
    P3D::FastCopy32(&cacheMemory[evictSlot * P3D::TEX_SIZE_PIXELS], texture, P3D::TEX_SIZE_PIXELS);
}

void TextureCacheGBA::RemoveTexture(const P3D::pixel* texture)
{
    for(unsigned int i = 0; i < slotCount; i++)
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
    for(unsigned int i = 0; i < slotCount; i++)
    {
        if(textureSlots[i] == texture)
            return &cacheMemory[i * P3D::TEX_SIZE_PIXELS];
    }

    return texture;
}

void TextureCacheGBA::ClearTextureCache()
{
    for(unsigned int i = 0; i < slotCount; i++)
    {
        textureSlots[i] = nullptr;
    }
}