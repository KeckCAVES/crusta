#ifndef _LodEvaluator_H_
#define _LodEvaluator_H_

#include <crusta/basics.h>

BEGIN_CRUSTA

class Node;

/**
    Base class for Level-of-Detail (LOD) evaluation visitors.
 
    The visitors are applied to the base scopes of the global grid. They 
    traverse the refinement and prioritize the active scopes based on their
    evaluated LOD-value.
 
    The LOD-value returned adheres to the following convention:
    - a negative value characterises the degree to which the evaluated scope is
      finer than the ideal. A value smaller or equal to -1 recommends
      coarsening.
    - a value equal to zero denotes the ideal.
    - a positive value characterises teh degree to which the evaluated scope is
      coarser than the ideal. A value greater or equal to 1 recommends
      refinement.
    The LOD-value can be scaled and biased (linearly) to relax or tighten the 
    recommendations at -1 and 1.
 
    \see Refinement
*/
class LodEvaluator
{
public:
    LodEvaluator();
    virtual ~LodEvaluator();
    
    /** computes the LOD-value of a given scope */
    float evaluate(const Node* scope);

    /** biasing factor applied to the computed LOD-value. (default biasing is
        0) */
    float bias;
    /** scaling factor applied to the computed LOD-value. (default scaling is
        1) */
    float scale;
    
protected:
    /** specializations must provide the custom computed LOD-value */
    virtual float compute(const Node* scope) = 0;
};

END_CRUSTA

#endif //_LodEvaluator_H_
