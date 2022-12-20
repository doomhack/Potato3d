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
            fp f_left, f_right;
            pixel* fb_ypos;
            z_val* zb_ypos;
            pixel fog_color;
        } TriEdgeTrace;

        typedef struct TriDrawXDeltaZWUV
        {
            fp u;
            fp v;
            fp w;
            fp z;
            fp f;
        } TriDrawXDeltaZWUV;

        typedef struct TriDrawYDeltaZWUV
        {
            fp x;
            fp u;
            fp v;
            fp w;
            fp z;
            fp f;
        } TriDrawYDeltaZWUV;


        class RenderTriangleBase
        {
        public:

            virtual ~RenderTriangleBase() {};
            virtual void DrawTriangle(TransformedTriangle& tri, const Material& material) = 0;
            virtual void SetRenderStateViewport(const RenderTargetViewport& viewport, const RenderDeviceNearFarPlanes& planes) = 0;
            virtual void SetTextureCache(const TextureCacheBase* texture_cache) = 0;
            virtual void SetFogParams(const RenderDeviceFogParameters& fog_params) = 0;

#ifdef RENDER_STATS
            virtual void SetRenderStats(RenderStats& render_stats) = 0;
#endif
        };

        template<const unsigned int render_flags, class TPixelShader> class RenderTriangle : public RenderTriangleBase
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
                        //max_w_tex_scale = fp((fp::max() * (int)planes.z_near) / (TEX_SIZE * TEX_MAX_TILE));
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

            void SetFogParams(const RenderDeviceFogParameters& fog) override
            {
                this->fog_params = &fog;
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

                    if constexpr (render_flags & (Fog))
                    {
                        clipSpacePoints[i].fog_factor = GetFogFactor(clipSpacePoints[i].pos);
                    }
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

                if constexpr (render_flags & Fog)
                {
                    pos.fog_color = fog_params->fog_color;
                }

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

                if constexpr (render_flags & Fog)
                {
                    pos.f_left = left.fog_factor + (stepY * y_delta_left.f);
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

                if constexpr (render_flags & Fog)
                {
                    pos.f_right = right.fog_factor + (stepY * y_delta_right.f);
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
                    SubdivideSpan(pos);

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

                    if constexpr (render_flags & Fog)
                    {
                        pos.f_left += y_delta_left.f;
                        pos.f_right += y_delta_right.f;
                    }
                }
            }

            void SubdivideSpan(const TriEdgeTrace& pos)
            {
                if constexpr (render_flags & (HalfPerspectiveMapping))
                {
                    if(current_material->type == Material::Texture)
                    {
                        if(((pos.x_right - pos.x_left) > MIN_SPLIT_SPAN_LEN) && (pAbs(pos.w_left - pos.w_right) > MAX_SPAN_W_DELTA))
                        {
                            TriEdgeTrace l,r;

                            SplitSpan(pos, l, r);

                            SubdivideSpan(l);
                            SubdivideSpan(r);
                        }
                        else
                        {
                            TriEdgeTrace s = pos;

                            s.u_left = s.u_left / s.w_left;
                            s.u_right = s.u_right / s.w_right;

                            s.v_left = s.v_left / s.w_left;
                            s.v_right = s.v_right / s.w_right;

                            DrawSpan(s);
                        }

                        return;
                    }
                }

                DrawSpan(pos);
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

                GetTriangleLerpXDeltas(delta, pos);

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    span_pos.zb_ypos = pos.zb_ypos;
                    span_pos.z_left = pos.z_left + (delta.z * stepX);
                }

                if constexpr (render_flags & Fog)
                {
                    span_pos.f_left = pos.f_left + (delta.f * stepX);
                    span_pos.fog_color = pos.fog_color;
                }

                if(current_material->type == Material::Texture)
                {
                    span_pos.u_left = pos.u_left + (delta.u * stepX);
                    span_pos.v_left = pos.v_left + (delta.v * stepX);

                    if constexpr (render_flags & FullPerspectiveMapping)
                    {
                        span_pos.w_left = pos.w_left + (delta.w * stepX);
                    }

                    const pixel* texture = tex_cache->GetTexture(current_material->pixels);

                    if constexpr (render_flags & FullPerspectiveMapping)
                    {
                        TPixelShader::DrawTriangleScanlinePerspectiveCorrect(span_pos, delta, texture);
                    }
                    else
                    {
                        TPixelShader::DrawTriangleScanlineAffine(span_pos, delta, texture);
                    }
                }
                else
                {
                    TPixelShader::DrawTriangleScanlineFlat(span_pos, delta, current_material->color);
                }

#ifdef RENDER_STATS
                render_stats->scanlines_drawn++;
#endif
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

            constexpr void GetTriangleLerpYDeltas(const Vertex4d& a, const Vertex4d& b, TriDrawYDeltaZWUV &y_delta)
            {
                if(current_material->type == Material::Texture)
                    GetTriangleLerpYDeltasZWUV(a, b, y_delta);
                else
                    GetTriangleLerpYDeltasZ(a, b, y_delta);
            }

            constexpr void GetTriangleLerpXDeltas(TriDrawXDeltaZWUV& x_delta, const TriEdgeTrace& pos)
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

                if constexpr (render_flags & FullPerspectiveMapping)
                {
                    x_delta.w = (pos.w_right - pos.w_left) / d_x;
                }

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    x_delta.z = (pos.z_right - pos.z_left) / d_x;
                }

                if constexpr (render_flags & Fog)
                {
                    x_delta.f = (pos.f_right - pos.f_left) / d_x;
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

                if constexpr (render_flags & Fog)
                {
                    y_delta.f = (a.fog_factor - b.fog_factor) / d_y;
                }
            }

            constexpr void GetTriangleLerpXDeltasZ(TriDrawXDeltaZWUV& x_delta, const TriEdgeTrace& pos)
            {
                const fp d_x = (pos.x_right - pos.x_left) != 0 ? (pos.x_right - pos.x_left) : fp(1);

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    x_delta.z = (pos.z_right - pos.z_left) / d_x;
                }

                if constexpr (render_flags & Fog)
                {
                    x_delta.f = (pos.f_right - pos.f_left) / d_x;
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

                if constexpr (render_flags & Fog)
                {
                    y_delta.f = (a.fog_factor - b.fog_factor) / d_y;
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

            constexpr void SplitSpan(const TriEdgeTrace& pos, TriEdgeTrace& left, TriEdgeTrace& right)
            {
                left.fb_ypos = right.fb_ypos = pos.fb_ypos;

                left.x_left = pos.x_left;
                left.x_right = pos.x_left + ((pos.x_right - pos.x_left) / 2);

                right.x_left = left.x_right;
                right.x_right = pos.x_right;

                left.u_left = pos.u_left;
                left.u_right = pos.u_left + ((pos.u_right - pos.u_left) / 2);

                right.u_left = left.u_right;
                right.u_right = pos.u_right;

                left.v_left = pos.v_left;
                left.v_right = pos.v_left + ((pos.v_right - pos.v_left) / 2);

                right.v_left = left.v_right;
                right.v_right = pos.v_right;

                left.w_left = pos.w_left;
                left.w_right = pos.w_left + ((pos.w_right - pos.w_left) / 2);

                right.w_left = left.w_right;
                right.w_right = pos.w_right;

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    left.z_left = pos.z_left;
                    left.z_right = pos.z_left + ((pos.z_right - pos.z_left) / 2);

                    right.z_left = left.z_right;
                    right.z_right = pos.z_right;

                    left.zb_ypos = right.zb_ypos = pos.zb_ypos;
                }

                if constexpr (render_flags & Fog)
                {
                    left.f_left = pos.f_left;
                    left.f_right = pos.f_left + ((pos.f_right - pos.f_left) / 2);

                    right.f_left = left.f_right;
                    right.f_right = pos.f_right;

                    left.fog_color = right.fog_color = pos.fog_color;
                }
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

            fp GetFogFactor(const V4<fp>& pos)
            {
                switch(fog_params->mode)
                {
                    case P3D::FogMode::FogLinear:
                        return GetLinearFogFactor(pos.w);

                    case P3D::FogMode::FogExponential:
                        return GetExponentialFogFactor(pos.w * pos.z);

                    case P3D::FogMode::FogExponential2:
                        return GetExponential2FogFactor(pos.w * pos.z);

                    default:
                        return 0;
                }
            }

            fp GetLinearFogFactor(const fp w)
            {
                if(w >= fog_params->fog_end)
                    return 1;
                else if(w <= fog_params->fog_start)
                    return 0;

                const fp x = fog_params->fog_end - w;
                const fp y = fog_params->fog_end - fog_params->fog_start;

                return pClamp(fp(0), fp(1)-(x/y), fp(1));
            }

            fp GetExponentialFogFactor(const fp w)
            {
                const fp d = (w * fog_params->fog_density);

                const fp r = exp(-d);

                return pClamp(fp(0), fp(1)-r, fp(1));
            }

            fp GetExponential2FogFactor(const fp w)
            {
                fp d = (w * fog_params->fog_density);

                d = (d * d);

                const fp r = exp(-d);

                return pClamp(fp(0), fp(1)-r, fp(1));
            }


        private:
            const TextureCacheBase* tex_cache = nullptr;
            const Material* current_material = nullptr;
            const RenderTargetViewport* current_viewport = nullptr;
            const RenderDeviceNearFarPlanes* z_planes = nullptr;
            const RenderDeviceFogParameters* fog_params = nullptr;
            fp max_w_tex_scale = 0;


#ifdef RENDER_STATS
            RenderStats* render_stats = nullptr;
#endif

        };
    };
};

#endif // RENDERTRIANGLE_H
