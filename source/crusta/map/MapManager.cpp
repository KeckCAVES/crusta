#include <GL/VruiGlew.h> //must be included before gl.h

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
#include <Vrui/OpenFile.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>
#include <crusta/map/MapTool.h>
#include <crusta/map/Polyline.h>
#include <crusta/map/PolylineRenderer.h>
#include <crusta/map/PolylineTool.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadNodeData.h>
#include <crusta/QuadTerrain.h>
#include <crusta/ResourceLocator.h>

///\todo remove dbg
#if CRUSTA_ENABLE_DEBUG
#include <crusta/Visualizer.h>
#endif //CRUSTA_ENABLE_DEBUG
#include <iostream>

#include <crusta/StatsManager.h>


BEGIN_CRUSTA


MapManager::
MapManager(Vrui::ToolFactory* parentToolFactory, Crusta* iCrusta) :
    CrustaComponent(iCrusta), selectDistance(0.2), pointSelectionBias(0.1),
    polylineIds(uint32(~0)), polylineRenderer(iCrusta)
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
    Geometry::Geoid<double> sphere(SETTINGS->globeRadius, 0.0);

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
            Point3s cps;
            for (int i=0; i<numPoints; ++i)
            {
                Point3 pos;
                pos[0] = Math::rad(in->getX(i));
                pos[1] = Math::rad(in->getY(i));
                pos[2] = in->getZ(i);
                pos = sphere.geodeticToCartesian(pos);

                cps.push_back(pos);
            }
            if (!cps.empty())
            {
                //create new polyline and assign the control points
                Polyline* out = createPolyline();
                out->setControlPoints(cps);

                //read in the symbol field and assign it
                int symbolId = feature->GetFieldAsInteger(symbolFieldIndex);
                SymbolMap::iterator symbol = symbolMap.find(symbolId);
                if (symbol != symbolMap.end())
                    out->setSymbol(symbol->second);
                else
                    out->setSymbol(Shape::DEFAULT_SYMBOL);
            }
        }

        OGRFeature::DestroyFeature(feature);
///\todo do not restrict number of features read in
++numFeature;
    }


int numSegments = 0;
for (PolylinePtrs::iterator pit=polylines.begin(); pit!=polylines.end(); ++pit)
    numSegments += (*pit)->getControlPoints().size()-1;
