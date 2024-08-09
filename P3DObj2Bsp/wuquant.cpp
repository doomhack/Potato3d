#include "wuquant.h"

namespace Obj2Bsp
{

    WuQuant::WuQuant() {}

    QImage WuQuant::QuantizeImage(QImage imageIn, int colors)
    {
        K = colors = std::min(colors, 256);

        size = imageIn.width() * imageIn.height();

        Qadd = new unsigned short[size];

        //input R,G,B components into Ir, Ig, Ib;
        Ir = new unsigned char[size];
        Ig = new unsigned char[size];
        Ib = new unsigned char[size];

        for(int y = 0; y < imageIn.height(); y++)
        {
            for(int x = 0; x < imageIn.width(); x++)
            {
                QColor p = QColor(imageIn.pixel(x,y));

                Ir[(y*imageIn.width()) + x] = p.red();
                Ig[(y*imageIn.width()) + x] = p.green();
                Ib[(y*imageIn.width()) + x] = p.blue();
            }
        }

        K = 256;

        Hist3d(&wt[0][0][0], &mr[0][0][0], &mg[0][0][0], &mb[0][0][0], &m2[0][0][0]);
        printf("Histogram done\n");

        delete[] Ig;
        delete[] Ib;
        delete[] Ir;

        M3d(&wt[0][0][0], &mr[0][0][0], &mg[0][0][0], &mb[0][0][0], &m2[0][0][0]);
        printf("Moments done\n");

        struct box cube[MAXCOLOR];

        cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
        cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;

        int next = 0;
        float vv[MAXCOLOR];

        for(int i = 1; i < K; ++i)
        {
            if (Cut(&cube[next], &cube[i]))
            {
                /* volume test ensures we won't try to cut one-cell box */
                vv[next] = (cube[next].vol>1) ? Var(&cube[next]) : 0.0;
                vv[i] = (cube[i].vol>1) ? Var(&cube[i]) : 0.0;
            }
            else
            {
                vv[next] = 0.0;   /* don't try to split this box again */
                i--;              /* didn't create box i */
            }

            next = 0;
            float temp = vv[0];

            for(int k = 1; k <= i; ++k)
            {
                if (vv[k] > temp)
                {
                    temp = vv[k]; next = k;
                }
            }

            if (temp <= 0.0)
            {
                K = i+1;
                fprintf(stderr, "Only got %d boxes\n", K);
                break;
            }
        }

        printf("Partition done\n");

        /* the space for array m2 can be freed now */

        unsigned char *tag = new unsigned char[33*33*33];

        unsigned char lut_r[MAXCOLOR], lut_g[MAXCOLOR], lut_b[MAXCOLOR];

        for(int k=0; k<K; ++k)
        {
            Mark(&cube[k], k, tag);

            int weight = Vol(&cube[k], wt);

            if (weight)
            {
                lut_r[k] = Vol(&cube[k], mr) / weight;
                lut_g[k] = Vol(&cube[k], mg) / weight;
                lut_b[k] = Vol(&cube[k], mb) / weight;
            }
            else
            {
                fprintf(stderr, "bogus box %d\n", k);
                lut_r[k] = lut_g[k] = lut_b[k] = 0;
            }
        }

        for(int i = 0; i < size; ++i)
        {
            Qadd[i] = tag[Qadd[i]];
        }

        /* output lut_r, lut_g, lut_b as color look-up table contents,
        Qadd as the quantized image (array of table addresses). */

        QImage image256 = QImage(imageIn.width(), imageIn.height(), QImage::Format_Indexed8);

        QList<QRgb> colorTable;

        for(int i = 0; i < 256; i++)
        {
            colorTable.append(QColor(lut_r[i], lut_g[i], lut_b[i]).rgb());
        }

        image256.setColorTable(colorTable);

        for(int y = 0; y < imageIn.height(); y++)
        {
            for(int x = 0; x < imageIn.width(); x++)
            {
                image256.setPixel(x, y, Qadd[(y*imageIn.width()) + x]);
            }
        }

        return image256;
    }

    void WuQuant::Hist3d(int* vwt, int* vmr, int* vmg, int* vmb, float* m2)
    {
        /* build 3-D color histogram of counts, r/g/b, c^2 */
        int ind;
        int	table[256];

        for(int i = 0; i < 256; ++i)
        {
            table[i] = i*i;
        }

        for(int i = 0; i < size; ++i)
        {
            int r = Ir[i];
            int g = Ig[i];
            int b = Ib[i];

            int inr=(r>>3)+1;
            int ing=(g>>3)+1;
            int inb=(b>>3)+1;

            Qadd[i]=ind=(inr<<10)+(inr<<6)+inr+(ing<<5)+ing+inb;

            /*[inr][ing][inb]*/
            ++vwt[ind];
            vmr[ind] += r;
            vmg[ind] += g;
            vmb[ind] += b;
            m2[ind] += (float)(table[r]+table[g]+table[b]);
        }
    }

