#include <Crusta.h>

#include <GL/GLContextData.h>
#include <Vrui/VisletManager.h>

BEGIN_CRUSTA

//- CrustaFactory -------------------------------------------------------------

CrustaFactory::
CrustaFactory(Vrui::VisletManager& visletManager) :
    VisletFactory("Crusta", visletManager)
{
    Crusta::factory = this;
}

CrustaFactory::
~CrustaFactory()
{
    Crusta::factory = NULL;
}


Vrui::Vislet* CrustaFactory::
createVislet(int numVisletArguments, const char* visletArguments[]) const
{
    return new Crusta;
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

//- Crusta --------------------------------------------------------------------

CrustaFactory* Crusta::factory = NULL;

Vrui::VisletFactory* Crusta::
getFactory() const
{
    return factory;
}

void Crusta::
frame()
{
}

void Crusta::
display(GLContextData& contextData) const
{
}


END_CRUSTA
