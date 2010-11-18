#ifndef _SubsampleFilter_H_
#define _SubsampleFilter_H_


#include <crusta/basics.h>


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


template <typename PixelParam, SubsampleFilterType FilterParam>
struct SubsampleFilter
{
    /** width of the (static) subsampling filter */
    static int width();
    
    /** arbitrary filtered lookups into two dimensional pixel domains */
    static PixelParam sample(
        const PixelParam* img, const int origin[2],
        const double at[2], const int size[2],
        const PixelParam& imgNodata, const PixelParam& defaultValue,
        const PixelParam& globeNodata);
    
    /** fixed subsampled lookup (requires knowledge on the row length of the
        domain) */
    static PixelParam sample(PixelParam* at, int rowLen,
                             const PixelParam& globeNodata);
};


END_CRUSTA


#include <construo/DemHeightSubsampleFilter.h>
#include <construo/TextureColorSubsampleFilter.h>


#endif //_SubsampleFilter_H_
