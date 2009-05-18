///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <construo/Filter.h>

BEGIN_CRUSTA

Filter::
Filter() :
    width(0), weights(NULL)
{}

Filter::
~Filter()
{
    delete[] (weights - width);
}

uint Filter::
getFilterWidth()
{
    return width;
}

Filter Filter::
createPointFilter()
{
    Filter filter;

    filter.width      = 0;
    filter.weights    = new Scalar[filter.width*2 + 1];
    filter.weights[0] = Scalar(1);

    return filter;
}

Filter Filter::
createTriangleFilter()
{
    Filter filter;

    filter.width       = 1;
    filter.weights     = new Scalar[filter.width*2 + 1] + filter.width;
    filter.weights[-1] = Scalar(0.25);
    filter.weights[0]  = Scalar(0.5);
    filter.weights[1]  = Scalar(0.25);

    return filter;
}

Filter Filter::
createFiveLobeLanczosFilter()
{
    Filter filter;

    filter.width      = 10;
    filter.weights    = new Scalar[filter.width*2 + 1] + filter.width;
    filter.weights[0] = Scalar(1);
    Scalar filterNorm = filter.weights[0];
    for(int i=1; i<=static_cast<int>(filter.width); ++i)
    {
        Scalar arg         = Math::Constants<Scalar>::pi * Scalar(i)/Scalar(2);
        Scalar value       = Math::sin(arg)/arg;
        arg               /= Scalar(5);
        value             *= Math::sin(arg)/arg;
        filter.weights[-i] = filter.weights[i] = value;
        filterNorm        += Scalar(2) * value;
    }
    filterNorm = Scalar(1) / filterNorm;
    for(int i=-filter.width; i<=static_cast<int>(filter.width); ++i)
        filter.weights[i] *= filterNorm;

    return filter;
}


END_CRUSTA
