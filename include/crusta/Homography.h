#ifndef _Homography_H_
#define _Homography_H_


#include <Geometry/ProjectiveTransformation.h>
#include <crusta/basics.h>


BEGIN_CRUSTA


class Homography
{
public:
    typedef Geometry::ProjectiveTransformation<double,3> Projective;
    typedef Projective::HVector                          HVector;

    void setSource(const HVector& p0, const HVector& p1, const HVector& p2,
                   const HVector& p3, const HVector& p4);
    void setDestination(const HVector& p0, const HVector& p1, const HVector& p2,
                   const HVector& p3, const HVector& p4);

    void computeProjective();

    const Projective& getProjective() const;

protected:
    HVector sources[5];
    HVector destinations[5];

    Projective projective;
};


END_CRUSTA


#endif //_Homography_H_
