#include "Config.h"
#include <stack>
#include "bspmodel.h"

namespace P3D
{
    BspContext context;


    void BspModel::Sort(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle*>& out, const bool backface_cull, const SortOrder order) const
    {
        context.point = p;
        context.frustrum = frustrum;
        context.output = &out;
        context.backface_cull = backface_cull;
        context.order = order;

        SortRecursive(this->GetNode(0));
    }

    void P3D::BspModel::VisitNode(const BspModelNode* n) const
    {
        if (context.frustrum.Intersect(n->node_bb))
        {
            for(int i = 0; i < n->front_tris.count; i++)
            {
                const BspModelTriangle* t = GetTriangle(i + n->front_tris.offset);
                if (context.frustrum.Intersect(t->tri_bb))
                    context.output->push_back(t);
            }

            if (!context.backface_cull)
            {
                for(int i = 0; i < n->back_tris.count; i++)
                {
                    const BspModelTriangle* t = GetTriangle(i + n->back_tris.offset);
                    if (context.frustrum.Intersect(t->tri_bb))
                        context.output->push_back(t);
                }
            }
        }
    }

    void BspModel::SortRecursive(const BspModelNode* n) const
    {
        if (!context.frustrum.Intersect(n->child_bb))
            return;

        const bool pointIsInFront = (Distance(n->plane, context.point) > 0);

        const unsigned int firstNodeIndex = ((context.order == SortOrder::FrontToBack) == pointIsInFront) ? n->front_node : n->back_node;
        const unsigned int secondNodeIndex = ((context.order == SortOrder::FrontToBack) == pointIsInFront) ? n->back_node : n->front_node;

        if (firstNodeIndex)
            SortRecursive(GetNode(firstNodeIndex));

        VisitNode(n);

        if (secondNodeIndex)
            SortRecursive(GetNode(secondNodeIndex));
    }

    void BspModel::SortFrontToBack(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle*>& out, const bool backface_cull) const
    {
        Sort(p, frustrum, out, backface_cull, SortOrder::FrontToBack);
    }

    void BspModel::SortBackToFront(const V3<fp>& p, const AABB<fp>& frustrum, std::vector<const BspModelTriangle*>& out, const bool backface_cull) const
    {
        Sort(p, frustrum, out, backface_cull, SortOrder::BackToFront);
    }
}
