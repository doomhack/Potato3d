#ifndef CONFIGINTERNAL_H
#define CONFIGINTERNAL_H

namespace P3D
{
    inline constexpr int constexpr_log2(int v) { return v ? 1 + constexpr_log2(v >> 1) : -1; }

    inline constexpr int TEX_SHIFT = constexpr_log2(TEX_SIZE);

    inline constexpr int TEX_MASK = (TEX_SIZE-1);

    inline constexpr int TEX_SIZE_PIXELS = (TEX_SIZE * TEX_SIZE);

    inline constexpr int TEX_SIZE_BYTES = (TEX_SIZE_PIXELS * sizeof(pixel));


    inline constexpr int SUBDIVIDE_SPAN_SHIFT = constexpr_log2(SUBDIVIDE_SPAN_LEN);
}

#endif // CONFIGINTERNAL_H
