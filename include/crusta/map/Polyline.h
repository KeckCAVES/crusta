#ifndef _Polyline_H_
#define _Polyline_H_

#include <crusta/map/Shape.h>


BEGIN_CRUSTA


class Polyline : public Shape
{
    friend class PolylineRenderer;

public:
    const Point3f&  getCentroid();
    const Point3fs& getRelativeControlPoints();

protected:
    void recomputeRelativePoints();

    Point3f  centroid;
    Point3fs relativeControlPoints;

//- Inherited from Shape
public:
    virtual Id addControlPoint(const Point3& pos, End end=END_BACK);
    virtual bool moveControlPoint(const Id& id, const Point3& pos);
    virtual void removeControlPoint(const Id& id);
};


END_CRUSTA


#endif //_Polyline_H_
