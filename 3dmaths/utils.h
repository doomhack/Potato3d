#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <cwchar>

#include "fp.h"
#include "recip.h"

#ifdef __arm__
    #include <gba_dma.h>
    #include <gba_systemcalls.h>
#endif

#define M_PI       3.14159265358979323846   // pi


namespace P3D
{
    template <class T>
    constexpr inline T pD2R(const T degrees)
    {
        return (degrees * T(M_PI)) / T(180);
    }

    template <class T>
    constexpr inline T pLerp(const T a, const T b, const T frac)
    {
        return a + frac * (b - a);
    }

    template <class T>
    constexpr inline int pRound(const T val)
    {
        return (int)(val + T(0.5));
    }

    template<unsigned int fracbits>
    constexpr FP<fracbits> pRound(const FP<fracbits> val)
    {
        return val + (FP<fracbits>(1) >> 1);
    }

    template <class T>
    constexpr inline T pASL(const T val, const int shift)
    {
        return val * (1 << shift);
    }

    template<unsigned int fracbits>
    constexpr inline FP<fracbits> pASL(const FP<fracbits> val, const int shift)
    {
        return val << shift;
    }

    template <class T>
    constexpr inline T pASR(const T val, const int shift)
    {
        return val / (1 << shift);
    }

    template<unsigned int fracbits>
    constexpr inline FP<fracbits> pASR(const FP<fracbits> val, const int shift)
    {
        return val >> shift;
    }

    template <class T>
    constexpr inline T pCeil(const T val)
    {
        return std::ceil(val);
    }

    template<unsigned int fracbits>
    constexpr inline FP<fracbits> pCeil(const FP<fracbits> val)
    {
        constexpr unsigned int fpbits = (FP<fracbits>(1).toFPInt()) - 1;

        return (val.toFPInt() + fpbits) >> fracbits;
    }

    template <class T>
    constexpr inline T pAbs(const T v)
    {
        if(v >= T(0))
            return v;

        return -v;
    }

    template <class T>
    constexpr inline T pMin(const T a, const T b)
    {
        return a < b ? a : b;
    }

    template <class T>
    constexpr inline T pMax(const T a, const T b)
    {
        return a > b ? a : b;
    }

    template <class T>
    constexpr inline T pClamp(const T min, const T v, const T max)
    {
        return pMin(pMax(min, v), max);
    }

    template <class T>
    constexpr inline bool pAllLTZ3(const T a, const T b, const T c)
    {
        return (a < 0) && (b < 0) && (c < 0);
    }

    template<unsigned int fracbits>
    constexpr inline bool pAllLTZ3(const FP<fracbits> a, const FP<fracbits> b, const FP<fracbits> c)
    {
        return (a & b & c) < 0;
    }

    template <class T>
    constexpr inline bool pAllGTEqZ3(const T a, const T b, const T c)
    {
        return (a >= 0) && (b >= 0) && (c >= 0);
    }

    template<unsigned int fracbits>
    constexpr inline bool pAllGTEqZ3(const FP<fracbits> a, const FP<fracbits> b, const FP<fracbits> c)
    {
        return (a | b | c) >= 0;
    }

    template <class T>
    constexpr inline bool pSameSignBit(const T a, const T b)
    {
        return (std::signbit(a) == std::signbit(b));
    }

    template<unsigned int fracbits>
    constexpr inline bool pSameSignBit(const FP<fracbits> a, const FP<fracbits> b)
    {
        return (a ^ b) >= 0;
    }

    inline void FastCopy32(void* dest, const void* src, const unsigned int len)
    {
    #ifdef __arm__
        const int words = len >> 2;

        DMA3COPY(src, dest, DMA_DST_INC | DMA_SRC_INC | DMA32 | DMA_IMMEDIATE | words)
    #else
        memcpy(dest, src, len & 0xfffffffc);
    #endif
    }

    inline void FastCopy16(void* dest, const void* src, const unsigned int len)
    {
    #ifdef __arm__
        const int words = len >> 1;

        DMA3COPY(src, dest, DMA_DST_INC | DMA_SRC_INC | DMA16 | DMA_IMMEDIATE | words)
    #else
        memcpy(dest, src, len & 0xfffffffe);
    #endif
    }

