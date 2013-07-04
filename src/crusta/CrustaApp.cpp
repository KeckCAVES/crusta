#include <crustavrui/GL/VruiGlew.h> //must be included before gl.h

#include <crusta/CrustaApp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>
#include <algorithm>

#include <ogr_api.h>
#include <ogrsf_frmts.h>

#include <crustavrui/GLMotif/PaletteEditor.h>
#include <crusta/ResourceLocator.h>
#include <crusta/ColorMapper.h>
#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadTerrain.h>
#include <crusta/SurfaceProbeTool.h>
#include <crusta/SurfaceTool.h>
#include <crusta/VruiCoordinateTransform.h>

#include <crusta/vrui.h>


#if CRUSTA_ENABLE_DEBUG
#include <crusta/DebugTool.h>
#endif //CRUSTA_ENABLE_DEBUG


static const Geometry::Point<int,2> DATALISTBOX_SIZE(30, 8);

using namespace GLMotif;


namespace crusta {

std::string get_extension(const std::string& name)
{
  size_t dot_pos = name.rfind(".");
  size_t slash_pos = name.rfind("/");
  if (dot_pos != std::string::npos && (slash_pos != std::string::npos || dot_pos > slash_pos)) {
    std::string ext = name.substr(dot_pos+1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
  } else {
    return "";
  }
}

void get_arg(
  const std::string& arg,
  std::vector<std::string>& dataNames, 
  std::vector<std::string>& settingsNames,
  std::vector<std::string>& sceneGraphNames)
{
  int file = open(arg.c_str(), O_RDONLY | O_NONBLOCK);
  if (file == -1) {
    Misc::throwStdErr((std::string("Cannot read file ") + arg).c_str()); }
  struct stat stat_buf;
  int stat_status = fstat(file, &stat_buf);
  if (stat_status == -1) {
    Misc::throwStdErr((std::string("Cannot stat file ") + arg).c_str());
  }
  std::string ext = get_extension(arg);
  if (ext == "cfg") {
    settingsNames.push_back(arg);
  } else if (ext == "wrl") {
    sceneGraphNames.push_back(arg);
  }
  else {
    if (S_ISDIR(stat_buf.st_mode)) {
      dataNames.push_back(arg);
    } else {
      std::string basedir = "./";
      size_t pos = arg.find_last_of("/");
      if (pos != string::npos) basedir = arg.substr(0, pos+1);
      std::string line;
      fcntl(file, F_SETFL, 0); // clear O_NONBLOCK
      while (true) {
        char buf;
        ssize_t s = read(file, &buf, 1);
        if (s <= 0 || buf == '\n' || buf == '\r') {
          if (!line.empty()) {
            if (line[0] != '/') line = basedir + line;
            get_arg(line, dataNames, settingsNames, sceneGraphNames);
          }
          if (s > 0) line.clear(); else break;
        } else {
          line += buf;
        }
      }
    }
  }
  close(file);
}

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults),
    dataDialog(NULL),
    paletteEditor(new PaletteEditor), layerSettings(paletteEditor)
{
    paletteEditor->getColorMapEditor()->getColorMapChangedCallbacks().add(
        this, &CrustaApp::changeColorMapCallback);
    paletteEditor->getRangeEditor()->getRangeChangedCallbacks().add(
        this, &CrustaApp::changeColorMapRangeCallback);

    std::vector<std::string> dataNames;
    std::vector<std::string> settingsNames;
    std::vector<std::string> sceneGraphNames;
    for (int i=1; i<argc; ++i) get_arg(argv[i], dataNames, settingsNames, sceneGraphNames);

    crusta = new Crusta;
    crusta->init(argv[0], settingsNames, "");
    //load data passed through command line?
    crusta->load(dataNames);
    for (std::vector<std::string>::const_iterator itr=sceneGraphNames.begin(); itr!=sceneGraphNames.end(); ++itr)
      crusta->loadSceneGraph(*itr);

    produceMainMenu();

    /* Set the navigational coordinate system unit: */
    Vrui::getCoordinateManager()->setUnit(
        Geometry::LinearUnit(Geometry::LinearUnit::METER, 1));

    /* Register a geodetic coordinate transformer with Vrui's coordinate manager: */
    VruiCoordinateTransform* userTransform;
    userTransform=new VruiCoordinateTransform;
    userTransform->setupComponent(crusta);
    Vrui::getCoordinateManager()->setCoordinateTransform(userTransform); // coordinate manager owns userTransform

    resetNavigationCallback(NULL);

#if CRUSTA_ENABLE_DEBUG
    DebugTool::init();
#endif //CRUSTA_ENABLE_DEBUG
}

CrustaApp::
~CrustaApp()
{
    delete popMenu;

    crusta->shutdown();
    delete crusta;
}


void CrustaApp::Dialog::
createMenuEntry(Container* menu)
{
    init();

    parentMenu = menu;

    toggle = new ToggleButton(
        (name+"Toggle").c_str(), parentMenu, label.c_str());
    toggle->setToggle(false);
    toggle->getValueChangedCallbacks().add(
        this, &CrustaApp::Dialog::showCallback);
}

void CrustaApp::Dialog::
init()
{
    dialog = new PopupWindow(
        (name+"Dialog").c_str(), Vrui::getWidgetManager(), label.c_str());
    dialog->setCloseButton(true);
    dialog->getCloseCallbacks().add(this, &CrustaApp::Dialog::closeCallback);
}

void CrustaApp::Dialog::
showCallback(ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the menu:
        Vrui::popupPrimaryWidget(dialog);
    }
    else
    {
        //close the dialog:
        Vrui::popdownPrimaryWidget(dialog);
    }
}

