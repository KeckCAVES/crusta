#include <crusta/map/Shape.h>

#include <cassert>
#include <stdexcept>

#include <Math/Constants.h>


BEGIN_CRUSTA

const Shape::Symbol    Shape::DEFAULT_SYMBOL;
const Shape::ControlId Shape::BAD_ID(-1);


Shape::Symbol::
Symbol() :
    id(0), color(0.8f, 0.7f, 0.5f, 1.0f)
{}

Shape::Symbol::
Symbol(int iId, const Color& iColor) :
    id(iId), color(iColor)
{}

Shape::Symbol::
Symbol(int iId, float iRed, float iGreen, float iBlue) :
    id(iId), color(iRed, iGreen, iBlue)
{}



Shape::ControlId::
ControlId():
    raw(BAD_ID.raw)
{}

Shape::ControlId::
ControlId(int iRaw) :
    raw(iRaw)
{}

Shape::ControlId::
ControlId(int iType, int iIndex) :
    type(iType), index(iIndex)
{}

bool Shape::ControlId::
operator ==(const ControlId& other) const
{
    return raw == other.raw;
}

bool Shape::ControlId::
operator !=(const ControlId& other) const
{
    return raw != other.raw;
}


Shape::
Shape() :
    symbol(DEFAULT_SYMBOL)
{
}

Shape::
~Shape()
{
}


Shape::ControlId Shape::
select(const Point3& pos, double& dist, double pointBias)
{
    double pointDist;
    ControlId pointId = selectPoint(pos, pointDist);
    double segmentDist;
    ControlId segmentId = selectSegment(pos, segmentDist);

    if (pointDist*pointBias <= segmentDist)
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

Shape::ControlId Shape::
selectPoint(const Point3& pos, double& dist)
{
    ControlId retId = BAD_ID;
    dist            = Math::Constants<double>::max;

    int numPoints = static_cast<int>(controlPoints.size());
    for (int i=0; i<numPoints; ++i)
    {
        double newDist = Geometry::dist(controlPoints[i], pos);
        if (newDist < dist)
        {
            retId.type   = CONTROL_POINT;
            retId.index  = i;
            dist         = newDist;
        }
    }

    return retId;
}

Shape::ControlId Shape::
selectSegment(const Point3& pos, double& dist)
{
    ControlId retId = BAD_ID;
    dist            = Math::Constants<double>::max;

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

        double newDist = abs(Vector3(pos) * normal);
        if (newDist < dist)
        {
            retId.type   = CONTROL_SEGMENT;
            retId.index  = i;
            dist         = newDist;
        }
    }

    return retId;
}

Shape::ControlId Shape::
selectExtremity(const Point3& pos, double& dist, End& end)
{
    double frontDist = Geometry::dist(pos, controlPoints.front());
    double backDist  = Geometry::dist(pos, controlPoints.back());

    if (frontDist < backDist)
    {
        dist = frontDist;
        end  = END_FRONT;
        return ControlId(CONTROL_POINT, 0);
    }
    else
    {
        dist = backDist;
        end  = END_BACK;
        return ControlId(CONTROL_POINT, controlPoints.size() - 1);
    }
}


bool Shape::
isValid(const ControlId& id)
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
isValidPoint(const ControlId& id)
{
    int numPoints = static_cast<int>(controlPoints.size());
    return id!=BAD_ID && id.type==CONTROL_POINT && id.index<numPoints;
}

bool Shape::
isValidSegment(const ControlId& id)
{
    int numSegments = static_cast<int>(controlPoints.size()) - 1;
    return id!=BAD_ID && id.type==CONTROL_SEGMENT && id.index<numSegments;
}


Shape::ControlId Shape::
addControlPoint(const Point3& pos, End end)
{
    if (end == END_FRONT)
    {
        controlPoints.insert(controlPoints.begin(), pos);
        return ControlId(CONTROL_POINT, 0);
    }
    else
    {
        controlPoints.push_back(pos);
        return ControlId(CONTROL_POINT,
                         static_cast<int>(controlPoints.size()-1));
    }
}

bool Shape::
moveControlPoint(const ControlId& id, const Point3& pos)
{
    if (!isValidPoint(id))
        return false;

    controlPoints[id.index] = pos;
    return true;
}

void Shape::
removeControlPoint(const ControlId& id)
{
    if (!isValidPoint(id))
        return;

    controlPoints.erase(controlPoints.begin()+id.index);
}


Shape::ControlId Shape::
previousControl(const ControlId& id)
{
    switch (id.type)
    {
        case CONTROL_POINT:
            if (id.index>0)
                return ControlId(CONTROL_SEGMENT, id.index-1);
            else
                return BAD_ID;

        case CONTROL_SEGMENT:
            if (id.index>=0)
                return ControlId(CONTROL_POINT, id.index);
            else
                return BAD_ID;

        default:
            return BAD_ID;
    }
}

Shape::ControlId Shape::
nextControl(const ControlId& id)
{
    int lastPoint = static_cast<int>(controlPoints.size()) - 1;
    if (id==BAD_ID || id.index>=lastPoint)
        return BAD_ID;

    switch (id.type)
    {
        case CONTROL_POINT:
            return ControlId(CONTROL_SEGMENT, id.index);

        case CONTROL_SEGMENT:
            return ControlId(CONTROL_POINT, id.index+1);

        default:
            return BAD_ID;
    }
}


Shape::Symbol& Shape::
getSymbol()
{
    return symbol;
}

const Shape::Symbol& Shape::
getSymbol() const
{
    return symbol;
}

Point3& Shape::
getControlPoint(const ControlId& id)
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


Shape::ControlId Shape::
refine(const ControlId& id, const Point3& pos)
{
    if (!isValidSegment(id))
        return BAD_ID;

    ControlId end = nextControl(id);
    assert(isValidPoint(end));
    Point3s::iterator it = controlPoints.begin() + end.index;
    controlPoints.insert(it, pos);

    return end;
}


END_CRUSTA
