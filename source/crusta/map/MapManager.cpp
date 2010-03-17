#include <crusta/map/MapManager.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>

#if __APPLE__
#include <GDAL/ogr_api.h>
#include <GDAL/ogrsf_frmts.h>
#else
#include <ogr_api.h>
#include <ogrsf_frmts.h>
#endif

#include <Geometry/Geoid.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <GL/GLContextData.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/Menu.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/ScrolledListBox.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/WidgetManager.h>
#include <Misc/CreateNumberedFileName.h>
#include <Vrui/Vrui.h>
#include <Vrui/DisplayState.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/DemSpecs.h>
#include <crusta/map/MapTool.h>
#include <crusta/map/Polyline.h>
#include <crusta/map/PolylineRenderer.h>
#include <crusta/map/PolylineTool.h>
#include <crusta/QuadNodeData.h>

///\todo remove dbg
#if CRUSTA_ENABLE_DEBUG
#include <crusta/Visualizer.h>
#endif //CRUSTA_ENABLE_DEBUG
#include <iostream>

BEGIN_CRUSTA


MapManager::
MapManager(Vrui::ToolFactory* parentToolFactory, Crusta* iCrusta) :
    CrustaComponent(iCrusta), selectDistance(0.2), pointSelectionBias(0.1),
    polylineIds(uint32(~0)), polylineRenderer(new PolylineRenderer(iCrusta))
{
    Vrui::ToolFactory* factory = MapTool::init(parentToolFactory);
    PolylineTool::init(factory);

///\todo actually track multiple tools
    activeShape = NULL;

    OGRRegisterAll();
}

MapManager::
~MapManager()
{
    deleteAllShapes();
    delete polylineRenderer;

    OGRCleanupAll();
}

void MapManager::
deleteAllShapes()
{
    for (PolylinePtrs::iterator it=polylines.begin(); it!=polylines.end(); ++it)
    {
        polylineIds.release((*it)->getId());
        delete *it;
    }
    polylines.clear();

///\todo actually track multiple tools
    activeShape = NULL;
}

void MapManager::
load(const char* filename)
{
    //get rid of any existing shapes
    deleteAllShapes();

    OGRDataSource* source = OGRSFDriverRegistrar::Open(filename);
    if (source == NULL)
    {
        std::cout << "MapManager::Load: Error opening file: " <<
                     CPLGetLastErrorMsg() << std::endl;
        return;
    }

    //for now just look at the first layer
    OGRLayer* layer = source->GetLayerByName("Crusta_Polylines");
    if (layer == NULL)
    {
        std::cout << "MapManager::Load: Error retrieving Crusta_Polylines" <<
                     "Layer: " << CPLGetLastErrorMsg() << std::endl;
        OGRDataSource::DestroyDataSource(source);
        return;
    }

    //grab the index of the symbol field from the layer
    OGRFeatureDefn* featureDef       = layer->GetLayerDefn();
    int             symbolFieldIndex = featureDef->GetFieldIndex("Symbol");

///\todo check the feature set of the layer here to make sure it has the needed

    //create a sphere-geoid to convert the cartesian points to lat,lon,elevation
    Geometry::Geoid<double> sphere(SPHEROID_RADIUS, 0.0);

    //grab all the features and their control points (we assume polylines only)
    OGRFeature* feature = NULL;

///\todo do not restrict number of features read in
int numFeatureLimit = 5000;
int numFeature = 0;

    layer->ResetReading();
    while ( (feature = layer->GetNextFeature()) != NULL  && numFeature<numFeatureLimit)
//    while ( (feature = layer->GetNextFeature()) != NULL)
    {
        OGRGeometry* geo = feature->GetGeometryRef();

        if (geo!=NULL && geo->getGeometryType()==wkbLineString25D)
        {
            OGRLineString* in = (OGRLineString*)geo;
            int numPoints = in->getNumPoints();

///\todo figure out how to extract the shapes without the MapManager having to know all the shapes
            Polyline* out = NULL;
            if (numPoints > 0)
            {
                //create an internal polyline
                out = new Polyline;
                addPolyline(out);
            }

            for (int i=0; i<numPoints; ++i)
            {
                Point3 pos;
                pos[0] = Math::rad(in->getX(i));
                pos[1] = Math::rad(in->getY(i));
                pos[2] = in->getZ(i);
                pos = sphere.geodeticToCartesian(pos);

                out->addControlPoint(pos);
            }

            //read in the symbol field
            int symbolId     = feature->GetFieldAsInteger(symbolFieldIndex);
            out->getSymbol() = symbolMap[symbolId];
        }

        OGRFeature::DestroyFeature(feature);
///\todo do not restrict number of features read in
++numFeature;
    }

    OGRDataSource::DestroyDataSource(source);
}

