#include "bspbuilder.h"

namespace Obj2Bsp
{

    BspBuilder::BspBuilder() {}

    BspNode* BspBuilder::BuildBSPTree(Model3d* model)
    {
        qDebug() << "Bulding BSP tree...";

        std::vector<BspTriangle*> triangles;

        for (auto it = model->mesh.begin() ; it != model->mesh.end(); ++it)
        {
            const Mesh3d* mesh = *it;

            for (auto it = mesh->tris.begin() ; it != mesh->tris.end(); ++it)
            {
                Triangle3d* triangle = *it;

                //Check for degnerate polygons.
                BspPlane p = CalculatePlane(triangle);

                //NaN always compares != to anything. Even itself.
                //Hopefully, the optimiser won't break this.
                if(p.distance != p.distance)
                    continue;

                BspTriangle* bsptri = new BspTriangle();

                bsptri->tri = triangle;
                bsptri->color = mesh->color;
                bsptri->texture = mesh->texture;

                triangles.push_back(bsptri);
            }
        }

        return BuildTreeRecursive(triangles);
    }

    float BspBuilder::BspPlaneScore(const BspPlaneStats& s) const
    {

        constexpr float eps = 1e-6f;


        constexpr float wSplits        = 5.0f;
        constexpr float wCountBalance  = 0.35f;
        constexpr float wVolumeBalance = 0.50f;
        constexpr float wOverlap       = 1.25f;
        constexpr float wOn            = 0.10f;

        if(s.on == 0)
            return std::numeric_limits<float>::max();

        const float f  = static_cast<float>(s.front);
        const float b  = static_cast<float>(s.back);
        const float on = static_cast<float>(s.on);
        const float sp = static_cast<float>(s.splits);


        const float totalCount = f + b + on;

        // Normalize split cost by node size.
        const float splitPenalty = sp / (totalCount + eps);

        // Count imbalance in [0,1].
        const float countBalance = std::fabs(f - b) / (f + b + eps);

        // Volume imbalance in [0,1].
        const float vf = std::max(s.aabb_volume_front, 0.0f);
        const float vb = std::max(s.aabb_volume_back,  0.0f);
        const float volumeBalance = std::fabs(vf - vb) / (vf + vb + eps);

        // Penalize overlap between child AABBs.
        const float overlap = std::max(s.aabb_overlap, 0.0f);
        const float overlapPenalty = overlap / (vf + vb + eps);

        // Mild coplanar penalty.
        const float onPenalty = on / (totalCount + eps);

        return
            wSplits        * splitPenalty +
            wCountBalance  * countBalance +
            wVolumeBalance * volumeBalance +
            wOverlap       * overlapPenalty +
            wOn            * onPenalty;
    }

    BspNode* BspBuilder::BuildTreeRecursive(std::vector<BspTriangle*>& triangles)
    {
        BspPlaneStats stats, best_stats;

        if(triangles.size() == 0)
            return nullptr;

        qDebug() << "Bulding node with" << triangles.size() << "triangles";

        float best_score = 0; //Higher is worse!

        BspPlane best_plane = CheckPlane(triangles, 0, stats);

        best_score = BspPlaneScore(stats);
        best_stats = stats;

        const int numTris = triangles.size();

        for(unsigned int i = 0; i < numTris; i++)
        {
            BspPlane plane = CheckPlane(triangles, i, stats);

            float score = BspPlaneScore(stats);

            if (score < best_score)
            {
                best_plane = plane;
                best_score = score;
                best_stats = stats;
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

        if(node->front)
            node->front->parent = node;

        if(node->back)
            node->back->parent = node;

        if(node->back)
            node->child_node_bb.AddAABB(node->back->child_node_bb);

        if(node->front)
            node->child_node_bb.AddAABB(node->front->child_node_bb);


        return node;
    }

    static constexpr int SplitType(int a, int b, int c)
    {
        return (a+1)*16 + (b+1)*4 + (c+1);
    }

    void BspBuilder::SeperateTriangles(BspPlane& plane, std::vector<BspTriangle*>& triangles, std::vector<BspTriangle*>& front_tris, std::vector<BspTriangle*>& back_tris, std::vector<BspTriangle*>& plane_tris_front, std::vector<BspTriangle *> &plane_tris_back)
    {
        for(unsigned int i = 0; i < triangles.size(); i++)
        {
            float dist[3];

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

                if(triangles[i]->texture && triangles[i]->texture->alpha)
                {
                    //Triangles with alpha blending are placed on both sides of the node to disable backface culling.
                    plane_tris_back.push_back(triangles[i]);
                    plane_tris_front.push_back(triangles[i]);
                }
                else
                {
                    //Figure out if triangle faces the same way as the BSP plane.
                    QVector3D normal = QVector3D::normal(triangles[i]->tri->verts[0].pos, triangles[i]->tri->verts[1].pos, triangles[i]->tri->verts[2].pos);
                    float sp = QVector3D::dotProduct(plane.normal, normal);

                    if(sp < 0)
                        plane_tris_back.push_back(triangles[i]);
                    else
                        plane_tris_front.push_back(triangles[i]);
                }
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

    BspPlane BspBuilder::CheckPlane(std::vector<BspTriangle*>& triangles, unsigned int index, BspPlaneStats& stats)
    {
        stats.front = 0;
        stats.back = 0;
        stats.on = 0;

        P3D::AABB<float> front_bb;
        P3D::AABB<float> back_bb;
        P3D::AABB<float> on_bb;

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
            {
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));

                stats.back++;
                break;
            }


            case SplitType( 0,  0,  1):
            case SplitType( 0,  1,  0):
            case SplitType( 0,  1,  1):
            case SplitType( 1,  0,  0):
            case SplitType( 1,  0,  1):
            case SplitType( 1,  1,  0):
            case SplitType( 1,  1,  1):
            {
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));

                stats.front++;
                break;
            }

            case SplitType( 0,  0,  0):
            {
                on_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                on_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                on_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));

