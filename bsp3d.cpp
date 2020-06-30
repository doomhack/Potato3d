#include "bsp3d.h"

//BSP tree building.
//Based on https://github.com/roever/BSP
//

namespace P3D
{

    BspTree* Bsp3d::BuildBspTree(const Model3d* model)
    {
        std::vector<BspTriangle*> triangles;

        for (auto it = model->mesh.begin() ; it != model->mesh.end(); ++it)
        {
            const Mesh3d* mesh = *it;

            for (auto it = mesh->tris.begin() ; it != mesh->tris.end(); ++it)
            {
                Triangle3d* triangle = *it;

                BspTriangle* bsptri = new BspTriangle();

                bsptri->tri = triangle;
                bsptri->color = mesh->color;
                bsptri->texture = mesh->texture;

                triangles.push_back(bsptri);
            }
        }

        BspTree* tree = new BspTree();

        tree->root = BuildTreeRecursive(triangles);

        return tree;
    }

    BspNode* Bsp3d::BuildTreeRecursive(std::vector<BspTriangle*>& triangles)
    {
        if(triangles.size() == 0)
            return nullptr;

        int front = 0, back = 0, on = 0;
        int best_index = 0;

        int best_score = 0; //Higher is worse!

        BspPlane best_plane = CheckPlane(triangles, 0, front, back, on);

        best_score = pAbs(back - front) + (back + front + on)*10;

        for(unsigned int i = 0; i < triangles.size(); i++)
        {
            BspPlane plane = CheckPlane(triangles, i, front, back, on);

            int score = pAbs(back - front) + (back + front + on)*10;

            if (score < best_score)
            {
                best_plane = plane;
                best_index = i;

                best_score = score;
            }
        }

        BspNode* node = new BspNode();

        node->plane = best_plane;
        std::vector<BspTriangle*> front_tris;
        std::vector<BspTriangle*> back_tris;

        SeperateTriangles(best_plane, triangles, front_tris, back_tris, node->front_tris, node->back_tris);

        for(unsigned int i = 0; i < node->front_tris.size(); i++)
        {
            node->node_bb.AddTriangle(*node->front_tris[i]->tri);
        }

        for(unsigned int i = 0; i < node->back_tris.size(); i++)
        {
            node->node_bb.AddTriangle(*node->back_tris[i]->tri);
        }

        node->child_node_bb.AddAABB(node->node_bb);

        node->back = BuildTreeRecursive(back_tris);
        node->front = BuildTreeRecursive(front_tris);

        if(node->back)
            node->child_node_bb.AddAABB(node->back->child_node_bb);

        if(node->front)
            node->child_node_bb.AddAABB(node->front->child_node_bb);


        return node;
    }

