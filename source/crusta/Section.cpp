#include <crusta/Section.h>

#if CRUSTA_ENABLE_DEBUG
#include <iomanip>
#include <iostream>
#include <limits>
#endif //CRUSTA_ENABLE_DEBUG



BEGIN_CRUSTA

#define EPSILON (0.000000000001)


Section::
Section(const Point3& iStart, const Point3& iEnd) :
    start(iStart[0], iStart[1], iStart[2]), end(iEnd[0], iEnd[1], iEnd[2])
{
    computeNormal();
}

Section::
Section(const Vector3& iStart, const Vector3& iEnd) :
    start(iStart), end(iEnd)
{
    computeNormal();
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

const Vector3& Section::
getNormal() const
{
    return normal;
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


HitResult Section::
intersectPlane(const Ray& ray, bool cullBackFace) const
{
    Vector3 rayOrig(ray.getOrigin()[0],ray.getOrigin()[1],ray.getOrigin()[2]);
    const Vector3& rayDir = ray.getDirection();

    /* the ray intersects the plane at poing t for which (ray(t)-PlaneOrigin) is
       orthogonal to the normal. See http://softsurfer.com/Archive/algorithm_0104/algorithm_0104B.htm#Line-Plane Intersection
       for nice description */
    double nDotDir = normal * rayDir;

CRUSTA_DEBUG(91, CRUSTA_DEBUG_OUT <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"Section: s(" << start[0] << ", " << start[1] << ", " << start[2] << ")  e(" <<
end[0] << ", " << end[1] << ", " << end[2] << ") n(" <<
normal[0] << ", " << normal[1] << ", " << normal[2] << ")\n" <<
"Ray: o(" << rayOrig[0] << ", " << rayOrig[1] << ", " << rayOrig[2] << ") d(" <<
rayDir[0] << ", " << rayDir[1] << ", " << rayDir[2] << ")\n" <<
"nDotDir: " << nDotDir << "\n\n";)

    //exit on back-facing planes if so desired
    if (cullBackFace && nDotDir>0.0)
        return HitResult();

    //handle the parallel case
    if (nDotDir>-EPSILON && nDotDir<EPSILON)
    {
        Vector3 toOrig    = rayOrig - start;
        double nDotToOrig = normal * toOrig;
        //coincident
        if (nDotToOrig>-EPSILON && nDotToOrig<EPSILON)
            return HitResult(0.0);
        //disjoint
        else
            return HitResult();
    }

    //compute the intersection parameter
    return HitResult((normal * (start - rayOrig)) / nDotDir);
}

HitResult Section::
intersect(const Ray& ray, bool cullBackFace) const
{
    //find the plane intersection
    HitResult hit = intersectPlane(ray, cullBackFace);
    if (!hit.isValid())
        return HitResult();

    Vector3 hitPoint   = Vector3(ray(hit.getParameter()));

    Vector3 startUp = start;
    startUp.normalize();
    Vector3 startToEnd   = end - start;
    Vector3 startTan     = startToEnd - (startToEnd*startUp)*startUp;
    Vector3 startToPoint = hitPoint - start;
    if (startTan*startToPoint < 0.0)
        return HitResult();

    Vector3 endUp = end;
    endUp.normalize();
    Vector3 endToStart = -startToEnd;
    Vector3 endTan     = endToStart - (endToStart*endUp)*endUp;
    Vector3 endToPoint = hitPoint - end;
    if (endTan*endToPoint < 0.0)
        return HitResult();

    return hit;
}

bool Section::
isContained(const Point3& point) const
{
    return (point-start)*normal >= 0.0;
}


void Section::
computeNormal()
{
CRUSTA_DEBUG(92, CRUSTA_DEBUG_OUT <<
"^ New Section:" <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"start(" << start[0] << ", " << start[1] << ", " << start[2] << ")  " <<
"end(" << end[0] << ", " << end[1] << ", " << end[2] << ")\n";)

    Vector3 up = start;
CRUSTA_DEBUG(92, CRUSTA_DEBUG_OUT <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"up(" << up[0] << ", " << up[1] << ", " << up[2] << ")  ";)
    up.normalize();
CRUSTA_DEBUG(92, CRUSTA_DEBUG_OUT <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"nup(" << up[0] << ", " << up[1] << ", " << up[2] << ")\n";)

    Vector3 right = end - start;
CRUSTA_DEBUG(92, CRUSTA_DEBUG_OUT <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"right(" << right[0] << ", " << right[1] << ", " << right[2] << ")  ";)
    right.normalize();
CRUSTA_DEBUG(92, CRUSTA_DEBUG_OUT <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"nright(" << right[0] << ", " << right[1] << ", " << right[2] << ")\n";)

    normal = Geometry::cross(up, right);
CRUSTA_DEBUG(92, CRUSTA_DEBUG_OUT <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"normal(" << normal[0] << ", " << normal[1] << ", " << normal[2] << ") ";)
    normal.normalize();
CRUSTA_DEBUG(92, CRUSTA_DEBUG_OUT <<
std::setprecision(std::numeric_limits<double>::digits10) <<
"nnormal(" << normal[0] << ", " << normal[1] << ", " << normal[2] << ")\n";)
}

END_CRUSTA
