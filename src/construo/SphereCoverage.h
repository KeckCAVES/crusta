///\todo duplication between ImageCoverage and SphereCoverage

#ifndef _SphereCoverage_H_
#define _SphereCoverage_H_

#include <vector>

#include <Geometry/Vector.h>

#include <construo/GeometryTypes.h>
#include <crustacore/Scope.h>

namespace crusta {

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
    size_t overlaps(const Box& box) const;
    /** checks if a given spherical coverage overlaps the coverage region */
    size_t overlaps(const SphereCoverage& coverage) const;

    /** shift the coverage */
    void shift(const Vector& vec);

protected:
    Points vertices;
    Box box;

    void addVertex(Point& vertex);
    size_t checkOverlap(const SphereCoverage& coverage) const;
};

class StaticSphereCoverage : public SphereCoverage
{
public:
    StaticSphereCoverage(size_t subdivisions, const Scope& scope);
    StaticSphereCoverage(size_t subdivisions, const ImageCoverage* coverage,
                         const ImageTransform* transform);
};

class AdaptiveSphereCoverage : public SphereCoverage
{
public:
    AdaptiveSphereCoverage(size_t maxAngle, const Scope& scope);
    AdaptiveSphereCoverage(size_t maxAngle, const ImageCoverage* coverage,
                           const ImageTransform* transform);
};

} //namespace crusta

#endif //_SphereCoverage_H_
