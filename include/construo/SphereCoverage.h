///\todo duplication between ImageCoverage and SphereCoverage

#ifndef _SphereCoverage_H_
#define _SphereCoverage_H_

#include <vector>

#include <Geometry/Vector.h>

#include <construo/GeometryTypes.h>
#include <crusta/Scope.h>

BEGIN_CRUSTA

class ImageCoverage;
class ImageTransform;

class SphereCoverage
{
public:
    typedef std::vector<Point> Points;
    typedef Geometry::Vector<Point::Scalar, Point::dimension> Vector;
    
    enum Intersection
    {
        SEPARATE,
        OVERLAPS,
        CONTAINS,
        ISCONTAINED
    };

    SphereCoverage();

    /** checks if a given vertex (in spherical coordinates) is inside the
        coverage region */
    bool contains(const Point& v) const;
    /** checks if a given bounding box overlaps the coverage region */
    Intersection overlaps(const Box& box) const;
    /** checks if a given spherical coverage overlaps the coverage region */
    Intersection overlaps(const SphereCoverage& coverage) const;

    /** query the centroid of the coverage */
    const Point& getCentroid();
    /** shift the coverage */
    void shift(const Vector& vec);

protected:
    Points vertices;
    Point centroid;
    Box box;
};

class StaticSphereCoverage : public SphereCoverage
{
public:
    StaticSphereCoverage(uint subdivisions, const Scope& scope);
    StaticSphereCoverage(uint subdivisions, const ImageCoverage* coverage,
                         const ImageTransform* transform);
};

class AdaptiveSphereCoverage : public SphereCoverage
{
public:
    AdaptiveSphereCoverage(uint maxAngle, const Scope& scope);
    AdaptiveSphereCoverage(uint maxAngle, const ImageCoverage* coverage,
                           const ImageTransform* transform);
};

END_CRUSTA

#endif //_SphereCoverage_H_
