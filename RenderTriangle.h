#ifndef RENDERTRIANGLE_H
#define RENDERTRIANGLE_H

#include <algorithm>
#include "RenderCommon.h"
#include "TextureCache.h"

namespace P3D
{
    namespace Internal
    {
        typedef struct TriEdgeTrace
        {
            fp x_left, x_right;
            fp u_left, u_right;
            fp v_left, v_right;
            fp w_left, w_right;
            fp z_left, z_right;
            pixel* fb_ypos;
            z_val* zb_ypos;
        } TriEdgeTrace;

        typedef struct TriDrawXDeltaZWUV
        {
            fp u;
            fp v;
            fp w;
            fp z;
        } TriDrawXDeltaZWUV;

        typedef struct TriDrawYDeltaZWUV
        {
            fp x;
            fp u;
            fp v;
            fp w;
            fp z;
        } TriDrawYDeltaZWUV;


        class RenderTriangleBase
        {
        public:

            virtual ~RenderTriangleBase() {};
            virtual void DrawTriangle(TransformedTriangle& tri, const Material& material) = 0;
            virtual void SetRenderStateViewport(const RenderTargetViewport& viewport, const RenderDeviceNearFarPlanes& planes) = 0;
            virtual void SetTextureCache(const TextureCacheBase* texture_cache) = 0;

#ifdef RENDER_STATS
            virtual void SetRenderStats(RenderStats& render_stats) = 0;
#endif
        };

