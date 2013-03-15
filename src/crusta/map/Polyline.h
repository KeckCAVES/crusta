#ifndef _Polyline_H_
#define _Polyline_H_

#include <crusta/map/Shape.h>


BEGIN_CRUSTA


class Polyline : public Shape
{
    friend class PolylineRenderer;

public:
    Polyline(Crusta* iCrusta);

    void recomputeCoords(ControlPointHandle start);

//- Inherited from Shape
public:
    virtual void setControlPoints(const std::vector<Geometry::Point<double,3> >& newControlPoints);

    virtual ControlId addControlPoint(const Geometry::Point<double,3>& pos, End end=END_BACK);
    virtual void moveControlPoint(const ControlId& id, const Geometry::Point<double,3>& pos);
    virtual void removeControlPoint(const ControlId& id);
    virtual ControlId refine(const ControlId& id, const Geometry::Point<double,3>& pos);
};


END_CRUSTA


#endif //_Polyline_H_
