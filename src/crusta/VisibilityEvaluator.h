#ifndef _VisibilityEvaluator_H_
#define _VisibilityEvaluator_H_

#include <crustacore/basics.h>

namespace crusta {

class NodeData;

class VisibilityEvaluator
{
public:
    virtual ~VisibilityEvaluator() {}

    virtual bool evaluate(const NodeData& node) = 0;
};

} //namespace crusta

#endif //_VisibilityEvaluator_H_
