#ifndef UTILS_H
#define UTILS_H

#include "fp.h"

template <class T>
constexpr inline T lerp(T a, T b, T frac)
{

#ifdef FAST_LERP
    return a + frac * (b - a);
#else
    T ifrac = fp(1) - frac;

    return (a * ifrac) + (b * frac);
#endif
}

template <class T>
constexpr inline int round(T val)
{
    return val + T(0.5);
}

template <>
inline int round(FP val)
{
    return val + (FP(1) >> 1u);
}


#endif // UTILS_H
