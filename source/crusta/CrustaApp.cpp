#include <crusta/CrustaApp.h>

#include <sstream>

#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/Label.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Menu.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/TextField.h>
#include <GLMotif/WidgetManager.h>
#include <GL/GLContextData.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>
#include <crusta/MapManager.h>
#include <crusta/QuadTerrain.h>


#include <Geometry/Geoid.h>
#include <Misc/FunctionCalls.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Tools/SurfaceNavigationTool.h>

BEGIN_CRUSTA

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults),
    newVerticalScale(1.0), enableSun(false),
    viewerHeadlightStates(new bool[Vrui::getNumViewers()]),
	sun(0),sunAzimuth(180.0),sunElevation(45.0)
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
    Crusta::init(demName, colorName);

	/* Create the sun lightsource: */
	sun=Vrui::getLightsourceManager()->createLightsource(false);
	updateSun();

	/* Save all viewers' headlight enable states: */
	for(int i=0;i<Vrui::getNumViewers();++i)
		viewerHeadlightStates[i]=true; // Temporary hack; should be Vrui::getViewer(i)->getHeadlight().isEnabled();

    produceMainMenu();
    produceVerticalScaleDialog();
    produceLightingDialog();

    resetNavigationCallback(NULL);
}

CrustaApp::
~CrustaApp()
{
    delete lightingDialog;
    delete popMenu;
	/* Delete the viewer headlight states: */
	delete[] viewerHeadlightStates;
    delete verticalScaleDialog;

    Crusta::shutdown();
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

    /* Create a button to toogle display of the debugging grid: */
    GLMotif::ToggleButton* debugGridToggle = new GLMotif::ToggleButton(
        "DebugGridToggle", mainMenu, "Debug Grid");
    debugGridToggle->setToggle(false);
    debugGridToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::debugGridCallback);

    /* Create a button to toogle display of the lighting dialog: */
    GLMotif::ToggleButton* lightingToggle = new GLMotif::ToggleButton(
        "LightingToggle", mainMenu, "Light Settings");
    lightingToggle->setToggle(false);
    lightingToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showLightingDialogCallback);

    /* Create a button: */
    GLMotif::Button* resetNavigationButton = new GLMotif::Button(
        "ResetNavigationButton",mainMenu,"Reset Navigation");

    /* Add a callback to the button: */
    resetNavigationButton->getSelectCallbacks().add(
        this, &CrustaApp::resetNavigationCallback);


    produceToolSubMenu(mainMenu);


    /* Finish building the main menu: */
    mainMenu->manageChild();

    Vrui::setMainMenu(popMenu);
}

void CrustaApp::
produceToolSubMenu(GLMotif::Menu* mainMenu)
{
    GLMotif::Popup* toolMenuPopup = new GLMotif::Popup(
        "ToolPopup", Vrui::getWidgetManager());
    toolMenuPopup->setTitle("Tools");

    curTool = new GLMotif::RadioBox("ToolMenu", toolMenuPopup, false);
    curTool->setOrientation(GLMotif::RowColumn::VERTICAL);
    curTool->setNumMinorWidgets(1);
    curTool->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);

    GLMotif::ToggleButton* cpe = new GLMotif::ToggleButton(
        "ControlPointEditor", curTool, "Control Point Editor");

    curTool->manageChild();
    curTool->setSelectedToggle(cpe);

    GLMotif::CascadeButton* toolCascade = new GLMotif::CascadeButton(
        "ToolCascade", mainMenu, "Tools");
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

    slider->setValue(0.0);
    slider->setValueRange(-0.5, 2.5, 0.00001);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::changeScaleCallback);

    root->setNumMinorWidgets(2);
    root->manageChild();
}

void CrustaApp::
produceLightingDialog()
{
    const GLMotif::StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();
	lightingDialog=new GLMotif::PopupWindow("LightingDialog",Vrui::getWidgetManager(),"Light Settings");
	GLMotif::RowColumn* lightSettings=new GLMotif::RowColumn("LightSettings",lightingDialog,false);
	lightSettings->setNumMinorWidgets(2);

	/* Create a toggle button and two sliders to manipulate the sun light source: */
	GLMotif::Margin* enableSunToggleMargin=new GLMotif::Margin("SunToggleMargin",lightSettings,false);
	enableSunToggleMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::VCENTER));
	GLMotif::ToggleButton* enableSunToggle=new GLMotif::ToggleButton("SunToggle",enableSunToggleMargin,"Sun Light Source");
	enableSunToggle->setToggle(enableSun);
	enableSunToggle->getValueChangedCallbacks().add(this,&CrustaApp::enableSunToggleCallback);
	enableSunToggleMargin->manageChild();

	GLMotif::RowColumn* sunBox=new GLMotif::RowColumn("SunBox",lightSettings,false);
	sunBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	sunBox->setNumMinorWidgets(2);
	sunBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);

	sunAzimuthTextField=new GLMotif::TextField("SunAzimuthTextField",sunBox,5);
	sunAzimuthTextField->setFloatFormat(GLMotif::TextField::FIXED);
	sunAzimuthTextField->setFieldWidth(3);
	sunAzimuthTextField->setPrecision(0);
	sunAzimuthTextField->setValue(double(sunAzimuth));

	sunAzimuthSlider=new GLMotif::Slider("SunAzimuthSlider",sunBox,GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
	sunAzimuthSlider->setValueRange(0.0,360.0,1.0);
	sunAzimuthSlider->setValue(double(sunAzimuth));
	sunAzimuthSlider->getValueChangedCallbacks().add(this,&CrustaApp::sunAzimuthSliderCallback);

	sunElevationTextField=new GLMotif::TextField("SunElevationTextField",sunBox,5);
	sunElevationTextField->setFloatFormat(GLMotif::TextField::FIXED);
	sunElevationTextField->setFieldWidth(2);
	sunElevationTextField->setPrecision(0);
	sunElevationTextField->setValue(double(sunElevation));

	sunElevationSlider=new GLMotif::Slider("SunElevationSlider",sunBox,GLMotif::Slider::HORIZONTAL,style->fontHeight*10.0f);
	sunElevationSlider->setValueRange(-90.0,90.0,1.0);
	sunElevationSlider->setValue(double(sunElevation));
	sunElevationSlider->getValueChangedCallbacks().add(this,&CrustaApp::sunElevationSliderCallback);

	sunBox->manageChild();
	lightSettings->manageChild();
}

