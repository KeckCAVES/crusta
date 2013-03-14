#ifndef _Refinement_H_
#define _Refinement_H_

BEGIN_CRUSTA

class Ellipsoid;
class Scope;

class Refinement
{
public:
    /** retrieve a refinement of the scope for given edge resolution */
    template <typename ScalarParam>
    static void refine(const Ellipsoid& ellipse, const Scope& scope,
                       size_t resolution,
                       Geometry::Point<ScalarParam, 3>* vertices);
    /** retrieve a refinement of the scope where the coordiantes are relative
     to the centroid for given edge resolution */
    template <typename ScalarParam>
    static void centroidRefine(const Ellipsoid& ellipse, const Scope& scope,
                               size_t resolution,
                               Geometry::Point<ScalarParam, 3>* vertices);
    
    /** generate the next refinement of the scope */
    static void splitScope(const Ellipsoid& ellipse, const Scope& parent,
                           Scope children[4]);

protected:
    template <typename ScalarParam>
    void mid(const Ellipsoid& ellipse, size_t oneIndex, size_t twoIndex,
             Geometry::Point<ScalarParam, 3>* vertices) const;
    template <typename ScalarParam>
    void centroid(const Ellipsoid& ellipse, size_t oneIndex, size_t twoIndex,
                  size_t threeIndex, size_t fourIndex,
                  Geometry::Point<ScalarParam, 3>* vertices) const;
};

END_CRUSTA

#include <crusta/Refinement.hpp>

#endif //_Refinement_H_
