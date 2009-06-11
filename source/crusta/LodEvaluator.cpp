#include <crusta/LodEvaluator.h>

#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

LodEvaluator::
LodEvaluator() :
    bias(0.0f), scale(1.0f)
{}

LodEvaluator::
~LodEvaluator()
{}

float LodEvaluator::
evaluate(const QuadNodeMainData& mainData)
{
    return (scale * compute(mainData)) + bias;
}

END_CRUSTA
