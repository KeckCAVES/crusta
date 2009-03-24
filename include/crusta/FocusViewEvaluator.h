#ifndef _FocusViewEvaluator_H_
#define _FocusViewEvaluator_H_

#include <crusta/LodEvaluator.h>

#include <GL/GLFrustum.h>

BEGIN_CRUSTA

/**
    Specialized evaluator that considers coverage of the screen projection and
    location of a point of focus in determining a scope's level-of-detail (LOD)
    value.
*/
class FocusViewEvaluator : public LodEvaluator
{
public:
    /** the specification of the viewing parameters */
    GLFrustum<float> frustum;
    /** the position of the point of focus */

//- inherited from LodEvaluator
public:
    virtual float compute(const Scope& scope);
};

END_CRUSTA

#endif //_FocusViewEvaluator_H_
