#include <LodEvaluator.h>

#include <Scope.h>

BEGIN_CRUSTA

LodEvaluator::
LodEvaluator() :
    scale(1.0f)
{}

LodEvaluator::
~LodEvaluator()
{}

float LodEvaluator::
evaluate(const Scope& scope)
{
    return scale * compute(scope);
}

END_CRUSTA
