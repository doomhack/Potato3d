#ifndef BSPMODEL_H
#define BSPMODEL_H

#include <vector>
#include "BspModelDefs.h"

namespace P3D
{
    class BspModel
    {
    public:
        BspModelHeader header;

        void SortFrontToBack(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;
        void SortBackToFront(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;

        const BspNodeTexture* GetTexture(int n) const
        {
            if(n == -1)
                return nullptr;

            return &((const BspNodeTexture*)(GetBasePtr() + header.texture_offset))[n];
        }

        const pixel* GetTexturePixels(unsigned int n) const
        {
            return &((const pixel*)(GetBasePtr() + header.texture_pixels_offset))[n];
        }

        unsigned int GetColorMapColor(unsigned int n) const
        {
            return GetColorMap()[n];
        }

        const unsigned int* GetColorMap() const
        {
            return ((const unsigned int*)(GetBasePtr() + header.texture_palette_offset));
        }

        const unsigned char* GetFogLightMap() const
        {
            return ((const unsigned char*)(GetBasePtr() + header.fog_lightmap_offset));
        }

    private:

        void SortBackToFrontRecursive(const V3<fp>& p, const AABB<fp>& frustrum, const BspModelNode* n, std::vector<const BspModelTriangle*>& out, bool backface_cull) const;
        void SortFrontToBackRecursive(const V3<fp>& p, const AABB<fp>& frustrum, const BspModelNode* n, std::vector<const BspModelTriangle*>& out, bool backface_cull) const;

        bool TriAABBIntersect(const BspModelTriangle* tri, const AABB<fp>& aabb) const
        {
            AABB<fp> polybb;
            polybb.AddPoint(tri->tri.verts[0].pos);
            polybb.AddPoint(tri->tri.verts[1].pos);
            polybb.AddPoint(tri->tri.verts[2].pos);

            return (aabb.Intersect(polybb));
        }

        const unsigned char* GetBasePtr() const
        {
            return (const unsigned char*)&header;
        }

        fp Distance(const Plane<fp>& plane, const V3<fp>& pos) const
        {
            fp dot = plane.Normal().DotProduct(pos);

            return dot - plane.Distance();
        }

        const BspModelTriangle* GetTriangle(unsigned int n) const
        {
            return &((const BspModelTriangle*)(GetBasePtr() + header.triangle_offset))[n];
        }

        const BspModelNode* GetNode(unsigned int n) const
        {
            return &((const BspModelNode*)(GetBasePtr() + header.node_offset))[n];
        }

        const unsigned int NodeIndex(const BspModelNode* node) const
        {
            return node - GetNode(0);
        }
    };
}
#endif // BSPMODEL_H