                stats.on++;
                break;
            }

            case SplitType(-1, -1,  1):
            case SplitType(-1,  1, -1):
            case SplitType( 1, -1, -1):
            {
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));

                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));


                stats.back+= 2;
                stats.front++;
                break;
            }

            case SplitType(-1,  0,  1):
            case SplitType( 1,  0, -1):
            case SplitType(-1,  1,  0):
            case SplitType( 1, -1,  0):
            case SplitType( 0, -1,  1):
            case SplitType( 0,  1, -1):
            {
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));

                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));


                stats.back++;
                stats.front++;
                break;
            }

            case SplitType(-1,  1,  1):
            case SplitType( 1, -1,  1):
            case SplitType( 1,  1, -1):
            {
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                front_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));

                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[0].pos.x(), triangles[i]->tri->verts[0].pos.y(), triangles[i]->tri->verts[0].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[1].pos.x(), triangles[i]->tri->verts[1].pos.y(), triangles[i]->tri->verts[1].pos.z()));
                back_bb.AddPoint(P3D::V3<float>(triangles[i]->tri->verts[2].pos.x(), triangles[i]->tri->verts[2].pos.y(), triangles[i]->tri->verts[2].pos.z()));


                stats.back++;
                stats.front+=2;
                break;
            }

            }
        }

        stats.splits = (stats.front + stats.back + stats.on) - triangles.size();

        stats.aabb_volume_front = front_bb.GetVolume();
        stats.aabb_volume_back = back_bb.GetVolume();
        stats.aabb_volume_on = on_bb.GetVolume();

        stats.aabb_overlap = front_bb.GetIntersection(back_bb).GetVolume();

        return plane;
    }


    float BspBuilder::Distance(const BspPlane& plane, const QVector3D& pos)
    {
        float dot = QVector3D::dotProduct(plane.normal, pos);

        return dot - plane.distance;
    }

    int BspBuilder::Sign(float i)
    {
        if (i > epsilon)
            return 1;

        if (i < -epsilon)
            return -1;

        return 0;
    }

    float BspBuilder::Relation(float a, float b)
    {
        return std::abs(a) / (std::abs(a) + std::abs(b));
    }

    Vertex3d BspBuilder::LerpVertex(Vertex3d& out, const Vertex3d& vx1, const Vertex3d& vx2, float frac)
    {
        out.pos.setX(std::lerp(vx1.pos.x(), vx2.pos.x(), frac));
        out.pos.setY(std::lerp(vx1.pos.y(), vx2.pos.y(), frac));
        out.pos.setZ(std::lerp(vx1.pos.z(), vx2.pos.z(), frac));

        out.uv.setX(std::lerp(vx1.uv.x(), vx2.uv.x(), frac));
        out.uv.setY(std::lerp(vx1.uv.y(), vx2.uv.y(), frac));

        return out;
    }

    BspPlane BspBuilder::CalculatePlane(const Triangle3d* triangle)
    {
        QVector3D normal = QVector3D::normal(triangle->verts[0].pos, triangle->verts[1].pos, triangle->verts[2].pos);

        float plane = QVector3D::dotProduct(normal, triangle->verts[0].pos);

        BspPlane p;

        p.distance = plane;
        p.normal = normal;

        return p;
    }
}
