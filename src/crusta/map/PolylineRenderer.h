#ifndef _PolylineRenderer_H_
#define _PolylineRenderer_H_


#include <crustacore/basics.h>
#include <crusta/CrustaComponent.h>
#include <crusta/SurfaceApproximation.h>


class GLContextData;


namespace crusta {


class PolylineRenderer : public CrustaComponent
{
public:
    PolylineRenderer(Crusta* iCrusta);

    void display(GLContextData& contextData,
                 const SurfaceApproximation& surface) const;
};


} //namespace crusta


#endif //_PolylineRenderer_H_
