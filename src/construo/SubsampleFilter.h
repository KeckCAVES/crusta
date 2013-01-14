#ifndef _SubsampleFilter_H_
#define _SubsampleFilter_H_


#include <crustacore/basics.h>


BEGIN_CRUSTA


/*- fixed filtered lookups into two dimensional pixel domains. The lookups must
 be aligned with the centers of the pixels in the domain. They are used for
 subsampling. */
enum SubsampleFilterType
{
    SUBSAMPLEFILTER_POINT,
    SUBSAMPLEFILTER_PYRAMID,
    SUBSAMPLEFILTER_LANCZOS5
};


template <typename PixelType, SubsampleFilterType FilterParam>
struct SubsampleFilter
{
    /** width of the (static) subsampling filter */
    static int width();

    /** arbitrary filtered lookups into two dimensional pixel domains */
    static PixelType sample(
        const PixelType* img, const int origin[2],
        const double at[2], const int size[2],
        const PixelType& imgNodata, const PixelType& defaultValue,
        const PixelType& globeNodata);

    /** fixed subsampled lookup (requires knowledge on the row length of the
        domain) */
    static PixelType sample(PixelType* at, int rowLen,
                             const PixelType& globeNodata);
};


END_CRUSTA


#include <construo/SubsampleFilter.hpp>


#endif //_SubsampleFilter_H_
