#ifndef UTILS_H
#define UTILS_H

#include "fp.h"
#include "recip.h"

#ifdef __arm__
    #include <gba_dma.h>
#endif

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

    inline void FastFill32(unsigned int* dest, volatile unsigned int value, unsigned int words)
    {
#ifndef __arm__
        while(words--)
        {
            *dest++ = value;
        }
#else
        DMA3COPY(&value, dest, DMA_SRC_FIXED | DMA_DST_INC | DMA32 | DMA_IMMEDIATE | words)
#endif
    }

    inline void FastFill16(unsigned short* dest, volatile unsigned short value, unsigned int words)
    {
#ifndef __arm__
        while(words--)
        {
            *dest++ = value;
        }
#else
        DMA3COPY(&value, dest, DMA_SRC_FIXED | DMA_DST_INC | DMA16 | DMA_IMMEDIATE | words)
#endif
    }



    template <class T>
    constexpr inline T pReciprocal(T val)
    {
        return T(1)/val;
    }

    //Count leading zeros. 16bit.
    constexpr inline unsigned int clz16(unsigned int x)
    {
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;

        x -= x >> 1 & 0x5555;
        x = (x >> 2 & 0x3333) + (x & 0x3333);
        x = (x >> 4) + x & 0x0f0f;
        x += x >> 8;

        return 16 - (x & 0x003f);
    }

    template <>
    inline FP pReciprocal(FP v)
    {
        FP result;

        FP val = v < 0 ? -v : v;

        if(val <= 1)
        {
            result = FP::fromFPInt(reciprocalTable[val.toFPInt()]);
        }
        else if(val <= 2)
        {
            result = FP::fromFPInt(reciprocalTable[val.toFPInt() >> 1] >> 1);
        }
        else if(val <= 4)
        {
            result = FP::fromFPInt(reciprocalTable[val.toFPInt() >> 2] >> 2);
        }
        else if(val <= 8)
        {
            result = FP::fromFPInt(reciprocalTable[val.toFPInt() >> 3] >> 3);
        }
        else
        {
            const unsigned int shift = 16 - clz16(((val - FP::fromFPInt(1)).i()));

            result = FP::fromFPInt(reciprocalTable[val.toFPInt() >> shift] >> shift);
        }

        return v < 0 ? -result : result;
    }

    template <class T>
    constexpr inline T pScaledReciprocal(unsigned int shift, T val)
    {
        return pReciprocal(pASR(val, shift));
    }
}

#endif // UTILS_H
