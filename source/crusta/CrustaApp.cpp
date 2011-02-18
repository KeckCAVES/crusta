#include <GL/VruiGlew.h> //must be included before gl.h

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
#include <GLMotif/ScrolledListBox.h>
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

#include <crusta/ColorMapper.h>
#include <crusta/Crusta.h>
#include <crusta/map/MapManager.h>
#include <crusta/QuadTerrain.h>
#include <crusta/SurfaceProbeTool.h>
#include <crusta/SurfaceTool.h>


#include <Geometry/Geoid.h>
#include <Misc/FunctionCalls.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Tools/SurfaceNavigationTool.h>

#if CRUSTA_ENABLE_DEBUG
#include <crusta/DebugTool.h>
#endif //CRUSTA_ENABLE_DEBUG


static const crusta::Point2i DATALISTBOX_SIZE(30, 8);

using namespace GLMotif;


BEGIN_CRUSTA


CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults),
    newVerticalScale(1.0), dataDialog(NULL),
    enableSun(false),
    viewerHeadlightStates(new bool[Vrui::getNumViewers()]),
    sun(0), sunAzimuth(180.0), sunElevation(45.0),
    paletteEditor(new PaletteEditor), layerSettings(paletteEditor)
{
    paletteEditor->getColorMapEditor()->getColorMapChangedCallbacks().add(
        this, &CrustaApp::changeColorMapCallback);
    paletteEditor->getRangeEditor()->getRangeChangedCallbacks().add(
        this, &CrustaApp::changeColorMapRangeCallback);

    typedef std::vector<std::string> Strings;

    Strings dataNames;
    Strings settingsNames;
    for (int i=1; i<argc; ++i)
    {
        std::string token = std::string(argv[i]);
        if (token == std::string("-settings"))
            settingsNames.push_back(argv[++i]);
		else if (token == std::string("-version"))
			std::cout << "Crusta version: " << CRUSTA_VERSION << std::endl;
        else
            dataNames.push_back(token);
    }

    crusta = new Crusta;
    crusta->init(settingsNames);
    //load data passed through command line?
    crusta->load(dataNames);

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
createMenuEntry(Container* menu)
{
    init();

    parentMenu = menu;

    ToggleButton* toggle = new ToggleButton(
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
}

void CrustaApp::Dialog::
showCallback(ToggleButton::ValueChangedCallbackData* cbData)
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

    const StyleSheet* style = Vrui::getWidgetManager()->getStyleSheet();

    colorPicker.getColorPicker()->getColorChangedCallbacks().add(
            this, &CrustaApp::SpecularSettingsDialog::colorChangedCallback);


    RowColumn* root = new RowColumn("Root", dialog, false);

    RowColumn* colorRoot = new RowColumn(
        "ColorRoot", root, false);

    Color sc = SETTINGS->terrainSpecularColor;
    new Label("SSColor", colorRoot, "Color: ");
    colorButton = new Button("SSColorButton", colorRoot, "");
    colorButton->setBackgroundColor(Color(sc[0], sc[1], sc[2], sc[3]));
    colorButton->getSelectCallbacks().add(
        this, &CrustaApp::SpecularSettingsDialog::colorButtonCallback);

    colorRoot->setNumMinorWidgets(2);
    colorRoot->manageChild();

    RowColumn* shininessRoot = new RowColumn(
        "ShininessRoot", root, false);

    float shininess = SETTINGS->terrainShininess;
    new Label("SSShininess", shininessRoot, "Shininess: ");
    Slider* slider = new Slider(
        "SSShininessSlider", shininessRoot, Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    slider->setValue(log(shininess)/log(2));
    slider->setValueRange(0.00f, 7.0f, 0.00001f);
    slider->getValueChangedCallbacks().add(
        this, &CrustaApp::SpecularSettingsDialog::shininessChangedCallback);
    shininessField = new TextField(
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
    Button::SelectCallbackData* cbData)
{
    WidgetManager* manager = Vrui::getWidgetManager();
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
        ColorPicker::ColorChangedCallbackData* cbData)
{
    const Color& c = cbData->newColor;
    colorButton->setBackgroundColor(c);
    crusta->setTerrainSpecularColor(Color(c[0], c[1], c[2], c[3]));
}

void CrustaApp::SpecularSettingsDialog::
shininessChangedCallback(Slider::ValueChangedCallbackData* cbData)
{
    float shininess = pow(2, cbData->value) - 1.0f;
    crusta->setTerrainShininess(shininess);
    shininessField->setValue(shininess);
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

    /* Create a button to open or hide the vertical scale adjustment dialog: */
    ToggleButton* showVerticalScaleToggle = new ToggleButton(
        "ShowVerticalScaleToggle", mainMenu, "Vertical Scale");
    showVerticalScaleToggle->setToggle(false);
    showVerticalScaleToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showVerticalScaleCallback);

    /* Create a button to toogle display of the lighting dialog: */
    ToggleButton* lightingToggle = new ToggleButton(
        "LightingToggle", mainMenu, "Light Settings");
    lightingToggle->setToggle(false);
    lightingToggle->getValueChangedCallbacks().add(
        this, &CrustaApp::showLightingDialogCallback);

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

    //specular settings dialog toggle
    specularSettings.createMenuEntry(settingsMenu);

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
    Vrui::getWidgetManager()->popupPrimaryWidget(dataDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(popMenu));
}

void CrustaApp::
addDataCallback(Button::SelectCallbackData*)
{
    FileAndFolderSelectionDialog* fileDialog =
        new FileAndFolderSelectionDialog(Vrui::getWidgetManager(),
            "Load Crusta Globe File", 0, NULL);
    fileDialog->getOKCallbacks().add(this,
        &CrustaApp::addDataFileOkCallback);
    fileDialog->getCancelCallbacks().add(this,
        &CrustaApp::addDataFileCancelCallback);
    Vrui::getWidgetManager()->popupPrimaryWidget(fileDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(dataDialog));
}

void CrustaApp::
addDataFileOkCallback(FileAndFolderSelectionDialog::OKCallbackData* cbData)
{
    //record the selected file
    dataPaths.push_back(cbData->selectedFileName);
    dataListBox->addItem(cbData->selectedFileName.c_str());
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void CrustaApp::
addDataFileCancelCallback(
    FileAndFolderSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
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
produceVerticalScaleDialog()
{
    const StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();

    verticalScaleDialog = new PopupWindow(
        "ScaleDialog", Vrui::getWidgetManager(), "Vertical Scale");
    RowColumn* root = new RowColumn(
        "ScaleRoot", verticalScaleDialog, false);
    Slider* slider = new Slider(
        "ScaleSlider", root, Slider::HORIZONTAL,
        10.0 * style->fontHeight);
    verticalScaleLabel = new Label("ScaleLabel", root, "1.0x");

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
    const StyleSheet* style =
        Vrui::getWidgetManager()->getStyleSheet();
    lightingDialog=new PopupWindow("LightingDialog",
        Vrui::getWidgetManager(), "Light Settings");
    RowColumn* lightSettings=new RowColumn("LightSettings",
        lightingDialog, false);
    lightSettings->setNumMinorWidgets(2);

    /* Create a toggle button and two sliders to manipulate the sun light source: */
    Margin* enableSunToggleMargin=new Margin("SunToggleMargin",lightSettings,false);
    enableSunToggleMargin->setAlignment(Alignment(Alignment::HFILL,Alignment::VCENTER));
    ToggleButton* enableSunToggle=new ToggleButton("SunToggle",enableSunToggleMargin,"Sun Light Source");
    enableSunToggle->setToggle(enableSun);
    enableSunToggle->getValueChangedCallbacks().add(this,&CrustaApp::enableSunToggleCallback);
    enableSunToggleMargin->manageChild();

    RowColumn* sunBox=new RowColumn("SunBox",lightSettings,false);
    sunBox->setOrientation(RowColumn::VERTICAL);
    sunBox->setNumMinorWidgets(2);
    sunBox->setPacking(RowColumn::PACK_TIGHT);

    sunAzimuthTextField=new TextField("SunAzimuthTextField",sunBox,5);
    sunAzimuthTextField->setFloatFormat(TextField::FIXED);
    sunAzimuthTextField->setFieldWidth(3);
    sunAzimuthTextField->setPrecision(0);
    sunAzimuthTextField->setValue(double(sunAzimuth));

    sunAzimuthSlider=new Slider("SunAzimuthSlider",sunBox,Slider::HORIZONTAL,style->fontHeight*10.0f);
    sunAzimuthSlider->setValueRange(0.0,360.0,1.0);
    sunAzimuthSlider->setValue(double(sunAzimuth));
    sunAzimuthSlider->getValueChangedCallbacks().add(this,&CrustaApp::sunAzimuthSliderCallback);

    sunElevationTextField=new TextField("SunElevationTextField",sunBox,5);
    sunElevationTextField->setFloatFormat(TextField::FIXED);
    sunElevationTextField->setFieldWidth(2);
    sunElevationTextField->setPrecision(0);
    sunElevationTextField->setValue(double(sunElevation));

    sunElevationSlider=new Slider("SunElevationSlider",sunBox,Slider::HORIZONTAL,style->fontHeight*10.0f);
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

    SurfacePoint surfacePoint = crusta->snapToSurface(origin);

    //misuse the Geoid just to build a surface tangent frame
    Geometry::Geoid<double> geoid(SETTINGS->globeRadius, 0.0);
    Point3 lonLatEle = geoid.cartesianToGeodetic(surfacePoint.position);
    Geometry::Geoid<double>::Frame frame =
        geoid.geodeticToCartesianFrame(lonLatEle);
    surfaceFrame = Vrui::NavTransform(
        frame.getTranslation(), frame.getRotation(), surfaceFrame.getScaling());
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
showVerticalScaleCallback(
    ToggleButton::ValueChangedCallbackData* cbData)
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
changeScaleCallback(Slider::ValueChangedCallbackData* cbData)
{
    double newVerticalScale = pow(10, cbData->value);
    crusta->setVerticalScale(newVerticalScale);

    std::ostringstream oss;
    oss.precision(2);
    oss << newVerticalScale << "x";
    verticalScaleLabel->setLabel(oss.str().c_str());
}


void CrustaApp::
showLightingDialogCallback(ToggleButton::ValueChangedCallbackData* cbData)
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
enableSunToggleCallback(ToggleButton::ValueChangedCallbackData* cbData)
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
sunAzimuthSliderCallback(Slider::ValueChangedCallbackData* cbData)
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
sunElevationSliderCallback(Slider::ValueChangedCallbackData* cbData)
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
    ToggleButton::ValueChangedCallbackData* cbData)
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
        surfaceNavigationTool->setAlignFunction(
            Misc::createFunctionCall<Vrui::NavTransform&,CrustaApp>(
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



END_CRUSTA


int main(int argc, char* argv[])
{
    char** appDefaults = 0; // This is an additional parameter no one ever uses
    crusta::CrustaApp app(argc, argv, appDefaults);

    app.run();

    return 0;
}
