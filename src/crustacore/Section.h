#ifndef _Section_H_
#define _Section_H_


#include <crustacore/basics.h>

BEGIN_CRUSTA


///\todo deprecate & remove unused stuff
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

    const Vector3& getNormal() const;

    /** project a 3D point onto the section's plane */
    Point3 projectOntoPlane(const Point3& point) const;

    /** intersect a 3D ray with the section */
    HitResult intersectRay(const Ray& ray) const;
    /** find the closest point on the segment to the ray from the section origin
        to the specified 3D query point (should be projected onto the
        section plane). The returned hit result is relative to the section's
        segment. */
    HitResult intersectWithSegment(const Point3& point) const;

    /** intersect a 3D ray with the section's plane */
    HitResult intersectPlane(const Ray& ray, bool cullBackFace=true) const;
    /** intersect a 3D ray with the bounded section */
    HitResult intersect(const Ray& ray, bool cullBackFace=true) const;

    /** check for containment of a point in the positive plane */
    bool isContained(const Point3& point) const;

protected:
    /** compute the normal from the starting and ending points */
    void computeNormal();

    Vector3 start;
    Vector3 end;
    Vector3 normal;
};


END_CRUSTA


#endif //_Section_H_
