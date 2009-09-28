#ifndef _Ellipsoid_H_
#define _Ellipsoid_H_

BEGIN_CRUSTA

class Scope;

class Ellipsoid
{
public:
    typedef Geometry::Point<double, 3> Point;
    typedef Point::Scalar             Scalar;

    /** retrieve the centroid of the scope */
    static Point getCentroid(const Scope& scope);

    /** check if a point is contained in the solid angle subtended by the
     scope */
    static bool contains(const Point& p, const Scope& scope);
};

END_CRUSTA

#endif //_Ellipsoid_H_
