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
            fp u_left;
            fp v_left;
            fp w_left;
            fp z_left;
            fp f_left;
            fp l_left;
            pixel* fb_ypos;
            z_val* zb_ypos;
        } TriEdgeTrace;

        typedef struct
        {
            fp u;
            fp v;
            fp w;
            fp z;
            fp f;
            fp l;
        } TriDrawXDeltaZWUV;

        typedef struct TriDrawYDeltaZWUV
        {
            fp x;
            fp u;
            fp v;
            fp w;
            fp z;
            fp f;
            fp l;
        } TriDrawYDeltaZWUV;

        class RenderTriangleBase
        {
        public:

            virtual ~RenderTriangleBase() {};
            virtual void DrawTriangle(TransformedTriangle& tri, const Material& material) = 0;
            virtual void SetRenderStateViewport(const RenderTargetViewport& viewport) = 0;
            virtual void SetZPlanes(const RenderDeviceNearFarPlanes& planes) = 0;
            virtual void SetTextureCache(const TextureCacheBase* texture_cache) = 0;
            virtual void SetFogParams(const RenderDeviceFogParameters& fog_params) = 0;
            virtual void SetFogLightMap(const unsigned char* fog_light_map) = 0;

#ifdef RENDER_STATS
            virtual void SetRenderStats(RenderStats& render_stats) = 0;
#endif
        };

        template<const unsigned int render_flags, class TPixelShader> class RenderTriangle final : public RenderTriangleBase
        {
        public:
            void no_inline DrawTriangle(TransformedTriangle& tri, const Material& material) override
            {

#ifdef RENDER_STATS
                render_stats->triangles_submitted++;
#endif

                if(material.type == Material::Texture)
                    current_texture = tex_cache->GetTexture(material.pixels);
                else
                {
                    current_texture = nullptr;
                    current_color = material.color;
                }

                if constexpr (render_flags & (SubdividePerspectiveMapping))
                {
                    subdivide_spans = (GetZDelta(tri.verts) > SUBDIVIDE_Z_THREASHOLD);
/*
                    if(!subdivide_spans)
                    {
                        current_texture = nullptr;
                        current_color = material.pixels[0];
                    }
*/
                }

                unsigned int vxCount = ClipTriangle(tri);

                if(vxCount < 3)
                    return;

                for(unsigned int i = 0; i < vxCount; i++)
                {
                    tri.verts[i].pos.ToScreenSpace();
                }

                if(!CullTriangle(tri.verts))
                    return;

                for(unsigned int i = 0; i < vxCount; i++)
                {
                    tri.verts[i].pos.x = fracToX(tri.verts[i].pos.x);
                    tri.verts[i].pos.y = fracToY(tri.verts[i].pos.y);

                    if constexpr (render_flags & Fog)
                    {
                        tri.verts[i].fog_factor = GetFogFactor(tri.verts[i].pos);
                    }

                    if constexpr (render_flags & FullPerspectiveMapping)
                    {
                        if(current_texture)
                        {
                            tri.verts[i].toPerspectiveCorrect(max_w_tex_scale);
                        }
                    }

                    if constexpr (render_flags & SubdividePerspectiveMapping)
                    {
                        if(current_texture && subdivide_spans)
                        {
                            tri.verts[i].toPerspectiveCorrect(max_w_tex_scale);
                        }
                    }
                }

                TriangulatePolygon(tri.verts, vxCount);
            }

            void SetRenderStateViewport(const RenderTargetViewport& viewport) override
            {
                current_viewport = &viewport;
            }

            void SetZPlanes(const RenderDeviceNearFarPlanes& planes) override
            {
                z_planes = &planes;

                if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                {
                    if constexpr(!std::is_floating_point<fp>::value)
                    {
                        int z_near = planes.z_near;

                        max_w_tex_scale = fp((int(std::numeric_limits<fp>::max()) * z_near) / (TEX_SIZE * TEX_MAX_TILE));
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
                fog_params = &fog;
            }

            void SetFogLightMap(const unsigned char* fog_light_map) override
            {
                this->fog_light_map = fog_light_map;
            }


#ifdef RENDER_STATS
            void SetRenderStats(RenderStats& stats) override
            {
                render_stats = &stats;
            }
#endif


        private:

            bool no_inline CullTriangle(const Vertex4d screenSpacePoints[3]) const
            {
                if constexpr (render_flags & (BackFaceCulling | FrontFaceCulling))
                {
                    const bool is_front = IsTriangleFrontface(screenSpacePoints);

                    if constexpr(render_flags & BackFaceCulling)
                    {
                        if(!is_front)
                            return false;
                    }
                    else if constexpr (render_flags & FrontFaceCulling)
                    {
                        if(is_front)
                            return false;
                    }
                }
                else
                {
                    if  (
                        screenSpacePoints[0].pos.x == screenSpacePoints[1].pos.x &&
                        screenSpacePoints[0].pos.x == screenSpacePoints[2].pos.x
                        ) [[unlikely]]
                        return false;

                    if  (
                        screenSpacePoints[0].pos.y == screenSpacePoints[1].pos.y &&
                        screenSpacePoints[0].pos.y == screenSpacePoints[2].pos.y
                        ) [[unlikely]]
                        return false;
                }

                return true;
            }

            unsigned int no_inline ClipTriangle(TransformedTriangle& clipSpacePoints) const
            {
                fp z_far = z_planes->z_far;
                fp z_near = z_planes->z_near;
                unsigned int clip = 0;


                fp w0 = clipSpacePoints.verts[0].pos.w;
                fp w1 = clipSpacePoints.verts[1].pos.w;
                fp w2 = clipSpacePoints.verts[2].pos.w;

                if(pAllGTEqZ3(w0 - z_far, w1 - z_far, w2 - z_far))
                    return 0; //One or more outside far plane. Reject

                if(pAllLTZ3(w0 - z_near, w1 - z_near, w2 - z_near))
                    return 0; //All outside near plane. Reject
                else if (!pAllGTEqZ3(w0 - z_near, w1 - z_near, w2 - z_near))
                    clip = W_Near; //Intersects near plane

                //
                for(unsigned int i = X_W_Left; i < W_Far; i <<= 1)
                {
                    ClipOperation op = GetClipOperation(clipSpacePoints.verts, ClipPlane(i));

                    if(op == Reject)
                        return 0;
                    else if(op == Clip)
                        clip |= i;
                }

                if (clip == NoClip)
                    return 3;

#ifdef RENDER_STATS
                render_stats->triangles_clipped++;
#endif

                unsigned int vxCount = 3;

                if(clip & W_Near)
                    vxCount = ClipWNear(clipSpacePoints.verts);

                Vertex4d outputVxB[8];

                //As we clip against each frustrum plane, we swap the buffers
                //so the output of the last clip is used as input to the next.
                Vertex4d* inBuffer = clipSpacePoints.verts;
                Vertex4d* outBuffer = outputVxB;

                for(unsigned int i = X_W_Left; i < W_Far; i <<= 1)
                {
                    if(clip & i)
                    {
                        vxCount = ClipPolygonToPlane(inBuffer, vxCount, outBuffer, ClipPlane(i));

                        if(vxCount == 0)
                            return 0;

                        std::swap(inBuffer, outBuffer);
                    }
                }

                //Now inBuffer and countA contain the final result.
                if(inBuffer != clipSpacePoints.verts)
                    FastCopy32(clipSpacePoints.verts, inBuffer, sizeof(Vertex4d) * vxCount);

                return vxCount;
            }

            unsigned int no_inline ClipWNear(Vertex4d clipSpacePointsIn[3]) const
            {
                Vertex4d tmpVx[4];
                unsigned int vxCountOut = 0;
                fp z_near = z_planes->z_near;

                for(int i = 0; i < 3; i++)
                {
                    int i2 = i < 2 ? i+1 : 0;

                    fp w1 = clipSpacePointsIn[i].pos.w;
                    fp w2 = clipSpacePointsIn[i2].pos.w;

                    if(w1 >= z_near)
                    {
                        FastCopy32(&tmpVx[vxCountOut], &clipSpacePointsIn[i], sizeof(Vertex4d));
                        vxCountOut++;
                    }

                    if(!pSameSignBit(w1 - z_near, w2 - z_near))
                    {
                        fp frac = (w1 - z_near) / (w1 - w2);

                        LerpVertex(tmpVx[vxCountOut], clipSpacePointsIn[i], clipSpacePointsIn[i2], frac);
                        vxCountOut++;
                    }
                }

                FastCopy32(clipSpacePointsIn, tmpVx, sizeof(Vertex4d) * vxCountOut);

                return vxCountOut;
            }

            unsigned int no_inline ClipPolygonToPlane(const Vertex4d clipSpacePointsIn[], const int vxCount, Vertex4d clipSpacePointsOut[], ClipPlane clipPlane) const
            {
                unsigned int vxCountOut = 0;

                for(int i = 0; i < vxCount; i++)
                {
                    int i2 = (i < (vxCount-1)) ? i+1 : 0;

                    const fp b1 = GetClipPointForVertex(clipSpacePointsIn[i], clipPlane);
                    const fp b2 = GetClipPointForVertex(clipSpacePointsIn[i2], clipPlane);

                    if(ClipW(clipSpacePointsIn[i].pos.w) >= b1)
                    {
                        FastCopy32(&clipSpacePointsOut[vxCountOut], &clipSpacePointsIn[i], sizeof(Vertex4d));
                        vxCountOut++;
                    }

                    fp frac = GetLineIntersectionFrac(ClipW(clipSpacePointsIn[i].pos.w), ClipW(clipSpacePointsIn[i2].pos.w), b1, b2);

                    if(frac >= 0)
                    {
                        LerpVertex(clipSpacePointsOut[vxCountOut], clipSpacePointsIn[i], clipSpacePointsIn[i2], frac);
                        vxCountOut++;
                    }
                }

                return vxCountOut;
            }

            fp GetClipPointForVertex(const Vertex4d& vertex, const ClipPlane clipPlane) const
            {
                if(clipPlane == X_W_Left)
                    return -vertex.pos.x;
                else if(clipPlane == X_W_Right)
                    return vertex.pos.x;
                else if(clipPlane == Y_W_Top)
                    return vertex.pos.y;

                return -vertex.pos.y;
            }

            ClipOperation no_inline GetClipOperation(const Vertex4d vertexes[3], const ClipPlane plane) const
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

                d0 = ClipW(w0) - p0;
                d1 = ClipW(w1) - p1;
                d2 = ClipW(w2) - p2;

                if(pAllGTEqZ3(d0, d1, d2))
                    return Accept;

                return Clip;
            }

            fp no_inline GetLineIntersectionFrac(const fp a1, const fp a2, const fp b1, const fp b2) const
            {
                fp diff1 = a1 - b1;
                fp diff2 = a2 - b2;

                if(pSameSignBit(diff1, diff2))
                    return -1;

                fp cp = (diff1 - a2 + b2);
                return (diff1 / cp);
            }

            inline constexpr fp ClipW(const fp w) const
            {
                return pASL(w, CLIP_GUARD_BAND_SHIFT);
            }

            void no_inline GetVertexYOrder(const Vertex4d screenSpacePoints[3], unsigned int vxOrder[3]) const
            {
                if(screenSpacePoints[vxOrder[0]].pos.y > screenSpacePoints[vxOrder[2]].pos.y)
                    std::swap(vxOrder[0], vxOrder[2]);

                if(screenSpacePoints[vxOrder[0]].pos.y > screenSpacePoints[vxOrder[1]].pos.y)
                    std::swap(vxOrder[0], vxOrder[1]);

                if(screenSpacePoints[vxOrder[1]].pos.y > screenSpacePoints[vxOrder[2]].pos.y)
                    std::swap(vxOrder[1], vxOrder[2]);
            }

            void no_inline TriangulatePolygon(Vertex4d clipSpacePoints[], const int vxCount) const
            {
                DrawTriangleEdge(clipSpacePoints);

                const int rounds = vxCount - 3;

                for(int i = 0; i < rounds; i++)
                {
                    FastCopy32(&clipSpacePoints[i+1], &clipSpacePoints[0], sizeof(Vertex4d));

                    DrawTriangleEdge(&clipSpacePoints[i+1]);
                }
            }

            void no_inline DrawTriangleEdge(const Vertex4d points[3]) const
            {

#ifdef RENDER_STATS
                render_stats->triangles_drawn++;
#endif

                TriEdgeTrace pos;
                TriDrawYDeltaZWUV y_delta_left, y_delta_right;
                TriDrawXDeltaZWUV x_delta;

                unsigned int vxOrder[3] = {0,1,2};

                GetVertexYOrder(points, vxOrder);

                const Vertex4d& top     = points[vxOrder[0]];
                const Vertex4d& middle  = points[vxOrder[1]];
                const Vertex4d& bottom  = points[vxOrder[2]];

                const bool left_is_long = PointOnLineSide2d(top.pos, bottom.pos, middle.pos) > 0;

                if(top.pos.y == middle.pos.y) [[unlikely]]
                {
                    GetTriangleLerpXDeltas(top, middle, x_delta);
                }
                else if(middle.pos.y == bottom.pos.y) [[unlikely]]
                {
                    GetTriangleLerpXDeltas(middle, bottom, x_delta);
                }
                else
                {
                    fp frac = ((middle.pos.y - top.pos.y) / (bottom.pos.y - top.pos.y));

                    Vertex4d m;
                    LerpVertex(m, top, bottom, frac);

                    if(left_is_long)
                    {
                        GetTriangleLerpXDeltas(m, middle, x_delta);
                    }
                    else
                    {
                        GetTriangleLerpXDeltas(middle, m, x_delta);
                    }
                }

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

                DrawTriangleSpans(yStart, yEnd, pos, y_delta_left, y_delta_right, x_delta);

                //Draw bottom half.
                pixelCentreTopY = fp(pMax(yEnd, 0)) + fp(0.5);
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

                DrawTriangleSpans(yStart, yEnd, pos, y_delta_left, y_delta_right, x_delta);
            }

            void no_inline PreStepYTriangleLeft(const fp stepY, const Vertex4d& left, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_left) const
            {
                if(current_texture)
                {
                    pos.u_left = left.uv.x + (stepY * y_delta_left.u);
                    pos.v_left = left.uv.y + (stepY * y_delta_left.v);

                    if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                    {
                        pos.w_left = left.pos.w + (stepY * y_delta_left.w);
                    }
                }

                pos.x_left = left.pos.x + (stepY * y_delta_left.x);

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    pos.z_left = left.pos.z + (stepY * y_delta_left.z);
                }

                if constexpr (render_flags & Fog)
                {
                    pos.f_left = left.fog_factor + (stepY * y_delta_left.f);
                }

                if constexpr (render_flags & VertexLight)
                {
                    pos.l_left = left.light_factor + (stepY * y_delta_left.l);
                }
            }

            void no_inline PreStepYTriangleRight(const fp stepY, const Vertex4d& right, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_right) const
            {
                pos.x_right = right.pos.x + (stepY * y_delta_right.x);
            }

            void no_inline DrawTriangleSpans(const int yStart, const int yEnd, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_left, const TriDrawYDeltaZWUV& y_delta_right, const TriDrawXDeltaZWUV x_delta) const
            {
                pos.fb_ypos = &current_viewport->start[yStart * current_viewport->y_pitch];

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    pos.zb_ypos = &current_viewport->z_start[yStart * current_viewport->z_y_pitch];
                }

                for (int y = yStart; y < yEnd; y++)
                {
                    DrawSpan(pos, x_delta);

                    pos.x_left += y_delta_left.x;
                    pos.x_right += y_delta_right.x;
                    pos.fb_ypos += current_viewport->y_pitch;

                    if constexpr (render_flags & (ZTest | ZWrite))
                    {
                        pos.zb_ypos += current_viewport->z_y_pitch;
                        pos.z_left += y_delta_left.z;
                    }

                    if(current_texture)
                    {
                        pos.u_left += y_delta_left.u;

                        pos.v_left += y_delta_left.v;

                        if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                        {
                            pos.w_left += y_delta_left.w;
                        }
                    }

                    if constexpr (render_flags & Fog)
                    {
                        pos.f_left += y_delta_left.f;
                    }

                    if constexpr (render_flags & VertexLight)
                    {
                        pos.l_left += y_delta_left.l;
                    }
                }
            }

            void no_inline DrawSpan(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta) const
            {
                TriEdgeTrace span_pos;

                const int fb_width = current_viewport->width;

                const fp pixelCentreLeftX = PixelCentre(pMax(pos.x_left, fp(0)));
                const fp stepX = pixelCentreLeftX - pos.x_left;

                const int x_start = pixelCentreLeftX;
                const int x_end = PixelCentre(pMin(pos.x_right, fp(fb_width)));

                if(x_start >= x_end) [[unlikely]]
                    return;

                if(x_start >= fb_width) [[unlikely]]
                    return;

                span_pos.x_left = x_start;
                span_pos.x_right = x_end;
                span_pos.fb_ypos = pos.fb_ypos;

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    span_pos.zb_ypos = pos.zb_ypos;
                    span_pos.z_left = pos.z_left + (delta.z * stepX);
                }

                if constexpr (render_flags & Fog)
                {
                    span_pos.f_left = pos.f_left + (delta.f * stepX);
                }

                if constexpr (render_flags & VertexLight)
                {
                    span_pos.l_left = pos.l_left + (delta.l * stepX);
                }

                if(current_texture)
                {
                    span_pos.u_left = pos.u_left + (delta.u * stepX);
                    span_pos.v_left = pos.v_left + (delta.v * stepX);

                    if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                    {
                        span_pos.w_left = pos.w_left + (delta.w * stepX);
                    }

                    if constexpr (render_flags & FullPerspectiveMapping)
                    {
                        DrawTriangleScanlinePerspectiveCorrect(span_pos, delta, current_texture);
                    }
                    else
                    {
                        if constexpr (render_flags & SubdividePerspectiveMapping)
                        {
                            if(subdivide_spans)
                                SubdivideSpan(span_pos, delta, current_texture);
                            else
                                DrawTriangleScanlineAffine(span_pos, delta, current_texture);
                        }
                        else
                        {
                            DrawTriangleScanlineAffine(span_pos, delta, current_texture);
                        }
                    }
                }
                else
                {
                    DrawTriangleScanlineFlat(span_pos, delta, current_color);
                }

