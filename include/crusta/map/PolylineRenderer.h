#ifndef _PolylineRenderer_H_
#define _PolylineRenderer_H_


#include <crusta/basics.h>
#include <crusta/CrustaComponent.h>


class GLContextData;


BEGIN_CRUSTA


class QuadNodeMainData;

class PolylineRenderer : public CrustaComponent
{
public:
    PolylineRenderer(Crusta* iCrusta);

    void display(std::vector<QuadNodeMainData*>& renderNodes,
                 GLContextData& contextData) const;
};


END_CRUSTA


#endif //_PolylineRenderer_H_
