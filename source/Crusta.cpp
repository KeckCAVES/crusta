#include <Crusta.h>

#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/VisletManager.h>
#include <Vrui/Vrui.h>

#if 0
///DBG
#include <Geometry/ProjectiveTransformation.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
///DBG
#endif
#include <SpheroidGrid.h>

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

Crusta::
Crusta()
{
#if 0
///\todo remove. Debug
Scope patch;
patch.corners[0] = Point(-1, 0.5, 1);
patch.corners[1] = Point(-1, 0.5,-1);
patch.corners[2] = Point( 1, 0.5,-1);
patch.corners[3] = Point( 1, 0.5, 1);

for (uint i=0; i<4; ++i)
{
    float mag = Geometry::mag(patch.corners[i]);
    for (uint j=0; j<3; ++j)
        patch.corners[i][j] /= mag;
}

tree = new QuadTree(patch);
lod = new ViewLod;
visibility = new FrustumVisibility;
#endif

    globalGrid = new SpheroidGrid;
}

Crusta::
~Crusta()
{
#if 0
///\todo remove. Debug
delete tree;
delete lod;
delete visibility;
#endif
    delete globalGrid;
}

Vrui::VisletFactory* Crusta::
getFactory() const
{
    return factory;
}

void Crusta::
frame()
{
    globalGrid->frame();
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

#if 0
lod->frustum.setFromGL();
visibility->frustum.setFromGL();

/** offset the view in Z */
#if 1
//    glTranslatef(0.0f, 2.0f, 0.0f);
#else
typedef Geometry::ProjectiveTransformation<double,3> PTransform;
/* Read projection and modelview matrices from OpenGL: */
PTransform pmv=glGetProjectionMatrix<double>();
pmv*=glGetModelviewMatrix<double>();
glLoadMatrix
#endif


tree->refine(*visibility, *lod);
tree->draw();
#endif
    globalGrid->display(contextData);

glPopMatrix();
}


END_CRUSTA