#ifdef RENDER_STATS
                render_stats->scanlines_drawn++;
#endif
            }


            void no_inline SubdivideSpan(TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture) const
            {
                TriDrawXDeltaZWUV delta2;

                fp span_right = pos.x_right;
                fp u = pos.u_left, v = pos.v_left, w = pos.w_left;

                if constexpr(render_flags & Fog)
                {
                    delta2.f = delta.f;
                }

                if constexpr(render_flags & VertexLight)
                {
                    delta2.l = delta.l;
                }

                do
                {
                    pos.x_right = pMin(span_right, pos.x_left + SUBDIVIDE_SPAN_LEN);

                    fp invw_0 = pReciprocal(w);
                    fp invw_15 = pReciprocal(w += pASL(delta.w, SUBDIVIDE_SPAN_SHIFT));

                    fp u0 = pos.u_left = u * invw_0;
                    fp u15 = (u += pASL(delta.u, SUBDIVIDE_SPAN_SHIFT)) * invw_15;
                    delta2.u = pASR(u15-u0, SUBDIVIDE_SPAN_SHIFT);

                    fp v0 = pos.v_left = v * invw_0;
                    fp v15 = (v += pASL(delta.v, SUBDIVIDE_SPAN_SHIFT)) * invw_15;
                    delta2.v = pASR(v15-v0, SUBDIVIDE_SPAN_SHIFT);

                    DrawTriangleScanlineAffine(pos, delta2, texture);

                    pos.x_left += SUBDIVIDE_SPAN_LEN;

                    if constexpr(render_flags & Fog)
                    {
                        pos.f_left += pASL(delta.f, SUBDIVIDE_SPAN_SHIFT);
                    }

                    if constexpr(render_flags & VertexLight)
                    {
                        pos.l_left += pASL(delta.l, SUBDIVIDE_SPAN_SHIFT);
                    }

                } while(pos.x_left < span_right);
            }

            void no_inline DrawTriangleScanlineAffine(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture) const
            {
                const int x_start = (int)pos.x_left;
                const int x_end = (int)pos.x_right;

                pixel* fb = pos.fb_ypos + x_start;
                z_val* zb = pos.zb_ypos + x_start;

                unsigned int count = (x_end - x_start);

                const pixel fog_color = fog_params->fog_color;

                fp z = pos.z_left;
                const fp dz = delta.z;


                fp f = pos.f_left, df = delta.f;
                fp l = pos.l_left, dl = delta.l;

                //Should be fracbits.
                constexpr int uv_shift = 16-TEX_SHIFT;

                fp u = pASL(pos.u_left, uv_shift);
                fp v = pASL(pos.v_left, uv_shift);
                const fp du = pASL(delta.u, uv_shift);
                const fp dv = pASL(delta.v, uv_shift);

                if((size_t)fb & 1)
                {
                    TPixelShader::DrawScanlinePixelHigh(fb, zb, z, texture, pASR(u, uv_shift), pASR(v, uv_shift), f, l, fog_color, fog_light_map); fb++, zb++, z += dz, u += du, v+= dv, f += df, l += dl, count--;
                }

                unsigned int q = count >> 3;

                while(q--)
                {
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f + df, l, l + dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2), l += (dl * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f + df, l, l + dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2), l += (dl * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f + df, l, l + dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2), l += (dl * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f + df, l, l + dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2), l += (dl * 2);
                }

                const unsigned int r = ((count & 7) >> 1);

                switch(r)
                {
                case 3: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f + df, l, l + dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2), l += (dl * 2); [[fallthrough]];
                case 2: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f + df, l, l + dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2), l += (dl * 2); [[fallthrough]];
                case 1: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f + df, l, l + dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2), l += (dl * 2);
                }

                if(count & 1)
                    TPixelShader::DrawScanlinePixelLow(fb, zb, z, texture, pASR(u, uv_shift), pASR(v, uv_shift), f, l, fog_color, fog_light_map);
            }


            void no_inline DrawTriangleScanlinePerspectiveCorrect(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture) const
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

                fp f = pos.f_left;
                const fp df = delta.f;
                const pixel fog_color = fog_params->fog_color;

                fp l = pos.l_left;
                const fp dl = delta.l;

                if((size_t)fb & 1)
                {
                    TPixelShader::DrawScanlinePixelHigh(fb, zb, z, texture, u * pReciprocal(w), v * pReciprocal(w), f, l, fog_color, fog_light_map); fb++, zb++, z += dz, u += du, v += dv, w += dw, f += df, l += dl, count--;
                }

                unsigned int s = count >> 3;

                while(s--)
                {
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2), l += (dl * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2), l += (dl * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2), l += (dl * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2), l += (dl * 2);
                }

                unsigned int t = ((count & 7) >> 1);

                switch(t)
                {
                    case 3: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2), l += (dl * 2);
                    case 2: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2), l += (dl * 2);
                    case 1: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2), l += (dl * 2);
                }

                if(count & 1)
                    TPixelShader::DrawScanlinePixelLow(fb, zb, z, texture, u * pReciprocal(w), v * pReciprocal(w), f, l, fog_color, fog_light_map);
            }

            void no_inline DrawTriangleScanlineFlat(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta,  const pixel color) const
            {
                const int x_start = (int)pos.x_left;
                const int x_end = (int)pos.x_right;

                unsigned int count = (x_end - x_start);

                pixel* fb = pos.fb_ypos + x_start;
                z_val* zb = pos.zb_ypos + x_start;

                fp z = pos.z_left;
                const fp dz = delta.z;


                fp f = pos.f_left;
                const fp df = delta.f;
                const pixel fog_color = fog_params->fog_color;

                fp l = pos.l_left;
                const fp dl = delta.l;

                if((size_t)fb & 1)
                {
                    TPixelShader::DrawScanlinePixelHigh(fb, zb, z, &color, 0, 0, f, l, fog_color, fog_light_map); fb++, zb++, z += dz, f += df, l += dl, count--;
                }

                if constexpr (render_flags & (ZBuffer | Fog | VertexLight))
                {
                    unsigned int s = count >> 3;

                    while(s--)
                    {
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), f += (df * 2), l += (dl * 2);
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), f += (df * 2), l += (dl * 2);
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), f += (df * 2), l += (dl * 2);
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2, zb+=2, z += (dz * 2), f += (df * 2), l += (dl * 2);
                    }

                    const unsigned int t = ((count & 7) >> 1);

                    switch(t)
                    {
                        case 3: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2; zb+=2, z += (dz * 2), f += (df * 2), l += (dl*2);
                        case 2: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2; zb+=2, z += (dz * 2), f += (df * 2), l += (dl*2);
                        case 1: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, l, l+dl, fog_color, fog_light_map); fb+=2; zb+=2, z += (dz * 2), f += (df * 2), l += (dl*2);
                    }
                }
                else
                {
                    if(count >> 1)
                    {
                        FastFill16((unsigned short*)fb, color | color << 8, count >> 1), fb+=count-1;
                    }
                }

                if(count & 1)
                    TPixelShader::DrawScanlinePixelLow(fb, zb, z, &color, 0, 0, f, l, fog_color, fog_light_map);
            }

            constexpr bool no_inline IsTriangleFrontface(const Vertex4d screenSpacePoints[]) const
            {
                const fp x1 = (screenSpacePoints[0].pos.x - screenSpacePoints[1].pos.x);
                const fp y1 = (screenSpacePoints[1].pos.y - screenSpacePoints[0].pos.y);

                const fp x2 = (screenSpacePoints[1].pos.x - screenSpacePoints[2].pos.x);
                const fp y2 = (screenSpacePoints[2].pos.y - screenSpacePoints[1].pos.y);

                return ((x1 * y2) >= (y1 * x2));
            }

            constexpr void no_inline GetTriangleLerpXDeltas(const Vertex4d& left, const Vertex4d& right, TriDrawXDeltaZWUV& x_delta) const
            {
                const fp dx = (right.pos.x != left.pos.x) ? (right.pos.x - left.pos.x) : fp(1);

                if(current_texture)
                {
                    x_delta.u = (right.uv.x - left.uv.x) / dx;
                    x_delta.v = (right.uv.y - left.uv.y) / dx;

                    if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                    {
                        x_delta.w = (right.pos.w - left.pos.w) / dx;
                    }
                }

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    x_delta.z = (right.pos.z - left.pos.z) / dx;
                }

                if constexpr (render_flags & Fog)
                {
                    x_delta.f = (right.fog_factor - left.fog_factor) / dx;
                }

                if constexpr (render_flags & VertexLight)
                {
                    x_delta.l = (right.light_factor - left.light_factor) / dx;
                }
            }

            constexpr void no_inline GetTriangleLerpYDeltas(const Vertex4d& a, const Vertex4d& b, TriDrawYDeltaZWUV& y_delta) const
            {
                const fp dy = (a.pos.y != b.pos.y) ? (a.pos.y - b.pos.y) : fp(1);

                y_delta.x = (a.pos.x - b.pos.x) / dy;

                if(current_texture)
                {
                    y_delta.u = (a.uv.x - b.uv.x) / dy;
                    y_delta.v = (a.uv.y - b.uv.y) / dy;

                    if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                    {
                        y_delta.w = (a.pos.w - b.pos.w) / dy;
                    }
                }

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    y_delta.z = (a.pos.z - b.pos.z) / dy;
                }

                if constexpr (render_flags & Fog)
                {
                    y_delta.f = (a.fog_factor - b.fog_factor) / dy;
                }

                if constexpr (render_flags & VertexLight)
                {
                    y_delta.l = (a.light_factor - b.light_factor) / dy;
                }
            }

            constexpr void no_inline LerpVertex(Vertex4d& out, const Vertex4d& left, const Vertex4d& right, const fp frac) const
            {
                out.pos.x = pLerp(left.pos.x, right.pos.x, frac);
                out.pos.y = pLerp(left.pos.y, right.pos.y, frac);
                out.pos.w = pLerp(left.pos.w, right.pos.w, frac);

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    out.pos.z = pLerp(left.pos.z, right.pos.z, frac);
                }

                if(current_texture)
                {
                    out.uv.x = pLerp(left.uv.x, right.uv.x, frac);
                    out.uv.y = pLerp(left.uv.y, right.uv.y, frac);
                }

                if constexpr (render_flags & Fog)
                {
                    out.fog_factor = pLerp(left.fog_factor, right.fog_factor, frac);
                }

                if constexpr (render_flags & VertexLight)
                {
                    out.light_factor = pLerp(left.light_factor, right.light_factor, frac);
                }
            }

            constexpr fp fracToY(const fp frac) const
            {
                const fp halfFbY = pASR(current_viewport->height, 1);
                return (halfFbY * -frac) + halfFbY;
            }

            constexpr fp fracToX(const fp frac) const
            {
                const fp halfFbX = pASR(current_viewport->width, 1);
                return (halfFbX * frac) + halfFbX;
            }

            constexpr fp PixelCentre(const fp x) const
            {                
                return pCeil(x - fp(0.5)) + fp(0.5);
            }

            constexpr fp no_inline PointOnLineSide2d(const V4<fp>& l1, const V4<fp>& l2, const V4<fp>& p) const
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

            constexpr fp WtoZ(const fp w) const
            {
                const fp zr1 = z_planes->z_ratio_1;
                const fp zr2 = z_planes->z_ratio_2;

                return fp(1) - (zr1 + (pReciprocal(w) * zr2));
            }

            constexpr fp LinearW(const fp w) const
            {
                const fp near = z_planes->z_near;
                const fp far = z_planes->z_far;

                return fp(1) - ((w - near) * z_planes->z_ratio_3);
            }

            constexpr fp no_inline GetZDelta(const Vertex4d verts[3]) const
            {
                fp z0 = WtoZ(verts[0].pos.w);
                fp z1 = WtoZ(verts[1].pos.w);
                fp z2 = WtoZ(verts[2].pos.w);

                fp d1 = pAbs(z0 - z1);
                fp d2 = pAbs(z0 - z2);
                fp d3 = pAbs(z1 - z2);

                fp maxd = pMin(fp(1), pMax(d1, pMax(d2, d3)));

                return maxd * fp(256); //Scale to 0..256 range.
            }

            constexpr fp no_inline GetFogFactor(const V4<fp>& pos) const
            {
                switch(fog_params->mode)
                {
                    case P3D::FogMode::FogLinear:
                        return GetLinearFogFactor(pos.w);

                    case P3D::FogMode::FogExponential:
                        return GetExponentialFogFactor(pos.w);

                    case P3D::FogMode::FogExponential2:
                        return GetExponential2FogFactor(pos.w);

                    default:
                        return 0;
                }
            }

            constexpr fp GetLinearFogFactor(const fp w) const
            {
                if(w >= fog_params->fog_end)
                    return FOG_MAX;
                else if(w <= fog_params->fog_start)
                    return 0;

                const fp x = fog_params->fog_end - w;
                const fp y = fog_params->fog_end - fog_params->fog_start;
                const fp z = pApproxDiv(x, y);

                return pClamp(fp(0), fp(1)-z, FOG_MAX);
            }

            constexpr fp GetExponentialFogFactor(const fp w) const
            {
                const fp z = fp(1) - LinearW(w);

                const fp d = (z * fog_params->fog_density);

                const fp r = pReciprocal(pExp(d));

                return pClamp(fp(0), fp(1)-r, FOG_MAX);
            }

            constexpr fp GetExponential2FogFactor(const fp w) const
            {
                const fp z = fp(1) - LinearW(w);

                fp d = (z * fog_params->fog_density);

                d = (d * d);

                const fp r = pReciprocal(pExp(d));

                return pClamp(fp(0), fp(1)-r, FOG_MAX);
            }


        private:
            const TextureCacheBase* tex_cache = nullptr;
            const RenderTargetViewport* current_viewport = nullptr;
            const RenderDeviceNearFarPlanes* z_planes = nullptr;
            const RenderDeviceFogParameters* fog_params = nullptr;
            fp max_w_tex_scale = 0;

            const pixel* current_texture = nullptr;
            pixel current_color = 0;
            bool subdivide_spans = false;

            const unsigned char* fog_light_map = nullptr;

#ifdef RENDER_STATS
            RenderStats* render_stats = nullptr;
#endif

        };
    };
};

#endif // RENDERTRIANGLE_H
