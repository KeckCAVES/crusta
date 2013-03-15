#ifndef _ConstruoVisualizer_H_
#define _ConstruoVisualizer_H_

#if CRUSTA_ENABLE_DEBUG

#include <crusta/Visualizer.h>

#include <crustacore/Scope.h>


namespace crusta {


class ConstruoVisualizer : public Visualizer
{
public:
    static void addSphereCoverage(const SphereCoverage& coverage, int temp=-1,
                                  const Color& color=defaultColor);
    static void addScopeRefinement(
        int resolution, Scope::Scalar* s, int temp=-1,
        const Color& color=defaultColor);
};


} //namespace crusta

#endif //CRUSTA_ENABLE_DEBUG

#endif //_ConstruoVisualizer_H_
