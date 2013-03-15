#ifndef _Crusta_SurfacePoint_H_
#define _Crusta_SurfacePoint_H_


#include <crustacore/TreeIndex.h>


BEGIN_CRUSTA


struct SurfacePoint
{
    SurfacePoint();
    SurfacePoint(const Geometry::Point<double,3>& position_, const TreeIndex& nodeIndex_,
                 const Geometry::Point<int,2>& cellIndex_, const Geometry::Point<double,2>& cellPosition_);

    bool isValid() const;

    Geometry::Point<double,3>    position;
    TreeIndex nodeIndex;
    Geometry::Point<int,2>   cellIndex;
    Geometry::Point<double,2>    cellPosition;
};


END_CRUSTA


#endif //_Crusta_SurfacePoint_H_
