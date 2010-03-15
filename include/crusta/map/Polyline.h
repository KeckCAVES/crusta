#ifndef _Polyline_H_
#define _Polyline_H_

#include <crusta/map/Shape.h>


BEGIN_CRUSTA


class Polyline : public Shape
{
    friend class PolylineRenderer;

public:
    typedef std::vector<Scalar> Coords;

    const Coords& getCoords();
    Coords::const_iterator getCoord(Point3s::const_iterator pit);

protected:
    void recomputeCoords();

    Coords coords;

//- Inherited from Shape
public:
    virtual ControlId addControlPoint(const Point3& pos, End end=END_BACK);
    virtual bool moveControlPoint(const ControlId& id, const Point3& pos);
    virtual void removeControlPoint(const ControlId& id);
};


END_CRUSTA


#endif //_Polyline_H_
