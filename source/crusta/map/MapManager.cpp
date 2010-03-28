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
#include <crusta/QuadCache.h>
#include <crusta/QuadNodeData.h>
#include <crusta/QuadTerrain.h>

///\todo remove dbg
#if CRUSTA_ENABLE_DEBUG
#include <crusta/Visualizer.h>
#endif //CRUSTA_ENABLE_DEBUG
#include <iostream>

#define CHECK_CONVERAGE_VALIDITY 0

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
                Polyline* out = new Polyline(crusta);
                //add the polyline to the managed set (this gives it its id)
                addPolyline(out);

                //configure the polyline
                out->setControlPoints(cps);

                //read in the symbol field and assign it
                int symbolId = feature->GetFieldAsInteger(symbolFieldIndex);
                out->setSymbol(symbolMap[symbolId]);

            }
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
        mapSymbolLabel->setLabel("-");
    }
    else
    {
        const Shape::Symbol& symbol = activeShape->getSymbol();
        std::string& symbolName     = symbolReverseNameMap[symbol.id];
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
    line->setId(polylineIds.grab());
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

    ShapeCoverageAdder adder;
    adder.setShape(shape);

    for (Shape::ControlPointHandle start=startCP; end!=endCP; ++start, ++end)
    {
CRUSTA_DEBUG(42, std::cerr << "adding segment " << start << "\n");
        adder.setSegment(start);
        crusta->intersect(start, adder);
CRUSTA_DEBUG(42, std::cerr << "\n");
    }

#if CHECK_CONVERAGE_VALIDITY
CRUSTA_DEBUG_ONLY(crusta->validateLineCoverage();)
#endif //CHECK_CONVERAGE_VALIDITY
CRUSTA_DEBUG(41, std::cerr << "--ADD\n\n");
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

    ShapeCoverageRemover remover;
    remover.setShape(shape);

    for (Shape::ControlPointHandle start=startCP; end!=endCP; ++start, ++end)
    {
CRUSTA_DEBUG(42, std::cerr << "removing segment " << start << "\n");
        remover.setSegment(start);
        crusta->intersect(start, remover);
#if CHECK_CONVERAGE_VALIDITY
CRUSTA_DEBUG_ONLY(crusta->confirmLineCoverageRemoval(shape, start);)
#endif //CHECK_CONVERAGE_VALIDITY
CRUSTA_DEBUG(42, std::cerr << "\n");
    }

#if CHECK_CONVERAGE_VALIDITY
CRUSTA_DEBUG_ONLY(crusta->validateLineCoverage();)
#endif //CHECK_CONVERAGE_VALIDITY
CRUSTA_DEBUG(41, std::cerr << "--REM\n\n");
}

void MapManager::
inheritShapeCoverage(const QuadNodeMainData& parent, QuadNodeMainData& child)
{
    typedef QuadNodeMainData::ShapeCoverage                    Coverage;
    typedef QuadNodeMainData::AgeStampedControlPointHandleList HandleList;
    typedef Shape::ControlPointList::const_iterator            ConstHandle;

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
        Scalar tin= 0, tout= 0;
        int    sin=-1, sout=-1;
        for (HandleList::const_iterator hit=srcHandles.begin();
             hit!=srcHandles.end(); ++hit)
        {
            ConstHandle start = hit->handle;
            ConstHandle end   = start; ++end;
            assert(start!=shape->getControlPoints().end() &&
                   end  !=shape->getControlPoints().end());

            //intersect ray and check for overlap
            Ray ray(start->pos, end->pos);
            QuadTerrain::intersectNodeSides(child, ray, tin, sin, tout, sout);
            if (tin>=1.0 || tout<=0.0)
                continue;

            //insert segment into child coverage
            if (dstHandles == NULL)
                dstHandles = &child.lineCoverage[shape];

            dstHandles->push_back(*hit);
            child.lineCoverageDirty |= true;
        }
    }

    //invalidate the child's line data
    child.lineData.clear();

#if CHECK_CONVERAGE_VALIDITY
CRUSTA_DEBUG_ONLY(crusta->validateLineCoverage();)
#endif //CHECK_CONVERAGE_VALIDITY
}


