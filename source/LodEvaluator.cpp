#include <LodEvaluator.h>

#include <Scope.h>

BEGIN_CRUSTA

LodEvaluator::
LodEvaluator() :
    bias(0.0f), scale(1.0f)
{}

LodEvaluator::
~LodEvaluator()
{}

float LodEvaluator::
evaluate(const Scope& scope)
{
    return (scale * compute(scope)) + bias;
}

END_CRUSTA
