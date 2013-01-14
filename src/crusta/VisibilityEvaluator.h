#ifndef _VisibilityEvaluator_H_
#define _VisibilityEvaluator_H_

#include <crustacore/basics.h>

BEGIN_CRUSTA

class NodeData;

class VisibilityEvaluator
{
public:
    virtual ~VisibilityEvaluator() {}

    virtual bool evaluate(const NodeData& node) = 0;
};

END_CRUSTA

#endif //_VisibilityEvaluator_H_
