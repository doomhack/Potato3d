#ifndef BSPMODEL_H
#define BSPMODEL_H

#include <vector>
#include <stack>
#include "BspModelDefs.h"

namespace P3D
{
    //Bare minimum monday stack.
    template <class T> class Stack
    {
    public:
        Stack(unsigned int size = 2048) { pos = mem = (T*)new char[sizeof(T) * size]; }
        ~Stack()                        { delete[] mem; }
        void Push(T item)               { *pos = item; pos++; }
        T Pop()                         { pos--; return *pos; }
        bool Empty() const              { return pos == mem; }
        void Clear()                    { pos = mem; }

    private:
        T* mem; T* pos;
    };

    //Fuck it friday list.
    template <class T> class List
    {
    public:
        List(unsigned int size = 2048) { pos = mem = (T*)new char[sizeof(T) * size]; }
        ~List()                        { delete[] mem; }
        void Add(T item)               { *pos = item; pos++; }
        T At(unsigned int index) const { return mem[index]; }
        unsigned int Size() const      { return pos - mem; }
        void Clear()                   { pos = mem; }

    private:
        T* mem; T* pos;
    };


    class BspModel
    {
    public:
        BspModelHeader header;

        void Sort(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;

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

        static Stack<unsigned int> stack;
        static List<unsigned int> node_list;

        void OutputTris(const AABB<P3D::fp> &frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const;
        void SortBackToFront(const V3<P3D::fp> &p, const AABB<fp>& frustrum) const;

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
