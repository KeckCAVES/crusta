#include <crusta/QuadNodeData.h>

#include <cassert>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>

BEGIN_CRUSTA

NodeData::
NodeData() :
    lineCoverageDirty(false), lineCoverageAge(0), lineNumSegments(0),
    index(TreeIndex::invalid),
    boundingAge(0), boundingCenter(0,0,0), boundingRadius(0)
{
    demTile   = DemFile::INVALID_TILEINDEX;
    colorTile = ColorFile::INVALID_TILEINDEX;
    for (int i=0; i<4; ++i)
    {
        childDemTiles[i]   = DemFile::INVALID_TILEINDEX;
        childColorTiles[i] = ColorFile::INVALID_TILEINDEX;
    }

    centroid[0] = centroid[1] = centroid[2] = DemHeight(0.0);
    elevationRange[0] = elevationRange[1]   = DemHeight(0.0);
}

void NodeData::
computeBoundingSphere(Scalar radius, Scalar verticalScale)
{
    DemHeight avgElevation = (elevationRange[0] + elevationRange[1]);
    avgElevation          *= DemHeight(0.5)* verticalScale;

    boundingCenter = scope.getCentroid(radius + avgElevation);

    boundingRadius = Scope::Scalar(0);
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<2; ++j)
        {
            Scope::Vertex corner = scope.corners[i];
            Scope::Scalar norm   = Scope::Scalar(radius);
            norm += elevationRange[j]*verticalScale;
            norm /= sqrt(corner[0]*corner[0] + corner[1]*corner[1] +
                         corner[2]*corner[2]);
            Scope::Vertex toCorner;
            toCorner[0] = corner[0]*norm - boundingCenter[0];
            toCorner[1] = corner[1]*norm - boundingCenter[1];
            toCorner[2] = corner[2]*norm - boundingCenter[2];
            norm = sqrt(toCorner[0]*toCorner[0] + toCorner[1]*toCorner[1] +
                        toCorner[2]*toCorner[2]);
            boundingRadius = std::max(boundingRadius, norm);
        }
    }

    //stamp the current bounding specification
    boundingAge = Vrui::getApplicationTime();
}

void NodeData::
init(Scalar radius, Scalar verticalScale)
{
    //compute the centroid on the average elevation (see split)
    Scope::Vertex scopeCentroid = scope.getCentroid(radius);
    centroid[0] = scopeCentroid[0];
    centroid[1] = scopeCentroid[1];
    centroid[2] = scopeCentroid[2];

    computeBoundingSphere(radius, verticalScale);
}


SubRegion::
SubRegion()
{
}

SubRegion::
SubRegion(const Point3f& iOffset, const Vector2f& iSize) :
    offset(iOffset), size(iSize)
{
}


StampedSubRegion::
StampedSubRegion()
{
}

StampedSubRegion::
StampedSubRegion(const Point3f& iOffset, const Vector2f& iSize) :
    SubRegion(iOffset, iSize), age(0)
{
}


std::ostream& operator<<(std::ostream& os,
                         const NodeData::ShapeCoverage& cov)
{
    for (NodeData::ShapeCoverage::const_iterator lit=cov.begin();
         lit!=cov.end(); ++lit)
    {
        os << "-line " << lit->first->getId() << " XX ";
        const Shape::ControlPointHandleList& handles = lit->second;
        for (Shape::ControlPointHandleList::const_iterator
             hit=handles.begin(); hit!=handles.end(); ++hit)
        {
            if (hit==handles.begin())
                os << *hit;
            else
                os << " | " << *hit;
        }
        os << "\n";
    }

    return os;
}


END_CRUSTA
