#include <crusta/CrustaVislet.h>

#include <Vrui/VisletManager.h>

#include <crusta/Crusta.h>

BEGIN_CRUSTA

//- CrustaFactory --------------------------------------------------------------

CrustaFactory::
CrustaFactory(Vrui::VisletManager& visletManager) :
    VisletFactory("CrustaVislet", visletManager)
{
    CrustaVislet::factory = this;
}

CrustaFactory::
~CrustaFactory()
{
    CrustaVislet::factory = NULL;
}


Vrui::Vislet* CrustaFactory::
createVislet(int numVisletArguments, const char* const visletArguments[]) const
{
    return new CrustaVislet;
}

void CrustaFactory::
destroyVislet(Vrui::Vislet* vislet) const
{
    delete vislet;
}

extern "C" void
resolveCrustaDependencies(
    Plugins::FactoryManager<Vrui::VisletFactory>& manager)
{
}
extern "C" Vrui::VisletFactory*
createCrustaFactory(Plugins::FactoryManager<Vrui::VisletFactory>& manager)
{
    //get a handle to the vislet manager
    Vrui::VisletManager* visletManager =
        static_cast<Vrui::VisletManager*>(&manager);
    //create factory object and insert it into class hierarchy
    CrustaFactory* crustaFactory = new CrustaFactory(*visletManager);
    //return factory object
    return crustaFactory;
}
extern "C" void
destroyCrustaFactory(Vrui::VisletFactory* factory)
{
    delete factory;
}

//- CrustaVislet ---------------------------------------------------------------

CrustaFactory* CrustaVislet::factory = NULL;

CrustaVislet::
CrustaVislet() :
    crusta(new Crusta)
{}
CrustaVislet::
~CrustaVislet()
{
    delete crusta;
}

Vrui::VisletFactory* CrustaVislet::
getFactory() const
{
    return factory;
}

void CrustaVislet::
frame()
{
    crusta->frame();
}

void CrustaVislet::
display(GLContextData& contextData) const
{
    crusta->display(contextData);
}


END_CRUSTA
