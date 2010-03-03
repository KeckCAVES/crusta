#include <crusta/Section.h>


BEGIN_CRUSTA

#define EPSILON (Scalar(0.000001))


Section::
Section(const Point3& iStart, const Point3& iEnd) :
    start(iStart[0], iStart[1], iStart[2]), end(iEnd[0], iEnd[1], iEnd[2])
{}

Section::
Section(const Vector3& iStart, const Vector3& iEnd) :
    start(iStart), end(iEnd)
{
}

Point3 Section::
getStart() const
{
    return Point3(start[0], start[1], start[2]);
}

Point3 Section::
getEnd() const
{
    return Point3(end[0], end[1], end[2]);
}

Point3 Section::
projectOntoPlane(const Point3& point) const
{
    Vector3 vec(point);
    Vector3 normal = Geometry::cross(start, end).normalize();
    return point - (vec*normal)*normal;
}

HitResult Section::
intersectRay(const Ray& ray) const
{
    /* based on "Fast, Minimum Storage Ray/Triangle Intersection" by Tomas
       Moeller and Ben Trumbore */
    Vector3 rayOrig(ray.getOrigin()[0],ray.getOrigin()[1],ray.getOrigin()[2]);
    const Vector3& rayDir = ray.getDirection();

    /* begin calculating determinant - also used to calculate U parameter */
    Vector3 pvec = Geometry::cross(rayDir, end);
    /* if determinant is near zero, ray lies in plane of triangle */
    Scalar det = start * pvec;

    if (det>-EPSILON && det<EPSILON)
        return HitResult();

    Scalar invDet = Scalar(1) / det;

    /* calculate U parameter and test bounds */
    Scalar u = (rayOrig*pvec) * invDet;
    if (u < Scalar(0))
        return HitResult();

    /* prepare to test V parameter */
    Vector3 qvec = Geometry::cross(rayOrig, start);
    /* calculate V parameter and test bounds */
    Scalar v = (rayDir*qvec) * invDet;
    if (v < Scalar(0))
        return HitResult();

    /* calculate t, ray intersects triangle */
    Scalar t = (end*qvec) * invDet;
    return HitResult(t);
}

HitResult Section::
intersectWithSegment(const Point3& point) const
{
    Vector3 u = end - start;
    Vector3 v(point[0], point[1], point[2]);

    Scalar a = u*u;
    Scalar b = u*v;
    Scalar c = v*v;
    Scalar d = u*start;
    Scalar e = v*start;

    Scalar denom = a*c - Math::sqr(b);
    if (denom>-EPSILON && denom<EPSILON)
        return HitResult();
    else
        return HitResult((b*e - c*d) / denom);
}


END_CRUSTA
