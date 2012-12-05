#ifndef _ViewLod_H_
#define _ViewLod_H_

#include <crusta/LodEvaluator.h>

#include <GL/GLFrustum.h>

BEGIN_CRUSTA

/**
 Specialized evaluator that considers coverage of the screen projection and
 location of a point of focus in determining a scope's level-of-detail (LOD)
 value.
 */
class ViewLod : public LodEvaluator
{
public:
    /** the specification of the viewing parameters */
    GLFrustum<float> frustum;

//- inherited from LodEvaluator
public:
    virtual float compute(const NodeData& node);
};

END_CRUSTA

#endif //_ViewLod_H_