void MapManager::
save(const char* fileName, const char* format)
{
///\todo change all the std::cout to throwing exceptions
    //initialize the output driver
    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
        format);
    if (driver == NULL)
    {
        std::cout << "MapManager::Save: Error initializing format driver: " <<
                     CPLGetLastErrorMsg() << std::endl;
        return;
    }

    //create the output data sink
    OGRDataSource* source = driver->CreateDataSource(fileName);
    if (source == NULL)
    {
        std::cout << "MapManager::Save: Error creating file: " <<
                     CPLGetLastErrorMsg() << std::endl;
        return;
    }

    //create a layer-field definition for outputting the symbol id
    OGRFieldDefn fieldDef("Symbol", OFTInteger);

///\todo create layers for all the different shape types to export
    //create a (georeferenced) layer for the polylines
    OGRSpatialReference crustaSys;
    crustaSys.SetWellKnownGeogCS("WGS84");

    OGRLayer* layer = source->CreateLayer("Crusta_Polylines", &crustaSys,
                                          wkbLineString25D);
    if (layer == NULL)
    {
        std::cout << "MapManager::Save: Error creating the Crusta_Polylines " <<
                     "layer: " <<CPLGetLastErrorMsg() << std::endl;
        OGRDataSource::DestroyDataSource(source);
        return;
    }

    if (layer->CreateField(&fieldDef) != OGRERR_NONE)
    {
        std::cout << "MapManager::Save: Error creating the symbol Field:" <<
                     CPLGetLastErrorMsg() << std::endl;
        OGRDataSource::DestroyDataSource(source);
        return;
    }

    //create a sphere-geoid to convert the cartesian points to lat,lon,elevation
    Geometry::Geoid<double> sphere(SPHEROID_RADIUS, 0.0);

    for (PolylinePtrs::iterator in=polylines.begin(); in!=polylines.end(); ++in)
    {
        OGRFeature* feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        if (feature == NULL)
        {
            std::cout << "MapManager::Save: Error creating feature: " <<
                         CPLGetLastErrorMsg() << std::endl;
            OGRDataSource::DestroyDataSource(source);
            return;
        }

        //output the symbol id
        feature->SetField("Symbol", (*in)->getSymbol().id);

        //output the polyline geometry
        OGRLineString out;
        Point3s& controlPoints = (*in)->getControlPoints();
        for (Point3s::iterator cp=controlPoints.begin();
             cp!=controlPoints.end(); ++cp)
         {
            Point3 lle = sphere.cartesianToGeodetic(*cp);
            out.addPoint(Math::deg(lle[0]), Math::deg(lle[1]), lle[2]);
         }

        feature->SetGeometry(&out);

        if (layer->CreateFeature(feature) != OGRERR_NONE)
        {
            std::cout << "MapManager::Save: Error adding feature to layer: " <<
                         CPLGetLastErrorMsg() << std::endl;
            OGRFeature::DestroyFeature(feature);
            OGRDataSource::DestroyDataSource(source);
            return;
        }

        OGRFeature::DestroyFeature(feature);
    }

    OGRDataSource::DestroyDataSource(source);
}


