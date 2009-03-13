#include <Crusta.h>

#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/VisletManager.h>
#include <Vrui/Vrui.h>

#include <Spheroid.h>

BEGIN_CRUSTA

//- CrustaFactory --------------------------------------------------------------

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
createVislet(int numVisletArguments, const char* const visletArguments[]) const
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
    spheroid.frame();
///\todo only update when something has changed
    Vrui::requestUpdate();
}

void Crusta::
display(GLContextData& contextData) const
{
///\todo remove. Debug
    
//push everything to navigational coordinates for vislet
glPushMatrix();
glMultMatrix(Vrui::getNavigationTransformation());
    
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(1.0, 1.0, 1.0);
	glPointSize(5.0f);
	glBegin(GL_POINTS);
    glVertex3f(0,0,0);
	glEnd();
    
    glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(200,0,0);
	glEnd();
    
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,200,0);
	glEnd();
    
	glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,0,200);
	glEnd();
    
    glEnable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);

    spheroid.display(contextData);

glPopMatrix();
}


END_CRUSTA
