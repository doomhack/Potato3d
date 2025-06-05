#include "Config.h"
#include <stack>
#include "bspmodel.h"

namespace P3D
{
    BspContext BspModel::context;

    void BspModel::Sort(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle *> &out, bool backface_cull) const
    {
        out.clear();

        context.point = p;
        context.frustrum = frustrum;
        context.output = &out;
        context.backface_cull = backface_cull;

        SortBackToFrontRecursive(this->GetNode(0));
    }

    void BspModel::SortBackToFrontRecursive(const BspModelNode* n) const
    {
        if(!context.frustrum.Intersect(n->child_bb))
            return;

        //if (n->plane.DistanceToPoint(context.point) < 0)
        if (Distance(n->plane, context.point) < 0)
        {
            if(n->front_node)
                SortBackToFrontRecursive(GetNode(n->front_node));

            if(context.frustrum.Intersect(n->node_bb))
            {
                for(int i = 0; i < n->front_tris.count; i++)
                {
                    const BspModelTriangle* t = GetTriangle(i + n->front_tris.offset);

                    if(context.frustrum.Intersect(t->tri_bb))
                        context.output->push_back(t);
                }

                if(!context.backface_cull)
                {
                    for(int i = 0; i < n->back_tris.count; i++)
                    {
                        const BspModelTriangle* t = GetTriangle(i + n->back_tris.offset);

                        if(context.frustrum.Intersect(t->tri_bb))
                            context.output->push_back(t);
                    }
                }
            }

            if(n->back_node)
                SortBackToFrontRecursive(GetNode(n->back_node));
        }
        else
        {
            if(n->back_node)
                SortBackToFrontRecursive(GetNode(n->back_node));

            if(context.frustrum.Intersect(n->node_bb))
            {
                for(int i = 0; i < n->back_tris.count; i++)
                {
                    const BspModelTriangle* t = GetTriangle(i + n->back_tris.offset);

                    if(context.frustrum.Intersect(t->tri_bb))
                        context.output->push_back(t);
                }

                if(!context.backface_cull)
                {
                    for(int i = 0; i < n->front_tris.count; i++)
                    {
                        const BspModelTriangle* t = GetTriangle(i + n->front_tris.offset);

                        if(context.frustrum.Intersect(t->tri_bb))
                            context.output->push_back(t);
                    }
                }
            }

            if(n->front_node)
                SortBackToFrontRecursive(GetNode(n->front_node));
        }
    }
}
