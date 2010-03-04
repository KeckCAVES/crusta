#ifndef _Section_H_
#define _Section_H_


#include <crusta/basics.h>

BEGIN_CRUSTA


/** A planar infinite wedge specified by the center of the globe and two other
    3D points forming the section's segment. The segment side of the section
    is open. */
class Section
{
public:
    Section(const Point3& iStart, const Point3& iEnd);
    Section(const Vector3& iStart, const Vector3& iEnd);

    Point3 getStart() const;
    Point3 getEnd()   const;

    /** project a 3D point onto the section's plane */
    Point3 projectOntoPlane(const Point3& point) const;

    /** intersect a 3D ray with the section */
    HitResult intersectRay(const Ray& ray) const;
    /** find the closest point on the segment to the ray from the section origin
        to the specified 3D query point (should be projected onto the
        section plane). The returned hit result is relative to the section's
        segment. */
    HitResult intersectWithSegment(const Point3& point) const;

protected:
    Vector3 start;
    Vector3 end;
};


END_CRUSTA


#endif //_Section_H_
