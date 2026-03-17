#include "Config.h"
#include <stack>
#include "bspmodel.h"

namespace P3D
{
    Stack<unsigned int> BspModel::stack;
    List<unsigned int> BspModel::node_list;
    const VisData* BspModel::vis_data = nullptr;

    constexpr unsigned int NO_PVS_NODE = -1;

    void BspModel::Sort(const V3<fp>& p, const AABB<fp>& frustrum, List<const BspModelTriangle *> &out, bool backface_cull, bool check_pvs) const
    {
        out.Clear();
        node_list.Clear();

        unsigned int pvs_node = NO_PVS_NODE;

        if(check_pvs)
            pvs_node = GetLeafNodeId(p);

        SortBackToFront(p, frustrum, pvs_node);
        OutputTris(out, backface_cull);
    }


    unsigned int BspModel::GetLeafNodeId(const V3<fp>& p) const
    {
        unsigned int nid = 0;

        while(true)
        {
            const BspModelNode* n = GetNode(nid);

            if(Distance(n->plane, p) >= 0)
            {
                if(n->front_node)
                    nid = n->front_node;
                else
                    return (nid * 2);
            }
            else
            {
                if(n->back_node)
                    nid = n->back_node;
                else
                    return (nid * 2) + 1;
            }
        }
    }

    constexpr unsigned int BACK_BIT = 1u << 31;
    constexpr unsigned int POST_BIT = 1u << 30;

    constexpr unsigned int NODE_MASK = ~(BACK_BIT | POST_BIT);


    void BspModel::SortBackToFront(const V3<fp>& p, const AABB<fp>& frustrum, const unsigned int pvs_node) const
    {
        stack.Push(0);

        while(!stack.Empty())
        {
            const unsigned int item = stack.Pop();

            const BspModelNode* n = GetNode(item & NODE_MASK);

            if (item & POST_BIT)
            {
                if(pvs_node != NO_PVS_NODE && !CheckPvs(pvs_node, item & NODE_MASK))
                    continue;

                if (!frustrum.Intersect(n->node_bb))
                    continue;

                if (item & BACK_BIT)
                    node_list.Add(item & NODE_MASK);
                else
                    node_list.Add((item & NODE_MASK) | BACK_BIT);
            }
            else
            {
                if (!frustrum.Intersect(n->child_bb))
                    continue;

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

    void BspModel::OutputTris(List<const BspModelTriangle *> &out, const bool backface_cull) const
    {
        for(unsigned int i = 0; i < node_list.Size(); i++)
        {
            const unsigned int node = node_list.At(i);

            const BspModelNode* n = GetNode(node & NODE_MASK);

            const TriIndexList* front = (node & BACK_BIT) ? &n->back_tris : &n->front_tris;

            for(unsigned int n = 0; n < front->count; n++)
            {
                const BspModelTriangle* tri = GetTriangle(front->offset + n);

#ifdef STORE_PVS
                *((unsigned int*)&tri->color) = node & NODE_MASK; // Store the node ID in the color field for debugging;
#endif
                out.Add(tri);
            }

            if(!backface_cull)
            {
                const TriIndexList* back = (node & BACK_BIT) ? &n->front_tris : &n->back_tris;

                for(unsigned int n = 0; n < back->count; n++)
                {
                    const BspModelTriangle* tri = GetTriangle(back->offset + n);

#ifdef STORE_PVS
                    *((unsigned int*)&tri->color) = node & NODE_MASK; // Store the node ID in the color field for debugging;
#endif
                    out.Add(tri);
                }
            }
        }
    }

    bool BspModel::CheckPvs(unsigned int src_node, unsigned int dst_node) const
    {
        if(vis_data)
            return vis_data->CheckPvs(src_node, dst_node);

        return true;
    }
}
