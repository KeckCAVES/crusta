#ifndef _Homography_H_
#define _Homography_H_


#include <Geometry/ProjectiveTransformation.h>
#include <crusta/basics.h>


BEGIN_CRUSTA


class Homography
{
public:
    typedef Geometry::ProjectiveTransformation<Scalar,3> Projective;

    void setSource(const Point3& p0, const Point3& p1, const Point3& p2,
                   const Point3& p3, const Point3& p4);
    void setDestination(const Point3& p0, const Point3& p1, const Point3& p2,
                   const Point3& p3, const Point3& p4);

    void computeProjective();

    const Projective& getProjective() const;

protected:
    Point3 sources[5];
    Point3 destinations[5];

    Projective projective;
};


END_CRUSTA


#endif //_Homography_H_
