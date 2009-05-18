#include <crusta/CrustaApp.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/Button.h>
#include <GLMotif/Menu.h>
#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>

BEGIN_CRUSTA

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults), crusta(NULL)
{
    if (argc<1)
    {
        Misc::throwStdErr("Crusta requires at least a preprocessed DEM database"
                          "to be provided as a command line argument");
    }
    crusta = new Crusta(argv[0]);

    produceMainMenu();
    resetNavigationCallback(NULL);
}

CrustaApp::
~CrustaApp()
{
    delete popMenu;
    delete crusta;
}

void CrustaApp::
produceMainMenu()
{
    /* Create a popup shell to hold the main menu: */
    popMenu = new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
    popMenu->setTitle("Crusta");
    
    /* Create the main menu itself: */
    GLMotif::Menu* mainMenu =
    new GLMotif::Menu("MainMenu",popMenu,false);
    
    /* Create a button: */
    GLMotif::Button* resetNavigationButton = new GLMotif::Button(
        "ResetNavigationButton",mainMenu,"Reset Navigation");
    
    /* Add a callback to the button: */
    resetNavigationButton->getSelectCallbacks().add(
        this, &CrustaApp::resetNavigationCallback);
    
    /* Finish building the main menu: */
    mainMenu->manageChild();
    
    Vrui::setMainMenu(popMenu);
}

void CrustaApp::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::NavTransform t=Vrui::NavTransform::identity;
    t*=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
    t*=Vrui::NavTransform::scale(/*(1.0/(SPHEROID_RADIUS * 39.3700787)) */
                                 0.1*Vrui::getInchFactor());
    Vrui::setNavigationTransformation(t);
}



void CrustaApp::
frame()
{
    crusta->frame();
}

void CrustaApp::
display(GLContextData& contextData) const
{
    crusta->display(contextData);
}


END_CRUSTA


int main(int argc, char* argv[])
{
	char** appDefaults = 0; // This is an additional parameter no one ever uses
    crusta::CrustaApp app(argc, argv, appDefaults);
	
	app.run();
	
    return 0;
}