    inline void FastFill32(unsigned int* dest, const unsigned int value, unsigned int words)
    {
#ifndef __arm__
        while(words--)
        {
            *dest++ = value;
        }
#else
        volatile unsigned int v = value;

        DMA3COPY(&v, dest, DMA_SRC_FIXED | DMA_DST_INC | DMA32 | DMA_IMMEDIATE | words)
#endif
    }

    inline void FastFill16(unsigned short* dest, const unsigned short value, unsigned int words)
    {
#ifndef __arm__
        std::wmemset((wchar_t*)dest, value, words);
#else
        volatile unsigned short v = value;

        DMA3COPY(&v, dest, DMA_SRC_FIXED | DMA_DST_INC | DMA16 | DMA_IMMEDIATE | words)
#endif
    }

    template <class T>
    constexpr inline T pReciprocal(const T val)
    {
        return T(1)/val;
    }

    template<unsigned int fracbits>
    constexpr inline FP<fracbits> pReciprocal(const FP<fracbits> v)
    {
        FP<fracbits> val = v < 0 ? -v : v;

        int shift = 0;

        if constexpr(fracbits < 16)
            shift = (16 - fracbits);
        else if constexpr(fracbits > 16)
            shift = (fracbits - 16);

        while(val > 4)
        {
            val >>= 1;
            shift++;
        }

        unsigned int recip = reciprocalTable[val.toFPInt()];

        if(shift < 0)
            recip <<= -shift;
        else if(shift > 0)
            recip >>= shift;

        FP<fracbits> result = FP<fracbits>::fromFPInt(recip);

        return v < 0 ? -result : result;
    }

    template<>
    constexpr inline FP16 pReciprocal(const FP16 v)
    {
        FP16 val = v < 0 ? -v : v;

        int shift = 0;

        while(val > 4)
        {
            val >>= 1;
            shift++;
        }

        FP16 result = FP16::fromFPInt(reciprocalTable[val.toFPInt()] >> shift);

        return v < 0 ? -result : result;
    }

    template <class T>
    constexpr inline T pScaledReciprocal(const unsigned int shift, const T val)
    {
        return pReciprocal(pASR(val, shift));
    }

    //Approx fixed point divide of a/b using reciprocal. -> a * (1/b).
    template <class T>
    constexpr inline T pApproxDiv(const T a, const T b)
    {
        return a * pReciprocal(b);
    }

    //Double width type. double_width<uint16> -> uint32.
    template <class> struct double_width;
    template <class T> using double_width_t = typename double_width<T>::type;

    template <class T> struct t { using type = T; };

    template <> struct double_width<uint8_t>        : t<uint16_t> {};
    template <> struct double_width<uint16_t>       : t<uint32_t> {};
    template <> struct double_width<uint32_t>       : t<uint64_t> {};

    template <> struct double_width<int8_t>         : t<int16_t> {};
    template <> struct double_width<int16_t>        : t<int32_t> {};
    template <> struct double_width<int32_t>        : t<int64_t> {};


    //Fast types. fast_int<uint16> -> uint32 on ARM.
    template <class> struct fast_int;
    template <class T> using fast_int_t = typename fast_int<T>::type;

    template <class T> struct f { using type = T; };

    template <> struct fast_int<uint8_t>  : t<uint_fast8_t> {};
    template <> struct fast_int<uint16_t> : t<uint_fast16_t> {};
    template <> struct fast_int<uint32_t> : t<uint_fast32_t> {};
    template <> struct fast_int<uint64_t> : t<uint_fast64_t> {};

    template <> struct fast_int<int8_t>  : t<int_fast8_t> {};
    template <> struct fast_int<int16_t> : t<int_fast16_t> {};
    template <> struct fast_int<int32_t> : t<int_fast32_t> {};
    template <> struct fast_int<int64_t> : t<int_fast64_t> {};

}

#endif // UTILS_H
