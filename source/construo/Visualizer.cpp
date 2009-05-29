///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <construo/Visualizer.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/Button.h>
#include <GLMotif/Menu.h>

#include <construo/Converters.h>
#include <construo/SphereCoverage.h>

BEGIN_CRUSTA

Visualizer* Visualizer::vis = NULL;

Visualizer::Primitive::
Primitive() :
    mode(GL_POINTS)
{
    color[0] = color[1] = color[2] = 1.0f;
}
Visualizer::Primitive::
Primitive(const Primitive& other) :
    mode(other.mode), vertices(other.vertices)
{
    color[0] = other.color[0];
    color[1] = other.color[1];
    color[2] = other.color[2];
}


Visualizer::
Visualizer(int& argc,char**& argv,char**& appDefaults) :
    Vrui::Application(argc,argv,appDefaults), shownSlot(0),
    popMenu(NULL), continueWindow(NULL)
{
    produceMainMenu();
    produceContinueWindow();
    resetNavigationCallback(0);
    
    appThread.start(this, &Visualizer::runWrapper);
}

Visualizer::
~Visualizer()
{
    appThread.cancel();
    appThread.join();

    delete continueWindow;
    delete popMenu;
}

void Visualizer::
init()
{
    if (vis == NULL)
    {
        int argc = 1;
        char v[] = "visualizer";
        char* vv = &v[0];
        char** argv = &vv;
        char** appDefaults = NULL;
        vis = new Visualizer(argc, argv, appDefaults);
    }
}

void Visualizer::
start(int& argc,char**& argv,char**& appDefaults)
{
    delete vis;
    vis = new Visualizer(argc, argv, appDefaults);
}
void Visualizer::
stop()
{
    delete vis;
    vis = NULL;
}

void Visualizer::
clear()
{
    init();

    Threads::Mutex::Lock lock(vis->showMut);
    vis->primitives[0].clear();
    vis->primitives[1].clear();
    vis->shownSlot = 1 - vis->shownSlot;

    Vrui::requestUpdate();
}

void Visualizer::
addPrimitive(GLenum drawMode, const Floats& verts, const float* color)
{
    init();

    vis->primitives[1-vis->shownSlot].push_back(Primitive());
    Primitive& newPrim  = vis->primitives[1-vis->shownSlot].back();

    newPrim.mode        = drawMode;
    if (color != NULL)
    {
        newPrim.color[0] = color[0];
        newPrim.color[1] = color[1];
        newPrim.color[2] = color[2];
    }
    newPrim.vertices    = verts;
}

void Visualizer::
addPrimitive(GLenum drawMode, const SphereCoverage& coverage,
             const float* color)
{
    init();

    uint num;
    uint numVertices;
    float* curVert;
#if 0
    vis->primitives[1-vis->shownSlot].push_back(Primitive());
    Primitive& box  = vis->primitives[1-vis->shownSlot].back();
    
    const Box& scb    = coverage.getBoundingBox();
    box.color[0]      = box.color[1] = box.color[2] = 0.5f;
    box.mode          = GL_LINES;
    numVertices       = 4*2*3;
    box.vertices.resize(numVertices);
    curVert           = &box.vertices[0];
    Point scbVerts[4] = {
        Point(scb.min[0], scb.min[1]), Point(scb.max[0], scb.min[1]),
        Point(scb.max[0], scb.max[1]), Point(scb.min[0], scb.max[1])
    };
    for (uint j=0; j<4; ++j)
    {
        for (uint k=0; k<2; ++k)
        {
            *curVert = scbVerts[(j+k)%4][0]; ++curVert;
            *curVert =                  0.0; ++curVert;
            *curVert = scbVerts[(j+k)%4][1]; ++curVert;
        }
    }
#endif
    
#if 0
    vis->primitives[1-vis->shownSlot].push_back(Primitive());
    Primitive& cen  = vis->primitives[1-vis->shownSlot].back();
    
    const Point& scc = coverage.getCentroid();
    cen.color[0]    = 1.0f;
    cen.color[1]    = cen.color[2] = 0.25f;
    cen.mode        = GL_POINTS;
    numVertices     = 3;
    cen.vertices.resize(numVertices);
    cen.vertices[0] = scc[0];
    cen.vertices[1] =    0.0;
    cen.vertices[2] = scc[1];
#endif

    vis->primitives[1-vis->shownSlot].push_back(Primitive());
    Primitive& cov = vis->primitives[1-vis->shownSlot].back();
    
    const SphereCoverage::Points& scv = coverage.getVertices();
    
    num          = (uint)scv.size();
    cov.mode     = drawMode;
    if (color != NULL)
    {
        cov.color[0] = color[0];
        cov.color[1] = color[1];
        cov.color[2] = color[2];
    }
    numVertices  = num*2*3;
    cov.vertices.resize(numVertices);
    curVert      = &cov.vertices[0];
    for (uint j=0; j<num; ++j)
    {
        for (uint k=0; k<2; ++k)
        {
            *curVert = scv[(j+k)%num][0]; ++curVert;
            *curVert =               0.0; ++curVert;
            *curVert = scv[(j+k)%num][1]; ++curVert;
        }
    }
}

