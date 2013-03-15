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
    Section(const Geometry::Point<double,3>& iStart, const Geometry::Point<double,3>& iEnd);
    Section(const Geometry::Vector<double,3>& iStart, const Geometry::Vector<double,3>& iEnd);

    Geometry::Point<double,3> getStart() const;
    Geometry::Point<double,3> getEnd()   const;

    const Geometry::Vector<double,3>& getNormal() const;

    /** project a 3D point onto the section's plane */
    Geometry::Point<double,3> projectOntoPlane(const Geometry::Point<double,3>& point) const;

    /** intersect a 3D ray with the section */
    Geometry::HitResult<double> intersectRay(const Geometry::Ray<double,3>& ray) const;
    /** find the closest point on the segment to the ray from the section origin
        to the specified 3D query point (should be projected onto the
        section plane). The returned hit result is relative to the section's
        segment. */
    Geometry::HitResult<double> intersectWithSegment(const Geometry::Point<double,3>& point) const;

    /** intersect a 3D ray with the section's plane */
    Geometry::HitResult<double> intersectPlane(const Geometry::Ray<double,3>& ray, bool cullBackFace=true) const;
    /** intersect a 3D ray with the bounded section */
    Geometry::HitResult<double> intersect(const Geometry::Ray<double,3>& ray, bool cullBackFace=true) const;

    /** check for containment of a point in the positive plane */
    bool isContained(const Geometry::Point<double,3>& point) const;

protected:
    /** compute the normal from the starting and ending points */
    void computeNormal();

    Geometry::Vector<double,3> start;
    Geometry::Vector<double,3> end;
    Geometry::Vector<double,3> normal;
};


END_CRUSTA


#endif //_Section_H_
