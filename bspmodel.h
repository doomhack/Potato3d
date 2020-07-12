#ifndef BSPMODEL_H
#define BSPMODEL_H

#include "rtypes.h"

namespace P3D
{
    typedef struct BspModelHeader
    {
        unsigned int texture_count;
        unsigned int texture_offset; //Offset in bytes from BspModel*

        unsigned int triangle_count;
        unsigned int triangle_offset; //Bytes from BspModel*

        unsigned int node_count;
        unsigned int node_offset;

        unsigned int texture_pixels_offset; //Bytes from BspModel*

    } BspModelHeader;

    typedef struct BspModelTriangle
    {
    public:
        Triangle3d tri;
        int texture;
        pixel color;
    } BspModelTriangle;

    typedef struct TriIndexList
    {
        unsigned short offset;
        unsigned short count;
    } TriIndexList;

    typedef struct BspNodeTexture
    {
        unsigned int texture_pixels_offset; //Pixels
        unsigned short width;
        unsigned short height;
        unsigned short u_mask;
        unsigned short v_mask;
        unsigned short v_shift;
        unsigned short alpha;
    } BspNodeTexture;


    typedef struct BspModelNode
    {
        BspPlane plane;
        AABB node_bb;
        AABB child_bb;
        unsigned int front_node;
        unsigned int back_node;
        TriIndexList front_tris;
        TriIndexList back_tris;
    } BspModelNode;

    class BspModel
    {
    public:
        BspModelHeader header;

        void SortFrontToBack(const V3<fp>& p, const AABB& frustrum, std::vector<const BspModelTriangle*>& out, bool backface_cull) const;

        const BspNodeTexture* GetTexture(unsigned int n) const
        {
            return &((const BspNodeTexture*)(GetBasePtr() + header.texture_offset))[n];
        }

        const pixel* GetTexturePixels(unsigned int n) const
        {
            return &((const pixel*)(GetBasePtr() + header.texture_pixels_offset))[n];
        }

    private:

        void SortFrontToBackRecursive(const V3<fp>& p, const AABB& frustrum, const BspModelNode* n, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;

        const unsigned char* GetBasePtr() const
        {
            return (const unsigned char*)&header;
        }

        fp Distance(const BspPlane& plane, const V3<fp>& pos) const
        {
            fp dot = plane.normal.DotProduct(pos);

            return dot - plane.plane;
        }

        const BspModelTriangle* GetTriangle(unsigned int n) const
        {
            return &((const BspModelTriangle*)(GetBasePtr() + header.triangle_offset))[n];
        }

        const BspModelNode* GetNode(unsigned int n) const
        {
            return &((const BspModelNode*)(GetBasePtr() + header.node_offset))[n];
        }
    };
}
#endif // BSPMODEL_H
