#include <crusta/CrustaApp.h>

#include <sstream>

#if __APPLE__
#include <GDAL/ogr_api.h>
#include <GDAL/ogrsf_frmts.h>
#else
#include <ogr_api.h>
#include <ogrsf_frmts.h>
#endif

#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/DropdownBox.h>
#include <GLMotif/Label.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Menu.h>
#include <GLMotif/PaletteEditor.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/TextField.h>
#include <GLMotif/WidgetManager.h>
#include <GL/GLColorMap.h>
#include <GL/GLContextData.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/Tools/LocatorTool.h>
#include <Vrui/Viewer.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadTerrain.h>


#include <Geometry/Geoid.h>
#include <Misc/FunctionCalls.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Tools/SurfaceNavigationTool.h>

#if CRUSTA_ENABLE_DEBUG
#include <crusta/DebugTool.h>
#endif //CRUSTA_ENABLE_DEBUG

BEGIN_CRUSTA

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults),
    newVerticalScale(1.0), enableSun(false),
    viewerHeadlightStates(new bool[Vrui::getNumViewers()]),
    sun(0), sunAzimuth(180.0), sunElevation(45.0),
    paletteEditor(new PaletteEditor)
{
    std::string demName;
    std::string colorName;
    std::string settingsFile;
    for (int i=0; i<argc; ++i)
    {
        if (strcmp(argv[i], "-dem")==0)
            demName   = std::string(argv[++i]);
        if (strcmp(argv[i], "-color")==0)
            colorName = std::string(argv[++i]);
        if (strcmp(argv[i], "-settings")==0)
            settingsFile = std::string(argv[++i]);
    }

    crusta = new Crusta;
    crusta->init(demName, colorName, settingsFile);

    /* Create the sun lightsource: */
    sun=Vrui::getLightsourceManager()->createLightsource(false);
    updateSun();

    /* Save all viewers' headlight enable states: */
    for(int i=0;i<Vrui::getNumViewers();++i)
    {
        viewerHeadlightStates[i] =
            Vrui::getViewer(i)->getHeadlight().isEnabled();
    }

    specularSettings.setupComponent(crusta);

    produceMainMenu();
    produceVerticalScaleDialog();
    produceLightingDialog();

    paletteEditor->getColorMapChangedCallbacks().add(
        this, &CrustaApp::changeColorMapCallback);

    GLMotif::ColorMap::ColorMapChangedCallbackData initMap(
        paletteEditor->getColorMap());
    changeColorMapCallback(&initMap);
    resetNavigationCallback(NULL);

#if CRUSTA_ENABLE_DEBUG
    DebugTool::init();
#endif //CRUSTA_ENABLE_DEBUG
}

CrustaApp::
~CrustaApp()
{
    delete popMenu;
    /* Delete the viewer headlight states: */
    delete[] viewerHeadlightStates;

    crusta->shutdown();
    delete crusta;
}


void CrustaApp::Dialog::
createMenuEntry(GLMotif::Container* menu)
{
    init();

    parentMenu = menu;

    GLMotif::ToggleButton* toggle = new GLMotif::ToggleButton(
        (name+"Toggle").c_str(), parentMenu, label.c_str());
    toggle->setToggle(false);
    toggle->getValueChangedCallbacks().add(
        this, &CrustaApp::Dialog::showCallback);
}

void CrustaApp::Dialog::
init()
{
    dialog = new GLMotif::PopupWindow(
        (name+"Dialog").c_str(), Vrui::getWidgetManager(), label.c_str());
}

void CrustaApp::Dialog::
showCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(dialog,
            Vrui::getWidgetManager()->calcWidgetTransformation(parentMenu));
    }
    else
    {
        //close the dialog:
        Vrui::popdownPrimaryWidget(dialog);
    }
}


CrustaApp::SpecularSettingsDialog::
SpecularSettingsDialog() :
    CrustaComponent(NULL), colorPicker("SpecularSettingsColorPicker",
                                       Vrui::getWidgetManager(),
                                       "Pick Specular Color")
{
    name  = "SpecularSettings";
    label = "Specular Reflectance Settings";
}