void CrustaApp::Dialog::
closeCallback(Misc::CallbackData* cbData)
{
    toggle->setToggle(false);
}


CrustaApp::VerticalScaleDialog::
VerticalScaleDialog() :
    crusta(NULL), scaleLabel(NULL)
{
    name  = "VerticalScaleDialog";
    label = "Vertical Scale";
}

void CrustaApp::VerticalScaleDialog::
setCrusta(Crusta* newCrusta)
{
    crusta = newCrusta;
}

void CrustaApp::VerticalScaleDialog::
init()
{
    Dialog::init();

    const StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    RowColumn* root = new RowColumn("ScaleRoot", dialog, false);

    Slider* slider = new Slider(
        "ScaleSlider", root, Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    slider->setValue(0.0);
    slider->setValueRange(-0.5, 2.5, 0.00001);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::VerticalScaleDialog::changeScaleCallback);

    scaleLabel = new Label("ScaleLabel", root, "1.0x");

    root->setNumMinorWidgets(2);
    root->manageChild();
}

void CrustaApp::VerticalScaleDialog::
changeScaleCallback(Slider::ValueChangedCallbackData* cbData)
{
    double newVerticalScale = pow(10, cbData->value);
    crusta->setVerticalScale(newVerticalScale);

    std::ostringstream oss;
    oss.precision(2);
    oss << newVerticalScale << "x";
    scaleLabel->setString(oss.str().c_str());
}

CrustaApp::OpacityDialog::
OpacityDialog() :
    crusta(NULL), opacityLabel(NULL)
{
    name  = "OpacityDialog";
    label = "Opacity";
}

void CrustaApp::OpacityDialog::
setCrusta(Crusta* newCrusta)
{
    crusta = newCrusta;
}

void CrustaApp::OpacityDialog::
init()
{
    Dialog::init();

    const StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    RowColumn* root = new RowColumn("OpacityRoot", dialog, false);

    Slider* slider = new Slider(
        "OpacitySlider", root, Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    slider->setValue(1.0);
    slider->setValueRange(0.0, 1.0, 0.01);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::OpacityDialog::changeOpacityCallback);

    opacityLabel = new Label("OpacityLabel", root, "1.0");

    root->setNumMinorWidgets(2);
    root->manageChild();
}

void CrustaApp::OpacityDialog::
changeOpacityCallback(Slider::ValueChangedCallbackData* cbData)
{
    double newOpacity = cbData->value;
    crusta->setOpacity(newOpacity);

    std::ostringstream oss;
    oss.precision(2);
    oss << newOpacity;
    opacityLabel->setString(oss.str().c_str());
}

CrustaApp::LightSettingsDialog::
LightSettingsDialog() :
    viewerHeadlightStates(Vrui::getNumViewers()),
    enableSun(false),
    sun(Vrui::getLightsourceManager()->createLightsource(false)),
    sunAzimuth(180.0), sunElevation(45.0)
{
    name  = "LightSettingsDialog";
    label = "Light Settings";

    //save all viewers' headlight enable states
    for (int i=0; i<Vrui::getNumViewers(); ++i)
    {
        viewerHeadlightStates[i] =
            Vrui::getViewer(i)->getHeadlight().isEnabled();
    }

    //update the sun parameters
    updateSun();
}

CrustaApp::LightSettingsDialog::
~LightSettingsDialog()
{
    Vrui::getLightsourceManager()->destroyLightsource(sun);
}

