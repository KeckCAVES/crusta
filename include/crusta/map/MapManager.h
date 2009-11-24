#ifndef _MapManager_H_
#define _MapManager_H_

///\todo turn into unordered_map once all workstations are on gcc > 4.1
#include <map>

#include <crusta/basics.h>

class GLContextData;

namespace Vrui {
    class LocatorTool;
}


BEGIN_CRUSTA


class ControlPointEditor;
class Polyline;
class PolylineRenderer;
class Shape;


class MapManager
{
public:
    typedef std::vector<Shape*> ShapePtrs;

    enum MapToolId
    {
        MAPTOOL_CONTROLPOINTEDITOR
    };

    MapManager();
    ~MapManager();

    double getSelectDistance() const;
    Shape* getSelectedShape() const;

    Shape* createShape();

    void frame();
    void display(GLContextData& contextData) const;

    void createMapTool(MapTool toolId, Vrui::LocatorTool* locator);
    void destroyMapTool(Vrui::LocatorTool* locator);

protected:
    typedef std::map<Vrui::LocatorTool*, ControlPointEditor*>
        ControlPointEditorPtrs;
    typedef std::vector<Polyline*>           PolylinePtrs;

    double selectDistance;

    ControlPointEditorPtrs controlPointEditors;

    ShapePtrs shapes;
    PolylinePtrs polylines
    Shape* selectedShape;

    PolylineRenderer* polylineRenderer;
};


END_CRUSTA


#endif //_MapManager_H_
