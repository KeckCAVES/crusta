#include <crusta/map/Shape.h>

#include <stdexcept>

#include <Math/Constants.h>


BEGIN_CRUSTA
const Shape::Id Shape::BAD_ID(-1);


Shape::Id::
Id():
    raw(BAD_ID.raw)
{}

Shape::Id::
Id(int iRaw) :
    raw(iRaw)
{}

Shape::Id::
Id(int iUnique, int iType, int iIndex) :
    unique(iUnique), type(iType), index(iIndex)
{}

bool Shape::Id::
operator ==(const Id& other) const
{
    return raw == other.raw;
}

bool Shape::Id::
operator !=(const Id& other) const
{
    return raw != other.raw;
}


Shape::
~Shape()
{
}


Shape::Id Shape::
select(const Point3& pos, double& dist)
{
    double pointDist;
    Id pointId = selectPoint(pos, pointDist);
    double segmentDist;
    Id segmentId = selectSegment(pos, segmentDist);

    if (pointDist <= segmentDist)
    {
        dist = pointDist;
        return pointId;
    }
    else
    {
        dist = segmentDist;
        return segmentId;
    }
}

Shape::Id Shape::
selectPoint(const Point3& pos, double& dist)
{
    Id retId = BAD_ID;
    dist     = Math::Constants<double>::max;

    int numPoints = static_cast<int>(controlPoints.size());
    for (int i=0; i<numPoints; ++i)
    {
        double newDist = Geometry::dist(controlPoints[i], pos);
        if (newDist < dist)
        {
            retId.unique = getUnique();
            retId.type   = CONTROL_POINT;
            retId.index  = i;
            dist         = newDist;
        }
    }

    return retId;
}

Shape::Id Shape::
selectSegment(const Point3& pos, double& dist)
{
    Id retId = BAD_ID;
    dist     = Math::Constants<double>::max;

    int numSegments = static_cast<int>(controlPoints.size()) - 1;
    for (int i=0; i<numSegments; ++i)
    {
        const Point3& start = controlPoints[i];
        const Point3& end   = controlPoints[i+1];

        Vector3 seg   = end - start;
        Vector3 toPos = pos - start;

        double segSqrLen = seg*seg;
        double u = (toPos * seg) / segSqrLen;
        if (u<0 || u>1.0)
            continue;

        //assumes the center of the sphere is at the origin
        Vector3 toStart(start);
        toStart.normalize();
        seg           /= Math::sqrt(segSqrLen);
        Vector3 normal = Geometry::cross(toStart, seg);

        double newDist = Vector3(pos) * normal;
        if (newDist < dist)
        {
            retId.unique = getUnique();
            retId.type   = CONTROL_SEGMENT;
            retId.index  = i;
            dist         = newDist;
        }
    }

    return retId;
}


bool Shape::
isValid(const Id& id)
{
    if (id==BAD_ID)
        return false;

    int numPoints = static_cast<int>(controlPoints.size());
    switch (id.type)
    {
        case CONTROL_POINT:
            return id.index<numPoints;
        case CONTROL_SEGMENT:
            return id.index<numPoints-1;
        default:
            return false;
    }
}

bool Shape::
isValidPoint(const Id& id)
{
    int numPoints = static_cast<int>(controlPoints.size());
    return id!=BAD_ID && id.type==CONTROL_POINT && id.index<numPoints;
}

bool Shape::
isValidSegment(const Id& id)
{
    int numSegments = static_cast<int>(controlPoints.size()) - 1;
    return id!=BAD_ID && id.type==CONTROL_SEGMENT && id.index<numSegments;
}


Shape::Id Shape::
addControlPoint(const Point3& pos)
{
    ///\todo get proper height of the control point
    controlPoints.push_back(pos);
    return Id(getUnique(),CONTROL_POINT,static_cast<int>(controlPoints.size()));
}

bool Shape::
moveControlPoint(const Id& id, const Point3& pos)
{
    if (!isValidPoint(id))
        return false;

    controlPoints[id.index] = pos;
    return true;
}

void Shape::
removeControlPoint(const Id& id)
{
    if (!isValidPoint(id))
        return;

    controlPoints.erase(controlPoints.begin()+id.index);
}


Shape::Id Shape::
previousControl(const Id& id)
{
    if (id==BAD_ID || id.index==0)
        return BAD_ID;

    switch (id.type)
    {
        case CONTROL_POINT:
            return Id(id.unique, CONTROL_SEGMENT, id.index-1);

        case CONTROL_SEGMENT:
            return Id(id.unique, CONTROL_POINT, id.index);

        default:
            return BAD_ID;
    }
}

Shape::Id Shape::
nextControl(const Id& id)
{
    int lastPoint = static_cast<int>(controlPoints.size()) - 1;
    if (id==BAD_ID || id.index>=lastPoint)
        return BAD_ID;

    switch (id.type)
    {
        case CONTROL_POINT:
            return Id(id.unique, CONTROL_SEGMENT, id.index);

        case CONTROL_SEGMENT:
            return Id(id.unique, CONTROL_POINT, id.index+1);

        default:
            return BAD_ID;
    }
}


Point3& Shape::
getControlPoint(const Id& id)
{
    if (!isValidPoint(id))
        throw std::runtime_error("invalid control point id");

    return controlPoints[id.index];
}

Point3s& Shape::
getControlPoints()
{
    return controlPoints;
}

const Point3s& Shape::
getControlPoints() const
{
    return controlPoints;
}


Shape::Id Shape::
refine(const Id& id, const Point3& pos)
{
    if (!isValidSegment(id))
        return BAD_ID;

    Id end = nextControl(id);
    Point3s::iterator it = controlPoints.begin() + id.index;
    controlPoints.insert(it, pos);

    return id;
}


int Shape::
getUnique() const
{
    return 0;
}


END_CRUSTA