void CrustaApp::LightSettingsDialog::
init()
{
    Dialog::init();

    const StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();
    RowColumn* lightSettings = new RowColumn(
        "LightSettings", dialog, false);
    lightSettings->setNumMinorWidgets(2);

    /* Create a toggle button and two sliders to manipulate the sun light
       source: */
    Margin* enableSunToggleMargin = new Margin(
        "SunToggleMargin", lightSettings, false);
    enableSunToggleMargin->setAlignment(Alignment(
        Alignment::HFILL,Alignment::VCENTER));
    ToggleButton* enableSunToggle = new ToggleButton(
        "SunToggle", enableSunToggleMargin, "Sun Light Source");
    enableSunToggle->setToggle(enableSun);
    enableSunToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::LightSettingsDialog::enableSunToggleCallback);
    enableSunToggleMargin->manageChild();

    RowColumn* sunBox = new RowColumn("SunBox", lightSettings, false);
    sunBox->setOrientation(RowColumn::VERTICAL);
    sunBox->setNumMinorWidgets(2);
    sunBox->setPacking(RowColumn::PACK_TIGHT);

    sunAzimuthTextField = new TextField("SunAzimuthTextField", sunBox, 5);
    sunAzimuthTextField->setFloatFormat(TextField::FIXED);
    sunAzimuthTextField->setFieldWidth(3);
    sunAzimuthTextField->setPrecision(0);
    sunAzimuthTextField->setValue(double(sunAzimuth));

    sunAzimuthSlider = new Slider("SunAzimuthSlider", sunBox,
        Slider::HORIZONTAL,style->fontHeight*10.0f);
    sunAzimuthSlider->setValueRange(0.0,360.0,1.0);
    sunAzimuthSlider->setValue(double(sunAzimuth));
    sunAzimuthSlider->getValueChangedCallbacks().add(
        this, &CrustaApp::LightSettingsDialog::sunAzimuthSliderCallback);

    sunElevationTextField = new TextField("SunElevationTextField", sunBox, 5);
    sunElevationTextField->setFloatFormat(TextField::FIXED);
    sunElevationTextField->setFieldWidth(2);
    sunElevationTextField->setPrecision(0);
    sunElevationTextField->setValue(double(sunElevation));

    sunElevationSlider = new Slider("SunElevationSlider", sunBox,
        Slider::HORIZONTAL,style->fontHeight*10.0f);
    sunElevationSlider->setValueRange(-90.0,90.0,1.0);
    sunElevationSlider->setValue(double(sunElevation));
    sunElevationSlider->getValueChangedCallbacks().add(
        this, &CrustaApp::LightSettingsDialog::sunElevationSliderCallback);

    sunBox->manageChild();
    lightSettings->manageChild();
}

void CrustaApp::LightSettingsDialog::
updateSun()
{
    /* Enable or disable the light source: */
    if(enableSun)
        sun->enable();
    else
        sun->disable();

    /* Compute the light source's direction vector: */
    Vrui::Scalar z  = Math::sin(Math::rad(sunElevation));
    Vrui::Scalar xy = Math::cos(Math::rad(sunElevation));
    Vrui::Scalar x  = xy * Math::sin(Math::rad(sunAzimuth));
    Vrui::Scalar y  = xy * Math::cos(Math::rad(sunAzimuth));
    sun->getLight().position = GLLight::Position(
        GLLight::Scalar(x), GLLight::Scalar(y), GLLight::Scalar(z),
        GLLight::Scalar(0));
}

void CrustaApp::LightSettingsDialog::
enableSunToggleCallback(ToggleButton::ValueChangedCallbackData* cbData)
{
    /* Set the sun enable flag: */
    enableSun = cbData->set;

    /* Enable/disable all viewers' headlights: */
    if (enableSun)
    {
        for (int i=0; i<Vrui::getNumViewers(); ++i)
            Vrui::getViewer(i)->setHeadlightState(false);
    }
    else
    {
        for (int i=0; i<Vrui::getNumViewers(); ++i)
            Vrui::getViewer(i)->setHeadlightState(viewerHeadlightStates[i]);
    }

    /* Update the sun light source: */
    updateSun();

    Vrui::requestUpdate();
}

void CrustaApp::LightSettingsDialog::
sunAzimuthSliderCallback(Slider::ValueChangedCallbackData* cbData)
{
    //update the sun azimuth angle
    sunAzimuth = Vrui::Scalar(cbData->value);

    //update the sun azimuth value label
    sunAzimuthTextField->setValue(double(cbData->value));

    //update the sun light source
    updateSun();

    Vrui::requestUpdate();
}

void CrustaApp::LightSettingsDialog::
sunElevationSliderCallback(Slider::ValueChangedCallbackData* cbData)
{
    //update the sun elevation angle
    sunElevation=Vrui::Scalar(cbData->value);

    //update the sun elevation value label
    sunElevationTextField->setValue(double(cbData->value));

    //update the sun light source
    updateSun();

    Vrui::requestUpdate();
}


CrustaApp::TerrainColorSettingsDialog::
TerrainColorSettingsDialog() :
    currentButton(NULL),
    colorPicker("TerrainColorSettingsColorPicker",
                Vrui::getWidgetManager(),
                "Pick Color")
{
    name  = "TerrainColorSettings";
    label = "Terrain Color Settings";
}

