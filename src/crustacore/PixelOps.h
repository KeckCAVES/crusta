#ifndef _PixelOps_H_
#define _PixelOps_H_


#include <crustacore/basics.h>


namespace crusta {


template <typename PixelType>
struct PixelOps
{
    /** average two values */
    static PixelType average(const PixelType& a, const PixelType& b,
                             const PixelType& nodata);
    /** average four values */
    static PixelType average(const PixelType& a, const PixelType& b,
                             const PixelType& c, const PixelType& d,
                             const PixelType& nodata);
    /** compute the minimum of two values */
    static PixelType minimum(const PixelType& a, const PixelType& b,
                             const PixelType& nodata);
    /** compute the maximum of two values */
    static PixelType maximum(const PixelType& a, const PixelType& b,
                             const PixelType& nodata);
};


} //namespace crusta


#include <crustacore/PixelOps.hpp>


#endif //_PixelOps_H_
