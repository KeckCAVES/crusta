#ifndef _FrustumVisibility_H_
#define _FrustumVisibility_H_

#include <crusta/VisibilityEvaluator.h>

#include <crusta/vrui.h>

namespace crusta {

/**
    Specialization of the VisibilityEvaluator that considers a viewing frustum
    to determine the visibility of a scope.
*/
class FrustumVisibility : public VisibilityEvaluator
{
public:
    /** the specification of the viewing parameters */
    GLFrustum<double> frustum;

//- inherited from VisibilityEvaluator
public:
    virtual bool evaluate(const NodeData& node);
};

} //namespace crusta

#endif //_FrustumVisibility_H_
