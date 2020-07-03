#ifndef BSP3D_H
#define BSP3D_H

#include <QtCore>

#include <vector>

#include "../rtypes.h"
#include "../bspmodel.h"

namespace P3D
{
    class BspTriangle
    {
    public:
        Triangle3d* tri;
        const Texture* texture;
        pixel color;
    };

    class BspNode
    {
    public:
        BspPlane plane; //Plane that this node splits on.
        std::vector<BspTriangle*> back_tris; //Back facing triangles that lie on this plane.
        std::vector<BspTriangle*> front_tris; //Triangles that lie on this plane.
        BspNode* front = nullptr; //Front children.
        BspNode* back = nullptr; //Back children.
        AABB node_bb; //AABB of the triangles in this node.
        AABB child_node_bb; //AABB of this node + children.
    };

    class BspTree
    {
    public:
        BspTree() {};
        BspNode* root = nullptr;

        void SortBackToFront(const V3<fp>& p, const AABB &frustrum, std::vector<BspTriangle*>& out, bool backface_cull = true) const;

        bool SaveBspTree(QByteArray *bytes);

    private:
        void SortBackToFrontRecursive(const V3<fp>& p, const AABB &frustrum, const BspNode* n, std::vector<BspTriangle*>& out, bool backface_cull) const;

        void TraverseNodesRecursive(const BspNode* n, QList<const BspNode *> &nodeList) const;
    };

    class Bsp3d
    {
    public:
        Bsp3d() {};

        BspTree* BuildBspTree(const Model3d* model);

        static fp Distance(const BspPlane& plane, const V3<fp> &pos);

    private:
        BspNode* BuildTreeRecursive(std::vector<BspTriangle*>& triangles);



        void SeperateTriangles(BspPlane& plane, std::vector<BspTriangle*>& triangles, std::vector<BspTriangle*>& front_tris, std::vector<BspTriangle*>& back_tris, std::vector<BspTriangle*>& plane_tris_front, std::vector<BspTriangle*>& plane_tris_back);

        BspPlane CheckPlane(std::vector<BspTriangle*>& triangles, unsigned int index, int& front, int& back, int& onplane);

        BspPlane CalculatePlane(const Triangle3d* triangle);


        int Sign(fp i);

        Vertex3d LerpVertex(Vertex3d& out, const Vertex3d &vx1, const Vertex3d &vx2, fp frac);

        static constexpr int SplitType(int a, int b, int c)
        {
            return (a+1)*16 + (b+1)*4 + (c+1);
        }

        static fp Relation(fp a, fp b)
        {
            return pAbs(a) / (pAbs(a) + pAbs(b));
        }

        const fp epsilon = fp(0.01f);

    };
}
#endif // BSP3D_H