void CrustaApp::SpecularSettingsDialog::
init()
{
    Dialog::init();

    const GLMotif::StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    colorPicker.getColorPicker()->getColorChangedCallbacks().add(
            this, &CrustaApp::SpecularSettingsDialog::colorChangedCallback);


    GLMotif::RowColumn* root = new GLMotif::RowColumn(
        "Root", dialog, false);

    GLMotif::RowColumn* colorRoot = new GLMotif::RowColumn(
        "ColorRoot", root, false);

    Color sc = SETTINGS->terrainSpecularColor;
    new GLMotif::Label("SSColor", colorRoot, "Color: ");
    colorButton = new GLMotif::Button("SSColorButton", colorRoot, "");
    colorButton->setBackgroundColor(GLMotif::Color(sc[0], sc[1], sc[2], sc[3]));
    colorButton->getSelectCallbacks().add(
        this, &CrustaApp::SpecularSettingsDialog::colorButtonCallback);

    colorRoot->setNumMinorWidgets(2);
    colorRoot->manageChild();

    GLMotif::RowColumn* shininessRoot = new GLMotif::RowColumn(
        "ShininessRoot", root, false);

    float shininess = SETTINGS->terrainShininess;
    new GLMotif::Label("SSShininess", shininessRoot, "Shininess: ");
    GLMotif::Slider* slider = new GLMotif::Slider(
        "SSShininessSlider", shininessRoot, GLMotif::Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    slider->setValue(log(shininess)/log(2));
    slider->setValueRange(0.00f, 7.0f, 0.00001f);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::SpecularSettingsDialog::shininessChangedCallback);
    shininessField = new GLMotif::TextField(
        "SSShininessField", shininessRoot, 7);
    shininessField->setPrecision(4);
    shininessField->setValue(shininess);

    shininessRoot->setNumMinorWidgets(3);
    shininessRoot->manageChild();


    root->setNumMinorWidgets(2);
    root->manageChild();
}

void CrustaApp::SpecularSettingsDialog::
colorButtonCallback(
    GLMotif::Button::SelectCallbackData* cbData)
{
    GLMotif::WidgetManager* manager = Vrui::getWidgetManager();
    if (!manager->isVisible(&colorPicker))
    {
        //bring up the color picker
        colorPicker.getColorPicker()->setCurrentColor(
            SETTINGS->terrainSpecularColor);
        manager->popupPrimaryWidget(
            &colorPicker, manager->calcWidgetTransformation(cbData->button));
    }
    else
    {
        //close the color picker
        Vrui::popdownPrimaryWidget(&colorPicker);
    }
}

void CrustaApp::SpecularSettingsDialog::
colorChangedCallback(
        GLMotif::ColorPicker::ColorChangedCallbackData* cbData)
{
    const GLMotif::Color& c = cbData->newColor;
    colorButton->setBackgroundColor(c);
    crusta->setTerrainSpecularColor(Color(c[0], c[1], c[2], c[3]));
}

