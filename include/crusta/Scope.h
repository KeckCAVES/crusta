#ifndef _Scope_H_
#define _Scope_H_

#include <Geometry/Point.h>
#include <crusta/basics.h>

BEGIN_CRUSTA

/**
    A scope is used to define an area on the surface of the earth at a 
    particular resolution.
*/
class Scope
{
public:
    enum {
        LOWER_LEFT  = 0,
        LOWER_RIGHT,
        UPPER_LEFT,
        UPPER_RIGHT
    };

    typedef Geometry::Point<double, 3> Vertex;
    typedef std::vector<Vertex>        Vertices;
    typedef Vertex::Scalar             Scalar;

    Scope();
    Scope(const Vertex& p1,const Vertex& p2,const Vertex& p3,const Vertex& p4);

    /** retrieve the radius of the sphere the scope is associated with */
    Scalar getRadius();
    /** retrieve a refinement of the scope for given edge resolution */
    template <typename ScalarParam>
    void getRefinement(uint resolution, ScalarParam* vertices);
    
    /** generate the next refinement of the scope */
    void split(Scope scopes[4]);

    /** corner points of the scope in cartesian space in order lower-left,
        lower-right, upper-left, upper-right*/
    Vertex corners[4];

protected:
    template <typename ScalarParam>
    void mid(uint oneIndex, uint twoIndex, ScalarParam* vertices,
             ScalarParam radius);
    template <typename ScalarParam>
    centroid(uint oneIndex, uint twoIndex, uint threeIndex, uint fourIndex,
             ScalarParam* vertices, ScalarParam radius);
};

END_CRUSTA

#include <crusta/Scope.hpp>

#endif //_Scope_H_
