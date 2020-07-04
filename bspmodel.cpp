#include "bspmodel.h"

namespace P3D
{
    void BspModel::SortBackToFront(const V3<fp>& p, const AABB& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const
    {
        SortBackToFrontRecursive(p, frustrum ,this->GetNode(0), out, backface_cull);
    }


    void BspModel::SortBackToFrontRecursive(const V3<fp>& p, const AABB& frustrum, const BspModelNode* n, std::vector<const BspModelTriangle*>& out, bool backface_cull) const
    {
        if(!frustrum.Intersect(n->child_bb))
            return;

        if (Distance(n->plane, p) < 0)
        {
            if(n->front_node)
                SortBackToFrontRecursive(p, frustrum, GetNode(n->front_node), out, backface_cull);

            if(frustrum.Intersect(n->node_bb))
            {
                for(int i = 0; i < n->front_tris.count; i++)
                {
                    out.push_back(GetTriangle(i + n->front_tris.offset));
                }

                if(!backface_cull)
                {
                    for(int i = 0; i < n->back_tris.count; i++)
                    {
                        out.push_back(GetTriangle(i + n->back_tris.offset));
                    }
                }
            }

            if(n->back_node)
                SortBackToFrontRecursive(p, frustrum, GetNode(n->back_node), out, backface_cull);
        }
        else
        {
            if(n->back_node)
                SortBackToFrontRecursive(p, frustrum, GetNode(n->back_node), out, backface_cull);

            if(frustrum.Intersect(n->node_bb))
            {
                for(int i = 0; i < n->back_tris.count; i++)
                {
                    out.push_back(GetTriangle(i + n->back_tris.offset));
                }

                if(!backface_cull)
                {
                    for(int i = 0; i < n->front_tris.count; i++)
                    {
                        out.push_back(GetTriangle(i + n->front_tris.offset));
                    }
                }

            }

            if(n->front_node)
                SortBackToFrontRecursive(p, frustrum, GetNode(n->front_node), out, backface_cull);
        }
    }
}