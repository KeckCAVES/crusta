#ifndef _Scope_H_
#define _Scope_H_


#include <vector>

#include <Geometry/Point.h>
#include <crustacore/basics.h>


namespace crusta {


/**\todo hide corners behind getters, setters to add an explicitely stored
    centroid and relative to centroid corner positions, make API explicit
    about specifying an optional radius*/
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

///\todo deprecate the Vertex name
    typedef Geometry::Point<double, 3> Vertex;
    typedef std::vector<Vertex>        Vertices;
    typedef Vertex::Scalar             Scalar;

    Scope();
    Scope(const Vertex& p1,const Vertex& p2,const Vertex& p3,const Vertex& p4);

    /** retrieve the radius of the sphere the scope is associated with */
    Scalar getRadius() const;
    /** retrieve the centroid of the scope */
    Vertex getCentroid(Scalar radius=0.0) const;
    /** retrieve a refinement of the scope for given edge resolution */
    template <typename ScalarParam>
    void getRefinement(size_t resolution, ScalarParam* vertices) const;
    /** retrieve a refinement of the scope where the coordiantes are relative
        to the centroid for given edge resolution */
    template <typename ScalarParam>
    void getCentroidRefinement(size_t resolution, ScalarParam* vertices) const;
    /** retrieve a refinement of the scope for given edge resolution on a sphere
        of given radius */
    template <typename ScalarParam>
    void getRefinement(ScalarParam radius, size_t resolution,
                       ScalarParam* vertices) const;
    /** retrieve a refinement of the scope where the coordiantes are relative
        to the centroid for given edge resolution on a sphere of given radius */
    template <typename ScalarParam>
    void getCentroidRefinement(ScalarParam radius, size_t resolution,
                               ScalarParam* vertices) const;

    /** generate the next refinement of the scope */
    void split(Scope scopes[4]) const;

    /** check if a point is contained in the solid angle subtended by the
        scope */
    bool contains(const Scope::Vertex& p) const;
    /** check if a line segment intersects the solid angle subtended by the
        scope */
    bool intersects(const Geometry::Point<double,3>& start, const Geometry::Point<double,3>& end);

    /** corner points of the scope in cartesian space in order lower-left,
        lower-right, upper-left, upper-right*/
    Vertex corners[4];

protected:
    template <typename ScalarParam>
    void mid(size_t oneIndex, size_t twoIndex, ScalarParam* vertices,
             ScalarParam radius) const;
    template <typename ScalarParam>
    void centroid(size_t oneIndex, size_t twoIndex, size_t threeIndex, size_t fourIndex,
             ScalarParam* vertices, ScalarParam radius) const;
};


} //namespace crusta


#include <crustacore/Scope.hpp>


#endif //_Scope_H_