        template<const unsigned int render_flags> class RenderTriangle : public RenderTriangleBase
        {
        public:
            void DrawTriangle(TransformedTriangle& tri, const Material& material) override
            {
                current_material = &material;

                ClipTriangle(tri);

#ifdef RENDER_STATS
                render_stats->triangles_submitted++;
#endif
            }

            void SetRenderStateViewport(const RenderTargetViewport& viewport, const RenderDeviceNearFarPlanes& planes) override
            {
                current_viewport = &viewport;
                z_planes = &planes;

                if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                {
                    if constexpr(!std::is_floating_point<fp>::value)
                    {
                        max_w_tex_scale = fp((32767 * (int)planes.z_near) / (TEX_SIZE * TEX_MAX_TILE));
                    }
                    else
                    {
                        max_w_tex_scale = 1;
                    }
                }
            }

            void SetTextureCache(const TextureCacheBase* texture_cache) override
            {
                tex_cache = texture_cache;
            }

#ifdef RENDER_STATS
            void SetRenderStats(RenderStats& stats) override
            {
                render_stats = &stats;
            }
#endif


        private:

            void ClipTriangle(TransformedTriangle& clipSpacePoints)
            {

                if(GetClipOperation(clipSpacePoints.verts, W_Far) == Accept)
                    return; //All > means outside plane.

                unsigned int clip = 0;

                //
                for(unsigned int i = W_Near; i < W_Far; i <<= 1)
                {
                    ClipOperation op = GetClipOperation(clipSpacePoints.verts, ClipPlane(i));

                    if(op == Reject)
                        return;
                    else if(op == Clip)
                        clip |= i;
                }


                if (clip == NoClip)
                {
                    TriangulatePolygon(clipSpacePoints.verts, 3);
                }
                else
                {
                    Vertex4d outputVxB[8];
                    unsigned int countA = 3;

                    //As we clip against each frustrum plane, we swap the buffers
                    //so the output of the last clip is used as input to the next.
                    Vertex4d* inBuffer = clipSpacePoints.verts;
                    Vertex4d* outBuffer = outputVxB;

                    for(unsigned int i = W_Near; i < W_Far; i <<= 1)
                    {
                        if(clip & i)
                        {
                            countA = ClipPolygonToPlane(inBuffer, countA, outBuffer, ClipPlane(i));
                            std::swap(inBuffer, outBuffer);
                        }
                    }

                    //Now outBuffer and outCount contain the final result.
                    TriangulatePolygon(inBuffer, countA);
                }
            }

            unsigned int ClipPolygonToPlane(const Vertex4d clipSpacePointsIn[], const int vxCount, Vertex4d clipSpacePointsOut[], ClipPlane clipPlane)
            {
                if(vxCount < 3)
                    return 0;

                unsigned int vxCountOut = 0;

                for(int i = 0; i < vxCount; i++)
                {
                    int i2 = (i < (vxCount-1)) ? i+1 : 0;

                    const fp b1 = GetClipPointForVertex(clipSpacePointsIn[i], clipPlane);
                    const fp b2 = GetClipPointForVertex(clipSpacePointsIn[i2], clipPlane);

                    if(clipSpacePointsIn[i].pos.w >= b1)
                    {
                        clipSpacePointsOut[vxCountOut] = clipSpacePointsIn[i];
                        vxCountOut++;
                    }

                    fp frac = GetLineIntersectionFrac(clipSpacePointsIn[i].pos.w, clipSpacePointsIn[i2].pos.w, b1, b2);

                    if(frac >= 0)
                    {
                        LerpVertexXYZWUV(clipSpacePointsOut[vxCountOut], clipSpacePointsIn[i], clipSpacePointsIn[i2], frac);

                        vxCountOut++;
                    }
                }

                return vxCountOut;
            }

            fp GetClipPointForVertex(const Vertex4d& vertex, const ClipPlane clipPlane) const
            {
                switch(clipPlane)
                {
                    case W_Near:
                        return z_planes->z_near;

                    case X_W_Left:
                        return -vertex.pos.x;

                    case X_W_Right:
                        return vertex.pos.x;

                    case Y_W_Top:
                        return vertex.pos.y;

                    case Y_W_Bottom:
                        return -vertex.pos.y;

                    case W_Far:
                        return z_planes->z_far;

                    default:
                        return 0;
                }
            }

            ClipOperation GetClipOperation(const Vertex4d vertexes[3], const ClipPlane plane) const
            {
                fp w0 = vertexes[0].pos.w;
                fp w1 = vertexes[1].pos.w;
                fp w2 = vertexes[2].pos.w;

                fp p0 = GetClipPointForVertex(vertexes[0], plane);
                fp p1 = GetClipPointForVertex(vertexes[1], plane);
                fp p2 = GetClipPointForVertex(vertexes[2], plane);

                fp d0 = w0 - p0;
                fp d1 = w1 - p1;
                fp d2 = w2 - p2;


                if(pAllLTZ3(d0, d1, d2))
                    return Reject;

                if(pAllGTEqZ3(d0, d1, d2))
                    return Accept;

                return Clip;
            }

            fp GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2) const
            {
                if((a1 <= b1 && a2 <= b2) || (a1 >= b1 && a2 >= b2))
                    return -1;

                fp l1 = (a1 - b1);

                fp cp = (l1 - a2 + b2);

                return (l1 / cp);
            }

            void TriangulatePolygon(Vertex4d clipSpacePoints[], const int vxCount)
            {
                if(vxCount < 3)
                    return;

                for(int i = 0; i < vxCount; i++)
                {
                    clipSpacePoints[i].pos.ToScreenSpace();

                    clipSpacePoints[i].pos.x = fracToX(clipSpacePoints[i].pos.x);
                    clipSpacePoints[i].pos.y = fracToY(clipSpacePoints[i].pos.y);
                }

                CullTriangle(clipSpacePoints);

                int rounds = vxCount - 3;

                for(int i = 0; i < rounds; i++)
                {
                    clipSpacePoints[i+1] = clipSpacePoints[0];
                    CullTriangle(&clipSpacePoints[i+1]);
                }
            }

