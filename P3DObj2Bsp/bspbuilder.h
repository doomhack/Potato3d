#ifndef BSPBUILDER_H
#define BSPBUILDER_H

#include <QtCore>
#include <QVector3D>

#include "objloader.h"
#include "config.h"

namespace Obj2Bsp
{

    class BspAABB
    {
    public:
        float x1,x2;
        float y1,y2;
        float z1,z2;

        BspAABB()
        {
            x1 = y1 = z1 = std::numeric_limits<float>::max();
            x2 = y2 = z2 = std::numeric_limits<float>::min();
        }

        void AddTriangle(const Triangle3d& tri)
        {
            for(unsigned int i = 0; i < 3; i++)
            {
                AddPoint(tri.verts[i].pos);
            }
        }

        void AddPoint(const QVector3D point)
        {
            if(point.x() < x1)
                x1 = point.x();

            if(point.x() > x2)
                x2 = point.x();

            if(point.y() < y1)
                y1 = point.y();

            if(point.y() > y2)
                y2 = point.y();

            if(point.z() < z1)
                z1 = point.z();

            if(point.z() > z2)
                z2 = point.z();
        }

        void AddAABB(const BspAABB& other)
        {
            if(other.x1 < x1)
                x1 = other.x1;

            if(other.x2 > x2)
                x2 = other.x2;

            if(other.y1 < y1)
                y1 = other.y1;

            if(other.y2 > y2)
                y2 = other.y2;

            if(other.z1 < z1)
                z1 = other.z1;

            if(other.z2 > z2)
                z2 = other.z2;
        }

        bool Intersect(const QVector3D& point) const
        {
            if(x1 > point.x())
                return false;

            if(x2 < point.x())
                return false;

            if(y1 > point.y())
                return false;

            if(y2 < point.y())
                return false;

            if(z1 > point.z())
                return false;

            if(z2 < point.z())
                return false;

            return true;
        }

        bool Intersect(const BspAABB& other) const
        {
            if(x1 > other.x2)
                return false;

            if(x2 < other.x1)
                return false;

            if(y1 > other.y2)
                return false;

            if(y2 < other.y1)
                return false;

            if(z1 > other.z2)
                return false;

            if(z2 < other.z1)
                return false;

            return true;
        }
    };

    typedef struct BspPlane
    {
        QVector3D normal;
        float distance;
    } BspPlane;

    class BspTriangle
    {
    public:
        Triangle3d* tri;
        BspPlane normal_plane;
        BspPlane edge_plane_01;
        BspPlane edge_plane_12;
        BspPlane edge_plane_20;
        const Texture* texture;
        P3D::pixel color;

        void ComputePlanes()
        {
            //Edge vectors
            QVector3D side01 = tri->verts[1].pos - tri->verts[0].pos;
            QVector3D side02 = tri->verts[2].pos - tri->verts[0].pos;
            QVector3D side12 = tri->verts[2].pos - tri->verts[1].pos;
            QVector3D side20 = tri->verts[0].pos - tri->verts[2].pos;

            //Compute Normal
            normal_plane.normal = QVector3D::crossProduct(side02, side01).normalized();
            normal_plane.distance = -QVector3D::dotProduct(normal_plane.normal, tri->verts[0].pos);

            //Edge 0-1 plane
            edge_plane_01.normal = QVector3D::crossProduct(side01, normal_plane.normal).normalized();
            edge_plane_01.distance = -QVector3D::dotProduct(edge_plane_01.normal, tri->verts[0].pos);

            //Edge 1-2 plane
            edge_plane_12.normal = QVector3D::crossProduct(side12, normal_plane.normal).normalized();
            edge_plane_12.distance = -QVector3D::dotProduct(edge_plane_12.normal, tri->verts[1].pos);

            //Edge 2-0 plane
            edge_plane_20.normal = QVector3D::crossProduct(side20, normal_plane.normal).normalized();
            edge_plane_20.distance = -QVector3D::dotProduct(edge_plane_20.normal, tri->verts[2].pos);
        }
    };

    class BspNode
    {
    public:
        BspPlane plane; //Plane that this node splits on.
        std::vector<BspTriangle*> back_tris; //Back facing triangles that lie on this plane.
        std::vector<BspTriangle*> front_tris; //Triangles that lie on this plane.
        BspNode* front = nullptr; //Front children.
        BspNode* back = nullptr; //Back children.
        BspAABB node_bb; //AABB of the triangles in this node.
        BspAABB child_node_bb; //AABB of this node + children.
    };

    class BspBuilder
    {
    public:
        BspBuilder();
        BspNode* BuildBSPTree(Model3d* model);

    private:
        BspPlane CalculatePlane(const Triangle3d* triangle);
        BspNode* BuildTreeRecursive(std::vector<BspTriangle*>& triangles);
        BspPlane CheckPlane(std::vector<BspTriangle*>& triangles, unsigned int index, int& front, int& back, int& onplane);
        float Distance(const BspPlane& plane, const QVector3D& pos);
        int Sign(float i);
        void SeperateTriangles(BspPlane& plane, std::vector<BspTriangle*>& triangles, std::vector<BspTriangle*>& front_tris, std::vector<BspTriangle*>& back_tris, std::vector<BspTriangle*>& plane_tris_front, std::vector<BspTriangle *> &plane_tris_back);
        float Relation(float a, float b);
        Vertex3d LerpVertex(Vertex3d& out, const Vertex3d& vx1, const Vertex3d& vx2, float frac);

        static constexpr float epsilon = 0.25f;
    };

}

#endif // BSPBUILDER_H
