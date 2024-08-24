#include "bspmodelexport.h"

namespace Obj2Bsp
{
    BspModelExport::BspModelExport() {}

    QByteArray BspModelExport::ExportBSPModel(Obj2Bsp::BspNode* root, Model3d *model)
    {
        QList<BspNode*> nodeList;

        TraverseNodesRecursive(root, nodeList);

        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);

        //BspModel* bspModel = new BspModel;

        QList<const Texture*> textureList;
        QList<const BspTriangle*> triangleList;

        QList<P3D::BspModelNode> modelNodeList;
        QList<P3D::BspNodeTexture> modelTextureList;
        QList<P3D::BspModelTriangle> modelTriList;


        QByteArray texturePixels;


        for(int i = 0; i < nodeList.length(); i++)
        {
            //Sort nodes by texture to stop cache thrashing...
            std::sort(std::begin(nodeList[i]->front_tris),
                      std::end(nodeList[i]->front_tris),
                      [](const Obj2Bsp::BspTriangle* a, const Obj2Bsp::BspTriangle* b) -> bool {return a->texture > b->texture; });

            std::sort(std::begin(nodeList[i]->back_tris),
                      std::end(nodeList[i]->back_tris),
                      [](const Obj2Bsp::BspTriangle* a, const Obj2Bsp::BspTriangle* b) -> bool {return a->texture > b->texture; });



            P3D::BspModelNode bn;

            P3D::V3<P3D::fp> normal = P3D::V3<P3D::fp>(nodeList[i]->plane.normal.x(), nodeList[i]->plane.normal.y(), nodeList[i]->plane.normal.z());
            bn.plane = P3D::Plane<P3D::fp>(normal, nodeList[i]->plane.distance);

            bn.node_bb = P3D::AABB<P3D::fp>(nodeList[i]->node_bb.x1, nodeList[i]->node_bb.x2,
                                            nodeList[i]->node_bb.y1, nodeList[i]->node_bb.y2,
                                            nodeList[i]->node_bb.z1, nodeList[i]->node_bb.z2);

            bn.child_bb = P3D::AABB<P3D::fp>(nodeList[i]->child_node_bb.x1, nodeList[i]->child_node_bb.x2,
                                             nodeList[i]->child_node_bb.y1, nodeList[i]->child_node_bb.y2,
                                             nodeList[i]->child_node_bb.z1, nodeList[i]->child_node_bb.z2);

            bn.front_tris.count = nodeList[i]->front_tris.size() & 0xffff;
            bn.front_tris.offset = modelTriList.length();

            for(unsigned int j = 0; j < nodeList[i]->front_tris.size(); j++)
            {
                P3D::BspModelTriangle bmt;

                nodeList[i]->front_tris[j]->ComputePlanes();

                for(int v = 0; v < 3; v++)
                {
                    bmt.tri.verts[v].pos.x = nodeList[i]->front_tris[j]->tri->verts[v].pos.x();
                    bmt.tri.verts[v].pos.y = nodeList[i]->front_tris[j]->tri->verts[v].pos.y();
                    bmt.tri.verts[v].pos.z = nodeList[i]->front_tris[j]->tri->verts[v].pos.z();
                    bmt.tri.verts[v].uv.x = nodeList[i]->front_tris[j]->tri->verts[v].uv.x();
                    bmt.tri.verts[v].uv.y = nodeList[i]->front_tris[j]->tri->verts[v].uv.y();
                    bmt.tri.verts[v].vertex_id = nodeList[i]->front_tris[j]->tri->verts[v].vertex_id;
                }

                bmt.color = nodeList[i]->front_tris[j]->color;
                bmt.texture = -1;

                P3D::V3<P3D::fp> normal = P3D::V3<P3D::fp>(nodeList[i]->front_tris[j]->normal_plane.normal.x(),
                                                           nodeList[i]->front_tris[j]->normal_plane.normal.y(),
                                                           nodeList[i]->front_tris[j]->normal_plane.normal.z());
                bmt.normal_plane = P3D::Plane<P3D::fp>(normal, nodeList[i]->front_tris[j]->normal_plane.distance);

                normal = P3D::V3<P3D::fp>(  nodeList[i]->front_tris[j]->edge_plane_01.normal.x(),
                                            nodeList[i]->front_tris[j]->edge_plane_01.normal.y(),
                                            nodeList[i]->front_tris[j]->edge_plane_01.normal.z());
                bmt.edge_plane_0_1 = P3D::Plane<P3D::fp>(normal, nodeList[i]->front_tris[j]->edge_plane_01.distance);

                normal = P3D::V3<P3D::fp>(  nodeList[i]->front_tris[j]->edge_plane_12.normal.x(),
                                            nodeList[i]->front_tris[j]->edge_plane_12.normal.y(),
                                            nodeList[i]->front_tris[j]->edge_plane_12.normal.z());
                bmt.edge_plane_1_2 = P3D::Plane<P3D::fp>(normal, nodeList[i]->front_tris[j]->edge_plane_12.distance);

                normal = P3D::V3<P3D::fp>(  nodeList[i]->front_tris[j]->edge_plane_20.normal.x(),
                                            nodeList[i]->front_tris[j]->edge_plane_20.normal.y(),
                                            nodeList[i]->front_tris[j]->edge_plane_20.normal.z());
                bmt.edge_plane_2_0 = P3D::Plane<P3D::fp>(normal, nodeList[i]->front_tris[j]->edge_plane_20.distance);

                bmt.tri_bb.AddTriangle(bmt.tri.verts[0].pos, bmt.tri.verts[1].pos, bmt.tri.verts[2].pos);

                const Texture* tex = nodeList[i]->front_tris[j]->texture;

                if(tex)
                {
                    if(textureList.contains(tex))
                    {
                        bmt.texture = textureList.indexOf(tex);
                    }
                    else
                    {
                        P3D::BspNodeTexture bnt;
                        bnt.alpha = nodeList[i]->front_tris[j]->texture->alpha;
                        bnt.width = nodeList[i]->front_tris[j]->texture->width;
                        bnt.height = nodeList[i]->front_tris[j]->texture->height;
                        bnt.texture_pixels_offset = texturePixels.length() / sizeof(P3D::pixel);

                        texturePixels.append(nodeList[i]->front_tris[j]->texture->pixels);

                        bmt.texture = modelTextureList.length();

                        modelTextureList.append(bnt);
                        textureList.append(tex);
                    }
                }

                modelTriList.append(bmt);
            }



            bn.back_tris.count = nodeList[i]->back_tris.size() & 0xffff;
            bn.back_tris.offset = modelTriList.length();

            for(unsigned int j = 0; j < nodeList[i]->back_tris.size(); j++)
            {
                P3D::BspModelTriangle bmt;

                nodeList[i]->back_tris[j]->ComputePlanes();

                for(int v = 0; v < 3; v++)
                {
                    bmt.tri.verts[v].pos.x = nodeList[i]->back_tris[j]->tri->verts[v].pos.x();
                    bmt.tri.verts[v].pos.y = nodeList[i]->back_tris[j]->tri->verts[v].pos.y();
                    bmt.tri.verts[v].pos.z = nodeList[i]->back_tris[j]->tri->verts[v].pos.z();
                    bmt.tri.verts[v].uv.x = nodeList[i]->back_tris[j]->tri->verts[v].uv.x();
                    bmt.tri.verts[v].uv.y = nodeList[i]->back_tris[j]->tri->verts[v].uv.y();
                    bmt.tri.verts[v].vertex_id = nodeList[i]->back_tris[j]->tri->verts[v].vertex_id;
                }

                bmt.color = nodeList[i]->back_tris[j]->color;
                bmt.texture = -1;

                P3D::V3<P3D::fp> normal = P3D::V3<P3D::fp>(nodeList[i]->back_tris[j]->normal_plane.normal.x(),
                                                           nodeList[i]->back_tris[j]->normal_plane.normal.y(),
                                                           nodeList[i]->back_tris[j]->normal_plane.normal.z());
                bmt.normal_plane = P3D::Plane<P3D::fp>(normal, nodeList[i]->back_tris[j]->normal_plane.distance);

                normal = P3D::V3<P3D::fp>(  nodeList[i]->back_tris[j]->edge_plane_01.normal.x(),
                                            nodeList[i]->back_tris[j]->edge_plane_01.normal.y(),
                                            nodeList[i]->back_tris[j]->edge_plane_01.normal.z());
                bmt.edge_plane_0_1 = P3D::Plane<P3D::fp>(normal, nodeList[i]->back_tris[j]->edge_plane_01.distance);

                normal = P3D::V3<P3D::fp>(  nodeList[i]->back_tris[j]->edge_plane_12.normal.x(),
                                            nodeList[i]->back_tris[j]->edge_plane_12.normal.y(),
                                            nodeList[i]->back_tris[j]->edge_plane_12.normal.z());
                bmt.edge_plane_1_2 = P3D::Plane<P3D::fp>(normal, nodeList[i]->back_tris[j]->edge_plane_12.distance);

                normal = P3D::V3<P3D::fp>(  nodeList[i]->back_tris[j]->edge_plane_20.normal.x(),
                                            nodeList[i]->back_tris[j]->edge_plane_20.normal.y(),
                                            nodeList[i]->back_tris[j]->edge_plane_20.normal.z());
                bmt.edge_plane_2_0 = P3D::Plane<P3D::fp>(normal, nodeList[i]->back_tris[j]->edge_plane_20.distance);

                bmt.tri_bb.AddTriangle(bmt.tri.verts[0].pos, bmt.tri.verts[1].pos, bmt.tri.verts[2].pos);

                const Texture* tex = nodeList[i]->back_tris[j]->texture;

                if(tex)
                {
                    if(textureList.contains(tex))
                    {
                        bmt.texture = textureList.indexOf(tex);
                    }
                    else
                    {
                        P3D::BspNodeTexture bnt;
                        bnt.alpha = nodeList[i]->back_tris[j]->texture->alpha;
                        bnt.width = nodeList[i]->back_tris[j]->texture->width;
                        bnt.height = nodeList[i]->back_tris[j]->texture->height;
                        bnt.texture_pixels_offset = texturePixels.length() / sizeof(P3D::pixel);

                        texturePixels.append(nodeList[i]->back_tris[j]->texture->pixels);

                        bmt.texture = modelTextureList.length();

                        modelTextureList.append(bnt);
                        textureList.append(tex);
                    }
                }

                modelTriList.append(bmt);
            }



            bn.front_node = 0;
            bn.back_node = 0;

            if(nodeList[i]->front)
            {
                bn.front_node = nodeList.indexOf(nodeList[i]->front);
            }

            if(nodeList[i]->back)
            {
                bn.back_node = nodeList.indexOf(nodeList[i]->back);
            }


            modelNodeList.append(bn);
        }