void CrustaApp::SpecularSettingsDialog::
shininessChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    float shininess = pow(2, cbData->value) - 1.0f;
    crusta->setTerrainShininess(shininess);
    shininessField->setValue(shininess);
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

    /* Create a submenu to toggle texturing the terrain: */
    produceTexturingSubmenu(mainMenu);

    /* Create a button to open or hide the vertical scale adjustment dialog: */
    GLMotif::ToggleButton* showVerticalScaleToggle = new GLMotif::ToggleButton(
        "ShowVerticalScaleToggle", mainMenu, "Vertical Scale");
    showVerticalScaleToggle->setToggle(false);
    showVerticalScaleToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showVerticalScaleCallback);

    /* Create a button to toogle display of the lighting dialog: */
    GLMotif::ToggleButton* lightingToggle = new GLMotif::ToggleButton(
        "LightingToggle", mainMenu, "Light Settings");
    lightingToggle->setToggle(false);
    lightingToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showLightingDialogCallback);

    /* Inject the map management menu entries */
    crusta->getMapManager()->addMenuEntry(mainMenu);

    /* Create a button to open or hide the vertical scale adjustment dialog: */
    GLMotif::ToggleButton* showPaletteEditorToggle = new GLMotif::ToggleButton(
        "ShowPaletteEditorToggle", mainMenu, "Palette Editor");
    showPaletteEditorToggle->setToggle(false);
    showPaletteEditorToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showPaletteEditorCallback);

    /* Create settings submenu */
    GLMotif::Popup* settingsMenuPopup =
        new GLMotif::Popup("SettingsMenuPopup", Vrui::getWidgetManager());
    GLMotif::SubMenu* settingsMenu =
        new GLMotif::SubMenu("Settings", settingsMenuPopup, false);

    //line decoration toggle
    GLMotif::ToggleButton* decorateLinesToggle = new GLMotif::ToggleButton(
        "DecorateLinesToggle", settingsMenu, "Decorate Lines");
    decorateLinesToggle->setToggle(SETTINGS->decorateVectorArt);
    decorateLinesToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::decorateLinesCallback);

    //specular settings dialog toggle
    specularSettings.createMenuEntry(settingsMenu);

    /* Create the advanced submenu */
    GLMotif::Popup* advancedMenuPopup =
        new GLMotif::Popup("AdvancedMenuPopup", Vrui::getWidgetManager());
    GLMotif::SubMenu* advancedMenu =
        new GLMotif::SubMenu("Advanced", advancedMenuPopup, false);

    //toogle display of the debugging grid
    GLMotif::ToggleButton* debugGridToggle = new GLMotif::ToggleButton(
        "DebugGridToggle", advancedMenu, "Debug Grid");
    debugGridToggle->setToggle(false);
    debugGridToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::debugGridCallback);

    //toogle display of the debugging sphere
    GLMotif::ToggleButton* debugSpheresToggle = new GLMotif::ToggleButton(
        "DebugSpheresToggle", advancedMenu, "Debug Spheres");
    debugSpheresToggle->setToggle(false);
    debugSpheresToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::debugSpheresCallback);

    advancedMenu->manageChild();
    GLMotif::CascadeButton* advancedMenuCascade = new GLMotif::CascadeButton(
        "AdvancedMenuCascade", settingsMenu, "Advanced");
    advancedMenuCascade->setPopup(advancedMenuPopup);

    settingsMenu->manageChild();
    GLMotif::CascadeButton* settingsMenuCascade = new GLMotif::CascadeButton(
        "SettingsMenuCascade", mainMenu, "Settings");
    settingsMenuCascade->setPopup(settingsMenuPopup);


    /* Navigation reset: */
    GLMotif::Button* resetNavigationButton = new GLMotif::Button(
        "ResetNavigationButton",mainMenu,"Reset Navigation");
    resetNavigationButton->getSelectCallbacks().add(
        this, &CrustaApp::resetNavigationCallback);


    /* Finish building the main menu: */
    mainMenu->manageChild();

    Vrui::setMainMenu(popMenu);
}

void CrustaApp::
produceTexturingSubmenu(GLMotif::Menu* mainMenu)
{
    GLMotif::Popup* texturingMenuPopup =
        new GLMotif::Popup("TexturingMenuPopup", Vrui::getWidgetManager());

    GLMotif::SubMenu* texturingMenu =
        new GLMotif::SubMenu("Texturing", texturingMenuPopup, false);

    GLMotif::RadioBox* texturingBox =
        new GLMotif::RadioBox("TexturingBox", texturingMenu, false);

    GLMotif::ToggleButton* modeButton;
    modeButton = new GLMotif::ToggleButton(
        "Untextured", texturingBox, "Untextured");
    modeButton->getSelectCallbacks().add(
        this, &CrustaApp::changeTexturingModeCallback);
    modeButton = new GLMotif::ToggleButton(
        "ColorMap", texturingBox, "Color Map");
    modeButton->getSelectCallbacks().add(
        this, &CrustaApp::changeTexturingModeCallback);
    modeButton = new GLMotif::ToggleButton(
        "Image", texturingBox, "Image");
    modeButton->getSelectCallbacks().add(
        this, &CrustaApp::changeTexturingModeCallback);

    texturingBox->setSelectedToggle(modeButton);
    texturingBox->manageChild();

    texturingMenu->manageChild();

    GLMotif::CascadeButton* texturingMenuCascade =
        new GLMotif::CascadeButton("TexturingMenuCascade", mainMenu,
                                   "Texturing Modes");
    texturingMenuCascade->setPopup(texturingMenuPopup);
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
    lightingDialog=new GLMotif::PopupWindow("LightingDialog",
        Vrui::getWidgetManager(), "Light Settings");
    GLMotif::RowColumn* lightSettings=new GLMotif::RowColumn("LightSettings",
        lightingDialog, false);
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
    Point3 origin = surfaceFrame.getOrigin();
    if (origin == Point3::origin)
        origin = Point3(0.0, 1.0, 0.0);

    origin = crusta->snapToSurface(origin);
    origin = crusta->mapToScaledGlobe(origin);

    //misuse the Geoid just to build a surface tangent frame
    Geometry::Geoid<double> geoid(SETTINGS->globeRadius, 0.0);
    Point3 lonLatEle = geoid.cartesianToGeodetic(origin);
    Geometry::Geoid<double>::Frame frame =
        geoid.geodeticToCartesianFrame(lonLatEle);
    surfaceFrame = Vrui::NavTransform(
        frame.getTranslation(), frame.getRotation(), surfaceFrame.getScaling());
}


