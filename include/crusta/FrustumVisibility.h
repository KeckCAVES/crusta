#ifndef _FrustumVisibility_H_
#define _FrustumVisibility_H_

#include <crusta/VisibilityEvaluator.h>

#include <GL/GLFrustum.h>

BEGIN_CRUSTA

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
    virtual bool evaluate(const Node* scope);
};

END_CRUSTA

#endif //_FrustumVisibility_H_