int MapManager::
registerMappingTool()
{
///\todo actually track multiple tools
    return 0;
}

void MapManager::
unregisterMappingTool(int)
{
///\todo actually track multiple tools
}

Shape*& MapManager::
getActiveShape(int toolId)
{
///\todo actually track multiple tools
    return activeShape;
}

void MapManager::
updateActiveShape(int toolId)
{
///\todo actually track multiple tools
    if (activeShape == NULL)
    {
        mapSymbolLabel->setLabel("-");
    }
    else
    {
        Shape::Symbol& symbol   = activeShape->getSymbol();
        std::string& symbolName = symbolReverseNameMap[symbol.id];
        mapSymbolLabel->setLabel(symbolName.c_str());
    }
}


const Shape::Symbol& MapManager::
getActiveSymbol()
{
    return activeSymbol;
}


Scalar MapManager::
getSelectDistance() const
{
    return selectDistance;
}

Scalar MapManager::
getPointSelectionBias() const
{
    return pointSelectionBias;
}

void MapManager::
addPolyline(Polyline* line)
{
    line->getId() = polylineIds.grab();
    polylines.push_back(line);
}

MapManager::PolylinePtrs& MapManager::
getPolylines()
{
    return polylines;
}

void MapManager::
removePolyline(Polyline* line)
{
    PolylinePtrs::iterator it =
        std::find(polylines.begin(), polylines.end(), line);
    if (it != polylines.end())
    {
        polylineIds.release((*it)->getId());
        delete *it;
        polylines.erase(it);
    }
}


MapManager::LineDataGenerator::
LineDataGenerator(const Nodes& nodes) :
    curLine(NULL)
{
    //initialize the line data map with empty data
    for (Nodes::const_iterator it=nodes.begin(); it!=nodes.end(); ++it)
        lineInfo.insert(NodeLineSectionMap::value_type(*it, LineSectionMap()));
}

void MapManager::LineDataGenerator::
newLine(Shape* nLine)
{
    curLine = nLine;
}

void MapManager::LineDataGenerator::
writeToNodes()
{
    static const int& lineTexSize = Crusta::lineDataTexSize;

    static const Color symbolOriginSize[2] = { Color(0.0f, 0.25f, 1.0f, 0.25f),
                                               Color(0.0f, 0.75f, 1.0f, 0.25f) };
    float scaleFac  = Vrui::getNavigationTransformation().getScaling();

    //go through all the nodes encountered
    for (NodeLineSectionMap::iterator nit=lineInfo.begin(); nit!=lineInfo.end();
         ++nit)
    {
        //clear the node's current data
        QuadNodeMainData* node = nit->first;
        Colors& lineData = node->lineData;
        lineData.clear();
        const Point3& centroid = node->centroid;

        LineSectionMap& lineSectionMap = nit->second;
        if (lineSectionMap.empty())
            continue;

        //determine texture space requirements
        int numSections = static_cast<int>(lineSectionMap.size());
        int numSegs     = 0;
        for (LineSectionMap::iterator lit=lineSectionMap.begin();
             lit!=lineSectionMap.end(); ++lit)
        {
            numSegs += static_cast<int>(lit->second.size());
        }

///\todo hardcoded 2 samples for now (just end points)
//uint32 baseOffsetX = curOffset[0];
        int texelsNeeded = 5 + numSections*2 + numSegs*3;
        //node data must fit into the line data texture
        if (texelsNeeded >= lineTexSize)
            continue;

    //- dump the node dependent data, i.e.: relative to tile transform
        //dump the number of sections in this node
        lineData.push_back(Color(numSections, 0, 0, 0));

/**\todo insert another level here: collections of lines that use the same
symbol from the atlas. Then dump the atlas info and the number of lines
following that use it. For now just duplicate the atlas info */

    //- go through all the lines for that node and dump the data
        for (LineSectionMap::iterator lit=lineSectionMap.begin();
             lit!=lineSectionMap.end(); ++lit)
        {
            Polyline* line = dynamic_cast<Polyline*>(lit->first);
            assert(line != NULL);
            CPIterators& cpis = lit->second;

        //- dump the line dependent data, i.e.: atlas info, number of segments
///\todo for now just generate one of two symbol visuals
            int symbolId = line->getSymbol().id % 2;
            lineData.push_back(symbolOriginSize[symbolId]);
            lineData.push_back(Color(cpis.size(), 0, 0, 0));

        //- dump all the segments for the current line
            for (CPIterators::iterator cit=cpis.begin(); cit!=cpis.end(); ++cit)
            {
                const Point3& curP  = **cit;
                const Point3& nextP = *((*cit)+1);
                Point3f curPf(curP[0]-centroid[0],
                              curP[1]-centroid[1],
                              curP[2]-centroid[2]);
                Point3f nextPf(nextP[0]-centroid[0],
                               nextP[1]-centroid[1],
                               nextP[2]-centroid[2]);

                Polyline::Coords::const_iterator coit = line->getCoord(*cit);
                Scalar curC  = scaleFac * (*coit);
                Scalar nextC = scaleFac * (*(coit+1));

                //segment control points
                lineData.push_back(Color(curPf[0], curPf[1], curPf[2], curC));
                lineData.push_back(Color(nextPf[0],nextPf[1],nextPf[2],nextC));

                //section normal
                Vector3 normal = Geometry::cross(Vector3(nextP), Vector3(curP));
                normal.normalize();
                lineData.push_back(Color(normal[0], normal[1], normal[2], 0.0));
            }
        }
    }
}

