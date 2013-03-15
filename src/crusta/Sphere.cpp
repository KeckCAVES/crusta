#include <crusta/Sphere.h>


namespace crusta {


Sphere::
Sphere(const Geometry::Point<double,3>& iCenter, Scalar iRadius) :
    center(iCenter), radius(iRadius), sqrRadius(Math::sqr(radius))
{
}

Geometry::Point<double,3> Sphere::
getCenter()
{
    return center;
}

void Sphere::
setCenter(const Geometry::Point<double,3>& nCenter)
{
    center = nCenter;
}

Scalar Sphere::
getRadius()
{
    return radius;
}

void Sphere::
setRadius(const Scalar nRadius)
{
    radius    = nRadius;
    sqrRadius = Math::sqr(radius);
}

Geometry::HitResult<double> Sphere::
intersectRay(const Geometry::Ray<double,3>& ray) const
{
    Scalar  d2  = Geometry::sqr(ray.getDirection());
    Geometry::Vector<double,3> oc  = ray.getOrigin() - center;
    Scalar  ph  = oc * ray.getDirection();
    Scalar  det = Math::sqr(ph) - (Geometry::sqr(oc)-sqrRadius)*d2;
    if (det < Scalar(0))
        return Geometry::HitResult<double>();

    det           = Math::sqrt(det);
    Scalar lambda = (-ph-det) / d2; // First intersection
    if (lambda >= Scalar(0))
        return Geometry::HitResult<double>(lambda);

    lambda = (-ph+det) / d2; // Second intersection
    if (lambda >= Scalar(0))
        return Geometry::HitResult<double>(lambda);

    return Geometry::HitResult<double>();
}

bool Sphere::
intersectRay(const Geometry::Ray<double,3>& ray, Scalar& first, Scalar& second) const
{
    Scalar  d2  = Geometry::sqr(ray.getDirection());
    Geometry::Vector<double,3> oc  = ray.getOrigin() - center;
    Scalar  ph  = oc * ray.getDirection();
    Scalar  det = Math::sqr(ph) - (Geometry::sqr(oc)-sqrRadius)*d2;
    if (det < Scalar(0))
        return false;

    det           = Math::sqrt(det);
    Scalar lambda = (-ph-det) / d2; // First intersection
    first         = lambda;

    lambda = (-ph+det) / d2; // Second intersection
    second = lambda;

    return true;
}


} //namespace crusta
