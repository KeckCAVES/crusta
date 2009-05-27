#include <crusta/LodEvaluator.h>

#include <crusta/Node.h>

BEGIN_CRUSTA

LodEvaluator::
LodEvaluator() :
    bias(0.0f), scale(1.0f)
{}

LodEvaluator::
~LodEvaluator()
{}

float LodEvaluator::
evaluate(const Node* node)
{
    return (scale * compute(node)) + bias;
}

END_CRUSTA