void MapManager::LineDataGenerator::
operator()(const Point3s::const_iterator& cp, QuadNodeMainData* node)
{
    assert(curLine != NULL);

    //check that the node intersected is part of the set we care for
    NodeLineSectionMap::iterator it = lineInfo.find(node);
    if (it == lineInfo.end())
        return;

    //record the end control point (see writeToNodes)
    it->second[curLine].push_back(cp);
}


void MapManager::
generateLineData(Nodes& nodes)
{
    //initialize the line data generator
    LineDataGenerator generator(nodes);

    //collect the info for all the lines wrt current approximation
    for (PolylinePtrs::iterator it=polylines.begin(); it!=polylines.end();
         ++it)
    {
        generator.newLine(*it);
        const Point3s& cps = (*it)->getControlPoints();
        if (cps.size()>1)
            crusta->traverseCurrentLeaves(cps.begin(), cps.end(), generator);
    }

    //generate and upload the line data
    generator.writeToNodes();
}

void MapManager::
frame()
{
    polylineRenderer->lines = &polylines;
}

void MapManager::
display(GLContextData& contextData) const
{
    //go through all the simple polylines and draw them
    polylineRenderer->display(contextData);
}


void MapManager::
addMenuEntry(GLMotif::Menu* mainMenu)
{
    //add the control dialog
    produceMapControlDialog(mainMenu);
    //add the symbols submenu
    produceMapSymbolSubMenu(mainMenu);
}

void MapManager::
openSymbolsGroupCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    //open the dialog at the same position as the main menu
    Vrui::getWidgetManager()->popupPrimaryWidget(
        symbolGroupMap[cbData->button->getName()],
        Vrui::getWidgetManager()->calcWidgetTransformation(cbData->button));
}

void MapManager::
symbolChangedCallback(GLMotif::ListBox::ItemSelectedCallbackData* cbData)
{
    const char* symbolName = cbData->listBox->getItem(cbData->selectedItem);
    int symbolId           = symbolNameMap[symbolName];
    activeSymbol           = symbolMap[symbolId];

///\todo process the change for multiple activeShapes
    if (activeShape != NULL)
    {
        activeShape->getSymbol() = activeSymbol;
        mapSymbolLabel->setLabel(symbolName);
    }
}

