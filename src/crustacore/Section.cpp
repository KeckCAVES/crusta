#include <crustacore/Section.h>

#if CRUSTA_ENABLE_DEBUG
#include <iomanip>
#include <iostream>
#include <limits>
#endif //CRUSTA_ENABLE_DEBUG



BEGIN_CRUSTA

#define EPSILON (0.000000000001)


Section::
Section(const Geometry::Point<double,3>& iStart, const Geometry::Point<double,3>& iEnd) :
    start(iStart[0], iStart[1], iStart[2]), end(iEnd[0], iEnd[1], iEnd[2])
{
    computeNormal();
}

Section::
Section(const Geometry::Vector<double,3>& iStart, const Geometry::Vector<double,3>& iEnd) :
    start(iStart), end(iEnd)
{
    computeNormal();
}

Geometry::Point<double,3> Section::
getStart() const
{
    return Geometry::Point<double,3>(start[0], start[1], start[2]);
}

Geometry::Point<double,3> Section::
getEnd() const
{
    return Geometry::Point<double,3>(end[0], end[1], end[2]);
}

const Geometry::Vector<double,3>& Section::
getNormal() const
{
    return normal;
}

Geometry::Point<double,3> Section::
projectOntoPlane(const Geometry::Point<double,3>& point) const
{
    Geometry::Vector<double,3> vec(point);
    Geometry::Vector<double,3> normal = Geometry::cross(start, end).normalize();
    return point - (vec*normal)*normal;
}

Geometry::HitResult<double> Section::
intersectRay(const Geometry::Ray<double,3>& ray) const
{
    /* based on "Fast, Minimum Storage Ray/Triangle Intersection" by Tomas
       Moeller and Ben Trumbore */
    Geometry::Vector<double,3> rayOrig(ray.getOrigin()[0],ray.getOrigin()[1],ray.getOrigin()[2]);
    const Geometry::Vector<double,3>& rayDir = ray.getDirection();

    /* begin calculating determinant - also used to calculate U parameter */
    Geometry::Vector<double,3> pvec = Geometry::cross(rayDir, end);
    /* if determinant is near zero, ray lies in plane of triangle */
    Scalar det = start * pvec;

    if (det>-EPSILON && det<EPSILON)
        return Geometry::HitResult<double>();

    Scalar invDet = Scalar(1) / det;

    /* calculate U parameter and test bounds */
    Scalar u = (rayOrig*pvec) * invDet;
    if (u < Scalar(0))
        return Geometry::HitResult<double>();

    /* prepare to test V parameter */
    Geometry::Vector<double,3> qvec = Geometry::cross(rayOrig, start);
    /* calculate V parameter and test bounds */
    Scalar v = (rayDir*qvec) * invDet;
    if (v < Scalar(0))
        return Geometry::HitResult<double>();

    /* calculate t, ray intersects triangle */
    Scalar t = (end*qvec) * invDet;
    return Geometry::HitResult<double>(t);
}

Geometry::HitResult<double> Section::
intersectWithSegment(const Geometry::Point<double,3>& point) const
{
    Geometry::Vector<double,3> u = end - start;
    Geometry::Vector<double,3> v(point[0], point[1], point[2]);

    Scalar a = u*u;
    Scalar b = u*v;
    Scalar c = v*v;
    Scalar d = u*start;
    Scalar e = v*start;

    Scalar denom = a*c - Math::sqr(b);
    if (denom>-EPSILON && denom<EPSILON)
        return Geometry::HitResult<double>();
    else
        return Geometry::HitResult<double>((b*e - c*d) / denom);
}


Geometry::HitResult<double> Section::
intersectPlane(const Geometry::Ray<double,3>& ray, bool cullBackFace) const
{
    Geometry::Vector<double,3> rayOrig(ray.getOrigin()[0],ray.getOrigin()[1],ray.getOrigin()[2]);
    const Geometry::Vector<double,3>& rayDir = ray.getDirection();

    /* the ray intersects the plane at poing t for which (ray(t)-PlaneOrigin) is
       orthogonal to the normal. See http://softsurfer.com/Archive/algorithm_0104/algorithm_0104B.htm#Line-Plane Intersection
       for nice description */
    double nDotDir = normal * rayDir;

    //exit on back-facing planes if so desired
    if (cullBackFace && nDotDir>0.0)
        return Geometry::HitResult<double>();

    //handle the parallel case
    if (nDotDir>-EPSILON && nDotDir<EPSILON)
    {
        Geometry::Vector<double,3> toOrig    = rayOrig - start;
        double nDotToOrig = normal * toOrig;
        //coincident
        if (nDotToOrig>-EPSILON && nDotToOrig<EPSILON)
            return Geometry::HitResult<double>(0.0);
        //disjoint
        else
            return Geometry::HitResult<double>();
    }

    //compute the intersection parameter
    return Geometry::HitResult<double>((normal * (start - rayOrig)) / nDotDir);
}

Geometry::HitResult<double> Section::
intersect(const Geometry::Ray<double,3>& ray, bool cullBackFace) const
{
    //find the plane intersection
    Geometry::HitResult<double> hit = intersectPlane(ray, cullBackFace);
    if (!hit.isValid())
        return Geometry::HitResult<double>();

    Geometry::Vector<double,3> hitPoint   = Geometry::Vector<double,3>(ray(hit.getParameter()));

    Geometry::Vector<double,3> startUp = start;
    startUp.normalize();
    Geometry::Vector<double,3> startToEnd   = end - start;
    Geometry::Vector<double,3> startTan     = startToEnd - (startToEnd*startUp)*startUp;
    Geometry::Vector<double,3> startToPoint = hitPoint - start;
    if (startTan*startToPoint < 0.0)
        return Geometry::HitResult<double>();

    Geometry::Vector<double,3> endUp = end;
    endUp.normalize();
    Geometry::Vector<double,3> endToStart = -startToEnd;
    Geometry::Vector<double,3> endTan     = endToStart - (endToStart*endUp)*endUp;
    Geometry::Vector<double,3> endToPoint = hitPoint - end;
    if (endTan*endToPoint < 0.0)
        return Geometry::HitResult<double>();

    return hit;
}

bool Section::
isContained(const Geometry::Point<double,3>& point) const
{
    return (point-start)*normal >= 0.0;
}


void Section::
computeNormal()
{
    Geometry::Vector<double,3> up = start;
    up.normalize();

    Geometry::Vector<double,3> right = end - start;
    right.normalize();

    normal = Geometry::cross(up, right);
    normal.normalize();
}

END_CRUSTA
