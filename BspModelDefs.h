#include <numeric>

namespace P3D
{
    class Vertex3d
    {
    public:
        V3<fp> pos;
        unsigned int vertex_id = no_vx_id;
        V2<fp> uv;

        static const unsigned int no_vx_id = -1;
    };

    class Triangle3d
    {
    public:
        Vertex3d verts[3];
    };

    class Texture
    {
    public:
        const pixel* pixels;
        unsigned short width;
        unsigned short height;
        unsigned short u_mask;
        unsigned short v_mask;
        unsigned short v_shift;
        unsigned short alpha;
    };

    class AABB
    {
    public:
        fp x1,x2;
        fp y1,y2;
        fp z1,z2;

        AABB()
        {
            x1 = y1 = z1 = std::numeric_limits<short>::max();
            x2 = y2 = z2 = std::numeric_limits<short>::min();
        }

        void AddTriangle(const Triangle3d& tri)
        {
            for(unsigned int i = 0; i < 3; i++)
            {
                AddPoint(tri.verts[i].pos);
            }
        }

        void AddPoint(const V3<fp>& point)
        {
            if(point.x < x1)
                x1 = point.x;

            if(point.x > x2)
                x2 = point.x;

            if(point.y < y1)
                y1 = point.y;

            if(point.y > y2)
                y2 = point.y;

            if(point.z < z1)
                z1 = point.z;

            if(point.z > z2)
                z2 = point.z;
        }

        void AddAABB(const AABB& other)
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

        bool Intersect(const V3<fp>& point) const
        {
            if(x1 > point.x)
                return false;

            if(x2 < point.x)
                return false;

            if(y1 > point.y)
                return false;

            if(y2 < point.y)
                return false;

            if(z1 > point.z)
                return false;

            if(z2 < point.z)
                return false;

            return true;
        }

        bool Intersect(const AABB& other) const
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

    typedef struct BspModelHeader
    {
        unsigned int texture_count;
        unsigned int texture_offset; //Offset in bytes from BspModel*

        unsigned int triangle_count;
        unsigned int triangle_offset; //Bytes from BspModel*

        unsigned int node_count;
        unsigned int node_offset;

        unsigned int texture_pixels_offset; //Bytes from BspModel*

        unsigned int texture_palette_offset; //Bytes from BspModel*

        unsigned int fog_lightmap_offset; //Bytes from BspModel*

    } BspModelHeader;

    typedef struct BspModelTriangle
    {
    public:
        Triangle3d tri;
        int texture;
        pixel color;
        AABB tri_bb;
    } BspModelTriangle;

    typedef struct TriIndexList
    {
        unsigned short offset;
        unsigned short count;
    } TriIndexList;

    typedef struct BspNodeTexture
    {
        unsigned int texture_pixels_offset; //Pixels
        unsigned short width;
        unsigned short height;
        unsigned short u_mask;
        unsigned short v_mask;
        unsigned short v_shift;
        unsigned short alpha;
    } BspNodeTexture;

    typedef struct BspPlane
    {
        V3<fp> normal;
        fp plane;
    } BspPlane;

    typedef struct BspModelNode
    {
        BspPlane plane;
        AABB node_bb;
        AABB child_bb;
        unsigned int front_node;
        unsigned int back_node;
        TriIndexList front_tris;
        TriIndexList back_tris;
    } BspModelNode;
}
