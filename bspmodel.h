#ifndef BSPMODEL_H
#define BSPMODEL_H

#include <vector>
#include <cassert>
#include "BspModelDefs.h"

namespace P3D
{
    //Bare minimum monday stack.
    template <class T> class Stack
    {
    public:
        Stack(unsigned int size = 256)  { pos = mem = (T*)new char[sizeof(T) * size]; this->size = size; }
        ~Stack()                        { delete[] mem; }
        void Push(T item)               { assert(pos < (mem + size)); *pos = item; pos++; }
        T Pop()                         { assert(pos > mem); pos--; return *pos; }
        bool Empty() const              { return pos == mem; }
        void Clear()                    { pos = mem; }

    private:
        T* mem; T* pos;
        unsigned int size;
    };

    //Fuck it friday list.
    template <class T> class List
    {
    public:
        List(unsigned int size = 10000) { pos = mem = (T*)new char[sizeof(T) * size]; this->size = size; }
        ~List()                        { delete[] mem; }
        void Add(T item)               { assert(pos < (mem + size)); *pos = item; pos++; }
        T At(unsigned int index) const { assert((index >= 0) && (index < size)); return mem[index]; }
        unsigned int Size() const      { return pos - mem; }
        void Clear()                   { pos = mem; }

    private:
        T* mem; T* pos;
        unsigned int size;
    };


    class VisData
    {
    public:
        VisDataHeader header;

        bool CheckPvs(const unsigned int src_node, const unsigned int dst_node) const
        {
            const unsigned char* pvs_data = GetNodeVisData(src_node);

            if(!pvs_data) //No vis data for node.
                return true;

            return pvs_data[dst_node >> 3] & (1 << (dst_node & 7));
        }

    private:
        const unsigned char* GetBasePtr() const
        {
            return (const unsigned char*)&header;
        }

        const unsigned char* GetNodeVisData(const unsigned int node) const
        {
            const unsigned int offset = ((const unsigned int*)(GetBasePtr() + header.leaf_index_offset))[node];

            if(offset)
                return GetBasePtr() + offset;

            return nullptr;
        }
    };

    class BspModel
    {
    public:
        BspModelHeader header;

        void Sort(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull, bool check_pvs) const;

        unsigned int GetLeafNodeId(const V3<fp>& p) const;

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

        const void SetVisData(const VisData* data) const
        {
            vis_data = data;
        }

        const AABB<P3D::fp>& GetModelAABB() const
        {
            const BspModelNode* root_node = GetNode(0);

            return root_node->child_bb;
        }

    private:

        static Stack<unsigned int> stack;
        static List<unsigned int> node_list;
        static const VisData* vis_data;

        void OutputTris(const AABB<P3D::fp> &frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;
        void SortBackToFront(const V3<P3D::fp> &p, const AABB<fp>& frustrum, unsigned int pvs_node) const;

        bool CheckPvs(unsigned int src_node, unsigned int dst_node) const;

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

        unsigned int NodeIndex(const BspModelNode* node) const
        {
            return node - GetNode(0);
        }
    };
}
#endif // BSPMODEL_H