    void Bsp3d::SeperateTriangles(BspPlane& plane, std::vector<BspTriangle*>& triangles, std::vector<BspTriangle*>& front_tris, std::vector<BspTriangle*>& back_tris, std::vector<BspTriangle*>& plane_tris_front, std::vector<BspTriangle *> &plane_tris_back)
    {
        for(unsigned int i = 0; i < triangles.size(); i++)
        {
            fp dist[3];

            dist[0] = Distance(plane, triangles[i]->tri->verts[0].pos);
            dist[1] = Distance(plane, triangles[i]->tri->verts[1].pos);
            dist[2] = Distance(plane, triangles[i]->tri->verts[2].pos);

            int side[3];
            side[0] = Sign(dist[0]);
            side[1] = Sign(dist[1]);
            side[2] = Sign(dist[2]);

            BspTriangle split_tri;

            split_tri.color = triangles[i]->color;
            split_tri.texture = triangles[i]->texture;
            split_tri.tri = new Triangle3d;

            if(side[0] * side[1] == -1)
            {
                split_tri.tri->verts[0] = LerpVertex(split_tri.tri->verts[0], triangles[i]->tri->verts[0], triangles[i]->tri->verts[1], Relation(dist[0], dist[1]));
            }

            if (side[1] * side[2] == -1)
            {
                split_tri.tri->verts[1] = LerpVertex(split_tri.tri->verts[1], triangles[i]->tri->verts[1], triangles[i]->tri->verts[2], Relation(dist[1], dist[2]));
            }

            if (side[2] * side[0] == -1)
            {
                split_tri.tri->verts[2] = LerpVertex(split_tri.tri->verts[2], triangles[i]->tri->verts[2], triangles[i]->tri->verts[0], Relation(dist[2], dist[0]));
            }

            switch (SplitType(side[0], side[1], side[2]))
            {
                case SplitType(-1, -1, -1):
                case SplitType(-1, -1,  0):
                case SplitType(-1,  0, -1):
                case SplitType(-1,  0,  0):
                case SplitType( 0, -1, -1):
                case SplitType( 0, -1,  0):
                case SplitType( 0,  0, -1):
                    back_tris.push_back(triangles[i]);
                    break;

                case SplitType( 0,  0,  1):
                case SplitType( 0,  1,  0):
                case SplitType( 0,  1,  1):
                case SplitType( 1,  0,  0):
                case SplitType( 1,  0,  1):
                case SplitType( 1,  1,  0):
                case SplitType( 1,  1,  1):
                    front_tris.push_back(triangles[i]);
                    break;

                case SplitType( 0,  0,  0):
                {
                    //Figure out if triangle faces the same way as the BSP plane.
                    V3<fp> edge1 = triangles[i]->tri->verts[1].pos - triangles[i]->tri->verts[0].pos;
                    V3<fp> edge2 = triangles[i]->tri->verts[2].pos - triangles[i]->tri->verts[1].pos;

                    V3<fp> normal = edge1.CrossProductNormalised(edge2);

                    if(plane.normal.DotProduct(normal) < 0)
                        plane_tris_back.push_back(triangles[i]);
                    else
                        plane_tris_front.push_back(triangles[i]);
                }
                break;

                case SplitType( 1, -1,  0):
                {
                    BspTriangle *ft, *bt;
                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[1];
                    bt->tri->verts[1] = triangles[i]->tri->verts[2];
                    bt->tri->verts[2] = split_tri.tri->verts[0];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[2];
                    ft->tri->verts[1] = triangles[i]->tri->verts[0];
                    ft->tri->verts[2] = split_tri.tri->verts[0];

                    front_tris.push_back(ft);
                }
                break;

                case SplitType(-1,  0,  1):
                {
                    BspTriangle *ft, *bt;
                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[0];
                    bt->tri->verts[1] = triangles[i]->tri->verts[1];
                    bt->tri->verts[2] = split_tri.tri->verts[2];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[1];
                    ft->tri->verts[1] = triangles[i]->tri->verts[2];
                    ft->tri->verts[2] = split_tri.tri->verts[2];

                    front_tris.push_back(ft);
                }
                break;

                case SplitType( 0,  1, -1):
                {
                    BspTriangle *ft, *bt;
                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[2];
                    bt->tri->verts[1] = triangles[i]->tri->verts[0];
                    bt->tri->verts[2] = split_tri.tri->verts[1];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[0];
                    ft->tri->verts[1] = triangles[i]->tri->verts[1];
                    ft->tri->verts[2] = split_tri.tri->verts[1];

                    front_tris.push_back(ft);
                }
                break;

                case SplitType(-1,  1,  0):
                {
                    BspTriangle *ft, *bt;
                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[2];
                    bt->tri->verts[1] = triangles[i]->tri->verts[0];
                    bt->tri->verts[2] = split_tri.tri->verts[0];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[1];
                    ft->tri->verts[1] = triangles[i]->tri->verts[2];
                    ft->tri->verts[2] = split_tri.tri->verts[0];

                    front_tris.push_back(ft);
                }
                break;

                case SplitType( 1,  0, -1):
                {
                    BspTriangle *ft, *bt;
                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[1];
                    bt->tri->verts[1] = triangles[i]->tri->verts[2];
                    bt->tri->verts[2] = split_tri.tri->verts[2];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[0];
                    ft->tri->verts[1] = triangles[i]->tri->verts[1];
                    ft->tri->verts[2] = split_tri.tri->verts[2];

                    front_tris.push_back(ft);
                }
                break;

                case SplitType( 0, -1,  1):
                {
                    BspTriangle *ft, *bt;
                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[0];
                    bt->tri->verts[1] = triangles[i]->tri->verts[1];
                    bt->tri->verts[2] = split_tri.tri->verts[1];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[2];
                    ft->tri->verts[1] = triangles[i]->tri->verts[0];
                    ft->tri->verts[2] = split_tri.tri->verts[1];

                    front_tris.push_back(ft);
                }
                break;

                case SplitType( 1, -1, -1):
                {
                    BspTriangle *ft, *bt, *bt2;

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[0];
                    ft->tri->verts[1] = split_tri.tri->verts[0];
                    ft->tri->verts[2] = split_tri.tri->verts[2];

                    front_tris.push_back(ft);


                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[1];
                    bt->tri->verts[1] = split_tri.tri->verts[2];
                    bt->tri->verts[2] = split_tri.tri->verts[0];

                    back_tris.push_back(bt);

                    bt2 = new BspTriangle;

                    bt2->color = split_tri.color;
                    bt2->texture = split_tri.texture;

                    bt2->tri = new Triangle3d;
                    bt2->tri->verts[0] = triangles[i]->tri->verts[1];
                    bt2->tri->verts[1] = triangles[i]->tri->verts[2];
                    bt2->tri->verts[2] = split_tri.tri->verts[2];

                    back_tris.push_back(bt2);
                }
                break;

                case SplitType(-1,  1, -1):
                {
                    BspTriangle *ft, *bt, *bt2;

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[1];
                    ft->tri->verts[1] = split_tri.tri->verts[1];
                    ft->tri->verts[2] = split_tri.tri->verts[0];

                    front_tris.push_back(ft);


                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[2];
                    bt->tri->verts[1] = split_tri.tri->verts[0];
                    bt->tri->verts[2] = split_tri.tri->verts[1];


                    back_tris.push_back(bt);

                    bt2 = new BspTriangle;

                    bt2->color = split_tri.color;
                    bt2->texture = split_tri.texture;

                    bt2->tri = new Triangle3d;
                    bt2->tri->verts[0] = triangles[i]->tri->verts[2];
                    bt2->tri->verts[1] = triangles[i]->tri->verts[0];
                    bt2->tri->verts[2] = split_tri.tri->verts[0];

                    back_tris.push_back(bt2);
                }
                break;

                case SplitType(-1, -1,  1):
                {
                    BspTriangle *ft, *bt, *bt2;

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[2];
                    ft->tri->verts[1] = split_tri.tri->verts[2];
                    ft->tri->verts[2] = split_tri.tri->verts[1];

                    front_tris.push_back(ft);

                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[0];
                    bt->tri->verts[1] = split_tri.tri->verts[1];
                    bt->tri->verts[2] = split_tri.tri->verts[2];

                    back_tris.push_back(bt);

                    bt2 = new BspTriangle;

                    bt2->color = split_tri.color;
                    bt2->texture = split_tri.texture;

                    bt2->tri = new Triangle3d;
                    bt2->tri->verts[0] = triangles[i]->tri->verts[0];
                    bt2->tri->verts[1] = triangles[i]->tri->verts[1];
                    bt2->tri->verts[2] = split_tri.tri->verts[1];

                    back_tris.push_back(bt2);
                }
                break;

                case SplitType(-1,  1,  1):
                {
                    BspTriangle *ft, *ft2, *bt;

                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[0];
                    bt->tri->verts[1] = split_tri.tri->verts[0];
                    bt->tri->verts[2] = split_tri.tri->verts[2];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[1];
                    ft->tri->verts[1] = split_tri.tri->verts[2];
                    ft->tri->verts[2] = split_tri.tri->verts[0];

                    front_tris.push_back(ft);

                    ft2 = new BspTriangle;

                    ft2->color = split_tri.color;
                    ft2->texture = split_tri.texture;

                    ft2->tri = new Triangle3d;
                    ft2->tri->verts[0] = triangles[i]->tri->verts[1];
                    ft2->tri->verts[1] = triangles[i]->tri->verts[2];
                    ft2->tri->verts[2] = split_tri.tri->verts[2];

                    front_tris.push_back(ft2);
                }
                break;

                case SplitType( 1, -1,  1):
                {
                    BspTriangle *ft, *ft2, *bt;

                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[1];
                    bt->tri->verts[1] = split_tri.tri->verts[1];
                    bt->tri->verts[2] = split_tri.tri->verts[0];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[0];
                    ft->tri->verts[1] = split_tri.tri->verts[0];
                    ft->tri->verts[2] = split_tri.tri->verts[1];

                    front_tris.push_back(ft);

                    ft2 = new BspTriangle;

                    ft2->color = split_tri.color;
                    ft2->texture = split_tri.texture;

                    ft2->tri = new Triangle3d;
                    ft2->tri->verts[0] = triangles[i]->tri->verts[2];
                    ft2->tri->verts[1] = triangles[i]->tri->verts[0];
                    ft2->tri->verts[2] = split_tri.tri->verts[1];

                    front_tris.push_back(ft2);
                }
                break;

                case SplitType( 1,  1, -1):
                {
                    BspTriangle *ft, *ft2, *bt;

                    bt = new BspTriangle;

                    bt->color = split_tri.color;
                    bt->texture = split_tri.texture;

                    bt->tri = new Triangle3d;
                    bt->tri->verts[0] = triangles[i]->tri->verts[2];
                    bt->tri->verts[1] = split_tri.tri->verts[2];
                    bt->tri->verts[2] = split_tri.tri->verts[1];

                    back_tris.push_back(bt);

                    ft = new BspTriangle;

                    ft->color = split_tri.color;
                    ft->texture = split_tri.texture;

                    ft->tri = new Triangle3d;
                    ft->tri->verts[0] = triangles[i]->tri->verts[0];
                    ft->tri->verts[1] = split_tri.tri->verts[1];
                    ft->tri->verts[2] = split_tri.tri->verts[2];

                    front_tris.push_back(ft);

                    ft2 = new BspTriangle;

                    ft2->color = split_tri.color;
                    ft2->texture = split_tri.texture;

                    ft2->tri = new Triangle3d;
                    ft2->tri->verts[0] = triangles[i]->tri->verts[0];
                    ft2->tri->verts[1] = triangles[i]->tri->verts[1];
                    ft2->tri->verts[2] = split_tri.tri->verts[1];

                    front_tris.push_back(ft2);
                }
                break;
            }
        }
    }