            void CullTriangle(const Vertex4d screenSpacePoints[3])
            {
                if constexpr (render_flags & (BackFaceCulling | FrontFaceCulling))
                {
                    const bool is_front = IsTriangleFrontface(screenSpacePoints);

                    if constexpr(render_flags & BackFaceCulling)
                    {
                        if(!is_front)
                            return;
                    }
                    else if constexpr (render_flags & FrontFaceCulling)
                    {
                        if(is_front)
                            return;
                    }
                }
                else
                {
                    if  (
                            screenSpacePoints[0].pos.x == screenSpacePoints[1].pos.x &&
                            screenSpacePoints[0].pos.x == screenSpacePoints[2].pos.x
                        )
                        return;

                    if  (
                            screenSpacePoints[0].pos.y == screenSpacePoints[1].pos.y &&
                            screenSpacePoints[0].pos.y == screenSpacePoints[2].pos.y
                        )
                        return;
                }

                SortTrianglePoints(screenSpacePoints);
            }

            void SortTrianglePoints(const Vertex4d screenSpacePoints[3])
            {
                Vertex4d points[3];

                SortPointsByY(screenSpacePoints, points);

                if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                {
                    if(current_material->type == Material::Texture)
                    {
                        points[0].toPerspectiveCorrect(max_w_tex_scale);
                        points[1].toPerspectiveCorrect(max_w_tex_scale);
                        points[2].toPerspectiveCorrect(max_w_tex_scale);
                    }
                }

                DrawTriangleEdge(points);

#ifdef RENDER_STATS
                render_stats->triangles_drawn++;
#endif
            }

            void DrawTriangleEdge(const Vertex4d points[3])
            {
                TriEdgeTrace pos;
                TriDrawYDeltaZWUV y_delta_left, y_delta_right;

                const Vertex4d& top     = points[0];
                const Vertex4d& middle  = points[1];
                const Vertex4d& bottom  = points[2];

                const bool left_is_long = PointOnLineSide2d(top.pos, bottom.pos, middle.pos) > 0;

                TriDrawYDeltaZWUV& short_y_delta = left_is_long ? y_delta_right : y_delta_left;
                TriDrawYDeltaZWUV& long_y_delta = left_is_long ? y_delta_left : y_delta_right;

                const int fb_y = current_viewport->height;

                fp pixelCentreTopY = PixelCentre(pMax(top.pos.y, fp(0)));
                fp stepY = pixelCentreTopY - top.pos.y;

                int yStart = pixelCentreTopY;
                int yEnd = PixelCentre(pMin(middle.pos.y, fp(fb_y)));

                GetTriangleLerpYDeltas(top, bottom, long_y_delta);
                GetTriangleLerpYDeltas(top, middle, short_y_delta);

                //Draw top half of triangle.
                PreStepYTriangleLeft(stepY, top, pos, y_delta_left);
                PreStepYTriangleRight(stepY, top, pos, y_delta_right);

                DrawTriangleSpans(yStart, yEnd, pos, y_delta_left, y_delta_right);

                //Draw bottom half.
                pixelCentreTopY = PixelCentre(pMax(middle.pos.y, fp(0)));
                stepY = pixelCentreTopY - middle.pos.y;

                yStart = pixelCentreTopY;
                yEnd = PixelCentre(pMin(bottom.pos.y, fp(fb_y)));

                if(yStart == yEnd)
                    return;

                GetTriangleLerpYDeltas(middle, bottom, short_y_delta);

                if(left_is_long)
                {
                    PreStepYTriangleRight(stepY, middle, pos, y_delta_right);
                }
                else
                {
                    PreStepYTriangleLeft(stepY, middle, pos, y_delta_left);
                }

                DrawTriangleSpans(yStart, yEnd, pos, y_delta_left, y_delta_right);
            }

            void PreStepYTriangleLeft(const fp stepY, const Vertex4d& left, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_left)
            {
                if(current_material->type == Material::Texture)
                {
                    pos.u_left = left.uv.x + (stepY * y_delta_left.u);

                    pos.v_left = left.uv.y + (stepY * y_delta_left.v);

                    if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                    {
                        pos.w_left = left.pos.w + (stepY * y_delta_left.w);
                    }
                }

                pos.x_left =  left.pos.x + (stepY * y_delta_left.x);

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    pos.z_left = left.pos.z + (stepY * y_delta_left.z);
                }
            }

