#ifndef CONFIGINTERNAL_H
#define CONFIGINTERNAL_H

namespace P3D
{
    constexpr int constexpr_log2(int v) { return v ? 1 + constexpr_log2(v >> 1) : -1; }

    constexpr int TEX_SHIFT = constexpr_log2(TEX_SIZE);

    constexpr int TEX_MASK = (TEX_SIZE-1);

    constexpr int TEX_SIZE_PIXELS = (TEX_SIZE * TEX_SIZE);

    constexpr int TEX_SIZE_BYTES = (TEX_SIZE_PIXELS * sizeof(pixel));
}

#endif // CONFIGINTERNAL_H
