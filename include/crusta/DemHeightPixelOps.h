#ifndef _DemHeightPixelOps_H_
#define _DemHeightPixelOps_H_


#include <crusta/PixelOps.h>

#include <crusta/DemHeight.h>

BEGIN_CRUSTA


template <>
struct PixelOps<DemHeight>
{
    static DemHeight average(const DemHeight& a, const DemHeight& b,
                             const DemHeight& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;
        else
            return (a+b) * DemHeight(0.5);
    }

    static DemHeight average(const DemHeight& a, const DemHeight& b,
                             const DemHeight& c, const DemHeight& d,
                             const DemHeight& nodata)
    {
        const DemHeight p[4] = {a,b,c,d};
        
        DemHeight sum = 0;
        DemHeight sumWeights = 0;
        for (int i=0; i<4; ++i)
        {
            if (p[i] != nodata)
            {
                sum        += p[i] * DemHeight(0.25f);
                sumWeights += DemHeight(0.25f);
            }
        }

        if (sumWeights == 0)
            return nodata;
        else
            return sum / sumWeights;
    }

    static DemHeight minimum(const DemHeight& a, const DemHeight& b,
                             const DemHeight& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;
        else
            return std::min(a, b);
    }

    static DemHeight maximum(const DemHeight& a, const DemHeight& b,
                             const DemHeight& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;
        else
            return std::max(a, b);
    }
};


END_CRUSTA


#endif //_DemHeightPixelOps_H_