std::cerr << "Crusta: imported " << numSegments << " polyline segments\n";

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
    crustaSys.SetGeogCS((std::string("Crusta_")+SETTINGS->globeName).c_str(),
                        "Crusta_Sphere_Datum", SETTINGS->globeName.c_str(),
                        SETTINGS->globeRadius, 0.0,
                        "Reference_Meridian", 0.0, SRS_UA_DEGREE,
                        atof(SRS_UA_DEGREE_CONV));

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
    Geometry::Geoid<double> sphere(SETTINGS->globeRadius, 0.0);

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
        Shape::ControlPointList& controlPoints = (*in)->getControlPoints();
        for (Shape::ControlPointHandle cp=controlPoints.begin();
             cp!=controlPoints.end(); ++cp)
         {
            Point3 lle = sphere.cartesianToGeodetic(cp->pos);
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
        mapSymbolLabel->setString("-");
    }
    else
    {
        const Shape::Symbol& symbol = activeShape->getSymbol();
        std::string& symbolName     = symbolReverseNameMap[symbol.id];
        mapSymbolLabel->setString(symbolName.c_str());
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

Polyline* MapManager::
createPolyline()
{
    Polyline* line = new Polyline(crusta);
    line->setId(polylineIds.grab());
    polylines.push_back(line);
    return line;
}

MapManager::PolylinePtrs& MapManager::
getPolylines()
{
    return polylines;
}

void MapManager::
deletePolyline(Polyline* line)
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


void MapManager::
addShapeCoverage(Shape* shape, const Shape::ControlPointHandle& startCP,
    const Shape::ControlPointHandle& endCP)
{
CRUSTA_DEBUG(41, std::cerr << "++ADD line " << shape->getId() << " ( ";
for (Shape::ControlPointHandle s=startCP; s!=endCP; ++s)
{
    if (s==startCP)
        std::cerr << s;
    else
        std::cerr << " | " << s;
}
std::cerr << " )\n\n";
)

    Shape::ControlPointHandle end = startCP;
    if (end != endCP) ++end;

    //setup the segment invariant part of the adder
    ShapeCoverageAdder adder;
    adder.setShape(shape);

    //traverse all the segments of the specified range
    for (Shape::ControlPointHandle start=startCP; end!=endCP; ++start, ++end)
    {
CRUSTA_DEBUG(42, std::cerr << "adding segment " << start << "\n";)
        //setup the current segment and traverse
        adder.setSegment(start);
        crusta->segmentCoverage(start->pos, end->pos, adder);
CRUSTA_DEBUG(42, std::cerr << "\n";)
    }

CRUSTA_DEBUG(49, crusta->validateLineCoverage();)
CRUSTA_DEBUG(41, std::cerr << "--ADD\n\n";)
}

void MapManager::
removeShapeCoverage(Shape* shape, const Shape::ControlPointHandle& startCP,
    const Shape::ControlPointHandle& endCP)
{
CRUSTA_DEBUG(41, std::cerr << "++REM line " << shape->getId() << " ( ";
for (Shape::ControlPointHandle s=startCP; s!=endCP; ++s)
{
    if (s==startCP)
        std::cerr << s;
    else
        std::cerr << " | " << s;
}
std::cerr << " )\n";
)

    Shape::ControlPointHandle end = startCP;
    if (end != endCP) ++end;

    //setup the segment invariant part the remover
    ShapeCoverageRemover remover;
    remover.setShape(shape);

    //traverse all the segments of the specified range
    for (Shape::ControlPointHandle start=startCP; end!=endCP; ++start, ++end)
    {
CRUSTA_DEBUG(42, std::cerr << "removing segment " << start << "\n";)
        //setup the current segment and traverse
        remover.setSegment(start);
        crusta->segmentCoverage(start->pos, end->pos, remover);
CRUSTA_DEBUG(49, crusta->confirmLineCoverageRemoval(shape, start);)
CRUSTA_DEBUG(42, std::cerr << "\n";)
    }

CRUSTA_DEBUG(49, crusta->validateLineCoverage();)
CRUSTA_DEBUG(41, std::cerr << "--REM\n\n";)
}

void MapManager::
inheritShapeCoverage(const NodeData& parent, NodeData& child)
{
statsMan.start(StatsManager::INHERITSHAPECOVERAGE);

    typedef NodeData::ShapeCoverage        Coverage;
    typedef Shape::ControlPointHandleList  HandleList;
    typedef Shape::ControlPointConstHandle Handle;

    child.lineCoverage.clear();

    //go through all lines from the parent's coverage
    for (Coverage::const_iterator lit=parent.lineCoverage.begin();
         lit!=parent.lineCoverage.end(); ++lit)
    {
        const Shape* const shape      = lit->first;
        const HandleList&  srcHandles = lit->second;
        HandleList*        dstHandles = NULL;

        assert(shape!=NULL);
        assert(srcHandles.size()>0);

        //intersect all the segments
        for (HandleList::const_iterator hit=srcHandles.begin();
             hit!=srcHandles.end(); ++hit)
        {
            Handle start = *hit;
            Handle end   = start; ++end;
            assert(start!=shape->getControlPoints().end() &&
                   end  !=shape->getControlPoints().end());

            //check for segment overlap
            if (!child.scope.intersects(start->pos, end->pos))
                continue;

            //insert segment into child coverage
            if (dstHandles == NULL)
                dstHandles = &child.lineCoverage[shape];

            dstHandles->push_back(*hit);
        }
    }

    //invalidate the child's line data
    child.lineNumSegments = 0;
    child.lineData.clear();

CRUSTA_DEBUG(49, crusta->validateLineCoverage();)

statsMan.stop(StatsManager::INHERITSHAPECOVERAGE);
}


void MapManager::
updateLineData(SurfaceApproximation& surface)
{
statsMan.start(StatsManager::UPDATELINEDATA);

    typedef NodeData::ShapeCoverage       Coverage;
    typedef Shape::ControlPointHandleList HandleList;
    typedef Shape::ControlPointHandle     Handle;

    const int lineTexSize = SETTINGS->lineDataTexSize;

    //go through all the nodes provided
    size_t numNodes = surface.visibles.size();
    for (size_t i=0; i<numNodes; ++i)
    {
        NodeData&         node          = *surface.visible(i).node;
        Coverage&         coverage      = node.lineCoverage;
        std::vector<int>& offsets       = node.lineCoverageOffsets;
        int&              numSegments   = node.lineNumSegments;
        Colors&           data          = node.lineData;
        FrameStamp&       dataStamp     = node.lineDataStamp;

        /* 1 trigger: there is coverage but no line data (result of coverage
           modifications since the mapmanager clears the data) */
        bool needToUpdate = !coverage.empty() && data.empty();
        //2 trigger: properties of specific segments have changed
        if (!needToUpdate)
        {
            for (Coverage::iterator lit=coverage.begin();
                 lit!=coverage.end() && !needToUpdate; ++lit)
            {
                HandleList& handles = lit->second;

                for (HandleList::iterator hit=handles.begin();
                     hit!=handles.end(); ++hit)
                {
                    //current tool stamps are a frame behind, hence <=
                    if (dataStamp <= (*hit)->stamp)
                    {
CRUSTA_DEBUG(51, std::cerr << "~~~INV n(" << node.index << ") has old " <<
"segment from line " << lit->first->getId() << " (stamps " << dataStamp <<
" vs " << (*hit)->stamp << ")\n\n";)
                        needToUpdate = true;
                        break;
                    }
                }
            }
        }

        //we're done here if none of the triggers fired
        if (!needToUpdate)
            continue;

CRUSTA_DEBUG(49, crusta->validateLineCoverage();)

//record the update to this node
statsMan.incrementDataUpdated();

CRUSTA_DEBUG(50, std::cerr << "###REGEN n(" << node.index << ") :\n" <<
coverage << "\n\n";)

    //- reset the current data offsets
        numSegments = 0;
        data.clear();
        offsets.clear();
        int curOff = 0;

    //- go through all the lines for that node and dump the data
        for (Coverage::iterator lit=coverage.begin();
             lit!=coverage.end() && curOff<lineTexSize; ++lit)
        {
            const Polyline* line = dynamic_cast<const Polyline*>(lit->first);
            assert(line != NULL);
            HandleList& handles = lit->second;
            const Shape::Symbol& symbol = line->getSymbol();

        //- dump all the segments for the current line
            for (HandleList::iterator hit=handles.begin();
                 hit!=handles.end() && curOff<lineTexSize; ++hit)
            {
                //dump the data
                Handle cur  = *hit;
                Handle next = cur; ++cur;

                Point3 curP  = crusta->mapToScaledGlobe(cur->pos);
                Point3 nextP = crusta->mapToScaledGlobe(next->pos);
                Point3f curPf(curP[0] - node.centroid[0],
                              curP[1] - node.centroid[1],
                              curP[2] - node.centroid[2]);
                Point3f nextPf(nextP[0] - node.centroid[0],
                               nextP[1] - node.centroid[1],
                               nextP[2] - node.centroid[2]);

                const Scalar& curC  = cur->coord;
                const Scalar& nextC = next->coord;

                //save the offset to the data
                offsets.push_back(curOff);

                //the atlas information for this segment
                data.push_back(symbol.originSize);
                ++curOff;

                //segment control points
                data.push_back(Color( curPf[0],  curPf[1],  curPf[2],  curC));
                ++curOff;
                data.push_back(Color(nextPf[0], nextPf[1], nextPf[2], nextC));
                ++curOff;

                //section normal
                Vector3 normal = Geometry::cross(Vector3(curP), Vector3(nextP));
                normal.normalize();
                data.push_back(Color(normal[0], normal[1], normal[2], 0.0));
                ++curOff;
            }
        }

        //update the stamp of the line data and the segment count
        numSegments = static_cast<int>(offsets.size());
        dataStamp   = CURRENT_FRAME;
    }

statsMan.stop(StatsManager::UPDATELINEDATA);
}


void MapManager::
processVerticalScaleChange()
{
statsMan.start(StatsManager::PROCESSVERTICALSCALE);

    //need to recompute all the polylines' coordinates
    for (PolylinePtrs::iterator it=polylines.begin(); it!=polylines.end(); ++it)
        (*it)->recomputeCoords((*it)->getControlPoints().begin());

statsMan.stop(StatsManager::PROCESSVERTICALSCALE);
}

void MapManager::
frame()
{
}

void MapManager::
display(GLContextData& contextData, const SurfaceApproximation& surface) const
{
    if (!SETTINGS->lineDecorated)
        polylineRenderer.display(contextData, surface);
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
    Vrui::popupPrimaryWidget(symbolGroupMap[cbData->button->getName()]);
}

void MapManager::
symbolChangedCallback(GLMotif::ListBox::ItemSelectedCallbackData* cbData)
{
    std::string symbolName(cbData->listBox->getParent()->getName());
    symbolName  += std::string("-");
    symbolName  += std::string(cbData->listBox->getItem(cbData->selectedItem));
    int symbolId = symbolNameMap[symbolName];
    activeSymbol = symbolMap[symbolId];

///\todo process the change for multiple activeShapes
    if (activeShape != NULL)
    {
        activeShape->setSymbol(activeSymbol);
        mapSymbolLabel->setString(symbolName.c_str());
    }
}

void MapManager::
closeSymbolsGroupCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    Vrui::popdownPrimaryWidget(symbolGroupMap[cbData->button->getName()]);
}