void Visualizer::
addScopeRefinement(uint resolution, const Scope::Scalar* s, const float* color)
{
    Visualizer::Floats scopeRef;
    uint numScopeSamples = resolution*resolution*3;
    scopeRef.resize(numScopeSamples);
    float* out = &scopeRef[0];
    for (const Scope::Scalar* in=s; in<s+numScopeSamples; in+=3,out+=3)
    {
        Point p =
            Converter::cartesianToSpherical(Scope::Vertex(in[0], in[1], in[2]));
        out[0] = p[0];
        out[1] =  0.0;
        out[2] = p[1];
    }
    Visualizer::addPrimitive(GL_POINTS, scopeRef, color);
}

void Visualizer::
peek()
{
    init();
    
    Threads::Mutex::Lock lock(vis->showMut);
    
    vis->shownSlot = 1 - vis->shownSlot;
    vis->primitives[1-vis->shownSlot] = vis->primitives[vis->shownSlot];
    
    Vrui::requestUpdate();
}

void Visualizer::
show()
{
    init();

    peek();
    Threads::Mutex::Lock lock(vis->showMut);
    vis->showCond.wait(vis->showMut);
}

void Visualizer::
display(GLContextData& contextData) const
{
    Threads::Mutex::Lock lock(showMut);

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glPointSize(5.0f);

    for (Primitives::const_iterator it=primitives[shownSlot].begin();
         it!=primitives[shownSlot].end(); ++it)
    {
        glColor3fv(it->color);
        glBegin(it->mode);
            for (int i=0; i<(int)it->vertices.size(); i+=3)
                glVertex3f(it->vertices[i],it->vertices[i+1],it->vertices[i+2]);
        glEnd();
    }

    glPopAttrib();
}

void Visualizer::frame() {}

void Visualizer::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::NavTransform t=Vrui::NavTransform::identity;
    t*=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
    t*=Vrui::NavTransform::scale(Vrui::getInchFactor());
    Vrui::setNavigationTransformation(t);
}
void Visualizer::
continueCallback(Misc::CallbackData* cbData)
{
    showCond.signal();
}

    
void Visualizer::
produceMainMenu()
{
    /* Create a popup shell to hold the main menu: */
    popMenu = new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
    popMenu->setTitle("Visualizer");
    
    /* Create the main menu itself: */
    GLMotif::Menu* mainMenu =
        new GLMotif::Menu("MainMenu",popMenu,false);
    
    /* Create a button: */
    GLMotif::Button* resetNavigationButton = new GLMotif::Button(
        "ResetNavigationButton",mainMenu,"Reset Navigation");
    
    /* Add a callback to the button: */
    resetNavigationButton->getSelectCallbacks().add(
        this,&Visualizer::resetNavigationCallback);
    
    /* Finish building the main menu: */
    mainMenu->manageChild();
    
    Vrui::setMainMenu(popMenu);
}

void Visualizer::
produceContinueWindow()
{
    continueWindow = new GLMotif::PopupWindow(
        "VisualizerDialog", Vrui::getWidgetManager(), "Visualizer Dialog");

    GLMotif::Button* continueButton = new GLMotif::Button(
        "ContinueButton", continueWindow, "Continue");
    continueButton->getSelectCallbacks().add(this,
                                             &Visualizer::continueCallback);

    const Vrui::NavTransform& nav = Vrui::getNavigationTransformation();
    Vrui::popupPrimaryWidget(continueWindow,
                             nav.transform(Vrui::getDisplayCenter()));
}

void* Visualizer::
runWrapper()
{
    run();
    delete vis;
    vis = NULL;

    return NULL;
}

END_CRUSTA
