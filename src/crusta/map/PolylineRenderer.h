#ifndef _PolylineRenderer_H_
#define _PolylineRenderer_H_


#include <crusta/basics.h>
#include <crusta/CrustaComponent.h>
#include <crusta/SurfaceApproximation.h>


class GLContextData;


BEGIN_CRUSTA


class PolylineRenderer : public CrustaComponent
{
public:
    PolylineRenderer(Crusta* iCrusta);

    void display(GLContextData& contextData,
                 const SurfaceApproximation& surface) const;
};


END_CRUSTA


#endif //_PolylineRenderer_H_