void CrustaApp::TerrainColorSettingsDialog::
init()
{
    Dialog::init();

    const StyleSheet* style = Vrui::getWidgetManager()->getStyleSheet();

    colorPicker.setCloseButton(true);
    colorPicker.getColorPicker()->getColorChangedCallbacks().add(
            this, &CrustaApp::TerrainColorSettingsDialog::colorChangedCallback);

    RowColumn* root = new RowColumn("TCSRoot", dialog, false);
    root->setNumMinorWidgets(2);

    Button* button = NULL;

//- Emissive Color
    new Label("TCSEmissive", root, "Emissive:");
    button = new Button("TSCEmissiveButton", root, "");
    button->setBackgroundColor(SETTINGS->terrainEmissiveColor);
    button->getSelectCallbacks().add(
        this, &CrustaApp::TerrainColorSettingsDialog::colorButtonCallback);

//- Ambient Color
    new Label("TCSAmbient", root, "Ambient:");
    button = new Button("TSCAmbientButton", root, "");
    button->setBackgroundColor(SETTINGS->terrainAmbientColor);
    button->getSelectCallbacks().add(
        this, &CrustaApp::TerrainColorSettingsDialog::colorButtonCallback);

//- Diffuse Color
    new Label("TCSDiffuse", root, "Diffuse:");
    button = new Button("TSCDiffuseButton", root, "");
    button->setBackgroundColor(SETTINGS->terrainDiffuseColor);
    button->getSelectCallbacks().add(
        this, &CrustaApp::TerrainColorSettingsDialog::colorButtonCallback);

//- Specular Color
    new Label("TCSSpecular", root, "Specular:");
    button = new Button("TSCSpecularButton", root, "");
    button->setBackgroundColor(SETTINGS->terrainSpecularColor);
    button->getSelectCallbacks().add(
        this, &CrustaApp::TerrainColorSettingsDialog::colorButtonCallback);

//- Specular Shininess
    new Label("TCSShininess", root, "Shininess:");
    RowColumn* shininessRoot = new RowColumn(
        "TCSShininessRoot", root, false);
    shininessRoot->setNumMinorWidgets(2);

    Slider* slider = new Slider(
        "TCSShininessSlider", shininessRoot, Slider::HORIZONTAL,
        5.0 * style->fontHeight);
    slider->setValue(SETTINGS->terrainShininess);
    slider->setValueRange(0.0f, 128.0f, 1.0f);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::TerrainColorSettingsDialog::shininessChangedCallback);
    shininessField = new TextField(
        "SSShininessField", shininessRoot, 3);
    shininessField->setPrecision(3);
    shininessField->setValue(SETTINGS->terrainShininess);

    shininessRoot->manageChild();

    root->manageChild();
}

void CrustaApp::TerrainColorSettingsDialog::
colorButtonCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    WidgetManager* manager = Vrui::getWidgetManager();

    //hide the picker if user clicks on the same button when the picker is up
    if (manager->isVisible(&colorPicker) && cbData->button==currentButton)
    {
        Vrui::popdownPrimaryWidget(&colorPicker);
        return;
    }

    //update the current button
    currentButton = cbData->button;

    //update the color of the picker
    if (strcmp(currentButton->getName(), "TSCEmissiveButton") == 0)
    {
        colorPicker.setTitleString("Pick Emissive Color");
        colorPicker.getColorPicker()->setCurrentColor(
            SETTINGS->terrainEmissiveColor);
    }
    else if (strcmp(currentButton->getName(), "TSCAmbientButton") == 0)
    {
        colorPicker.setTitleString("Pick Ambient Color");
        colorPicker.getColorPicker()->setCurrentColor(
            SETTINGS->terrainAmbientColor);
    }
    else if (strcmp(currentButton->getName(), "TSCDiffuseButton") == 0)
    {
        colorPicker.setTitleString("Pick Diffuse Color");
        colorPicker.getColorPicker()->setCurrentColor(
            SETTINGS->terrainDiffuseColor);
    }
    else if (strcmp(currentButton->getName(), "TSCSpecularButton") == 0)
    {
        colorPicker.setTitleString("Pick Specular Color");
        colorPicker.getColorPicker()->setCurrentColor(
            SETTINGS->terrainSpecularColor);
    }

    //pop up the picker if necessary
    if (!manager->isVisible(&colorPicker))
    {
        manager->popupPrimaryWidget(
            &colorPicker, manager->calcWidgetTransformation(cbData->button));
    }
}

void CrustaApp::TerrainColorSettingsDialog::
colorChangedCallback(GLMotif::ColorPicker::ColorChangedCallbackData* cbData)
{
    assert(currentButton != NULL);

    //update the color of the corresponding button
    currentButton->setBackgroundColor(cbData->newColor);
    //update appropriate color setting
    if (strcmp(currentButton->getName(), "TSCEmissiveButton") == 0)
        SETTINGS->terrainEmissiveColor = cbData->newColor;
    else if (strcmp(currentButton->getName(), "TSCAmbientButton") == 0)
        SETTINGS->terrainAmbientColor = cbData->newColor;
    else if (strcmp(currentButton->getName(), "TSCDiffuseButton") == 0)
        SETTINGS->terrainDiffuseColor = cbData->newColor;
    else if (strcmp(currentButton->getName(), "TSCSpecularButton") == 0)
        SETTINGS->terrainSpecularColor = cbData->newColor;
}