void MapManager::
updateLineData(Nodes& nodes)
{
    typedef QuadNodeMainData::ShapeCoverage                    Coverage;
    typedef QuadNodeMainData::AgeStampedControlPointHandleList HandleList;
    typedef Shape::ControlPointHandle                          Handle;

    static const int& lineTexSize = Crusta::lineDataTexSize;

    //go through all the nodes provided
    for (Nodes::iterator nit=nodes.begin(); nit!=nodes.end(); ++nit)
    {
        QuadNodeMainData*                node     = *nit;
        QuadNodeMainData::ShapeCoverage& coverage = node->lineCoverage;
        Colors&                          offsets  = node->lineCoverageOffsets;
        Colors&                          data     = node->lineData;

/**\todo integrate check for deprecated data only to where nodes are added to
the representation. For now just check deprecation here...
Actually there is a problem with not checking this before drawing: the current
code assumes this is going to happen is this flags redraws by updating the age
of the changed segments (e.g. new symbol, new coords) */
if (!data.empty())
{
    for (Coverage::iterator lit=coverage.begin();
         lit!=coverage.end() && !data.empty(); ++lit)
    {
        HandleList& handles = lit->second;

        for (HandleList::iterator hit=handles.begin();hit!=handles.end();++hit)
        {
            if (hit->age != hit->handle->age)
            {
CRUSTA_DEBUG(51, std::cerr << "~~~INV n(" << node->index << ") has old " <<
"segment from line " << lit->first->getId() << " (ages " << hit->age <<
" vs " << hit->handle->age << ")\n\n";)
                data.clear();
                break;
            }
        }
    }
}

        //does not require update if 1. has data or 2. is not overlapped
        if (!data.empty() || coverage.empty())
            continue;

///\todo debug: check for duplicates in the line coverage
static bool checkForDuplicates = false;
if (checkForDuplicates) {
for (Coverage::iterator lit=coverage.begin(); lit!=coverage.end(); ++lit)
{
    const Shape* const shape = lit->first;
    HandleList& handles      = lit->second;
    assert(handles.size() > 0);
    assert(dynamic_cast<const Polyline*>(shape) != NULL);

    for (HandleList::iterator hit=handles.begin(); hit!=handles.end(); ++hit)
    {
        HandleList::iterator nhit = hit;
        for (++nhit; nhit!=handles.end(); ++nhit)
            assert(hit->handle != nhit->handle);
    }
}
}

        //determine texture space requirements
        int numLines = static_cast<int>(coverage.size());
        int numSegs  = 0;
        for (Coverage::iterator lit=coverage.begin();lit!=coverage.end();++lit)
        {
            numSegs += static_cast<int>(lit->second.size());
        }

///\todo hardcoded 2 samples for now (just end points)
//uint32 baseOffsetX = curOffset[0];
        int texelsNeeded = 5 + numLines*2 + numSegs*3;
        //node data must fit into the line data texture
        if (texelsNeeded >= lineTexSize)
            continue;

CRUSTA_DEBUG(50, std::cerr << "###REGEN n(" << node->index << ") :\n" <<
coverage << "\n\n";)

    //- reset the offsets
        offsets.clear();
        Colors off;
        uint32 symbolOff;
        uint32 curOff = 0.0;

    //- dump the node dependent data, i.e.: relative to tile transform
        //dump the number of sections in this node
        data.push_back(Color(numLines, 0, 0, 0));
        ++curOff;

/**\todo insert another level here: collections of lines that use the same
symbol from the atlas. Then dump the atlas info and the number of lines
following that use it. For now just duplicate the atlas info */

    //- go through all the lines for that node and dump the data
        const Point3& centroid = node->centroid;

        for (Coverage::iterator lit=coverage.begin();lit!=coverage.end();++lit)
        {
            const Polyline* line = dynamic_cast<const Polyline*>(lit->first);
            assert(line != NULL);
            HandleList& handles = lit->second;

        //- save the offset to the symbols definition
            symbolOff = curOff;

        //- dump the line dependent data, i.e.: atlas info, number of segments
            const Shape::Symbol& symbol = line->getSymbol();
            data.push_back(symbol.originSize);
            ++curOff;
            data.push_back(Color(handles.size(), 0, 0, 0));
            ++curOff;

        //- age stamp and dump all the segments for the current line
            for (HandleList::iterator hit=handles.begin(); hit!=handles.end();
                 ++hit)
            {
                //age stamp the current data to the shape's age
                hit->age = hit->handle->age;

                //dump the data
                Handle cur  = hit->handle;
                Handle next = cur; ++cur;

                Point3 curP  = crusta->mapToScaledGlobe(cur->pos);
                Point3 nextP = crusta->mapToScaledGlobe(next->pos);
                Point3f curPf(curP[0]-centroid[0],
                              curP[1]-centroid[1],
                              curP[2]-centroid[2]);
                Point3f nextPf(nextP[0]-centroid[0],
                               nextP[1]-centroid[1],
                               nextP[2]-centroid[2]);

                const Scalar& curC  = cur->coord;
                const Scalar& nextC = next->coord;

                //save the offset to the data
                Color coff((symbolOff&0xFF)             / 255.0f,
                           (((symbolOff>>8)&0xFF) + 64) / 255.0f,
                           (curOff&0xFF)                / 255.0f,
                           ((curOff>>8)&0xFF)           / 255.0f);
                offsets.push_back(coff);

                //segment control points
                data.push_back(Color( curPf[0],  curPf[1],  curPf[2],  curC));
                ++curOff;
                data.push_back(Color(nextPf[0], nextPf[1], nextPf[2], nextC));
                ++curOff;

                //section normal
                Vector3 normal = Geometry::cross(Vector3(nextP), Vector3(curP));
                normal.normalize();
                data.push_back(Color(normal[0], normal[1], normal[2], 0.0));
                ++curOff;
            }
        }

        //update the age of the line data
        node->lineCoverageAge = crusta->getCurrentFrame();
    }
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
        activeShape->setSymbol(activeSymbol);
        mapSymbolLabel->setLabel(symbolName);
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
operator()(QuadNodeMainData* node, bool isLeaf)
{
    typedef QuadNodeMainData::ShapeCoverage                    Coverage;
    typedef QuadNodeMainData::AgeStampedControlPointHandleList CHandleList;
    typedef QuadNodeMainData::AgeStampedControlPointHandle     CHandle;
    typedef Shape::ControlPointHandle                          SHandle;

    CHandleList& chandles = node->lineCoverage[shape];

CRUSTA_DEBUG(43, std::cerr << "+" << node->index;)

///\todo Vis2010 this needs to be put into debug
//CRUSTA_DEBUG_ONLY(
    CHandleList::iterator fit;
    for (fit=chandles.begin();
         fit!=chandles.end() && fit->handle!=segment;
         ++fit);
    if (fit != chandles.end())
    {
        std::cerr << "DUPLICATE FOUND: node(" << node->index << ") " <<
            "contains:\n" << node->lineCoverage << "Offending segment is "
            "fit." << fit->handle << " segment." << segment << "\n";
    }
//)

    chandles.push_back(CHandle(segment->age, segment));

    //invalidate current line data
    node->lineData.clear();
    //record the change for the culled tree if this is a leaf node
    if (isLeaf)
    {
CRUSTA_DEBUG(44, std::cerr << "~\n";)
        node->lineCoverageDirty |= true;
    }
    else
    {
CRUSTA_DEBUG(44, std::cerr << "\n";)
    }
}

void MapManager::ShapeCoverageRemover::
operator()(QuadNodeMainData* node, bool isLeaf)
{
    typedef QuadNodeMainData::ShapeCoverage                    Coverage;
    typedef QuadNodeMainData::AgeStampedControlPointHandleList CHandleList;
    typedef QuadNodeMainData::AgeStampedControlPointHandle     CHandle;
    typedef Shape::ControlPointHandle                          SHandle;

    Coverage::iterator lit = node->lineCoverage.find(shape);
    assert(lit != node->lineCoverage.end());
    CHandleList& chandles = lit->second;
    assert(chandles.size() > 0);

CRUSTA_DEBUG(43, std::cerr << "-" << node->index;)

    CHandleList::iterator fit;
    for (fit=chandles.begin();
         fit!=chandles.end() && fit->handle!=segment;
         ++fit);
    assert(fit != chandles.end());

    chandles.erase(fit);

    //clean up emptied coverages
    if (chandles.empty())
        node->lineCoverage.erase(lit);

    //invalidate current line data
    node->lineData.clear();
    //record the change for the culled tree if this is a leaf node
    if (isLeaf)
    {
CRUSTA_DEBUG(44, std::cerr << "~\n";)
        node->lineCoverageDirty |= true;
    }
    else
    {
CRUSTA_DEBUG(44, std::cerr << "\n";)
    }
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
///\todo fix the location of the configuration file
    std::ifstream symbolsConfig("Crusta_MapSymbols.cfg");
    if (!symbolsConfig.good())
        return;

    bool isDefault = false;

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
        if (token.compare("defaultSymbol") == 0)
        {
            isDefault = true;
        }
        else if (token.compare("group") == 0)
        {
            isDefault = false;

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
            if (symbolsGroup==NULL && !isDefault)
                continue;

            iss.seekg(0, std::ios::beg);

            //create a corresponding symbol
            std::string symbolName;
            iss >> symbolName;

            Shape::Symbol symbol;
            iss >> symbol.id >>
                   symbol.color[0] >> symbol.color[1] >>
                   symbol.color[2] >> symbol.color[3] >>
                   symbol.originSize[0] >> symbol.originSize[1] >>
                   symbol.originSize[2] >> symbol.originSize[3];

            /* flip the y scale to mirror the symbols (so that fragment program
               doesn't have to) */
            symbol.originSize[3] = -symbol.originSize[3];

            symbolNameMap[symbolName]       = symbol.id;
            symbolReverseNameMap[symbol.id] = symbolName;
            symbolMap[symbol.id]            = symbol;

            if (isDefault)
            {
///\todo create a menu entry for the default symbol and update that entry here
                //set the active shape to the default one
                activeSymbol = symbol;
            }
            else
            {
                //populate the current group
                symbolsGroup->getListBox()->addItem(symbolName.c_str());
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
