#ifndef _CrustaVisualizer_H_
#define _CrustaVisualizer_H_

#include <crusta/Visualizer.h>


BEGIN_CRUSTA


class Scope;
class Section;
class Triangle;

class CrustaVisualizer : public Visualizer
{
public:
    static Color defaultScopeColor;
    static Color defaultSectionColor;
    static Color defaultTriangleColor;
    static Color defaultRayColor;
    static Color defaultHitColor;

    static Color defaultSideInColor;

    static void addScope(const Scope& s, int temp=-1,
                         const Color& color=defaultScopeColor);
    static void addSection(const Section& s, int temp=-1,
                           const Color& color=defaultSectionColor);
    static void addTriangle(const Triangle& t, int temp=-1,
                            const Color& color=defaultTriangleColor);
    static void addRay(const Ray& r, int temp=-1,
                       const Color& color=defaultRayColor);
    static void addHit(const Ray& r, const HitResult& h, int temp=-1,
                       const Color& color=defaultHitColor);

    static void addSideIn(const int sideIn, const Scope& s, int temp=-1,
                          const Color& color=defaultSideInColor);

//- Inherited vrom Visualizer
public:
    virtual void resetNavigationCallback(Misc::CallbackData* cbData);
};


END_CRUSTA


#endif //_CrustaVisualizer_H_
