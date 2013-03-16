#include <crusta/map/Polyline.h>

#include <cassert>

#include <crusta/Crusta.h>

#include <crusta/vrui.h>


namespace crusta {


Polyline::
Polyline(Crusta* iCrusta) :
    Shape(iCrusta)
{
}


void Polyline::
recomputeCoords(ControlPointHandle cur)
{
    assert(cur != controlPoints.end());
    FrameStamp curStamp = CURRENT_FRAME;

    Geometry::Point<double,3> prevP, curP;
    ControlPointHandle prev = cur;
    if (prev != controlPoints.begin())
        --prev;
    else
    {
        prev->stamp = curStamp;
        prev->coord = 0.0;
        ++cur;
    }
    prevP = crusta->mapToScaledGlobe(prev->pos);
    for (; cur!=controlPoints.end(); ++prev, ++cur, prevP=curP)
    {
        cur->stamp = curStamp;
        curP = crusta->mapToScaledGlobe(cur->pos);
        cur->coord  = prev->coord + Geometry::dist(prevP, curP);
    }
}

void Polyline::
setControlPoints(const std::vector<Geometry::Point<double,3> >& newControlPoints)
{
    Shape::setControlPoints(newControlPoints);
    recomputeCoords(controlPoints.begin());
}

Shape::ControlId Polyline::
addControlPoint(const Geometry::Point<double,3>& pos, End end)
{
    Shape::ControlId ret = Shape::addControlPoint(pos, end);
    assert(ret.isValid());
    recomputeCoords(ret.handle);
    return ret;
}

void Polyline::
moveControlPoint(const ControlId& id, const Geometry::Point<double,3>& pos)
{
    Shape::moveControlPoint(id, pos);
    recomputeCoords(id.handle);
}

void Polyline::
removeControlPoint(const ControlId& id)
{
    assert(id.isValid());

    if (id.handle==controlPoints.begin() || id.handle==--controlPoints.end())
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
refine(const ControlId& id, const Geometry::Point<double,3>& pos)
{
    Shape::ControlId ret = Shape::refine(id, pos);
    assert(ret.isValid());
    recomputeCoords(ret.handle);
    return ret;
}

} //namespace crusta
