#ifndef PIXEL_H
#define PIXEL_H

namespace P3D
{
    template <const unsigned int RBits = 8, const unsigned int GBits = 8, const unsigned int BBits = 8, class TStorageType = unsigned int> class Pixel
    {
    public:

        constexpr Pixel(const Pixel& r)         : p(r.p)    {}
        constexpr Pixel(const TStorageType v)   : p(v)      {}

        constexpr operator TStorageType() const {return p;}

        constexpr Pixel(const unsigned int r, const unsigned int g, const unsigned int b)
        {
            Set(r, g, b);
        }

        constexpr unsigned int R() const
        {
            return (p & rmask()) >> rshift();
        }

        constexpr unsigned int G() const
        {
            return (p & gmask()) >> gshift();
        }

        constexpr unsigned int B() const
        {
            return (p & bmask()) >> bshift();
        }

        constexpr void Set(unsigned int r, unsigned int g, unsigned int b)
        {
            p = (
                    ((r << rshift()) & rmask()) |
                    ((g << gshift()) & gmask()) |
                    ((b << bshift()) & bmask())
                );
        }

    private:

        constexpr unsigned int rmask() const
        {
            return ((1 << RBits)-1) << rshift();
        }

        constexpr unsigned int gmask() const
        {
            return ((1 << GBits)-1) << gshift();
        }

        constexpr unsigned int bmask() const
        {
            return ((1 << BBits)-1) << bshift();
        }

        constexpr unsigned int rshift() const
        {
            return 0;
        }

        constexpr unsigned int gshift() const
        {
            return RBits;
        }

            constexpr unsigned int bshift() const
        {
            return (GBits + RBits);
        }

        TStorageType p;
    };

    typedef Pixel<8,8,8,unsigned int>   RGB888; //24bit color
    typedef Pixel<5,6,5,unsigned short> RGB565; //16bit color
    typedef Pixel<5,5,5,unsigned short> RGB555; //15bit color
}
#endif // PIXEL_H
