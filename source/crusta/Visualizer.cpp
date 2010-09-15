///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <crusta/Visualizer.h>

#include <iostream>

#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Button.h>
#include <GLMotif/Menu.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <construo/Converters.h>
#include <construo/SphereCoverage.h>
#include <crusta/checkGl.h>


BEGIN_CRUSTA


Visualizer* Visualizer::vis          = NULL;
Color       Visualizer::defaultColor = Color(1,1,1,1);

Visualizer::Primitive::
Primitive() :
    mode(0), color(Visualizer::defaultColor), centroid(0)
{}
Visualizer::Primitive::
Primitive(const Primitive& other) :
    mode(other.mode), color(other.color), centroid(other.centroid),
    vertices(other.vertices)
{}

void Visualizer::Primitive::
clear()
{
    mode     = 0;
    color    = Visualizer::defaultColor;
    centroid = Point3(0);
    vertices.clear();
}

void Visualizer::Primitive::
setVertices(const Point3s& verts)
{
    int numVertices = static_cast<int>(verts.size());
    //compute the centroid
    centroid = Point3(0);
    for (int i=0; i<numVertices; ++i)
    {
        for (int j=0; j<3; ++j)
            centroid[j] = centroid[j] + verts[i][j];
    }
    Scalar norm = Scalar(1) / numVertices;
    for (int i=0; i<3; ++i)
        centroid[i] = centroid[i] * norm;

    //store the centroid-centered vertices
    vertices.resize(numVertices);
    for (int i=0; i<numVertices; ++i)
    {
        for (int j=0; j<3; ++j)
            vertices[i][j] = verts[i][j] - centroid[j];
    }
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
clearAll()
{
    init();

    Threads::Mutex::Lock lock(vis->showMut);
    for (int i=0; i<10; ++i)
    {
        vis->tempPrimitives[i][0].clear();
        vis->tempPrimitives[i][1].clear();
    }
    vis->primitives[0].clear();
    vis->primitives[1].clear();

    vis->shownSlot = 1 - vis->shownSlot;
    Vrui::requestUpdate();
}

void Visualizer::
clear(int temp)
{
    init();

    Threads::Mutex::Lock lock(vis->showMut);
    if (temp!=-1)
    {
        vis->tempPrimitives[temp][0].clear();
        vis->tempPrimitives[temp][1].clear();
    }
    else
    {
        vis->primitives[0].clear();
        vis->primitives[1].clear();
    }

    vis->shownSlot = 1 - vis->shownSlot;
    Vrui::requestUpdate();
}

void Visualizer::
addPrimitive(GLenum drawMode, const Point3s& verts, int temp,
             const Color& color)
{
    Primitive& newPrim = vis->getNewPrimitive(temp);

    newPrim.mode  = drawMode;
    newPrim.color = color;
    newPrim.setVertices(verts);
}


void Visualizer::
peek()
{
    init();

    Threads::Mutex::Lock lock(vis->showMut);

    vis->shownSlot = 1 - vis->shownSlot;
    vis->primitives[1-vis->shownSlot] = vis->primitives[vis->shownSlot];
    for (int i=0; i<10; ++i)
    {
        vis->tempPrimitives[i][1-vis->shownSlot] =
            vis->tempPrimitives[i][vis->shownSlot];
    }

    Vrui::requestUpdate();
}

void Visualizer::
show(const char* message)
{
    init();

    peek();

    const char* msg = message!=NULL ? message : "Showing";

    std::cout << "Visualizer: " << msg << ", press continue...";
    std::cout.flush();

///\todo seems like the window segfaults on this
//    vis->continueWindow->setTitleString(msg);
//    vis->continueButton->setLabel("continue");

    Threads::Mutex::Lock lock(vis->showMut);
    vis->showCond.wait(vis->showMut);

//    vis->continueButton->setLabel("");
//    vis->continueWindow->setTitleString("Running...");

    std::cout << " resumed."  << std::endl;
    std::cout.flush();
}

void Visualizer::
display(GLContextData& contextData) const
{
    CHECK_GLA

    Threads::Mutex::Lock lock(showMut);

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
///\todo save client state and restore it later
    glEnableClientState(GL_VERTEX_ARRAY);

    glPointSize(5.0f);

    for (Primitives::const_iterator it=primitives[shownSlot].begin();
         it!=primitives[shownSlot].end(); ++it)
    {
        glPushMatrix();
        Vrui::Vector centroidTranslation(
            it->centroid[0], it->centroid[1], it->centroid[2]);
        Vrui::NavTransform nav =
            Vrui::getDisplayState(contextData).modelviewNavigational;
        nav *= Vrui::NavTransform::translate(centroidTranslation);
        glLoadMatrix(nav);

        glColor(it->color);
        glVertexPointer(3, GL_DOUBLE, 0, &it->vertices.front());
        glDrawArrays(it->mode, 0, it->vertices.size());

        glPopMatrix();

        CHECK_GLA
    }
    for (int i=0; i<10; ++i)
    {
        if (!tempPrimitives[i][shownSlot].vertices.empty())
        {
            const Primitive& p = tempPrimitives[i][shownSlot];

            glPushMatrix();
            Vrui::Vector centroidTranslation(
                p.centroid[0], p.centroid[1], p.centroid[2]);
            Vrui::NavTransform nav =
                Vrui::getDisplayState(contextData).modelviewNavigational;
            nav *= Vrui::NavTransform::translate(centroidTranslation);
            glLoadMatrix(nav);

            glColor(p.color);
            glVertexPointer(3, GL_DOUBLE, 0, &p.vertices.front());
            glDrawArrays(p.mode, 0, p.vertices.size());

            glPopMatrix();

            CHECK_GLA
        }
    }

    glPopAttrib();

    CHECK_GLA
}

void Visualizer::frame() {}

void Visualizer::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::setNavigationTransformation(Vrui::Point(0,0,0), 1.0);
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
        "VisualizerDialog", Vrui::getWidgetManager(), "Running...");

    continueButton = new GLMotif::Button(
        "ContinueButton", continueWindow, "");
    continueButton->getSelectCallbacks().add(this,
                                             &Visualizer::continueCallback);

    const Vrui::NavTransform& nav = Vrui::getNavigationTransformation();
    Vrui::popupPrimaryWidget(continueWindow,
                             nav.transform(Vrui::getDisplayCenter()));
}


Visualizer::Primitive& Visualizer::
getNewPrimitive(int temp)
{
    init();

    if (temp>=0 && temp<10)
    {
        return vis->tempPrimitives[temp][1-vis->shownSlot];
    }
    else
    {
        vis->primitives[1-vis->shownSlot].push_back(Primitive());
        return vis->primitives[1-vis->shownSlot].back();
    }
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