    void WuQuant::M3d(int* vwt, int* vmr, int* vmg, int* vmb, float* m2)
    {
        /* compute cumulative moments. */
        int area[33];
        int area_r[33];
        int area_g[33];
        int area_b[33];

        float area2[33];

        for(unsigned char r = 1; r <= 32; ++r)
        {
            for(int i=0; i<=32; ++i)
            {
                area2[i]=area[i]=area_r[i]=area_g[i]=area_b[i]=0;
            }

            for(unsigned char g = 1; g <= 32; ++g)
            {
                int line = 0, line_r = 0, line_g = 0, line_b = 0;
                float line2 = 0.0f;


                for(unsigned char b=1; b<=32; ++b)
                {
                    unsigned short ind1 = (r<<10) + (r<<6) + r + (g<<5) + g + b; /* [r][g][b] */
                    line += vwt[ind1];
                    line_r += vmr[ind1];
                    line_g += vmg[ind1];
                    line_b += vmb[ind1];
                    line2 += m2[ind1];
                    area[b] += line;
                    area_r[b] += line_r;
                    area_g[b] += line_g;
                    area_b[b] += line_b;
                    area2[b] += line2;

                    unsigned short ind2 = ind1 - 1089; /* [r-1][g][b] */
                    vwt[ind1] = vwt[ind2] + area[b];
                    vmr[ind1] = vmr[ind2] + area_r[b];
                    vmg[ind1] = vmg[ind2] + area_g[b];
                    vmb[ind1] = vmb[ind2] + area_b[b];
                    m2[ind1] = m2[ind2] + area2[b];
                }
            }
        }
    }

    int WuQuant::Cut(struct box* set1, struct box* set2)
    {
        int whole_r = Vol(set1, mr);
        int whole_g = Vol(set1, mg);
        int whole_b = Vol(set1, mb);
        int whole_w = Vol(set1, wt);

        int cutr = 0, cutg = 0, cutb = 0;

        float maxr = Maximize(set1, RED, set1->r0+1, set1->r1, &cutr, whole_r, whole_g, whole_b, whole_w);
        float maxg = Maximize(set1, GREEN, set1->g0+1, set1->g1, &cutg, whole_r, whole_g, whole_b, whole_w);
        float maxb = Maximize(set1, BLUE, set1->b0+1, set1->b1, &cutb, whole_r, whole_g, whole_b, whole_w);

        unsigned char dir = 0;

        if( (maxr>=maxg)&&(maxr>=maxb) )
        {
            dir = RED;

            if (cutr < 0)
                return 0; /* can't split the box */
        }
        else
        {
            if( (maxg>=maxr)&&(maxg>=maxb) )
                dir = GREEN;
            else
                dir = BLUE;
        }

        set2->r1 = set1->r1;
        set2->g1 = set1->g1;
        set2->b1 = set1->b1;

        switch (dir)
        {
            case RED:
                set2->r0 = set1->r1 = cutr;
                set2->g0 = set1->g0;
                set2->b0 = set1->b0;
                break;
            case GREEN:
                set2->g0 = set1->g1 = cutg;
                set2->r0 = set1->r0;
                set2->b0 = set1->b0;
                break;
            case BLUE:
                set2->b0 = set1->b1 = cutb;
                set2->r0 = set1->r0;
                set2->g0 = set1->g0;
                break;
        }

        set1->vol=(set1->r1-set1->r0)*(set1->g1-set1->g0)*(set1->b1-set1->b0);
        set2->vol=(set2->r1-set2->r0)*(set2->g1-set2->g0)*(set2->b1-set2->b0);
        return 1;
    }

    float WuQuant::Var(struct box *cube)
    {
        /* Compute the weighted variance of a box */
        /* NB: as with the raw statistics, this is really the variance * size */

        float dr, dg, db, xx;

        dr = Vol(cube, mr);
        dg = Vol(cube, mg);
        db = Vol(cube, mb);
        xx =  m2[cube->r1][cube->g1][cube->b1]
             -m2[cube->r1][cube->g1][cube->b0]
             -m2[cube->r1][cube->g0][cube->b1]
             +m2[cube->r1][cube->g0][cube->b0]
             -m2[cube->r0][cube->g1][cube->b1]
             +m2[cube->r0][cube->g1][cube->b0]
             +m2[cube->r0][cube->g0][cube->b1]
             -m2[cube->r0][cube->g0][cube->b0];

        return( xx - (dr*dr+dg*dg+db*db)/(float)Vol(cube,wt) );
    }

