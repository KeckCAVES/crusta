#ifndef _FocusViewEvaluator_H_
#define _FocusViewEvaluator_H_

#include <crusta/LodEvaluator.h>

#include <crusta/vrui.h>

namespace crusta {

/**
    Specialized evaluator that considers coverage of the screen projection and
    location of a point of focus in determining a scope's level-of-detail (LOD)
    value.
*/
class FocusViewEvaluator : public LodEvaluator
{
public:
    /** update the focus area from the display center */
    void setFocusFromDisplay();

    /** the specification of the viewing parameters */
    GLFrustum<double> frustum;
    /** the position of the point of focus */
    Geometry::Point<double, 3> focusCenter;
    /** the radius of the focus area */
    double focusRadius;

//- inherited from LodEvaluator
public:
    virtual float compute(const NodeData& node);
};

} //namespace crusta

#endif //_FocusViewEvaluator_H_
