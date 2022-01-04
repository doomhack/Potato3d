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

        unsigned int texture_palette_offset; //Bytes from BspModel*

    } BspModelHeader;

    typedef struct BspModelTriangle
    {
    public:
        Triangle3d tri;
        int texture;
        pixel color;
        AABB tri_bb;
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

        void SortBackToFront(const V3<fp>& p, const AABB& frustrum, std::vector<const BspModelTriangle*>& out, bool backface_cull) const;


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

        const unsigned int GetColorMapColor(unsigned int n) const
        {
            return ((const unsigned int*)(GetBasePtr() + header.texture_palette_offset))[n];
        }

    private:

        void SortFrontToBackRecursive(const V3<fp>& p, const AABB& frustrum, const BspModelNode* n, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;

        void SortBackToFrontRecursive(const V3<fp>& p, const AABB& frustrum, const BspModelNode* n, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;


        bool TriAABBIntersect(const BspModelTriangle* tri, const AABB& aabb) const
        {
            AABB polybb;
            polybb.AddPoint(tri->tri.verts[0].pos);
            polybb.AddPoint(tri->tri.verts[1].pos);
            polybb.AddPoint(tri->tri.verts[2].pos);

            return (aabb.Intersect(polybb));
        }

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