void CrustaApp::TerrainColorSettingsDialog::
shininessChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    //update the shininess setting and corresponding text field
    SETTINGS->terrainShininess = cbData->value;
    shininessField->setValue(cbData->value);
}


CrustaApp::LayerSettingsDialog::
LayerSettingsDialog(PaletteEditor* editor) :
    listBox(NULL), paletteEditor(editor),
    buttonRoot(NULL)
{
    name  = "LayerSettings";
    label = "Layer Settings";
}

void CrustaApp::LayerSettingsDialog::
init()
{
    Dialog::init();

    RowColumn* root = new RowColumn("LayerRoot", dialog, false);

    ScrolledListBox* box = new ScrolledListBox(
        "LayerListBox", root, ListBox::ALWAYS_ONE,
        DATALISTBOX_SIZE[0], DATALISTBOX_SIZE[1]);
    listBox = box->getListBox();
    listBox->setAutoResize(true);
    listBox->getValueChangedCallbacks().add(
        this, &LayerSettingsDialog::layerChangedCallback);

    buttonRoot = new RowColumn("LayerButtonRoot", root);
    buttonRoot->setNumMinorWidgets(2);
    buttonRoot->setPacking(RowColumn::PACK_GRID);

    updateLayerList();

    root->manageChild();
}


void CrustaApp::LayerSettingsDialog::
updateLayerList()
{
    listBox->clear();

    ColorMapper::Strings layerNames = COLORMAPPER->getLayerNames();
    for (ColorMapper::Strings::iterator it=layerNames.begin();
         it!=layerNames.end(); ++it)
    {
        listBox->addItem(it->c_str());
    }

    if (COLORMAPPER->getHeightColorMapIndex() != -1)
        listBox->selectItem(COLORMAPPER->getHeightColorMapIndex());
    else if (!layerNames.empty())
        listBox->selectItem(0);
    //make sure the buttonRoot doesn't remain empty
    else
         new Margin("LayerEmptyMargin", buttonRoot);
}

void CrustaApp::LayerSettingsDialog::
layerChangedCallback(ListBox::ValueChangedCallbackData* cbData)
{
    //remove all the buttons
    buttonRoot->removeWidgets(0);

    COLORMAPPER->setActiveLayer(cbData->newSelectedItem);
    if (cbData->newSelectedItem >= 0)
    {
        ToggleButton* visibleButton = new ToggleButton(
            "LayerVisibleButton", buttonRoot, "Visible");
        bool visible = COLORMAPPER->isVisible(cbData->newSelectedItem);
        visibleButton->setToggle(visible);
        visibleButton->getValueChangedCallbacks().add(
            this, &LayerSettingsDialog::visibleCallback);
    }

    if (COLORMAPPER->isFloatLayer(cbData->newSelectedItem))
    {
        ToggleButton* clampButton = new ToggleButton(
            "LayerClampButton", buttonRoot, "Clamp");
        bool clamped = COLORMAPPER->isClamped(cbData->newSelectedItem);
        clampButton->setToggle(clamped);
        clampButton->getValueChangedCallbacks().add(
            this, &LayerSettingsDialog::clampCallback);

        Misc::ColorMap& colorMap =
            COLORMAPPER->getColorMap(cbData->newSelectedItem);

        paletteEditor->getColorMapEditor()->getColorMap() = colorMap;
        paletteEditor->getColorMapEditor()->touch();
    }
}

void CrustaApp::LayerSettingsDialog::
visibleCallback(ToggleButton::ValueChangedCallbackData* cbData)
{
    int activeLayer = COLORMAPPER->getActiveLayer();
    COLORMAPPER->setVisible(activeLayer, cbData->set);
}

void CrustaApp::LayerSettingsDialog::
clampCallback(ToggleButton::ValueChangedCallbackData* cbData)
{
    int activeLayer = COLORMAPPER->getActiveLayer();
    COLORMAPPER->setClamping(activeLayer, cbData->set);
}