void CrustaApp::
changeTexturingModeCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if (cbData->set)
    {
        const char* button = cbData->toggle->getName();
        if (strcmp(button, "Untextured") == 0)
            crusta->setTexturingMode(0);
        else if (strcmp(button, "ColorMap") == 0)
            crusta->setTexturingMode(1);
        else
            crusta->setTexturingMode(2);
    }
}

void CrustaApp::
changeColorMapCallback(GLMotif::ColorMap::ColorMapChangedCallbackData* cbData)
{
    GLColorMap* colorMap = crusta->getColorMap();
    cbData->colorMap->exportColorMap(*colorMap);
    crusta->touchColorMap();
}


void CrustaApp::
showVerticalScaleCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if (cbData->set)
    {
        //open the dialog at the same position as the main menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(verticalScaleDialog,
            Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
    }
    else
    {
        //close the dialog
        Vrui::popdownPrimaryWidget(verticalScaleDialog);
    }
}

void CrustaApp::
changeScaleCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    double newVerticalScale = pow(10, cbData->value);
    crusta->setVerticalScale(newVerticalScale);

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
        //open the dialog at the same position as the main menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(lightingDialog,
            Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
    }
    else
    {
        //close the dialog:
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

void CrustaApp::
sunAzimuthSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    /* Update the sun azimuth angle: */
    sunAzimuth=Vrui::Scalar(cbData->value);

    /* Update the sun azimuth value label: */
    sunAzimuthTextField->setValue(double(cbData->value));

    /* Update the sun light source: */
    updateSun();

    Vrui::requestUpdate();
}

void CrustaApp::
sunElevationSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
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
showPaletteEditorCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the main menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(paletteEditor,
            Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
    }
    else
    {
        //close the dialog:
        Vrui::popdownPrimaryWidget(paletteEditor);
    }
}


void CrustaApp::
decorateLinesCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    crusta->setDecoratedVectorArt(cbData->set);
}

void CrustaApp::
debugGridCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    QuadTerrain::displayDebuggingGrid = cbData->set;
}

void CrustaApp::
debugSpheresCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    QuadTerrain::displayDebuggingBoundingSpheres = cbData->set;
}

void CrustaApp::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::setNavigationTransformation(Vrui::Point(0),1.5*SETTINGS->globeRadius);
    Vrui::concatenateNavigationTransformation(Vrui::NavTransform::translate(
        Vrui::Vector(0,1,0)));
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

    //all crusta components get the crusta instance assigned
    CrustaComponent* component = dynamic_cast<CrustaComponent*>(cbData->tool);
    if (component != NULL)
        component->setupComponent(crusta);

#if CRUSTA_ENABLE_DEBUG
    //check for the creation of the debug tool
    DebugTool* dbgTool = dynamic_cast<DebugTool*>(cbData->tool);
    if (dbgTool!=NULL && crusta->debugTool==NULL)
        crusta->debugTool = dbgTool;
#endif //CRUSTA_ENABLE_DEBUG


    Vrui::Application::toolCreationCallback(cbData);
}

void CrustaApp::
toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
{
#if CRUSTA_ENABLE_DEBUG
    if (cbData->tool == crusta->debugTool)
        crusta->debugTool = NULL;
#endif //CRUSTA_ENABLE_DEBUG
}



END_CRUSTA


int main(int argc, char* argv[])
{
    char** appDefaults = 0; // This is an additional parameter no one ever uses
    crusta::CrustaApp app(argc, argv, appDefaults);

    app.run();

    return 0;
}
