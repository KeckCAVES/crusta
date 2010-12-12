#include <crusta/SurfacePoint.h>


BEGIN_CRUSTA


SurfacePoint::
SurfacePoint() :
    nodeIndex(TreeIndex::invalid)
{
}

SurfacePoint::
SurfacePoint(const Point3& position_, const TreeIndex& nodeIndex_,
             const Point2i& cellIndex_, const Point2& cellPosition_) :
    position(position_), nodeIndex(nodeIndex_),
    cellIndex(cellIndex_), cellPosition(cellPosition_)
{
}


bool SurfacePoint::
isValid() const
{
    return nodeIndex != TreeIndex::invalid;
}


END_CRUSTA
