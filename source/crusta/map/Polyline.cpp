#include <crusta/map/Polyline.h>


BEGIN_CRUSTA


const Polyline::Coords& Polyline::
getCoords()
{
    return coords;
}

Polyline::Coords::const_iterator Polyline::
getCoord(Point3s::const_iterator pit)
{
    int offset = static_cast<int>(&(*pit) - &controlPoints.front());
    return coords.begin() + offset;
}


void Polyline::
recomputeCoords()
{
    int num = static_cast<int>(controlPoints.size());
    coords.resize(num);
    Scalar coord = 0.0;
    coords[0]    = coord;
    for (int i=1; i<num; ++i)
    {
        coord    += Geometry::dist(controlPoints[i-1], controlPoints[i]);
        coords[i] = coord;
    }
}

Shape::ControlId Polyline::
addControlPoint(const Point3& pos, End end)
{
    Shape::ControlId ret = Shape::addControlPoint(pos, end);
    recomputeCoords();
    return ret;
}

bool Polyline::
moveControlPoint(const ControlId& id, const Point3& pos)
{
    bool ret = Shape::moveControlPoint(id, pos);
    recomputeCoords();
    return ret;
}

void Polyline::
removeControlPoint(const ControlId& id)
{
    Shape::removeControlPoint(id);
    recomputeCoords();
}


END_CRUSTA