            void PreStepYTriangleRight(const fp stepY, const Vertex4d& right, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_right)
            {
                if(current_material->type == Material::Texture)
                {
                    pos.u_right = right.uv.x + (stepY * y_delta_right.u);

                    pos.v_right = right.uv.y + (stepY * y_delta_right.v);

                    if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                    {
                        pos.w_right = right.pos.w + (stepY * y_delta_right.w);
                    }
                }

                pos.x_right = right.pos.x + (stepY * y_delta_right.x);

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    pos.z_right = right.pos.z + (stepY * y_delta_right.z);
                }
            }

            void DrawTriangleSpans(const int yStart, const int yEnd, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_left, const TriDrawYDeltaZWUV& y_delta_right)
            {
                pos.fb_ypos = &current_viewport->start[yStart * current_viewport->y_pitch];

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    pos.zb_ypos = &current_viewport->z_start[yStart * current_viewport->z_y_pitch];
                }

                for (int y = yStart; y < yEnd; y++)
                {
                    DrawSpan(pos);

                    pos.x_left += y_delta_left.x;
                    pos.x_right += y_delta_right.x;
                    pos.fb_ypos += current_viewport->y_pitch;

                    if constexpr (render_flags & (ZTest | ZWrite))
                    {
                        pos.zb_ypos += current_viewport->z_y_pitch;
                        pos.z_left += y_delta_left.z;
                        pos.z_right += y_delta_right.z;
                    }

                    if(current_material->type == Material::Texture)
                    {
                        pos.u_left += y_delta_left.u;
                        pos.u_right += y_delta_right.u;

                        pos.v_left += y_delta_left.v;
                        pos.v_right += y_delta_right.v;

                        if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                        {
                            pos.w_left += y_delta_left.w;
                            pos.w_right += y_delta_right.w;
                        }
                    }
                }
            }

            void DrawSpan(const TriEdgeTrace& pos)
            {
                TriEdgeTrace span_pos;
                TriDrawXDeltaZWUV delta;

                const int fb_width = current_viewport->width;

                const fp pixelCentreLeftX = PixelCentre(pMax(pos.x_left, fp(0)));
                const fp stepX = pixelCentreLeftX - pos.x_left;

                int x_start = pixelCentreLeftX;
                int x_end = PixelCentre(pMin(pos.x_right, fp(fb_width)));

                if(x_start > (x_end-1))
                    return;

                if(x_start >= fb_width)
                    return;

                span_pos.x_left = x_start;
                span_pos.x_right = x_end;
                span_pos.fb_ypos = pos.fb_ypos;

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    span_pos.zb_ypos = pos.zb_ypos;
                }

                GetTriangleLerpXDeltas(delta, pos);

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    span_pos.zb_ypos = pos.zb_ypos;
                    span_pos.z_left = pos.z_left + (delta.z * stepX);
                }

                if(current_material->type == Material::Texture)
                {
                    span_pos.u_left = pos.u_left + (delta.u * stepX);
                    span_pos.v_left = pos.v_left + (delta.v * stepX);

                    if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                    {
                        span_pos.w_left = pos.w_left + (delta.w * stepX);
                    }

                    const pixel* texture = tex_cache->GetTexture(current_material->pixels);

                    if constexpr (render_flags & FullPerspectiveMapping)
                    {
                        DrawTriangleScanlinePerspectiveCorrect(span_pos, delta, texture);
                    }
                    else if constexpr (render_flags & HalfPerspectiveMapping)
                    {
                        DrawTriangleScanlineHalfPerspectiveCorrect(span_pos, delta, texture);
                    }
                    else
                    {
                        DrawTriangleScanlineAffine(span_pos, delta, texture);
                    }
                }
                else
                {
                    DrawTriangleScanlineFlat(span_pos, delta, current_material->color);
                }

#ifdef RENDER_STATS
                render_stats->scanlines_drawn++;