    Vertex3d Bsp3d::LerpVertex(Vertex3d& out, const Vertex3d& vx1, const Vertex3d& vx2, fp frac)
    {
        out.pos.x = pLerp(vx1.pos.x, vx2.pos.x, frac);
        out.pos.y = pLerp(vx1.pos.y, vx2.pos.y, frac);
        out.pos.z = pLerp(vx1.pos.z, vx2.pos.z, frac);

        out.uv.x = pLerp(vx1.uv.x, vx2.uv.x, frac);
        out.uv.y = pLerp(vx1.uv.y, vx2.uv.y, frac);

        return out;
    }

    BspPlane Bsp3d::CheckPlane(std::vector<BspTriangle*>& triangles, unsigned int index, int& front, int& back, int& onplane)
    {
        front = 0;
        back = 0;
        onplane = 0;

        BspPlane plane = CalculatePlane(triangles[index]->tri);

        for(unsigned int i = 0; i < triangles.size(); i++)
        {
            int side[3];

            side[0] = Sign(Distance(plane, triangles[i]->tri->verts[0].pos));
            side[1] = Sign(Distance(plane, triangles[i]->tri->verts[1].pos));
            side[2] = Sign(Distance(plane, triangles[i]->tri->verts[2].pos));

            switch (SplitType(side[0], side[1], side[2]))
            {
            case SplitType(-1, -1, -1):
            case SplitType(-1, -1,  0):
            case SplitType(-1,  0, -1):
            case SplitType(-1,  0,  0):
            case SplitType( 0, -1, -1):
            case SplitType( 0, -1,  0):
            case SplitType( 0,  0, -1):
                back++;
                break;

            case SplitType( 0,  0,  1):
            case SplitType( 0,  1,  0):
            case SplitType( 0,  1,  1):
            case SplitType( 1,  0,  0):
            case SplitType( 1,  0,  1):
            case SplitType( 1,  1,  0):
            case SplitType( 1,  1,  1):
                front++;
                break;

            case SplitType( 0,  0,  0):
                onplane++;
                break;

            case SplitType(-1, -1,  1):
            case SplitType(-1,  1, -1):
            case SplitType( 1, -1, -1):
                back+= 2;
                front++;
                break;

            case SplitType(-1,  0,  1):
            case SplitType( 1,  0, -1):
            case SplitType(-1,  1,  0):
            case SplitType( 1, -1,  0):
            case SplitType( 0, -1,  1):
            case SplitType( 0,  1, -1):
                back++;
                front++;
                break;

            case SplitType(-1,  1,  1):
            case SplitType( 1, -1,  1):
            case SplitType( 1,  1, -1):
                back++;
                front+=2;
                break;
            }
        }

        return plane;
    }

