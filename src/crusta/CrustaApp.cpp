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
  if (dot_pos != std::string::npos && (slash_pos == std::string::npos || dot_pos > slash_pos)) {
    std::string ext = name.substr(dot_pos+1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
  } else {
    return "";
  }
}

CrustaApp::
CrustaApp(int& argc, char**& argv, char**& appDefaults) :
    Vrui::Application(argc, argv, appDefaults),
    dataDialog(NULL),
    paletteEditor(new PaletteEditor), layerSettings(paletteEditor)
{
    crusta = new Crusta(argv[0]);
    paletteEditor->getColorMapEditor()->getColorMapChangedCallbacks().add(this, &CrustaApp::changeColorMapCallback);
    paletteEditor->getRangeEditor()->getRangeChangedCallbacks().add(this, &CrustaApp::changeColorMapRangeCallback);

    for (int i=1; i<argc; ++i) handleArg(argv[i]);
    crusta->start();

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

void CrustaApp::handleArg(const std::string& arg)
{
  int file = open(arg.c_str(), O_RDONLY | O_NONBLOCK);
  if (file == -1) {
    std::cerr << "Warning: skipping unreadable file " << arg << std::endl;
    return;
  }
  struct stat stat_buf;
  int stat_status = fstat(file, &stat_buf);
  if (stat_status == -1) {
    std::cerr << "Warning: skipping unstatable file " << arg << std::endl;
  }
  std::string ext = get_extension(arg);
  if (ext == "cfg") {
    std::cerr << "Config: " << arg << std::endl;
    SETTINGS->mergeFromCfgFile(arg);
  } else if (ext == "wrl") {
    std::cerr << "VRML: " << arg << std::endl;
    crusta->loadSceneGraph(arg);
  } else if (ext == "pal") {
    std::cerr << "Palette: " << arg << std::endl;
    crusta->setPalette(arg);
  } else if (S_ISDIR(stat_buf.st_mode)) {
      std::cerr << "Globe: " << arg << std::endl;
      crusta->loadGlobe(arg);
  } else {
    std::cerr << "Project: " << arg << std::endl;
    std::string basedir = "./";
    size_t pos = arg.find_last_of("/");
    if (pos != string::npos) basedir = arg.substr(0, pos+1);
    std::string line;
    fcntl(file, F_SETFL, 0); // clear O_NONBLOCK
    while (true) {
      char buf;
      ssize_t s = read(file, &buf, 1);
      if (s <= 0 || buf == '\n' || buf == '\r') {
        if (!line.empty() && line[0] != '#') {
          if (line[0] != '/') line = basedir + line;
          handleArg(line);
        }
        if (s > 0) line.clear(); else break;
      } else {
        line += buf;
      }
    }
  }
  close(file);
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

    verticalScaleSettings.createMenuEntry(mainMenu);
    opacitySettings.createMenuEntry(mainMenu);
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
    //crusta->load(dataPaths); //TODO re-implement

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
        //typedef Vrui::SurfaceNavigationTool::AlignmentData AlignmentData;
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
