#include "3dmaths/f3dmath.h"

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
        bool alpha;
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
        Triangle3d tri;
        AABB<fp> tri_bb;

        Plane<fp> normal_plane;
        Plane<fp> edge_plane_0_1;
        Plane<fp> edge_plane_1_2;
        Plane<fp> edge_plane_2_0;

        int texture;
        pixel color;
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
        bool alpha;
    } BspNodeTexture;

    typedef struct BspModelNode
    {
        Plane<fp> plane;
        AABB<fp> node_bb;
        AABB<fp> child_bb;
        unsigned int parent_node;
        unsigned int front_node;
        unsigned int back_node;
        TriIndexList front_tris;
        TriIndexList back_tris;
    } BspModelNode;


    //PVS Vis data
    typedef struct VisDataHeader
    {
        unsigned int leaf_count; //Number of leaf nodes.
        unsigned int leaf_index_offset; //Offset in bytes from VisData*
    } VisDataHeader;
}
