#include <crusta/map/Polyline.h>

#include <cassert>


BEGIN_CRUSTA


Polyline::
Polyline(Crusta* iCrusta) :
    Shape(iCrusta)
{
}


void Polyline::
recomputeCoords(ControlPointHandle cur)
{
    assert(cur != controlPoints.end());

    ControlPointHandle prev = cur;
    if (prev != controlPoints.begin())
        --prev;
    else
    {
        prev->coord = 0.0;
        ++cur;
    }
    for (; cur!=controlPoints.end(); ++prev, ++cur)
    {
        cur->coord = prev->coord + Geometry::dist(prev->pos, cur->pos);
        cur->age   = newestAge;
    }
    ++newestAge;
}

void Polyline::
setControlPoints(const Point3s& newControlPoints)
{
    Shape::setControlPoints(newControlPoints);
    recomputeCoords(controlPoints.begin());
}

Shape::ControlId Polyline::
addControlPoint(const Point3& pos, End end)
{
    Shape::ControlId ret = Shape::addControlPoint(pos, end);
    assert(ret.isValid());
    recomputeCoords(ret.handle);
    return ret;
}

void Polyline::
moveControlPoint(const ControlId& id, const Point3& pos)
{
    Shape::moveControlPoint(id, pos);
    recomputeCoords(id.handle);
}

void Polyline::
removeControlPoint(const ControlId& id)
{
    assert(id.isValid());

    if (id.handle==controlPoints.begin() || id.handle==controlPoints.end())
    {
        Shape::removeControlPoint(id);
    }
    else
    {
        ControlPointHandle next = id.handle;
        ++next;
        Shape::removeControlPoint(id);
        recomputeCoords(next);
    }
}

Shape::ControlId Polyline::
refine(const ControlId& id, const Point3& pos)
{
    Shape::ControlId ret = Shape::refine(id, pos);
    assert(ret.isValid());
    recomputeCoords(ret.handle);
    return ret;
}

END_CRUSTA
