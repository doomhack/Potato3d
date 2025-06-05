#include "Config.h"
#include <stack>
#include "bspmodel.h"

namespace P3D
{
    BspContext BspModel::context;

    void BspModel::Sort(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const
    {
        out.clear();

        context.node_list.clear();

        context.point = p;
        context.frustrum = frustrum;
        context.output = &out;
        context.backface_cull = backface_cull;

        SortBackToFrontRecursive(0);
        OutputTris();
    }

    void BspModel::SortBackToFrontRecursive(const unsigned int node) const
    {
        const BspModelNode* n = GetNode(node);

        if(!context.frustrum.Intersect(n->child_bb))
            return;

        if (Distance(n->plane, context.point) < 0)
        {
            if(n->front_node)
                SortBackToFrontRecursive(n->front_node);

            if(context.frustrum.Intersect(n->node_bb))
                context.node_list.push_back(node);

            if(n->back_node)
                SortBackToFrontRecursive(n->back_node);
        }
        else
        {
            if(n->back_node)
                SortBackToFrontRecursive(n->back_node);

            if(context.frustrum.Intersect(n->node_bb))
                context.node_list.push_back(-node);

            if(n->front_node)
                SortBackToFrontRecursive(n->front_node);
        }
    }

    void BspModel::OutputTris() const
    {
        for(const int node : context.node_list)
        {
            const TriIndexList front = node >= 0 ? GetNode(node)->front_tris : GetNode(-node)->back_tris;
            const TriIndexList back = node >= 0 ? GetNode(node)->back_tris : GetNode(-node)->front_tris;

            for(unsigned int i = 0; i < front.count; i++)
            {
                const BspModelTriangle* tri = GetTriangle(front.offset + i);

                if(context.frustrum.Intersect(tri->tri_bb))
                    context.output->push_back(tri);
            }

            if(!context.backface_cull)
            {
                for(unsigned int i = 0; i < back.count; i++)
                {
                    const BspModelTriangle* tri = GetTriangle(back.offset + i);

                    if(context.frustrum.Intersect(tri->tri_bb))
                        context.output->push_back(tri);
                }
            }
        }
    }
}
