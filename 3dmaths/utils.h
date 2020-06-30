#ifndef UTILS_H
#define UTILS_H

#include "fp.h"

namespace P3D
{
    template <class T>
    constexpr inline T pD2R(T degrees)
    {
        const T pi((3.14159265358979323846f));

        return (degrees * pi) / 180;
    }

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

    template <class T>
    constexpr inline T pASL(T val, unsigned int shift)
    {
        return val * (1 << shift);
    }

    template <>
    constexpr inline FP pASL(FP val, unsigned int shift)
    {
        return val << shift;
    }

    template <class T>
    constexpr inline T pASR(T val, unsigned int shift)
    {
        return val / (1 << shift);
    }

    template <>
    constexpr inline FP pASR(FP val, unsigned int shift)
    {
        return val >> shift;
    }

    template <class T>
    constexpr inline T pAbs(T v)
    {
        if(v >= T(0))
            return v;

        return -v;
    }
}

#endif // UTILS_H