void CrustaApp::
produceMainMenu()
{
    /* Create a popup shell to hold the main menu: */
    popMenu = new PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
    popMenu->setTitle("Crusta");

    /* Create the main menu itself: */
    Menu* mainMenu =
    new Menu("MainMenu",popMenu,false);

    /* Data Loading menu entry */
    produceDataDialog();
    Button* dataLoadButton = new Button(
        "DataLoadButton", mainMenu, "Load Data");
    dataLoadButton->getSelectCallbacks().add(
        this, &CrustaApp::showDataDialogCallback);

    //vertical scale dialog
    verticalScaleSettings.setCrusta(crusta);
    verticalScaleSettings.createMenuEntry(mainMenu);

    //opacity dialog
    opacitySettings.setCrusta(crusta);
    opacitySettings.createMenuEntry(mainMenu);

    //light settings dialog
    lightSettings.createMenuEntry(mainMenu);

    /* Inject the map management menu entries */
    crusta->getMapManager()->addMenuEntry(mainMenu);

    //color map settings dialog toggle
    layerSettings.createMenuEntry(mainMenu);

    /* Create a button to open or hide the palette editor dialog: */
    ToggleButton* showPaletteEditorToggle = new ToggleButton(
        "ShowPaletteEditorToggle", mainMenu, "Palette Editor");
    showPaletteEditorToggle->setToggle(false);
    showPaletteEditorToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showPaletteEditorCallback);

    /* Create settings submenu */
    Popup* settingsMenuPopup =
        new Popup("SettingsMenuPopup", Vrui::getWidgetManager());
    SubMenu* settingsMenu =
        new SubMenu("Settings", settingsMenuPopup, false);

    //line decoration toggle
    ToggleButton* decorateLinesToggle = new ToggleButton(
        "DecorateLinesToggle", settingsMenu, "Decorate Lines");
    decorateLinesToggle->setToggle(SETTINGS->lineDecorated);
    decorateLinesToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::decorateLinesCallback);

    //terrain color settings dialog toggle
    terrainColorSettings.createMenuEntry(settingsMenu);

    /* Create the advanced submenu */
    Popup* advancedMenuPopup =
        new Popup("AdvancedMenuPopup", Vrui::getWidgetManager());
    SubMenu* advancedMenu =
        new SubMenu("Advanced", advancedMenuPopup, false);

    //toogle display of the debugging grid
    ToggleButton* debugGridToggle = new ToggleButton(
        "DebugGridToggle", advancedMenu, "Debug Grid");
    debugGridToggle->setToggle(false);
    debugGridToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::debugGridCallback);

    //toogle display of the debugging sphere
    ToggleButton* debugSpheresToggle = new ToggleButton(
        "DebugSpheresToggle", advancedMenu, "Debug Spheres");
    debugSpheresToggle->setToggle(false);
    debugSpheresToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::debugSpheresCallback);

    advancedMenu->manageChild();
    CascadeButton* advancedMenuCascade = new CascadeButton(
        "AdvancedMenuCascade", settingsMenu, "Advanced");
    advancedMenuCascade->setPopup(advancedMenuPopup);

    settingsMenu->manageChild();
    CascadeButton* settingsMenuCascade = new CascadeButton(
        "SettingsMenuCascade", mainMenu, "Settings");
    settingsMenuCascade->setPopup(settingsMenuPopup);


    /* Navigation reset: */
    Button* resetNavigationButton = new Button(
        "ResetNavigationButton",mainMenu,"Reset Navigation");
    resetNavigationButton->getSelectCallbacks().add(
        this, &CrustaApp::resetNavigationCallback);


    /* Finish building the main menu: */
    mainMenu->manageChild();

    Vrui::setMainMenu(popMenu);
}

void CrustaApp::
produceDataDialog()
{
    dataDialog = new PopupWindow(
        "DataDialog", Vrui::getWidgetManager(), "Load Crusta Data");
    RowColumn* root = new RowColumn("DataRoot", dataDialog, false);

    RowColumn* top = new RowColumn("DataTop", root, false);
    top->setOrientation(RowColumn::HORIZONTAL);
    top->setColumnWeight(0, 1.0f);

    ScrolledListBox* box = new ScrolledListBox("DataListBox", top,
        ListBox::MULTIPLE, DATALISTBOX_SIZE[0], DATALISTBOX_SIZE[1]);
    dataListBox = box->getListBox();
    dataListBox->setAutoResize(true);

    Margin* addRemoveMargin = new Margin("DataAddRemoveMargin", top, false);
    addRemoveMargin->setAlignment(Alignment::VCENTER);
    RowColumn* addRemoveRoot = new RowColumn("DataAddRemoveRoot",
                                             addRemoveMargin, false);

    Button* button = new Button("DataAddButton", addRemoveRoot, "Add");
    button->getSelectCallbacks().add(this, &CrustaApp::addDataCallback);

    button = new Button("DataRemoveButton", addRemoveRoot, "Remove");
    button->getSelectCallbacks().add(this, &CrustaApp::removeDataCallback);

    button = new Button("DataClearButton", addRemoveRoot, "Clear");
    button->getSelectCallbacks().add(this, &CrustaApp::clearDataCallback);

    addRemoveRoot->manageChild();
    addRemoveMargin->manageChild();
    top->manageChild();

    Margin* actionMargin = new Margin("DataActionMargin", root, false);
    actionMargin->setAlignment(Alignment::RIGHT);
    RowColumn* actionRoot = new RowColumn("DataActionRoot",actionMargin,false);
    actionRoot->setOrientation(RowColumn::HORIZONTAL);

    button = new Button("DataCancelButton", actionRoot, "Cancel");
    button->getSelectCallbacks().add(this, &CrustaApp::loadDataCancelCallback);
    button = new Button("DataOkButton", actionRoot, "Load");
    button->getSelectCallbacks().add(this, &CrustaApp::loadDataOkCallback);

    actionRoot->manageChild();
    actionMargin->manageChild();
    root->manageChild();
}

