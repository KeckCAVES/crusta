#ifndef _Sphere_H_
#define _Sphere_H_

#include <crustacore/basics.h>


namespace crusta {

/** Extends the Sphere object from Vrui to provide both intersection points with
    a ray (if there is intersection). */
class Sphere
{
public:
    Sphere(const Geometry::Point<double,3>& iCenter, Scalar iRadius);

    Geometry::Point<double,3> getCenter();
    void setCenter(const Geometry::Point<double,3>& nCenter);
    Scalar getRadius();
    void setRadius(const Scalar nRadius);

    Geometry::HitResult<double> intersectRay(const Geometry::Ray<double,3>& ray) const;
    bool intersectRay(const Geometry::Ray<double,3>& ray, Scalar& first, Scalar& second) const;

protected:
    Geometry::Point<double,3> center;
    Scalar radius;
    Scalar sqrRadius;
};

} //namespace crusta


#endif //_Sphere_H_
