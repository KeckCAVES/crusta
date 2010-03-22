#ifndef _Polyline_H_
#define _Polyline_H_

#include <crusta/map/Shape.h>


BEGIN_CRUSTA


class Polyline : public Shape
{
    friend class PolylineRenderer;

public:
    Polyline(Crusta* iCrusta);

protected:
    void recomputeCoords(ControlPointHandle start);

//- Inherited from Shape
public:
    virtual void setControlPoints(const Point3s& newControlPoints);

    virtual ControlId addControlPoint(const Point3& pos, End end=END_BACK);
    virtual void moveControlPoint(const ControlId& id, const Point3& pos);
    virtual void removeControlPoint(const ControlId& id);
    virtual ControlId refine(const ControlId& id, const Point3& pos);
};


END_CRUSTA


#endif //_Polyline_H_