void CrustaApp::
alignSurfaceFrame(Vrui::NavTransform& surfaceFrame)
{
/* Do whatever to the surface frame, but don't change its scale factor: */
    Geometry::Geoid<double> geoid(SPHEROID_RADIUS, 0.0);

    Vrui::Point origin = surfaceFrame.getOrigin();
    Vrui::Point lonLat;
    if (origin == Vrui::Point::origin)
        lonLat = Vrui::Point::origin;
    else
        lonLat = geoid.cartesianToGeodetic(origin);
    lonLat[2] = Crusta::getHeight(origin[0], origin[1], origin[2]);

    Geometry::Geoid<double>::Frame frame = geoid.geodeticToCartesianFrame(lonLat);
    surfaceFrame = Vrui::NavTransform(frame.getTranslation(), frame.getRotation(), surfaceFrame.getScaling());
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
debugGridCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    QuadTerrain::displayDebuggingGrid = cbData->set;
}

void CrustaApp::
changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    newVerticalScale = pow(10, cbData->value);

    std::ostringstream oss;
    oss.precision(2);
    oss << newVerticalScale << "x";
    verticalScaleLabel->setLabel(oss.str().c_str());
}

void CrustaApp::
showLightingDialogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
	if(cbData->set)
	{
		/* Open the render dialog at the same position as the main menu: */
		Vrui::getWidgetManager()->popupPrimaryWidget(lightingDialog,Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
	}
	else
	{
		/* Close the render dialog: */
		Vrui::popdownPrimaryWidget(lightingDialog);
	}
}

void CrustaApp::
updateSun()
{
	/* Enable or disable the light source: */
	if(enableSun)
		sun->enable();
	else
		sun->disable();

	/* Compute the light source's direction vector: */
	Vrui::Scalar z=Math::sin(Math::rad(sunElevation));
	Vrui::Scalar xy=Math::cos(Math::rad(sunElevation));
	Vrui::Scalar x=xy*Math::sin(Math::rad(sunAzimuth));
	Vrui::Scalar y=xy*Math::cos(Math::rad(sunAzimuth));
	sun->getLight().position=GLLight::Position(GLLight::Scalar(x),GLLight::Scalar(y),GLLight::Scalar(z),GLLight::Scalar(0));
}

void CrustaApp::
enableSunToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
	/* Set the sun enable flag: */
	enableSun=cbData->set;

	/* Enable/disable all viewers' headlights: */
	if(enableSun)
	{
		for(int i=0;i<Vrui::getNumViewers();++i)
			Vrui::getViewer(i)->setHeadlightState(false);
	}
	else
	{
		for(int i=0;i<Vrui::getNumViewers();++i)
			Vrui::getViewer(i)->setHeadlightState(viewerHeadlightStates[i]);
	}

	/* Update the sun light source: */
	updateSun();

	Vrui::requestUpdate();
}

void CrustaApp::sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
	/* Update the sun azimuth angle: */
	sunAzimuth=Vrui::Scalar(cbData->value);

	/* Update the sun azimuth value label: */
	sunAzimuthTextField->setValue(double(cbData->value));

	/* Update the sun light source: */
	updateSun();

	Vrui::requestUpdate();
}

void CrustaApp::sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
	/* Update the sun elevation angle: */
	sunElevation=Vrui::Scalar(cbData->value);

	/* Update the sun elevation value label: */
	sunElevationTextField->setValue(double(cbData->value));

	/* Update the sun light source: */
	updateSun();

	Vrui::requestUpdate();
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
    Crusta::frame();
}

void CrustaApp::
display(GLContextData& contextData) const
{
    Crusta::display(contextData);
}


void CrustaApp::
toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
{
    /* Check if the new tool is a surface navigation tool: */
    Vrui::SurfaceNavigationTool* surfaceNavigationTool =
        dynamic_cast<Vrui::SurfaceNavigationTool*>(cbData->tool);
    if (surfaceNavigationTool != NULL)
    {
        /* Set the new tool's alignment function: */
        surfaceNavigationTool->setAlignFunction(
            Misc::createFunctionCall<Vrui::NavTransform&,CrustaApp>(
                this,&CrustaApp::alignSurfaceFrame));
    }

    //are we creating a new mapping tool
    Vrui::LocatorTool* locator = dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
    if (tool != NULL)
    {
        //determine which tool to create
        const char* toolName = curTool->getSelectedToggle()->getName();
        if (!strcmp(toolName, "ControlPointEditor"))
        {
            Crusta::getMapManager()->createMapTool(
                MapManager::MAPTOOL_CONTROLPOINTEDITOR, locator);
        }
    }
}

void CrustaApp::
toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
{
    //are we destroying a mapping tool
    Vrui::LocatorTool* locator = dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
    if (tool != NULL)
        Crusta::getMapManager()->destroyMapTool(locator);
}


END_CRUSTA


int main(int argc, char* argv[])
{
	char** appDefaults = 0; // This is an additional parameter no one ever uses
    crusta::CrustaApp app(argc, argv, appDefaults);

	app.run();

    return 0;
}