void MapManager::
closeSymbolsGroupCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    Vrui::popdownPrimaryWidget(symbolGroupMap[cbData->button->getName()]);
}


void MapManager::
produceMapControlDialog(GLMotif::Menu* mainMenu)
{
//- produce the dialog
    mapControlDialog = new GLMotif::PopupWindow(
        "MappingDialog", Vrui::getWidgetManager(), "Mapping Control");
    GLMotif::RowColumn* root = new GLMotif::RowColumn(
        "MappingRoot", mapControlDialog, false);

    GLMotif::RowColumn* infoRoot = new GLMotif::RowColumn(
        "MappingInfoRoot", root, false);

    new GLMotif::Label("CurSymbolText", infoRoot, "Current Shape Symbol:");
    mapSymbolLabel = new GLMotif::Label("CurSymbolLabel", infoRoot, "-");
    infoRoot->setNumMinorWidgets(2);
    infoRoot->manageChild();

    GLMotif::RowColumn* ioRoot = new GLMotif::RowColumn(
        "MappingInfoRoot", root, false);

    GLMotif::Button* load = new GLMotif::Button("LoadButton", ioRoot, "Load");
    load->getSelectCallbacks().add(this, &MapManager::loadMapCallback);
    GLMotif::Button* save = new GLMotif::Button("SaveButton", ioRoot, "Save");
    save->getSelectCallbacks().add(this, &MapManager::saveMapCallback);

    OGRSFDriverRegistrar* ogrRegistrar = OGRSFDriverRegistrar::GetRegistrar();
    int numDrivers = ogrRegistrar->GetDriverCount();
    std::vector<std::string> formats;
    for (int i=0; i<numDrivers; ++i)
    {
        OGRSFDriver* driver = ogrRegistrar->GetDriver(i);
        formats.push_back(std::string(driver->GetName()));
    }

    mapOutputFormat =
        new GLMotif::DropdownBox("MapFormatDrop", ioRoot, formats);

    ioRoot->setNumMinorWidgets(3);
    ioRoot->manageChild();

    root->setNumMinorWidgets(1);
    root->manageChild();

//- create the menu entry
    GLMotif::ToggleButton* mapControlToggle = new GLMotif::ToggleButton(
        "mapControlToggle", mainMenu, "Mapping Control");
    mapControlToggle->setToggle(false);
    mapControlToggle->getValueChangedCallbacks().add(
        this, &MapManager::showMapControlDialogCallback);
}

