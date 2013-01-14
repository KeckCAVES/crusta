///\todo duplication between ImageCoverage and SphereCoverage

#ifndef _SphereCoverage_H_
#define _SphereCoverage_H_

#include <vector>

#include <Geometry/Vector.h>

#include <construo/GeometryTypes.h>
#include <crustacore/Scope.h>

BEGIN_CRUSTA

class ImageCoverage;
class ImageTransform;

class SphereCoverage
{
public:
    typedef std::vector<Point> Points;
    typedef Geometry::Vector<Point::Scalar, Point::dimension> Vector;

    enum
    {
        SEPARATE,
        OVERLAPS,
        CONTAINS,
        ISCONTAINED
    };

    SphereCoverage();

    /** retrieve the vertices of the coverate */
    const Points& getVertices() const;
    /** retrieve the bounding box of the coverage */
    const Box& getBoundingBox() const;

    /** checks if a given vertex (in spherical coordinates) is inside the
        coverage region */
    bool contains(const Point& v) const;
    /** checks if a given bounding box overlaps the coverage region */
    uint overlaps(const Box& box) const;
    /** checks if a given spherical coverage overlaps the coverage region */
    uint overlaps(const SphereCoverage& coverage) const;

    /** shift the coverage */
    void shift(const Vector& vec);

protected:
    Points vertices;
    Box box;

    void addVertex(Point& vertex);
    uint checkOverlap(const SphereCoverage& coverage) const;
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
