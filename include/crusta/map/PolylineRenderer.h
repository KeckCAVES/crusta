#ifndef _PolylineRenderer_H_
#define _PolylineRenderer_H_


#include <crusta/basics.h>
#include <crusta/CrustaComponent.h>


class GLContextData;


BEGIN_CRUSTA


class Polyline;


class PolylineRenderer : public CrustaComponent
{
public:
    typedef std::vector<Polyline*> Ptrs;

    PolylineRenderer(Crusta* iCrusta);

    void display(GLContextData& contextData) const;

    Ptrs* lines;
};

END_CRUSTA


#endif //_PolylineRenderer_H_