    fp Bsp3d::Distance(const BspPlane& plane, const V3<fp>& pos)
    {
        fp dot = plane.normal.DotProduct(pos);

        return dot - plane.plane;
    }

    int Bsp3d::Sign(fp i)
    {
        if (i >  epsilon)
            return 1;

        if (i < -epsilon)
            return -1;

        return 0;
    }

    BspPlane Bsp3d::CalculatePlane(const Triangle3d* triangle)
    {
        V3<fp> edge1 = triangle->verts[1].pos - triangle->verts[0].pos;
        V3<fp> edge2 = triangle->verts[2].pos - triangle->verts[1].pos;

        V3<fp> normal = edge1.CrossProductNormalised(edge2);

        fp plane = normal.DotProduct(triangle->verts[0].pos);

        BspPlane p;

        p.plane = plane;
        p.normal = normal;

        return p;
    }


    void BspTree::SortBackToFront(const V3<fp>& p, const AABB& frustrum, std::vector<BspTriangle*>& out, bool backface_cull) const
    {
        SortBackToFrontRecursive(p, frustrum ,this->root, out, backface_cull);
    }


    void BspTree::SortBackToFrontRecursive(const V3<fp>& p, const AABB& frustrum, const BspNode* n, std::vector<BspTriangle*>& out, bool backface_cull) const
    {
        if (!n) return;

        if(!frustrum.Intersect(n->child_node_bb))
            return;

        if (Bsp3d::Distance(n->plane, p) < 0)
        {
            SortBackToFrontRecursive(p, frustrum, n->front, out, backface_cull);

            if(frustrum.Intersect(n->node_bb))
            {
                out.insert(out.end(), n->front_tris.begin(), n->front_tris.end());

                if(!backface_cull)
                    out.insert(out.end(), n->back_tris.begin(), n->back_tris.end());
            }

            SortBackToFrontRecursive(p, frustrum, n->back, out, backface_cull);
        }
        else
        {
            SortBackToFrontRecursive(p, frustrum, n->back, out, backface_cull);

            if(frustrum.Intersect(n->node_bb))
            {
                out.insert(out.end(), n->back_tris.begin(), n->back_tris.end());

                if(!backface_cull)
                    out.insert(out.end(), n->front_tris.begin(), n->front_tris.end());
            }

            SortBackToFrontRecursive(p, frustrum, n->front, out, backface_cull);
        }
    }
}