    void WuQuant::Mark(struct box *cube, int label, unsigned char *tag)
    {
        for(int r = cube->r0 + 1; r <= cube->r1; ++r)
        {
            for(int g = cube->g0 + 1; g <= cube->g1; ++g)
            {
                for(int b = cube->b0 + 1; b <= cube->b1; ++b)
                {
                    tag[(r<<10) + (r<<6) + r + (g<<5) + g + b] = label;
                }
            }
        }
    }

    int WuQuant::Vol(struct box *cube, int mmt[33][33][33])
    {
        /* Compute sum over a box of any given statistic */

        return( mmt[cube->r1][cube->g1][cube->b1]
                -mmt[cube->r1][cube->g1][cube->b0]
                -mmt[cube->r1][cube->g0][cube->b1]
                +mmt[cube->r1][cube->g0][cube->b0]
                -mmt[cube->r0][cube->g1][cube->b1]
                +mmt[cube->r0][cube->g1][cube->b0]
                +mmt[cube->r0][cube->g0][cube->b1]
                -mmt[cube->r0][cube->g0][cube->b0] );
    }

    float WuQuant::Maximize(struct box *cube, unsigned char dir, int first, int last, int* cut, int whole_r, int whole_g, int whole_b, int whole_w)
    {
        int base_r = Bottom(cube, dir, mr);
        int base_g = Bottom(cube, dir, mg);
        int base_b = Bottom(cube, dir, mb);
        int base_w = Bottom(cube, dir, wt);

        float temp = 0.0f;
        float max = 0.0f;
        *cut = -1;

        for(int i = first; i < last; ++i)
        {
            int half_r = base_r + Top(cube, dir, i, mr);
            int half_g = base_g + Top(cube, dir, i, mg);
            int half_b = base_b + Top(cube, dir, i, mb);
            int half_w = base_w + Top(cube, dir, i, wt);

            /* now half_x is sum over lower half of box, if split at i */
            if (half_w == 0)
            {
                continue;             /* never split into an empty box */
            }
            else
            {
                temp = ((float)half_r*half_r + (float)half_g*half_g + (float)half_b*half_b)/half_w;
            }

            half_r = whole_r - half_r;
            half_g = whole_g - half_g;
            half_b = whole_b - half_b;
            half_w = whole_w - half_w;

            if (half_w == 0)
            {
                continue;             /* never split into an empty box */
            }
            else
                temp += ((float)half_r*half_r + (float)half_g*half_g + (float)half_b*half_b)/half_w;

            if (temp > max)
            {
                max=temp; *cut=i;
            }
        }

        return(max);
    }

    int WuQuant::Bottom(struct box *cube, unsigned char dir, int mmt[33][33][33])
    {
        /* Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1 */
        /* (depending on dir) */

        switch(dir)
        {
            case RED:
                return( -mmt[cube->r0][cube->g1][cube->b1]
                        +mmt[cube->r0][cube->g1][cube->b0]
                        +mmt[cube->r0][cube->g0][cube->b1]
                        -mmt[cube->r0][cube->g0][cube->b0] );
                break;

            case GREEN:
                return( -mmt[cube->r1][cube->g0][cube->b1]
                        +mmt[cube->r1][cube->g0][cube->b0]
                        +mmt[cube->r0][cube->g0][cube->b1]
                        -mmt[cube->r0][cube->g0][cube->b0] );
                break;

            case BLUE:
                return( -mmt[cube->r1][cube->g1][cube->b0]
                        +mmt[cube->r1][cube->g0][cube->b0]
                        +mmt[cube->r0][cube->g1][cube->b0]
                        -mmt[cube->r0][cube->g0][cube->b0] );
                break;
        }

        return 0;
    }

    int WuQuant::Top(struct box *cube, unsigned char dir, int   pos, int mmt[33][33][33])
    {
        /* Compute remainder of Vol(cube, mmt), substituting pos for */
        /* r1, g1, or b1 (depending on dir) */

        switch(dir)
        {
            case RED:
                return( mmt[pos][cube->g1][cube->b1]
                        -mmt[pos][cube->g1][cube->b0]
                        -mmt[pos][cube->g0][cube->b1]
                        +mmt[pos][cube->g0][cube->b0] );
                break;

            case GREEN:
                return( mmt[cube->r1][pos][cube->b1]
                        -mmt[cube->r1][pos][cube->b0]
                        -mmt[cube->r0][pos][cube->b1]
                        +mmt[cube->r0][pos][cube->b0] );
                break;

            case BLUE:
                return( mmt[cube->r1][cube->g1][pos]
                        -mmt[cube->r1][cube->g0][pos]
                        -mmt[cube->r0][cube->g1][pos]
                        +mmt[cube->r0][cube->g0][pos] );
                break;
        }

        return 0;
    }
}


