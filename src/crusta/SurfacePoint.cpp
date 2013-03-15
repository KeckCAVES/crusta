#include <crusta/SurfacePoint.h>


namespace crusta {


SurfacePoint::
SurfacePoint() :
    nodeIndex(TreeIndex::invalid)
{
}

SurfacePoint::
SurfacePoint(const Geometry::Point<double,3>& position_, const TreeIndex& nodeIndex_,
             const Geometry::Point<int,2>& cellIndex_, const Geometry::Point<double,2>& cellPosition_) :
    position(position_), nodeIndex(nodeIndex_),
    cellIndex(cellIndex_), cellPosition(cellPosition_)
{
}


bool SurfacePoint::
isValid() const
{
    return nodeIndex != TreeIndex::invalid;
}


} //namespace crusta
