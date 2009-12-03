#ifndef _CrustaComponent_H_
#define _CrustaComponent_H_


#include <crusta/basics.h>


BEGIN_CRUSTA

class Crusta;

class CrustaComponent
{
public:
    CrustaComponent(Crusta* iCrusta) : crusta(iCrusta) {}
    void setupComponent(Crusta* nCrusta) { crusta=nCrusta; }

protected:
    Crusta* crusta;
};


END_CRUSTA


#endif //_CrustaComponent_H_
