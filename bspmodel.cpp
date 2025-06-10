#include "Config.h"
#include <stack>
#include "bspmodel.h"

namespace P3D
{
    Stack<unsigned int> BspModel::stack;
    List<unsigned int> BspModel::node_list;

    void BspModel::Sort(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const
    {
        out.clear();
        node_list.Clear();

        SortBackToFront(p, frustrum);
        OutputTris(frustrum, out, backface_cull);
    }

    constexpr unsigned int BACK_BIT = 1 << 31;
    constexpr unsigned int POST_BIT = 1 << 30;

    constexpr unsigned int NODE_MASK = ~(BACK_BIT | POST_BIT);

    void BspModel::SortBackToFront(const V3<fp>& p, const AABB<fp>& frustrum) const
    {
        stack.Push(0);

        while(!stack.Empty())
        {
            const unsigned int item = stack.Pop();

            const BspModelNode* n = GetNode(item & NODE_MASK);

            if (!frustrum.Intersect(n->child_bb))
                continue;

            if (item & POST_BIT)
            {
                if (frustrum.Intersect(n->node_bb))
                {
                    if (item & BACK_BIT)
                        node_list.Add(item & NODE_MASK);
                    else
                        node_list.Add((item & NODE_MASK) | BACK_BIT);
                }
            }
            else
            {
                if(Distance(n->plane, p) >= 0)
                {
                    if (n->front_node)
                        stack.Push(n->front_node);

                    stack.Push(item | POST_BIT);

                    if (n->back_node)
                        stack.Push(n->back_node);
                }
                else
                {
                    if (n->back_node)
                        stack.Push(n->back_node);

                    stack.Push(item | POST_BIT | BACK_BIT);

                    if (n->front_node)
                        stack.Push(n->front_node);
                }
            }
        }
    }

    void BspModel::OutputTris(const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const
    {
        for(unsigned int i = 0; i < node_list.Size(); i++)
        {
            const unsigned int node = node_list.At(i);

            const BspModelNode* n = GetNode(node & NODE_MASK);

            const TriIndexList* front = (node & BACK_BIT) ? &n->back_tris : &n->front_tris;

            for(unsigned int i = 0; i < front->count; i++)
            {
                const BspModelTriangle* tri = GetTriangle(front->offset + i);

                if(frustrum.Intersect(tri->tri_bb))
                    out.push_back(tri);
            }

            if(!backface_cull)
            {
                const TriIndexList* back = (node & BACK_BIT) ? &n->front_tris : &n->back_tris;

                for(unsigned int i = 0; i < back->count; i++)
                {
                    const BspModelTriangle* tri = GetTriangle(back->offset + i);

                    if(frustrum.Intersect(tri->tri_bb))
                        out.push_back(tri);
                }
            }
        }
    }
}
