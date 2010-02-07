#include <crusta/map/MapManager.h>

#if __APPLE__
#include <GDAL/ogr_api.h>
#include <GDAL/ogrsf_frmts.h>
#else
#include <ogr_api.h>
#include <ogrsf_frmts.h>
#endif

#include <Geometry/Geoid.h>

#include <crusta/map/MapTool.h>
#include <crusta/map/Polyline.h>
#include <crusta/map/PolylineRenderer.h>
#include <crusta/map/PolylineTool.h>

///\todo remove dbg
#include <iostream>

BEGIN_CRUSTA


MapManager::
MapManager(Vrui::ToolFactory* parentToolFactory) :
    selectDistance(0.01), pointSelectionBias(0.25),
    polylineRenderer(new PolylineRenderer)
{
    Vrui::ToolFactory* factory = MapTool::init(parentToolFactory);
    PolylineTool::init(factory);

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
        delete *it;
    polylines.clear();
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

///\todo check the feature set of the layer here to make sure it has the needed

    //create a sphere-geoid to convert the cartesian points to lat,lon,elevation
    Geometry::Geoid<double> sphere(SPHEROID_RADIUS, 0.0);

    //grab all the features and their control points (we assume polylines only)
    OGRFeature* feature = NULL;

    layer->ResetReading();
    while ( (feature = layer->GetNextFeature()) != NULL)
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
        }

        OGRFeature::DestroyFeature(feature);
    }

    OGRDataSource::DestroyDataSource(source);
}

void MapManager::
save(const char* filename, const char* format)
{
    std::string fullName(filename);
    fullName.append(".");
    fullName.append(format);

    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
        format);
    if (driver == NULL)
    {
        std::cout << "MapManager::Save: Error initializing format driver: " <<
                     CPLGetLastErrorMsg() << std::endl;
        return;
    }

    OGRDataSource* source = driver->CreateDataSource(fullName.c_str());
    if (source == NULL)
    {
        std::cout << "MapManager::Save: Error creating file: " <<
                     CPLGetLastErrorMsg() << std::endl;
        return;
    }

    OGRSpatialReference crustaSys;
    crustaSys.SetGeogCS("Crusta World Coordinate System", "WGS_1984",
                        "Crusta WGS84 Spheroid",
                        SRS_WGS84_SEMIMAJOR, SRS_WGS84_INVFLATTENING,
                        "Greenwich", 0.0,
                        SRS_UA_RADIAN, 1.0);

    OGRLayer* layer = source->CreateLayer("Crusta_Polylines", &crustaSys,
                                          wkbLineString25D);
    if (layer == NULL)
    {
        std::cout << "MapManager::Save: Error creating the Crusta_Polylines " <<
                     "layer: " <<CPLGetLastErrorMsg() << std::endl;
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
    for (PolylinePtrs::iterator it=polylines.begin(); it!=polylines.end(); ++it)
    {
        if (*it == line)
        {
            polylines.erase(it);
            break;
        }
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


END_CRUSTA
