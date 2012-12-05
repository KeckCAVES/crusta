#ifndef _ConstruoVisualizer_H_
#define _ConstruoVisualizer_H_

#include <crusta/Visualizer.h>

#include <crusta/Scope.h>


BEGIN_CRUSTA


class ConstruoVisualizer : public Visualizer
{
public:
    static void addSphereCoverage(const SphereCoverage& coverage, int temp=-1,
                                  const Color& color=defaultColor);
    static void addScopeRefinement(
        int resolution, Scope::Scalar* s, int temp=-1,
        const Color& color=defaultColor);
};


END_CRUSTA


#endif //_ConstruoVisualizer_H_
