#ifndef UTILS_H
#define UTILS_H

#include "fp.h"

namespace P3D
{
    template <class T>
    constexpr inline T pLerp(T a, T b, T frac)
    {

    #ifdef FAST_LERP
        return a + frac * (b - a);
    #else
        T ifrac = T(1) - frac;

        return (a * ifrac) + (b * frac);
    #endif
    }

    template <class T>
    constexpr inline int pRound(T val)
    {
        return val + T(0.5);
    }

    template <>
    constexpr inline int pRound(FP val)
    {
        return val + (FP(1) >> 1u);
    }
}

#endif // UTILS_H
