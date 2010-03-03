#include <crusta/Sphere.h>


BEGIN_CRUSTA


Sphere::
Sphere(const Point3& iCenter, Scalar iRadius) :
    center(iCenter), radius(iRadius), sqrRadius(Math::sqr(radius))
{
}

Point3 Sphere::
getCenter()
{
    return center;
}

void Sphere::
setCenter(const Point3& nCenter)
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

HitResult Sphere::
intersectRay(const Ray& ray) const
{
    Scalar  d2  = Geometry::sqr(ray.getDirection());
    Vector3 oc  = ray.getOrigin() - center;
    Scalar  ph  = oc * ray.getDirection();
    Scalar  det = Math::sqr(ph) - (Geometry::sqr(oc)-sqrRadius)*d2;
    if (det < Scalar(0))
        return HitResult();

    det           = Math::sqrt(det);
    Scalar lambda = (-ph-det) / d2; // First intersection
    if (lambda >= Scalar(0))
        return HitResult(lambda);

    lambda = (-ph+det) / d2; // Second intersection
    if (lambda >= Scalar(0))
        return HitResult(lambda);

    return HitResult();
}

bool Sphere::
intersectRay(const Ray& ray, Scalar& first, Scalar& second) const
{
    Scalar  d2  = Geometry::sqr(ray.getDirection());
    Vector3 oc  = ray.getOrigin() - center;
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


END_CRUSTA
