#ifndef _Triangle_H_
#define _Triangle_H_


#include <crusta/basics.h>


BEGIN_CRUSTA


/** A triangle, used for intersection */
class Triangle
{
public:
    Triangle(Scalar x0, Scalar y0, Scalar z0,
             Scalar x1, Scalar y1, Scalar z1,
             Scalar x2, Scalar y2, Scalar z2) :
        vert0(x0,y0,z0), edge1(x1-x0,y1-y0,z1-z0), edge2(x2-x0,y2-y0,z2-z0)
    {
    }
    Triangle(Vector3 p0, Vector3 p1, Vector3 p2) :
        vert0(p0), edge1(p1-p0), edge2(p2-p0)
    {
    }

    Point3 getVert0() const
    {
        return Point3(vert0[0], vert0[1], vert0[2]);
    }
    const Vector3& getEdge1() const
    {
        return edge1;
    }
    const Vector3& getEdge2() const
    {
        return edge2;
    }

    HitResult intersectRay(const Ray& ray) const
    {
        static const Scalar EPSILON = Scalar(0.000001);

        Vector3 rayOrig(
            ray.getOrigin()[0], ray.getOrigin()[1], ray.getOrigin()[2]);
        const Vector3& rayDir = ray.getDirection();

        /* begin calculating determinant - also used to calculate U parameter */
        Vector3 pvec = Geometry::cross(rayDir, edge2);
        /* if determinant is near zero, ray lies in plane of triangle */
        Scalar det = edge1 * pvec;

        if (det < EPSILON)
            return HitResult();

        /* calculate distance from vert0 to ray origin */
        Vector3 tvec = rayOrig - vert0;

        /* calculate U parameter and test bounds */
        Scalar u = tvec * pvec;
        if (u<Scalar(0) || u>det)
            return HitResult();

        /* prepare to test V parameter */
        Vector3 qvec = Geometry::cross(tvec, edge1);

        /* calculate V parameter and test bounds */
        Scalar v = rayDir * qvec;
        if (v<Scalar(0) || u+v>det)
           return HitResult();

        /* calculate t, scale parameters, ray intersects triangle */
        Scalar t      = edge2 * qvec;
        Scalar invDet = Scalar(1) / det;

        return HitResult(t*invDet);
    }

protected:
    const Vector3 vert0;
    const Vector3 edge1;
    const Vector3 edge2;
};

END_CRUSTA


#endif //_Triangle_H_
