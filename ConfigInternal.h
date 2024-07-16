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

    inline constexpr int FOG_SHIFT = constexpr_log2(FOG_LEVELS);
    inline constexpr int LIGHT_SHIFT = constexpr_log2(LIGHT_LEVELS);
    inline constexpr fp FOG_MAX = fp(1) - std::numeric_limits<fp>().epsilon();
    inline constexpr fp LIGHT_MAX = fp(1) - std::numeric_limits<fp>().epsilon();

}

#endif // CONFIGINTERNAL_H