#endif
            }

            void DrawTriangleScanlinePerspectiveCorrect(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture)
            {
                const int x_start = (int)pos.x_left;
                const int x_end = (int)pos.x_right;

                unsigned int count = (x_end - x_start);

                fp u = pos.u_left, v = pos.v_left, w = pos.w_left;
                const fp du = delta.u, dv = delta.v, dw = delta.w;

                pixel* fb = pos.fb_ypos + x_start;
                z_val* zb = pos.zb_ypos + x_start;

                fp z = pos.z_left;
                const fp dz = delta.z;

                if((size_t)fb & 1)
                {
                    DrawScanlinePixelLinearHighByte(fb, zb, z, texture, u/w, v/w); fb++, zb++, z += dz, u += du, v += dv, w += dw, count--;
                }

                unsigned int l = count >> 4;

                while(l--)
                {
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                }

                unsigned int r = ((count & 15) >> 1);

                switch(r)
                {
                    case 7: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    case 6: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    case 5: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    case 4: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    case 3: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    case 2: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                    case 1: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u/w, v/w, (u+du)/(w+dw), (v+dv)/(w+dw)); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2);
                }

                if(count & 1)
                    DrawScanlinePixelLinearLowByte(fb, zb, z, texture, u/w, v/w);
            }

            void DrawTriangleScanlineHalfPerspectiveCorrect(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture)
            {
                const int x_start = (int)pos.x_left;
                const int x_end = (int)pos.x_right;

                unsigned int count = (x_end - x_start);

                pixel* fb = pos.fb_ypos + x_start;
                z_val* zb = pos.zb_ypos + x_start;

                fp z = pos.z_left;
                const fp dz = delta.z;


                fp u = pos.u_left, v = pos.v_left, w = pos.w_left;
                const fp idu = delta.u, idv = delta.v, idw = delta.w;

                if((size_t)fb & 1)
                {
                    DrawScanlinePixelLinearHighByte(fb, zb, z, texture, u/w, v/w); fb++; zb++, z += dz, u += idu, v += idv, w += idw, count--;
                }

                unsigned int l = count >> 4;


                while(l--)
                {
                    fp u0 = u / w;
                    fp v0 = v / w;

                    w += (idw * 16);

                    const fp w15 = w;

                    u += (idu * 16);
                    v += (idv * 16);

                    const fp u15 = u / w15;
                    const fp v15 = v / w15;

                    const fp du = (u15 - u0) / 16;
                    const fp dv = (v15 - v0) / 16;

                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                }

                unsigned int r = ((count & 15) >> 1);

                fp u0 = u / w;
                fp v0 = v / w;

                w += (idw * 16);

                const fp w15 = w;

                u += (idu * 16);
                v += (idv * 16);

                const fp u15 = u / w15;
                const fp v15 = v / w15;

                const fp du = (u15 - u0) / 16;
                const fp dv = (v15 - v0) / 16;

                switch(r)
                {
                    case 7: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    case 6: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    case 5: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    case 4: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    case 3: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    case 2: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                    case 1: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u0, v0, u0+du, v0+dv); fb+=2, zb+=2, z += (dz * 2), u0 += (du * 2), v0 += (dv * 2);
                }

                if(count & 1)
                    DrawScanlinePixelLinearLowByte(fb, zb, z, texture, u0, v0);
            }


            void DrawTriangleScanlineAffine(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture)
            {
                const int x_start = (int)pos.x_left;
                const int x_end = (int)pos.x_right;

                unsigned int count = (x_end - x_start);

                pixel* fb = pos.fb_ypos + x_start;
                z_val* zb = pos.zb_ypos + x_start;

                fp z = pos.z_left;
                const fp dz = delta.z;

                fp u = pos.u_left;
                fp v = pos.v_left;

                const fp du = delta.u;
                const fp dv = delta.v;


                if((size_t)fb & 1)
                {
                    DrawScanlinePixelLinearHighByte(fb, zb, z, texture, u, v); fb++, zb++, z += dz, u += du; v += dv; count--;
                }

                unsigned int l = count >> 4;

                while(l--)
                {
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                }

                const unsigned int r = ((count & 15) >> 1);

                switch(r)
                {
                    case 7: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    case 6: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    case 5: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    case 4: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    case 3: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    case 2: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                    case 1: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, texture, u, v, u+du, v+dv); fb+=2; zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2);
                }

                if(count & 1)
                    DrawScanlinePixelLinearLowByte(fb, zb, z, texture, u, v);
            }

            void DrawTriangleScanlineFlat(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta,  const pixel color)
            {
                const int x_start = (int)pos.x_left;
                const int x_end = (int)pos.x_right;

                unsigned int count = (x_end - x_start);

                pixel* fb = pos.fb_ypos + x_start;
                z_val* zb = pos.zb_ypos + x_start;

                fp z = pos.z_left;
                const fp dz = delta.z;

                if((size_t)fb & 1)
                {
                    DrawScanlinePixelLinearHighByte(fb, zb, z, &color, 0, 0); fb++, zb++, z += dz, count--;
                }

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    unsigned int l = count >> 4;

                    while(l--)
                    {
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                        DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2, zb+=2, z += (dz * 2);
                    }

                    const unsigned int r = ((count & 15) >> 1);

                    switch(r)
                    {
                        case 7: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                        case 6: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                        case 5: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                        case 4: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                        case 3: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                        case 2: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                        case 1: DrawScanlinePixelLinearPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0); fb+=2; zb+=2, z += (dz * 2);
                    }
                }
                else
                {
                    if(count >> 1)
                    {
                        FastFill16((unsigned short*)fb, color | color << 8, count >> 1); fb+=count-1;
                    }

                }

                if(count & 1)
                    DrawScanlinePixelLinearLowByte(fb, zb, z, &color, 0, 0);
            }

            inline void DrawScanlinePixelLinearPair(pixel* fb, z_val *zb, const z_val zv1, const z_val zv2, const pixel* texels, const fp u1, const fp v1, const fp u2, const fp v2)
            {
                if constexpr (render_flags & ZTest)
                {
                    if(zv1 >= zb[0]) //Reject left?
                    {
                        if(zv2 >= zb[1]) //Reject right?
                            return; //Both Z Reject.

                        //Accept right.
                        DrawScanlinePixelLinearHighByte(fb+1, zb+1, zv2, texels, u2, v2);
                        return;
                    }
                    else //Accept left.
                    {
                        if(zv2 >= zb[1]) //Reject right?
                        {
                            DrawScanlinePixelLinearLowByte(fb, zb, zv1, texels, u1, v1);
                            return;
                        }
                    }
                }

                const unsigned int tx = (int)u1 & TEX_MASK;
                const unsigned int ty = ((int)v1 & TEX_MASK) << TEX_SHIFT;

                const unsigned int tx2 = (int)u2 & TEX_MASK;
                const unsigned int ty2 = ((int)v2 & TEX_MASK) << TEX_SHIFT;

                *(unsigned short*)fb = ((texels[ty + tx]) | (texels[(ty2 + tx2)] << 8));

                if constexpr (render_flags & ZWrite)
                {
                    zb[0] = zv1, zb[1] = zv2;
                }
            }

            inline void DrawScanlinePixelLinearLowByte(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v)
            {
                if constexpr (render_flags & ZTest)
                {
                    if(*zb <= zv)
                        return;
                }

                if constexpr (render_flags & ZWrite)
                {
                    *zb = zv;
                }

                const unsigned int tx = (int)u & TEX_MASK;
                const unsigned int ty = ((int)v & TEX_MASK) << TEX_SHIFT;

                unsigned short* p16 = (unsigned short*)(fb);
                const pixel* p8 = (pixel*)p16;

                const unsigned short texel = texels[(ty + tx)] | (p8[1] << 8);

                *p16 = texel;
            }

            inline void DrawScanlinePixelLinearHighByte(pixel *fb, z_val *zb, const z_val zv, const pixel* texels, const fp u, const fp v)
            {
                if constexpr (render_flags & ZTest)
                {
                    if(*zb <= zv)
                        return;
                }

                if constexpr (render_flags & ZWrite)
                {
                    *zb = zv;
                }

                const unsigned int tx = (int)u & TEX_MASK;
                const unsigned int ty = ((int)v & TEX_MASK) << TEX_SHIFT;

                unsigned short* p16 = (unsigned short*)(fb-1);
                const pixel* p8 = (pixel*)p16;

                const unsigned short texel = (texels[(ty + tx)] << 8) | *p8;

                *p16 = texel;
            }

            constexpr bool IsTriangleFrontface(const Vertex4d screenSpacePoints[]) const
            {
                const fp x1 = (screenSpacePoints[0].pos.x - screenSpacePoints[1].pos.x);
                const fp y1 = (screenSpacePoints[1].pos.y - screenSpacePoints[0].pos.y);

                const fp x2 = (screenSpacePoints[1].pos.x - screenSpacePoints[2].pos.x);
                const fp y2 = (screenSpacePoints[2].pos.y - screenSpacePoints[1].pos.y);

                return ((x1 * y2) < (y1 * x2));
            }

            void SortPointsByY(const Vertex4d pointsIn[3], Vertex4d pointsOut[3])
            {
                if(pointsIn[0].pos.y < pointsIn[1].pos.y)
                {
                    if(pointsIn[1].pos.y < pointsIn[2].pos.y)
                    {
                        pointsOut[0] = pointsIn[0];
                        pointsOut[1] = pointsIn[1];
                        pointsOut[2] = pointsIn[2];
                    }
                    else
                    {
                        pointsOut[2] = pointsIn[1];

                        if(pointsIn[0].pos.y < pointsIn[2].pos.y)
                        {
                            pointsOut[0] = pointsIn[0];
                            pointsOut[1] = pointsIn[2];
                        }
                        else
                        {
                            pointsOut[0] = pointsIn[2];
                            pointsOut[1] = pointsIn[0];
                        }
                    }
                }
                else
                {
                    if(pointsIn[1].pos.y < pointsIn[2].pos.y)
                    {
                        pointsOut[0] = pointsIn[1];

                        if(pointsIn[0].pos.y < pointsIn[2].pos.y)
                        {
                            pointsOut[1] = pointsIn[0];
                            pointsOut[2] = pointsIn[2];
                        }
                        else
                        {
                            pointsOut[1] = pointsIn[2];
                            pointsOut[2] = pointsIn[0];
                        }
                    }
                    else
                    {
                        pointsOut[0] = pointsIn[2];
                        pointsOut[1] = pointsIn[1];
                        pointsOut[2] = pointsIn[0];
                    }
                }
            }

            void GetTriangleLerpYDeltas(const Vertex4d& a, const Vertex4d& b, TriDrawYDeltaZWUV &y_delta)
            {
                if(current_material->type == Material::Texture)
                    GetTriangleLerpYDeltasZWUV(a, b, y_delta);
                else
                    GetTriangleLerpYDeltasZ(a, b, y_delta);
            }

            void GetTriangleLerpXDeltas(TriDrawXDeltaZWUV& x_delta, const TriEdgeTrace& pos)
            {
                if(current_material->type == Material::Texture)
                    GetTriangleLerpXDeltasZWUV(x_delta, pos);
                else
                    GetTriangleLerpXDeltasZ(x_delta, pos);
            }

            constexpr void GetTriangleLerpXDeltasZWUV(TriDrawXDeltaZWUV& x_delta, const TriEdgeTrace& pos)
            {
                const fp d_x = (pos.x_right - pos.x_left) != 0 ? (pos.x_right - pos.x_left) : fp(1);

                x_delta.u = (pos.u_right - pos.u_left) / d_x;
                x_delta.v = (pos.v_right - pos.v_left) / d_x;

                if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                {
                    x_delta.w = (pos.w_right - pos.w_left) / d_x;
                }

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    x_delta.z = (pos.z_right - pos.z_left) / d_x;
                }
            }

            constexpr void GetTriangleLerpYDeltasZWUV(const Vertex4d& a, const Vertex4d& b, TriDrawYDeltaZWUV &y_delta)
            {
                const fp d_y = (a.pos.y - b.pos.y) != 0 ? (a.pos.y - b.pos.y) : fp(1);

                y_delta.x = (a.pos.x - b.pos.x) / d_y;

                y_delta.u = (a.uv.x - b.uv.x) / d_y;
                y_delta.v = (a.uv.y - b.uv.y) / d_y;

                if constexpr (render_flags & (FullPerspectiveMapping | HalfPerspectiveMapping))
                {
                    y_delta.w = (a.pos.w - b.pos.w) / d_y;
                }

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    y_delta.z = (a.pos.z - b.pos.z) / d_y;
                }
            }

            constexpr void GetTriangleLerpXDeltasZ(TriDrawXDeltaZWUV& x_delta, const TriEdgeTrace& pos)
            {
                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    const fp d_x = (pos.x_right - pos.x_left) != 0 ? (pos.x_right - pos.x_left) : fp(1);

                    x_delta.z = (pos.z_right - pos.z_left) / d_x;
                }
            }

            constexpr void GetTriangleLerpYDeltasZ(const Vertex4d& a, const Vertex4d& b, TriDrawYDeltaZWUV &y_delta)
            {
                const fp d_y = (a.pos.y - b.pos.y) != 0 ? (a.pos.y - b.pos.y) : fp(1);

                y_delta.x = (a.pos.x - b.pos.x) / d_y;

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    y_delta.z = (a.pos.z - b.pos.z) / d_y;
                }
            }

            constexpr void LerpVertexXYZWUV(Vertex4d& out, const Vertex4d& left, const Vertex4d& right, const fp frac)
            {
                out.pos.x = pLerp(left.pos.x, right.pos.x, frac);
                out.pos.y = pLerp(left.pos.y, right.pos.y, frac);
                out.pos.z = pLerp(left.pos.z, right.pos.z, frac);
                out.pos.w = pLerp(left.pos.w, right.pos.w, frac);

                out.uv.x = pLerp(left.uv.x, right.uv.x, frac);
                out.uv.y = pLerp(left.uv.y, right.uv.y, frac);
            }

            fp fracToY(const fp frac) const
            {
                const fp halfFbY = pASR(current_viewport->height, 1);

                return ((halfFbY * -frac) + halfFbY);
            }

            fp fracToX(const fp frac) const
            {
                const fp halfFbX = pASR(current_viewport->width, 1);

                return ((halfFbX * frac) + halfFbX);
            }

            constexpr fp PixelCentre(const fp x) const
            {
                return pCeil(x - fp(0.5)) + fp(0.5);
            }

            constexpr fp PointOnLineSide2d(const V4<fp>& l1, const V4<fp>& l2, const V4<fp>& p) const
            {
                //Left < 0
                //Right > 0
                //On = 0

                //We shift down to ensure that cross doesn't overflow.
                const fp dx = pASR(l2.x - l1.x, 8);
                const fp dy = pASR(l2.y - l1.y, 8);
                const fp cy = pASR(p.y - l1.y, 8);
                const fp cx = pASR(p.x - l1.x, 8);

                return (dy*cx) - (dx*cy);
            }

        private:
            const TextureCacheBase* tex_cache = nullptr;
            const Material* current_material = nullptr;
            const RenderTargetViewport* current_viewport = nullptr;
            const RenderDeviceNearFarPlanes* z_planes = nullptr;
            fp max_w_tex_scale = 0;


#ifdef RENDER_STATS
            RenderStats* render_stats = nullptr;
#endif

        };
    };
};

#endif // RENDERTRIANGLE_H
