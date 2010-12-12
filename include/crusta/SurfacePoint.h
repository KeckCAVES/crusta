#ifndef _Crusta_SurfacePoint_H_
#define _Crusta_SurfacePoint_H_


#include <crusta/TreeIndex.h>


BEGIN_CRUSTA


struct SurfacePoint
{
    SurfacePoint();
    SurfacePoint(const Point3& position_, const TreeIndex& nodeIndex_,
                 const Point2i& cellIndex_, const Point2& cellPosition_);

    bool isValid() const;

    Point3    position;
    TreeIndex nodeIndex;
    Point2i   cellIndex;
    Point2    cellPosition;
};


END_CRUSTA


#endif //_Crusta_SurfacePoint_H_