void MapManager::ShapeCoverageManipulator::
setShape(Shape* nShape)
{
    shape   = nShape;
}

void MapManager::ShapeCoverageManipulator::
setSegment(const Shape::ControlPointHandle& nSegment)
{
    segment = nSegment;
}


void MapManager::ShapeCoverageAdder::
operator()(NodeData& node, bool isLeaf)
{
    Shape::ControlPointHandleList& handles = node.lineCoverage[shape];

CRUSTA_DEBUG(43, std::cerr << "+" << node.index;)

CRUSTA_DEBUG(49,
    Shape::ControlPointHandleList::const_iterator fit;
    for (fit=handles.begin(); fit!=handles.end() && *fit!=segment; ++fit);
    if (fit != handles.end())
    {
        std::cerr << "DUPLICATE FOUND: node(" << node.index << ") " <<
            "contains:\n" << node.lineCoverage << "Offending segment is "
            "fit." << *fit << " segment." << segment << "\n";
    }
)

    //add the segment to the coverage
    handles.push_back(segment);

    //invalidate current line data
    node.lineNumSegments = 0;
    node.lineData.clear();

    //make sure that the subtree inherits the proper coverage when refined
    if (isLeaf)
    {
CRUSTA_DEBUG(44, std::cerr << "~";)
        node.lineInheritCoverage = true;
    }
CRUSTA_DEBUG(44, std::cerr << "\n";)
}

