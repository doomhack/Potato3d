#ifndef FP_H
#define FP_H

#include <limits>
#include "divide.h"

//#define OVERFLOW_CHECK

#ifdef OVERFLOW_CHECK
    #include <iostream>
#endif

namespace P3D
{
    template<const unsigned int fracbits> class FP
    {
    public:
        constexpr FP()                                      {}

        constexpr FP(const FP& r)   : n(r.n)                {}
        constexpr FP(const int v)   : n(v << fracbits)      {}
        constexpr FP(const unsigned int v)  : n(v << fracbits)      {}
        constexpr FP(const float v) : n((int)(v * one))     {}
        constexpr FP(const double v): n((int)(v * one))     {}



        constexpr operator int() const                      {return i();}
        constexpr operator unsigned int() const             {return i();}
        constexpr operator float() const                    {return f();}

        constexpr int i() const                             {return n >> fracbits;}
        constexpr float f() const                           {return (float)n / one;}

        static constexpr FP fromFPInt(const int r)          {FP v(0); v.n = r; return v;}
        constexpr int toFPInt() const                       {return n;}

        constexpr int intMul(const int r) const             {return ((long long int)n * r) >> fracbits;}

        constexpr static int max()
        {
            return (std::numeric_limits<int>::max() >> fracbits);
        }

        constexpr static int min()
        {
            return (std::numeric_limits<int>::min() >> fracbits);
        }

        constexpr FP& operator=(const FP& r)
        {
            n = r.n;   return *this;
        }

        constexpr FP& operator=(const int r)
        {
    #ifdef OVERFLOW_CHECK

            if(r < min() || r > max())
            {
                std::cout << "int assign overflow: " <<  r;
            }
    #endif
            n = (r << fracbits);   return *this;
        }

        constexpr FP& operator=(const float r)
        {
    #ifdef OVERFLOW_CHECK

            if(r < min() || r > max())
            {
                std::cout << "float assign overflow: " <<  r;
            }
    #endif

            n = (int)(r * one);    return *this;
        }

        //Addition
        constexpr FP operator+(const FP& r) const           {FP v(r);   return v+=*this;}
        constexpr FP operator+(const int r) const           {FP v(r);   return v+=*this;}
        constexpr FP operator+(const float r) const         {FP v(r);   return v+=*this;}

        constexpr FP& operator+=(const FP& r)
        {
    #ifdef OVERFLOW_CHECK

            long long int tmp = r.n + n;

            if(tmp < std::numeric_limits<int>::min() || tmp > std::numeric_limits<int>::max())
            {
                std::cout << "addition overflow: " <<  r.i() << this->i();
            }
    #endif

            n += r.n;  return *this;
        }

        constexpr FP& operator+=(const int r)         {return *this+=FP(r);}
        constexpr FP& operator+=(const float r)       {return *this+=FP(r);}

        //Subtraction
        constexpr FP operator-(const FP& r) const
        {

    #ifdef OVERFLOW_CHECK

            long long int tmp = n - r.n;

            if(tmp < std::numeric_limits<int>::min() || tmp > std::numeric_limits<int>::max())
            {
                std::cout << "subtraction overflow: " <<  r.i() << this->i();
            }
    #endif

            FP v(r); v.n = n - v.n;  return v;
        }


        constexpr FP operator-(const int r) const           {FP v(r); return *this-v;}
        constexpr FP operator-(const float r) const         {FP v(r); return *this-v;}


        constexpr FP& operator-=(const FP& r)               {n -= r.n;  return *this;}
        constexpr FP& operator-=(const int& r)              {return *this-=FP(r);}
        constexpr FP& operator-=(const float& r)            {return *this-=FP(r);}

        constexpr FP operator-() const                      {FP r(0); r.n = -n; return r;}

        //Multiply
        constexpr FP operator*(const FP& r) const          {FP v(r);   return v*=*this;}
        constexpr FP operator*(const int r) const          {FP v(r);   return v*=*this;}
        constexpr FP operator*(const float r) const        {FP v(r);   return v*=*this;}

        constexpr FP& operator*=(const FP& r)
        {
            const long long int tmp = (((const long long int)n * r.n) >> fracbits);

    #ifdef OVERFLOW_CHECK

            if(tmp < std::numeric_limits<int>::min() || tmp > std::numeric_limits<int>::max())
            {
                std::cout << "multiply overflow: " <<  r.i() << this->i() << r.i() * this->i();
            }
    #endif

            n = (int)tmp;
            return *this;
        }

        constexpr FP& operator*=(const int& r)        {return *this*=FP(r);}
        constexpr FP& operator*=(const float& r)      {return *this*=FP(r);}

        //Divide
        constexpr FP operator/(const FP& r) const
        {
            FP v(r); v.n = FixedDiv(n, r.n, fracbits);
            return v;
        }

        constexpr FP operator/(const int r) const       {FP v(r);   return *this/v;}
        constexpr FP operator/(const float r) const     {FP v(r);   return *this/v;}

        constexpr FP& operator/=(const FP& r)
        {
            n = FixedDiv(n, r.n, fracbits);
            return *this;
        }

        constexpr FP& operator/=(const int& r)              {return *this/=FP(r);}
        constexpr FP& operator/=(const float& r)            {return *this/=FP(r);}

        //Equality
        constexpr bool operator==(const FP& r) const        {return n == r.n;}
        constexpr bool operator==(const int& r) const       {return *this==FP(r);}
        constexpr bool operator==(const float& r) const     {return *this==FP(r);}

        //Inequality
        constexpr bool operator!=(const FP& r) const        {return n != r.n;}
        constexpr bool operator!=(const int& r) const       {return *this!=FP(r);}
        constexpr bool operator!=(const float& r) const     {return *this!=FP(r);}

        //Less than
        constexpr bool operator<(const FP& r) const         {return n < r.n;}
        constexpr bool operator<(const int& r) const        {return *this<FP(r);}
        constexpr bool operator<(const float& r) const      {return *this<FP(r);}

        //Greater than
        constexpr bool operator>(const FP& r) const         {return n > r.n;}
        constexpr bool operator>(const int& r) const        {return *this>FP(r);}
        constexpr bool operator>(const float& r) const      {return *this>FP(r);}

        //Greater than=
        constexpr bool operator>=(const FP& r) const        {return n >= r.n;}
        constexpr bool operator>=(const int& r) const       {return *this>=FP(r);}
        constexpr bool operator>=(const float& r) const     {return *this>=FP(r);}

        //Less than=
        constexpr bool operator<=(const FP& r) const        {return n <= r.n;}
        constexpr bool operator<=(const int& r) const       {return *this<=FP(r);}
        constexpr bool operator<=(const float& r) const     {return *this<=FP(r);}

        //&
        constexpr FP operator&(const FP& r) const           {FP v(r); v.n &= n;  return v;}
        constexpr FP operator&(const int r) const           {FP v(r); return v&=*this;}

        constexpr FP& operator&=(const FP& r)               {n &= r.n;  return *this;}
        constexpr FP& operator&=(const int& r)              {return *this&=FP(r);}

        //|
        constexpr FP operator|(const FP& r) const           {FP v(r); v.n |= n;  return v;}
        constexpr FP operator|(const int r) const           {FP v(r);   return v|=*this;}
        constexpr FP& operator|=(const FP& r)               {n |= r.n;  return *this;}
        constexpr FP& operator|=(const int& r)              {return *this|=FP(r);}

        //^
        constexpr FP operator^(const FP& r) const           {FP v(r); v.n ^= n;  return v;}
        constexpr FP operator^(const int r) const           {FP v(r);   return v^=*this;}
        constexpr FP& operator^=(const FP& r)               {n ^= r.n;  return *this;}
        constexpr FP& operator^=(const int& r)              {return *this^=FP(r);}

        //<<
        constexpr FP operator<<(const int r) const {FP v(*this);   return v<<=r;}
        constexpr FP& operator<<=(const int r)     {n <<= r;  return *this;}

        //>>
        constexpr FP operator>>(const int r) const {FP v(*this);   return v>>=r;}
        constexpr FP& operator>>=(const int r)     {n >>= r;  return *this;}

    private:
        int n;

        static constexpr int one = (1 << fracbits);
    };

    typedef FP<16>  FP16;   //s + 15 + 16 (-32768 to +32767) (0.000015)
    typedef FP<8>   FP8;    //s + 23 + 8  (-8,388,608 to +8,388,608) (0.039)
    typedef FP<24>  FP24;   //s + 7 + 24  (-128 to +127) (0.0000000596)
}

template<const unsigned int fracbits>
struct std::numeric_limits<P3D::FP<fracbits>> : public std::numeric_limits<int>
{
    static constexpr bool is_specialized = true;
    static constexpr P3D::FP<fracbits> min() noexcept { return P3D::FP<fracbits>::fromFPInt(std::numeric_limits<int>::min()); }
    static constexpr P3D::FP<fracbits> max() noexcept { return P3D::FP<fracbits>::fromFPInt(std::numeric_limits<int>::max()); }
    static constexpr P3D::FP<fracbits> lowest() noexcept { return min(); }
    static constexpr P3D::FP<fracbits> epsilon() noexcept { return P3D::FP<fracbits>::fromFPInt(1); }
};

#endif // FP_H
