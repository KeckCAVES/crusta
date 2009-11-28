#ifndef _MapManager_H_
#define _MapManager_H_


#include <crusta/basics.h>


class GLContextData;

namespace Vrui {
    class ToolFactory;
}


BEGIN_CRUSTA


class Polyline;
class PolylineRenderer;


class MapManager
{
public:
    typedef std::vector<Polyline*> PolylinePtrs;

    MapManager(Vrui::ToolFactory* parentToolFactory);
    ~MapManager();

    double getSelectDistance() const;

    void addPolyline(Polyline* line);
    PolylinePtrs& getPolylines();
    void removePolyline(Polyline* line);

    void frame();
    void display(GLContextData& contextData) const;

protected:
    double selectDistance;

    PolylinePtrs      polylines;
    PolylineRenderer* polylineRenderer;
};


END_CRUSTA


#endif //_MapManager_H_
