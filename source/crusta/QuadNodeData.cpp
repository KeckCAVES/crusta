#include <crusta/QuadNodeData.h>

#include <cassert>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/DataManager.h>


BEGIN_CRUSTA


NodeData::Tile::
Tile() :
    dataId(~0), node(INVALID_TILEINDEX)
{
    for (int i=0; i<4; ++i)
        children[i] = INVALID_TILEINDEX;
}


NodeData::
NodeData() :
    lineInheritCoverage(false), lineNumSegments(0), lineDataStamp(0),
    index(TreeIndex::invalid),
    boundingAge(0), boundingCenter(0,0,0), boundingRadius(0)
{
    centroid[0] = centroid[1] = centroid[2] = DemHeight::Type(0.0);
    elevationRange[0] =  Math::Constants<DemHeight::Type>::max;
    elevationRange[1] = -Math::Constants<DemHeight::Type>::max;
}

void NodeData::
computeBoundingSphere(Scalar radius, Scalar verticalScale)
{
    DemHeight::Type range[2];
    getElevationRange(range);

    DemHeight::Type avgElevation = (range[0] + range[1]);
    avgElevation                *= DemHeight::Type(0.5)* verticalScale;

    boundingCenter = scope.getCentroid(radius + avgElevation);

    boundingRadius = Scope::Scalar(0);
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<2; ++j)
        {
            Scope::Vertex corner = scope.corners[i];
            Scope::Scalar norm   = Scope::Scalar(radius);
            norm += range[j]*verticalScale;
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
    boundingAge = CURRENT_FRAME;
}

void NodeData::
init(Scalar radius, Scalar verticalScale)
{
    //compute the centroid
    Scope::Vertex scopeCentroid = scope.getCentroid(radius);
    centroid[0] = scopeCentroid[0];
    centroid[1] = scopeCentroid[1];
    centroid[2] = scopeCentroid[2];

    //update the bounding sphere
    computeBoundingSphere(radius, verticalScale);
}

void NodeData::
getElevationRange(DemHeight::Type range[2]) const
{
    if (elevationRange[0]== Math::Constants<DemHeight::Type>::max ||
        elevationRange[1]==-Math::Constants<DemHeight::Type>::max)
    {
        range[0] = SETTINGS->terrainDefaultHeight;
        range[1] = SETTINGS->terrainDefaultHeight;
    }
    else
    {
        range[0] = elevationRange[0];
        range[1] = elevationRange[1];
    }
}

DemHeight::Type NodeData::
getHeight(const DemHeight::Type& test) const
{
    if (test == DATAMANAGER->getDemNodata())
        return SETTINGS->terrainDefaultHeight;
    else
        return test;
}

TextureColor::Type NodeData::
getColor(const TextureColor::Type& test) const
{
    if (test == DATAMANAGER->getColorNodata())
    {
        const Color& tdc = SETTINGS->terrainDefaultColor;
        return TextureColor::Type(255*tdc[0], 255*tdc[1], 255*tdc[2]);
    }
    else
        return test;
}

LayerDataf::Type NodeData::
getLayerData(const LayerDataf::Type& test) const
{
    if (test == DATAMANAGER->getLayerfNodata())
        return SETTINGS->terrainDefaultLayerfData;
    else
        return test;
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


std::ostream& operator<<(std::ostream&os, const SubRegion& sub)
{
    os << "Sub(" << sub.offset[0] << ", " << sub.offset[1] << ", " <<
          sub.offset[2] << " | " << sub.size[0] << ", " << sub.size[1] <<
          ", " << sub.size[2] << ")";
    return os;
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
