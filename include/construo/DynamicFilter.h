#ifndef _DynamicFilter_H_
#define _DynamicFilter_H_

#include <crusta/basics.h>

BEGIN_CRUSTA

template <typename PixelParam>
struct Nodata;

namespace filter {
   typedef double Scalar;

    ///provide dynamic filtered lookup from a pixel domain
    template <typename PixelParam>
    PixelParam nearestLookup(const PixelParam* img,
        const int origin[2], const Scalar at[2], const int size[2],
        const Nodata<PixelParam>& nodata, const PixelParam& defaultValue);

    template <typename PixelParam>
    PixelParam linearLookup(const PixelParam* img,
        const int origin[2], const Scalar at[2], const int size[2],
        const Nodata<PixelParam>& nodata, const PixelParam& defaultValue);
}

END_CRUSTA

#endif //_DynamicFilter_H_
