#include <crusta/CrustaApp.h>

#include <sstream>

#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/Button.h>
#include <GLMotif/Label.h>
#include <GLMotif/Menu.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>
#include <crusta/Crusta.h>

BEGIN_CRUSTA

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults), crusta(NULL),
    newVerticalScale(1.0)
{
    std::string demName;
    std::string colorName;
    for (int i=0; i<argc; ++i)
    {
        if (strcmp(argv[i], "-dem")==0)
            demName   = std::string(argv[++i]);
        if (strcmp(argv[i], "-color")==0)
            colorName = std::string(argv[++i]);
    }
    crusta = new Crusta(demName, colorName);

    produceMainMenu();
    produceVerticalScaleDialog();

    resetNavigationCallback(NULL);
}

CrustaApp::
~CrustaApp()
{
    delete popMenu;
    delete verticalScaleDialog;
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

    /* Create a button to open or hide the vertical scale adjustment dialog: */
    GLMotif::ToggleButton* showVerticalScaleToggle = new GLMotif::ToggleButton(
        "ShowVerticalScaleToggle", mainMenu, "Vertical Scale");
    showVerticalScaleToggle->setToggle(false);
    showVerticalScaleToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showVerticalScaleCallback);

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
produceVerticalScaleDialog()
{
    const GLMotif::StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    verticalScaleDialog = new GLMotif::PopupWindow(
        "ScaleDialog", Vrui::getWidgetManager(), "Vertical Scale");
    GLMotif::RowColumn* root = new GLMotif::RowColumn(
        "ScaleRoot", verticalScaleDialog, false);
    GLMotif::Slider* slider = new GLMotif::Slider(
        "ScaleSlider", root, GLMotif::Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    verticalScaleLabel = new GLMotif::Label("ScaleLabel", root, "1.0x");

    slider->setValue(1.0);
    slider->setValueRange(-1000.0, 1000.0, 10.0);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::changeScaleCallback);

    root->setNumMinorWidgets(2);
    root->manageChild();
}

void CrustaApp::
showVerticalScaleCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        const Vrui::NavTransform& nav = Vrui::getNavigationTransformation();
        Vrui::popupPrimaryWidget(
            verticalScaleDialog, nav.transform(Vrui::getDisplayCenter()));
    }
    else
        Vrui::popdownPrimaryWidget(verticalScaleDialog);
}

void CrustaApp::
changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    if (cbData->value >= 0)
        newVerticalScale = 1 + cbData->value;
    else
        newVerticalScale = exp(cbData->value);

    std::ostringstream oss;
    oss.precision(2);
    oss << newVerticalScale << "x";
    verticalScaleLabel->setLabel(oss.str().c_str());
}

void CrustaApp::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::setNavigationTransformation(Vrui::Point(0,0,0), 1.5*SPHEROID_RADIUS);
}



void CrustaApp::
frame()
{
    Crusta::setVerticalScale(newVerticalScale);
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
