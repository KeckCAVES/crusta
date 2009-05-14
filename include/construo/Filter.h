#ifndef _Filter_H_
#define _Filter_H_

#include <construo/GeometryTypes.h>

BEGIN_CRUSTA

class Filter
{
public:
    typedef Point::Scalar Scalar;

    Filter();
    ~Filter();

    ///retrieve the width of the filter (in pixels)
    uint getFilterWidth();
    ///provide filtered lookup from domain with given row length
    template <typename PixelParam>
    PixelParam lookup(PixelParam* at, uint rowLen);

    static Filter createPointFilter();
    static Filter createTriangleFilter();
    static Filter createFiveLobeLanczosFilter();

protected:
    ///the width of the filter
    uint width;
    ///the weights of the filter
    Scalar* weights;
};

END_CRUSTA

#include <construo/Filter.hpp>

#endif //_Filter_H_