void MapManager::
produceMapSymbolSubMenu(GLMotif::Menu* mainMenu)
{
//- create the main cascade
    GLMotif::Popup* symbolsMenuPopup =
        new GLMotif::Popup("SymbolsMenuPopup", Vrui::getWidgetManager());

    GLMotif::SubMenu* symbolsMenu =
        new GLMotif::SubMenu("Symbols", symbolsMenuPopup, false);

//- add the default crusta symbol
    Shape::Symbol crustaDefaultSymbol;
    symbolNameMap["Crusta Default"]              = crustaDefaultSymbol.id;
    symbolReverseNameMap[crustaDefaultSymbol.id] = "Crusta Default";
    symbolMap[crustaDefaultSymbol.id]            = crustaDefaultSymbol;

//- parse the symbols definition file to create the symbols lists
///\todo fix the location of the configuration file
    std::ifstream symbolsConfig("Crusta_MapSymbols.cfg");
    if (!symbolsConfig.good())
        return;

    GLMotif::ScrolledListBox* symbolsGroup = NULL;
    std::string cfgLine;
    for (std::getline(symbolsConfig, cfgLine); !symbolsConfig.eof();
         std::getline(symbolsConfig, cfgLine))
    {
        if (cfgLine.empty() || cfgLine[0]=='#')
            continue;

        std::istringstream iss(cfgLine);
        //check for a new group
        std::string token;
        iss >> token;
        if (token.compare("group") == 0)
        {
            //read in the group name
            iss >> token;

            //create menu entry button to pop-up the group's list
            GLMotif::Button* groupButton = new GLMotif::Button(
                token.c_str(), symbolsMenu, token.c_str());
            groupButton->getSelectCallbacks().add(
                this, &MapManager::openSymbolsGroupCallback);

            //create the group's popup dialog
            GLMotif::PopupWindow*& groupDialog = symbolGroupMap[token];
            groupDialog = new GLMotif::PopupWindow(
                (token + "Dialog").c_str(), Vrui::getWidgetManager(),
                (token + " Symbols").c_str());
            GLMotif::RowColumn* groupRoot = new GLMotif::RowColumn(
                (token + "Root").c_str(), groupDialog, false);
            symbolsGroup = new GLMotif::ScrolledListBox(
                (token + "List").c_str(), groupRoot,
                GLMotif::ListBox::ALWAYS_ONE, 50, 15);
            symbolsGroup->showHorizontalScrollBar(true);
            symbolsGroup->getListBox()->getItemSelectedCallbacks().add(
                this, &MapManager::symbolChangedCallback);

            GLMotif::Button* close = new GLMotif::Button(
                token.c_str(), groupRoot, "Close");
            close->getSelectCallbacks().add(this,
                &MapManager::closeSymbolsGroupCallback);

            groupRoot->setNumMinorWidgets(1);
            groupRoot->manageChild();
        }
        else
        {
            if (symbolsGroup == NULL)
                continue;

            iss.seekg(0, std::ios::beg);

            //create a corresponding symbol
            std::string symbolName;
            iss >> symbolName;

            Shape::Symbol symbol;
            iss >> symbol.id >> symbol.color[0] >> symbol.color[1] >>
                                symbol.color[2] >> symbol.color[3];

            symbolNameMap[symbolName]       = symbol.id;
            symbolReverseNameMap[symbol.id] = symbolName;
            symbolMap[symbol.id]            = symbol;

            //populate the current group
            symbolsGroup->getListBox()->addItem(symbolName.c_str());
        }
    }

    symbolsMenu->manageChild();

    GLMotif::CascadeButton* symbolsMenuCascade =
        new GLMotif::CascadeButton("SymbolsMenuCascade", mainMenu,
                                   "Map Symbols");
    symbolsMenuCascade->setPopup(symbolsMenuPopup);
}


void MapManager::
showMapControlDialogCallback(
    GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
{
    if(cbData->set)
    {
        //open the dialog at the same position as the main menu:
        Vrui::getWidgetManager()->popupPrimaryWidget(mapControlDialog,
            Vrui::getWidgetManager()->calcWidgetTransformation(cbData->toggle));
    }
    else
    {
        //close the dialog
        Vrui::popdownPrimaryWidget(mapControlDialog);
    }
}

void MapManager::
loadMapCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    GLMotif::FileSelectionDialog* mapFileDialog =
        new GLMotif::FileSelectionDialog(Vrui::getWidgetManager(),
                                         "Load Map File", 0, NULL);
    mapFileDialog->getOKCallbacks().add(this,
        &MapManager::loadMapFileOKCallback);
    mapFileDialog->getCancelCallbacks().add(this,
        &MapManager::loadMapFileCancelCallback);
    Vrui::getWidgetManager()->popupPrimaryWidget(mapFileDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(mapControlDialog));
}

void MapManager::
saveMapCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    int selected       = mapOutputFormat->getSelectedItem();
    const char* format = mapOutputFormat->getItem(selected);
    std::string fileName("Crusta_Map.");
    fileName.append(format);
    fileName = Misc::createNumberedFileName(fileName, 4);
    save(fileName.c_str(), format);
}

void MapManager::
loadMapFileOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
{
    //load the selected map file
    load(cbData->selectedFileName.c_str());
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void MapManager::
loadMapFileCancelCallback(
    GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}


END_CRUSTA