        P3D::BspModelHeader bmh;

        buffer.write((const char*)&bmh, sizeof(bmh));

        bmh.node_count = modelNodeList.length();
        bmh.node_offset = buffer.pos();

        for(int i = 0; i < modelNodeList.length(); i++)
        {
            buffer.write((const char*)&modelNodeList[i], sizeof(modelNodeList[i]));
        }

        bmh.triangle_count = modelTriList.length();
        bmh.triangle_offset = buffer.pos();

        for(int i = 0; i < modelTriList.length(); i++)
        {
            buffer.write((const char*)&modelTriList[i], sizeof(modelTriList[i]));
        }

        bmh.texture_count = modelTextureList.length();
        bmh.texture_offset = buffer.pos();

        for(int i = 0; i < modelTextureList.length(); i++)
        {
            buffer.write((const char*)&modelTextureList[i], sizeof(modelTextureList[i]));
        }

        bmh.texture_pixels_offset = buffer.pos();
        buffer.write(texturePixels.data(), texturePixels.length());

        bmh.texture_palette_offset = buffer.pos();
        buffer.write((const char*)model->colormap, 256 * 4);

        bmh.fog_lightmap_offset = buffer.pos();
        buffer.write((const char*)model->foglightmap, 256 * FOG_LEVELS * LIGHT_LEVELS);

        //Now overwrite the header with offsets.
        P3D::BspModelHeader* hdr = (P3D::BspModelHeader*)bytes.data();

        *hdr = bmh;

        return bytes;
    }

    void BspModelExport::TraverseNodesRecursive(BspNode* n, QList<BspNode*>& nodeList)
    {
        if (!n) return;

        nodeList.append(n);

        TraverseNodesRecursive(n->front, nodeList);
        TraverseNodesRecursive(n->back, nodeList);
    }
}

