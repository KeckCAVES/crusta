#ifndef _PixelOps_H_
#define _PixelOps_H_


#include <crusta/basics.h>


BEGIN_CRUSTA


template <typename PixelParam>
struct PixelOps
{
    /** average two values */
    static PixelParam average(const PixelParam& a, const PixelParam& b,
                              const PixelParam& nodata);
    /** average four values */
    static PixelParam average(const PixelParam& a, const PixelParam& b,
                              const PixelParam& c, const PixelParam& d,
                              const PixelParam& nodata);
    /** compute the minimum of two values */
    static PixelParam minimum(const PixelParam& a, const PixelParam& b,
                              const PixelParam& nodata);
    /** compute the maximum of two values */
    static PixelParam maximum(const PixelParam& a, const PixelParam& b,
                              const PixelParam& nodata);
};


END_CRUSTA


#include <crusta/DemHeightPixelOps.h>
#include <crusta/TextureColorPixelOps.h>


#endif //_PixelOps_H_
