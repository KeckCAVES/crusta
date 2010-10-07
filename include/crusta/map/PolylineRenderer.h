#ifndef _PolylineRenderer_H_
#define _PolylineRenderer_H_


#include <crusta/basics.h>
#include <crusta/DataManager.h>
#include <crusta/CrustaComponent.h>


class GLContextData;


BEGIN_CRUSTA


class PolylineRenderer : public CrustaComponent
{
public:
    PolylineRenderer(Crusta* iCrusta);

    void display(const DataManager::NodeMainDatas& renderNodes,
                 GLContextData& contextData) const;
};


END_CRUSTA


#endif //_PolylineRenderer_H_
