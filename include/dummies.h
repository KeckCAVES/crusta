#ifndef _dummies_H_
#define _dummies_H_

#include <LodEvaluator.h>
#include <VisibilityEvaluator.h>

BEGIN_CRUSTA

class LodDummy : public LodEvaluator
{
public:
    float evaluate(const Scope& scope) {
        return 0.0;
    }
};

class VisibilityDummy : public VisibilityEvaluator
{
public:
    bool evaluate(const Scope& scope) {
        return true;
    }
};

END_CRUSTA

#endif //_dummies_H_
