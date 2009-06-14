#ifndef _Filter_H_
#define _Filter_H_

#include <crusta/basics.h>

BEGIN_CRUSTA

class Filter
{
public:
    typedef double Scalar;

    Filter();
    ~Filter();

    ///retrieve the width of the filter (in pixels)
    uint getFilterWidth();
    ///provide filtered lookup from domain with given row length
    template <typename PixelParam>
    PixelParam lookup(PixelParam* at, uint rowLen);

    static void makePointFilter(Filter& filter);
    static void makeTriangleFilter(Filter& filter);
    static void makeFiveLobeLanczosFilter(Filter& filter);

protected:
    ///the width of the filter
    uint width;
    ///the weights of the filter
    Scalar* weights;
};

END_CRUSTA

#endif //_Filter_H_