void CrustaApp::
showDataDialogCallback(Button::SelectCallbackData*)
{
    //grab the current data paths from the data manager
    dataPaths.clear();
    if (DATAMANAGER->hasDem())
        dataPaths.push_back(DATAMANAGER->getDemFilePath());
    const DataManager::Strings& colorPaths = DATAMANAGER->getColorFilePaths();
    for (size_t i=0; i<colorPaths.size(); ++i)
        dataPaths.push_back(colorPaths[i]);
    const DataManager::Strings& layerfPaths = DATAMANAGER->getLayerfFilePaths();
    for (size_t i=0; i<layerfPaths.size(); ++i)
        dataPaths.push_back(layerfPaths[i]);

    //populate the list box with the current paths
    dataListBox->clear();
    for (size_t i=0; i<dataPaths.size(); ++i)
        dataListBox->addItem(dataPaths[i].c_str());

    //open the dialog at the same position as the main menu:
    Vrui::popupPrimaryWidget(dataDialog);
}

void CrustaApp::
addDataCallback(Button::SelectCallbackData*)
{
    FileSelectionDialog* fileDialog =
        new FileSelectionDialog(Vrui::getWidgetManager(),
            "Load Crusta Globe File", CURRENTDIRECTORY, NULL);
    fileDialog->setCanSelectDirectory(true);
    fileDialog->getOKCallbacks().add(this,
        &CrustaApp::addDataFileOkCallback);
    fileDialog->getCancelCallbacks().add(this,
        &CrustaApp::addDataFileCancelCallback);
    Vrui::popupPrimaryWidget(fileDialog);
}

void CrustaApp::
addDataFileOkCallback(FileSelectionDialog::OKCallbackData* cbData)
{
    /* Check if the user selected a file or a directory: */
    std::string newDataPath;
    if(cbData->selectedFileName==0)
      {
      /* User selected a directory: */
      newDataPath=cbData->selectedDirectory->getPath();
			
			CURRENTDIRECTORY=cbData->selectedDirectory->getParent();
      }
    else
      {
      /* User selected a file: */
      newDataPath=cbData->selectedDirectory->getPath(cbData->selectedFileName);
			
			CURRENTDIRECTORY=cbData->selectedDirectory;
      }
    
		//record the selected file
    dataPaths.push_back(newDataPath);
    dataListBox->addItem(newDataPath.c_str());
    
    //destroy the file selection dialog
    cbData->fileSelectionDialog->close();
}

void CrustaApp::
addDataFileCancelCallback(
    FileSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    cbData->fileSelectionDialog->close();
}

void CrustaApp::
removeDataCallback(Button::SelectCallbackData*)
{
    //remove all the selected paths
    std::vector<int> selected = dataListBox->getSelectedItems();
    std::sort(selected.begin(), selected.end(), std::greater<int>());
    for (size_t i=0; i<selected.size(); ++i)
    {
        dataPaths.erase(dataPaths.begin()+selected[i]);
        dataListBox->removeItem(selected[i]);
    }
}

void CrustaApp::
clearDataCallback(GLMotif::Button::SelectCallbackData*)
{
    dataPaths.clear();
    dataListBox->clear();
}

void CrustaApp::
loadDataOkCallback(Button::SelectCallbackData*)
{
    //load the current data selection
    crusta->load(dataPaths);

    layerSettings.updateLayerList();

    //close the dialog
    Vrui::popdownPrimaryWidget(dataDialog);
}

void CrustaApp::
loadDataCancelCallback(Button::SelectCallbackData*)
{
    //close the dialog
    Vrui::popdownPrimaryWidget(dataDialog);
}

void CrustaApp::
alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData)
{
/* Do whatever to the surface frame, but don't change its scale factor: */
    Geometry::Point<double,3> origin = alignmentData.surfaceFrame.getOrigin();
    if (origin == Geometry::Point<double,3>::origin)
        origin = Geometry::Point<double,3>(0.0, 1.0, 0.0);

    SurfacePoint surfacePoint = crusta->snapToSurface(origin);

    //misuse the Geoid just to build a surface tangent frame
    Geometry::Geoid<double> geoid(SETTINGS->globeRadius, 0.0);
    Geometry::Point<double,3> lonLatEle = geoid.cartesianToGeodetic(surfacePoint.position);
    Geometry::Geoid<double>::Frame frame =
        geoid.geodeticToCartesianFrame(lonLatEle);
    alignmentData.surfaceFrame = Vrui::NavTransform(
        frame.getTranslation(), frame.getRotation(), alignmentData.surfaceFrame.getScaling());
}


