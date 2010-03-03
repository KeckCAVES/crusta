#ifndef _Sphere_H_
#define _Sphere_H_

#include <crusta/basics.h>


BEGIN_CRUSTA

/** Extends the Sphere object from Vrui to provide both intersection points with
    a ray (if there is intersection). */
class Sphere
{
public:
    Sphere(const Point3& iCenter, Scalar iRadius);

    Point3 getCenter();
    void setCenter(const Point3& nCenter);
    Scalar getRadius();
    void setRadius(const Scalar nRadius);

    HitResult intersectRay(const Ray& ray) const;
    bool intersectRay(const Ray& ray, Scalar& first, Scalar& second) const;

protected:
    Point3 center;
    Scalar radius;
    Scalar sqrRadius;
};

END_CRUSTA


#endif //_Sphere_H_
