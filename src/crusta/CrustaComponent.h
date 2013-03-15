#ifndef _CrustaComponent_H_
#define _CrustaComponent_H_


#include <crustacore/basics.h>


namespace crusta {

class Crusta;

class CrustaComponent
{
public:
    CrustaComponent(Crusta* iCrusta) : crusta(iCrusta) {}
    virtual ~CrustaComponent() {}
    virtual void setupComponent(Crusta* nCrusta) { crusta=nCrusta; }

protected:
    Crusta* crusta;
};


} //namespace crusta


#endif //_CrustaComponent_H_
