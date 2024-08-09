#ifndef WUQUANT_H
#define WUQUANT_H

#include <QtCore>
#include <QtGui>

namespace Obj2Bsp
{

/*
    Having received many constructive comments and bug reports about my previous
    C implementation of my color quantizer (Graphics Gems vol. II, p. 126-133),
    I am posting the following second version of my program (hopefully 100%
    healthy) as a reply to all those who are interested in the problem.
    */



    /**********************************************************************
        C Implementation of Wu's Color Quantizer (v. 2)
        (see Graphics Gems vol. II, pp. 126-133)

Author:	Xiaolin Wu
    Dept. of Computer Science
    Univ. of Western Ontario
    London, Ontario N6A 5B7
    wu@csd.uwo.ca

Algorithm: Greedy orthogonal bipartition of RGB space for variance
       minimization aided by inclusion-exclusion tricks.
       For speed no nearest neighbor search is done. Slightly
       better performance can be expected by more sophisticated
       but more expensive versions.

The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much of
additional documentation and a cure to a previous bug.

Free to distribute, comments and suggestions are appreciated.
**********************************************************************/

    struct box {
        int r0;			 /* min value, exclusive */
        int r1;			 /* max value, inclusive */
        int g0;
        int g1;
        int b0;
        int b1;
        int vol;
    };

    class WuQuant
    {
    public:
        WuQuant();

        QImage QuantizeImage(QImage imageIn);

    private:

        void Hist3d(int* vwt, int* vmr, int* vmg, int* vmb, float* m2);
        void M3d(int* vwt, int* vmr, int* vmg, int* vmb, float* m2);
        int Cut(struct box* set1, struct box* set2);
        float Var(struct box *cube);
        void Mark(struct box *cube, int label, unsigned char *tag);
        int Vol(struct box *cube, int mmt[33][33][33]);
        float Maximize(struct box *cube, unsigned char dir, int first, int last, int* cut, int whole_r, int whole_g, int whole_b, int whole_w);
        int Bottom(struct box *cube, unsigned char dir, int mmt[33][33][33]);
        int Top(struct box *cube, unsigned char dir, int   pos, int mmt[33][33][33]);



        /* Histogram is in elements 1..HISTSIZE along each axis,
         * element 0 is for base or marginal value
         * NB: these must start out 0!
         */
        float		m2[33][33][33];
        int	wt[33][33][33], mr[33][33][33],	mg[33][33][33],	mb[33][33][33];
        unsigned char   *Ir, *Ig, *Ib;
        int	        size; /*image size*/
        int		K;    /*color look-up table size*/
        unsigned short int *Qadd;

        static constexpr int MAXCOLOR = 256;
        static constexpr int RED = 2;
        static constexpr int GREEN = 1;
        static constexpr int BLUE = 0;
    };



}
#endif // WUQUANT_H