void MapManager::ShapeCoverageRemover::
operator()(NodeData& node, bool isLeaf)
{
    //find the shape in the coverage
    NodeData::ShapeCoverage::iterator lit = node.lineCoverage.find(shape);
    assert(lit != node.lineCoverage.end());
    Shape::ControlPointHandleList& handles = lit->second;
    assert(handles.size() > 0);

CRUSTA_DEBUG(43, std::cerr << "-" << node.index;)

    //find the specific segment
    Shape::ControlPointHandleList::iterator fit;
    for (fit=handles.begin(); fit!=handles.end() && *fit!=segment; ++fit);
    assert(fit != handles.end());

    //remove the segment from the coverage
    handles.erase(fit);

    //clean up emptied coverages
    if (handles.empty())
        node.lineCoverage.erase(lit);

    //invalidate current line data
    node.lineNumSegments = 0;
    node.lineData.clear();

    //make sure that the subtree inherits the proper coverage when refined
    if (isLeaf)
    {
CRUSTA_DEBUG(44, std::cerr << "~";)
        node.lineInheritCoverage = true;
    }
CRUSTA_DEBUG(44, std::cerr << "\n";)
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


//- parse the symbols definition file to create the symbols lists
    std::string cfgFileName =
        RESOURCELOCATOR.locateFile("config/mapSymbols.cfg");
    std::ifstream symbolsConfig(cfgFileName.c_str());
    if (!symbolsConfig.good())
        return;

    bool isDefault = false;

    GLMotif::ScrolledListBox* symbolsGroup = NULL;
    std::string groupName;
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
        if (token.compare("defaultSymbol") == 0)
        {
            isDefault = true;
        }
        else if (token.compare("group") == 0)
        {
            isDefault = false;

            //read in the group name
            iss >> groupName;

            //create menu entry button to pop-up the group's list
            GLMotif::Button* groupButton = new GLMotif::Button(
                groupName.c_str(), symbolsMenu, groupName.c_str());
            groupButton->getSelectCallbacks().add(
                this, &MapManager::openSymbolsGroupCallback);

            //create the group's popup dialog
            GLMotif::PopupWindow*& groupDialog = symbolGroupMap[groupName];
            groupDialog = new GLMotif::PopupWindow(
                (groupName + "Dialog").c_str(), Vrui::getWidgetManager(),
                (groupName + " Symbols").c_str());
            GLMotif::RowColumn* groupRoot = new GLMotif::RowColumn(
                (groupName + "Root").c_str(), groupDialog, false);
            symbolsGroup = new GLMotif::ScrolledListBox(
                groupName.c_str(), groupRoot,
                GLMotif::ListBox::ALWAYS_ONE, 50, 15);
            symbolsGroup->showHorizontalScrollBar(true);
            symbolsGroup->getListBox()->getItemSelectedCallbacks().add(
                this, &MapManager::symbolChangedCallback);

            GLMotif::Button* close = new GLMotif::Button(
                groupName.c_str(), groupRoot, "Close");
            close->getSelectCallbacks().add(this,
                &MapManager::closeSymbolsGroupCallback);

            groupRoot->setNumMinorWidgets(1);
            groupRoot->manageChild();
        }
        else
        {
            if (symbolsGroup==NULL && !isDefault)
                continue;

            iss.seekg(0, std::ios::beg);

            //create a corresponding symbol
            iss >> token;
            std::string symbolName(groupName+"-"+token);

            Shape::Symbol symbol;
            iss >> symbol.id >>
                   symbol.color[0] >> symbol.color[1] >>
                   symbol.color[2] >> symbol.color[3] >>
                   symbol.originSize[0] >> symbol.originSize[1] >>
                   symbol.originSize[2] >> symbol.originSize[3];

            //flip the Y to map to OpenGL texture reference
            symbol.originSize[1] = 1.0 - symbol.originSize[1];

            symbolNameMap[symbolName]       = symbol.id;
            symbolReverseNameMap[symbol.id] = symbolName;
            symbolMap[symbol.id]            = symbol;

            if (isDefault)
            {
///\todo create a menu entry for the default symbol and update that entry here
                Shape::DEFAULT_SYMBOL = symbol;
                //set the active shape to the default one
                activeSymbol = symbol;
            }
            else
            {
                //populate the current group
                symbolsGroup->getListBox()->addItem(token.c_str());
            }
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
        Vrui::popupPrimaryWidget(mapControlDialog);
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
                                         "Load Map File", CURRENTDIRECTORY, NULL);
    mapFileDialog->getOKCallbacks().add(this,
        &MapManager::loadMapFileOKCallback);
    mapFileDialog->getCancelCallbacks().add(this,
        &MapManager::loadMapFileCancelCallback);
    Vrui::popupPrimaryWidget(mapFileDialog);
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
    CURRENTDIRECTORY=cbData->selectedDirectory;

    //load the selected map file
    load(cbData->getSelectedPath().c_str());
    //destroy the file selection dialog
    cbData->fileSelectionDialog->close();
}

void MapManager::
loadMapFileCancelCallback(
    GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    cbData->fileSelectionDialog->close();
}


END_CRUSTA
