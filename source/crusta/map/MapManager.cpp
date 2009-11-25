#include <crusta/map/MapManager.h>

#include <crusta/map/ControlPointEditor.h>
#include <crusta/map/Polyline.h>
#include <crusta/map/PolylineRenderer.h>

BEGIN_CRUSTA


MapManager::
MapManager() :
    selectDistance(0.001), polylineRenderer(new PolylineRenderer)
{}

MapManager::
~MapManager()
{
    for (PolylinePtrs::iterator it=polylines.begin(); it!=polylines.end(); ++it)
        delete *it;
    delete polylineRenderer;
}

double MapManager::
getSelectDistance() const
{
    return selectDistance;
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
    polylineRenderer->draw(contextData);
}


END_CRUSTA