void CrustaApp::
changeColorMapCallback(
    ColorMapEditor::ColorMapChangedCallbackData* cbData)
{
    int layerIndex = COLORMAPPER->getActiveLayer();
    if (!COLORMAPPER->isFloatLayer(layerIndex))
        return;

    Misc::ColorMap& colorMap = COLORMAPPER->getColorMap(layerIndex);
    colorMap = cbData->editor->getColorMap();
    COLORMAPPER->touchColor(layerIndex);
}

void CrustaApp::
changeColorMapRangeCallback(
    RangeEditor::RangeChangedCallbackData* cbData)
{
    int layerIndex = COLORMAPPER->getActiveLayer();
    if (!COLORMAPPER->isFloatLayer(layerIndex))
        return;

    Misc::ColorMap& colorMap = COLORMAPPER->getColorMap(layerIndex);
    colorMap.setValueRange(Misc::ColorMap::ValueRange(cbData->min,cbData->max));
    COLORMAPPER->touchRange(layerIndex);
}


void CrustaApp::
showPaletteEditorCallback(
    ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the main menu:
        Vrui::popupPrimaryWidget(paletteEditor);
    }
    else
    {
        //close the dialog:
        Vrui::popdownPrimaryWidget(paletteEditor);
    }
}


void CrustaApp::
decorateLinesCallback(ToggleButton::ValueChangedCallbackData* cbData)
{
    crusta->setDecoratedVectorArt(cbData->set);
}

void CrustaApp::
debugGridCallback(ToggleButton::ValueChangedCallbackData* cbData)
{
    QuadTerrain::displayDebuggingGrid = cbData->set;
}

void CrustaApp::
debugSpheresCallback(ToggleButton::ValueChangedCallbackData* cbData)
{
    QuadTerrain::displayDebuggingBoundingSpheres = cbData->set;
}

void CrustaApp::
surfaceSamplePickedCallback(SurfaceProbeTool::SampleCallbackData* cbData)
{
    int layerIndex = COLORMAPPER->getActiveLayer();
    if (!COLORMAPPER->isFloatLayer(layerIndex))
        return;

///\todo make this more generic not just for layerf
    LayerDataf::Type sample =
        DATAMANAGER->sampleLayerf(layerIndex, cbData->surfacePoint);
    if (sample == DATAMANAGER->getLayerfNodata())
        return;

    if (cbData->numSamples == 1)
        paletteEditor->getRangeEditor()->shift(sample);
    else if (cbData->sampleId == 1)
        paletteEditor->getRangeEditor()->setMin(sample);
    else
        paletteEditor->getRangeEditor()->setMax(sample);
}

void CrustaApp::
resetNavigationCallback(Misc::CallbackData* cbData)
{
    /* Reset the Vrui navigation transformation: */
    Vrui::setNavigationTransformation(Vrui::Point(0),1.5*SETTINGS->globeRadius);
    Vrui::concatenateNavigationTransformation(Vrui::NavTransform::translate(
        Vrui::Vector(0,SETTINGS->globeRadius,0)));
}



void CrustaApp::
frame()
{
    crusta->frame();
}

void CrustaApp::
display(GLContextData& contextData) const
{
    //make sure the glew context has been made current before any other display
    VruiGlew::enable(contextData);
    //let crusta do its thing
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
        typedef Vrui::SurfaceNavigationTool::AlignmentData AlignmentData;
        surfaceNavigationTool->setAlignFunction(
            Misc::createFunctionCall(
                this,&CrustaApp::alignSurfaceFrame));
    }

    //all crusta components get the crusta instance assigned
    CrustaComponent* component = dynamic_cast<CrustaComponent*>(cbData->tool);
    if (component != NULL)
        component->setupComponent(crusta);

    //connect probe tools
    SurfaceProbeTool* probe = dynamic_cast<SurfaceProbeTool*>(cbData->tool);
    if (probe != NULL)
    {
        probe->getSampleCallbacks().add(
            this, &CrustaApp::surfaceSamplePickedCallback);
    }

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
    SurfaceProjector* surface = dynamic_cast<SurfaceProjector*>(cbData->tool);
    if (surface != NULL)
        PROJECTION_FAILED = false;

#if CRUSTA_ENABLE_DEBUG
    if (cbData->tool == crusta->debugTool)
        crusta->debugTool = NULL;
#endif //CRUSTA_ENABLE_DEBUG
}



} //namespace crusta


int main(int argc, char* argv[])
{
    for (int i=1; i<argc; ++i)
    {
      if (strcasecmp(argv[i], "-version") == 0 || strcasecmp(argv[i], "-v") == 0)
      {
        std::cout << "Crusta version: " << CRUSTA_VERSION << std::endl;
        return 0;
      }
    }

    char** appDefaults = 0; // This is an additional parameter no one ever uses
    crusta::CrustaApp app(argc, argv, appDefaults);

    app.run();

    return 0;
}
