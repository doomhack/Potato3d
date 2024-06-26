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
            pixel* fb_ypos;
            z_val* zb_ypos;
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

        template<const unsigned int render_flags, class TPixelShader> class RenderTriangle final : public RenderTriangleBase
        {
        public:
            void DrawTriangle(TransformedTriangle& tri, const Material& material) override
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

                unsigned int vxCount = ClipTriangle(tri);

                if(!vxCount)
                    return;

                for(int i = 0; i < vxCount; i++)
                {
                    tri.verts[i].pos.ToScreenSpace();
                }

                if(!CullTriangle(tri.verts))
                    return;

                for(int i = 0; i < vxCount; i++)
                {
                    tri.verts[i].pos.x = fracToX(tri.verts[i].pos.x);
                    tri.verts[i].pos.y = fracToY(tri.verts[i].pos.y);

                    if constexpr (render_flags & (Fog))
                    {
                        tri.verts[i].fog_factor = GetFogFactor(tri.verts[i].pos);
                    }

                    if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                    {
                        if(current_texture)
                        {
                            tri.verts[i].toPerspectiveCorrect(max_w_tex_scale);
                        }
                    }
                }

                TriangulatePolygon(tri.verts, vxCount);
            }

            void SetRenderStateViewport(const RenderTargetViewport& viewport, const RenderDeviceNearFarPlanes& planes) override
            {
                current_viewport = &viewport;
                z_planes = &planes;

                if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
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

            unsigned int ClipTriangle(TransformedTriangle& clipSpacePoints)
            {
                fp z_far = z_planes->z_far;

                if((clipSpacePoints.verts[0].pos.w > z_far) || (clipSpacePoints.verts[1].pos.w > z_far) || (clipSpacePoints.verts[2].pos.w > z_far))
                    return 0; //One or more outside far plane. Reject

                unsigned int clip = 0;

                //
                for(unsigned int i = W_Near; i < W_Far; i <<= 1)
                {
                    ClipOperation op = GetClipOperation(clipSpacePoints.verts, ClipPlane(i));

                    if(op == Reject)
                        return 0;
                    else if(op == Clip)
                        clip |= i;
                }

                if (clip == NoClip)
                {
                    return 3;
                }
                else
                {
#ifdef RENDER_STATS
                    render_stats->triangles_clipped++;
#endif
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

                    //Now inBuffer and countA contain the final result.
                    if(inBuffer != clipSpacePoints.verts)
                        FastCopy32((unsigned int*)clipSpacePoints.verts, (unsigned int*)inBuffer, sizeof(Vertex4d) * countA);

                    return countA;
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

                    if(ClipW(clipSpacePointsIn[i].pos.w) >= b1)
                    {
                        clipSpacePointsOut[vxCountOut] = clipSpacePointsIn[i];
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

                d0 = ClipW(w0) - p0;
                d1 = ClipW(w1) - p1;
                d2 = ClipW(w2) - p2;

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

            inline constexpr fp ClipW(const fp w) const
            {
                return pASL(w, CLIP_GUARD_BAND_SHIFT);
            }

            void TriangulatePolygon(Vertex4d clipSpacePoints[], const int vxCount)
            {
                SortTrianglePoints(clipSpacePoints);

                int rounds = vxCount - 3;

                for(int i = 0; i < rounds; i++)
                {
                    clipSpacePoints[i+1] = clipSpacePoints[0];
                    SortTrianglePoints(&clipSpacePoints[i+1]);
                }
            }

            bool CullTriangle(const Vertex4d screenSpacePoints[3]) const
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
                        )
                        return false;

                    if  (
                            screenSpacePoints[0].pos.y == screenSpacePoints[1].pos.y &&
                            screenSpacePoints[0].pos.y == screenSpacePoints[2].pos.y
                        )
                        return false;
                }

                return true;
            }

            void SortTrianglePoints(const Vertex4d screenSpacePoints[3])
            {
                Vertex4d points[3];

                SortPointsByY(screenSpacePoints, points);

                DrawTriangleEdge(points);

#ifdef RENDER_STATS
                render_stats->triangles_drawn++;
#endif
            }

            void DrawTriangleEdge(const Vertex4d points[3])
            {
                TriEdgeTrace pos;
                TriDrawYDeltaZWUV y_delta_left, y_delta_right;
                TriDrawXDeltaZWUV x_delta;

                const Vertex4d& top     = points[0];
                const Vertex4d& middle  = points[1];
                const Vertex4d& bottom  = points[2];

                const bool left_is_long = PointOnLineSide2d(top.pos, bottom.pos, middle.pos) > 0;

                if(top.pos.y == middle.pos.y)
                {
                    GetTriangleLerpXDeltas(top, middle, x_delta);
                }
                else if(middle.pos.y == bottom.pos.y)
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

                DrawTriangleSpans(yStart, yEnd, pos, y_delta_left, y_delta_right, x_delta);
            }

            void PreStepYTriangleLeft(const fp stepY, const Vertex4d& left, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_left)
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
                pos.x_right = right.pos.x + (stepY * y_delta_right.x);
            }

            void DrawTriangleSpans(const int yStart, const int yEnd, TriEdgeTrace& pos, const TriDrawYDeltaZWUV& y_delta_left, const TriDrawYDeltaZWUV& y_delta_right, const TriDrawXDeltaZWUV x_delta)
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
                }
            }

            void DrawSpan(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta)
            {
                TriEdgeTrace span_pos;

                const int fb_width = current_viewport->width;

                const fp pixelCentreLeftX = PixelCentre(pMax(pos.x_left, fp(0)));
                const fp stepX = pixelCentreLeftX - pos.x_left;

                const int x_start = pixelCentreLeftX;
                const int x_end = PixelCentre(pMin(pos.x_right, fp(fb_width)));

                if(x_start >= x_end)
                    return;

                if(x_start >= fb_width)
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
                            SubdivideSpan(span_pos, delta, current_texture);
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

            void SubdivideSpan(TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture)
            {
                TriDrawXDeltaZWUV delta2;

                fp x_right = pos.x_right;

                fp u = pos.u_left, v = pos.v_left, w = pos.w_left;

                fp invw_0 = pReciprocal(w);
                fp invw_15 = pReciprocal(w += pASL(delta.w, SUBDIVIDE_SPAN_SHIFT));

                fp u0 = u * invw_0;
                fp u15 = (u += pASL(delta.u, SUBDIVIDE_SPAN_SHIFT)) * invw_15;

                fp v0 = v * invw_0;
                fp v15 = (v += pASL(delta.v, SUBDIVIDE_SPAN_SHIFT)) * invw_15;

                fp du = pASR(u15-u0, SUBDIVIDE_SPAN_SHIFT);
                fp dv = pASR(v15-v0, SUBDIVIDE_SPAN_SHIFT);


                while(true)
                {
                    pos.x_right = pMin(x_right, pos.x_left + SUBDIVIDE_SPAN_LEN);

                    delta2.u = du;
                    delta2.v = dv;

                    pos.u_left = u0;
                    pos.v_left = v0;

                    DrawTriangleScanlineAffine(pos, delta2, texture);

                    pos.x_left += SUBDIVIDE_SPAN_LEN;

                    if(pos.x_left >= x_right)
                        return;

                    invw_0 = pReciprocal(w);
                    invw_15 = pReciprocal(w += pASL(delta.w, SUBDIVIDE_SPAN_SHIFT));

                    u0 = u * invw_0;
                    u15 = (u += pASL(delta.u, SUBDIVIDE_SPAN_SHIFT)) * invw_15;

                    v0 = v * invw_0;
                    v15 = (v += pASL(delta.v, SUBDIVIDE_SPAN_SHIFT)) * invw_15;

                    du = pASR(u15-u0, SUBDIVIDE_SPAN_SHIFT);
                    dv = pASR(v15-v0, SUBDIVIDE_SPAN_SHIFT);
                }
            }

            void DrawTriangleScanlineAffine(const TriEdgeTrace& pos, const TriDrawXDeltaZWUV& delta, const pixel* texture)
            {
                const int x_start = (int)pos.x_left;
                const int x_end = (int)pos.x_right;

                pixel* fb = pos.fb_ypos + x_start;
                z_val* zb = pos.zb_ypos + x_start;

                unsigned int count = (x_end - x_start);

                fp z = pos.z_left;
                const fp dz = delta.z;

                fp f = pos.f_left;
                const fp df = delta.f;
                const pixel fog_color = fog_params->fog_color;

                constexpr int uv_shift = 16-TEX_SHIFT;

                fp u = pASL(pos.u_left, uv_shift);
                fp v = pASL(pos.v_left, uv_shift);
                const fp du = pASL(delta.u, uv_shift);
                const fp dv = pASL(delta.v, uv_shift);

                if((size_t)fb & 1)
                {
                    TPixelShader::DrawScanlinePixelHigh(fb, zb, z, texture, pASR(u, uv_shift), pASR(v, uv_shift), f, fog_color); fb++, zb++, z += dz, u += du, v+= dv, f += df, count--;
                }

                unsigned int l = count >> 3;

                while(l--)
                {
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                }

                const unsigned int r = ((count & 7) >> 1);

                switch(r)
                {
                case 3: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 2: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                case 1: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, pASR(u, uv_shift), pASR(v, uv_shift), pASR((u+du), uv_shift), pASR((v+dv), uv_shift), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), f += (df * 2);
                }

                if(count & 1)
                    TPixelShader::DrawScanlinePixelLow(fb, zb, z, texture, pASR(u, uv_shift), pASR(v, uv_shift), f, fog_color);
            }


            void DrawTriangleScanlinePerspectiveCorrect(const Internal::TriEdgeTrace& pos, const Internal::TriDrawXDeltaZWUV& delta, const pixel* texture)
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

                if((size_t)fb & 1)
                {
                    TPixelShader::DrawScanlinePixelHigh(fb, zb, z, texture, u * pReciprocal(w), v * pReciprocal(w), f, fog_color); fb++, zb++, z += dz, u += du, v += dv, w += dw, f += df, count--;
                }

                unsigned int l = count >> 3;

                while(l--)
                {
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                    TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                }

                unsigned int r = ((count & 7) >> 1);

                switch(r)
                {
                    case 3: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                    case 2: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                    case 1: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, texture, u * pReciprocal(w), v * pReciprocal(w), (u+du) * pReciprocal(w+dw), (v+dv) * pReciprocal(w+dw), f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), u += (du * 2), v += (dv * 2), w += (dw * 2), f += (df * 2);
                }

                if(count & 1)
                    TPixelShader::DrawScanlinePixelLow(fb, zb, z, texture, u * pReciprocal(w), v * pReciprocal(w), f, fog_color);
            }

            void DrawTriangleScanlineFlat(const Internal::TriEdgeTrace& pos, const Internal::TriDrawXDeltaZWUV& delta,  const pixel color)
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

                if((size_t)fb & 1)
                {
                    TPixelShader::DrawScanlinePixelHigh(fb, zb, z, &color, 0, 0, f, fog_color); fb++, zb++, z += dz, f += df, count--;
                }

                if constexpr (render_flags & (ZTest | ZWrite | Fog))
                {
                    unsigned int l = count >> 3;

                    while(l--)
                    {
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                        TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2, zb+=2, z += (dz * 2), f += (df * 2);
                    }

                    const unsigned int r = ((count & 7) >> 1);

                    switch(r)
                    {
                        case 3: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                        case 2: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
                        case 1: TPixelShader::DrawScanlinePixelPair(fb, zb, z, z+dz, &color, 0, 0, 0, 0, f, f+df, fog_color); fb+=2; zb+=2, z += (dz * 2), f += (df * 2);
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
                    TPixelShader::DrawScanlinePixelLow(fb, zb, z, &color, 0, 0, f, fog_color);
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

            constexpr void GetTriangleLerpXDeltas(const Vertex4d& left, const Vertex4d& right, TriDrawXDeltaZWUV& x_delta)
            {
                const fp d_x = (right.pos.x - left.pos.x) != 0 ? (right.pos.x - left.pos.x) : fp(1);

                if(current_texture)
                {
                    x_delta.u = (right.uv.x - left.uv.x) / d_x;
                    x_delta.v = (right.uv.y - left.uv.y) / d_x;

                    if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                    {
                        x_delta.w = (right.pos.w - left.pos.w) / d_x;
                    }
                }

                if constexpr (render_flags & (ZTest | ZWrite))
                {
                    x_delta.z = (right.pos.z - left.pos.z) / d_x;
                }

                if constexpr (render_flags & Fog)
                {
                    x_delta.f = (right.fog_factor - left.fog_factor) / d_x;
                }
            }

            constexpr void GetTriangleLerpYDeltas(const Vertex4d& a, const Vertex4d& b, TriDrawYDeltaZWUV &y_delta)
            {
                const fp d_y = (a.pos.y - b.pos.y) != 0 ? (a.pos.y - b.pos.y) : fp(1);

                y_delta.x = (a.pos.x - b.pos.x) / d_y;

                if(current_texture)
                {
                    y_delta.u = (a.uv.x - b.uv.x) / d_y;
                    y_delta.v = (a.uv.y - b.uv.y) / d_y;

                    if constexpr (render_flags & (FullPerspectiveMapping | SubdividePerspectiveMapping))
                    {
                        y_delta.w = (a.pos.w - b.pos.w) / d_y;
                    }
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

            constexpr void LerpVertex(Vertex4d& out, const Vertex4d& left, const Vertex4d& right, const fp frac)
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
            const RenderTargetViewport* current_viewport = nullptr;
            const RenderDeviceNearFarPlanes* z_planes = nullptr;
            const RenderDeviceFogParameters* fog_params = nullptr;
            fp max_w_tex_scale = 0;

            const pixel* current_texture = nullptr;
            pixel current_color = 0;

#ifdef RENDER_STATS
            RenderStats* render_stats = nullptr;
#endif

        };
    };
};

#endif // RENDERTRIANGLE_H
