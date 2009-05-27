#ifndef _VisibilityEvaluator_H_
#define _VisibilityEvaluator_H_

#include <crusta/basics.h>

BEGIN_CRUSTA

class Node;

class VisibilityEvaluator
{
public:
    virtual ~VisibilityEvaluator() {}
    
    virtual bool evaluate(const Node* scope) = 0;
};

END_CRUSTA

#endif //_VisibilityEvaluator_H_
