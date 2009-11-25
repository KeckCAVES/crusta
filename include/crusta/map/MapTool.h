#ifndef _MapTool_H_
#define _MapTool_H_

#include <crusta/Tool.h>


BEGIN_CRUSTA


class Shape;


class MapTool : public Tool
{
    friend class Vrui::GenericToolFactory<MapTool>;

public:
    typedef Vrui::GenericToolFactory<MapTool> Factory;

    MapTool(const Vrui::ToolFactory* iFactory,
            const Vrui::ToolInputAssignment& inputAssignment);

    static void init();

protected:
    enum Mode
    {
        MODE_DRAGGING,
        MODE_IDLE,
        MODE_SELECTING_CONTROL,
        MODE_SELECTING_SHAPE
    };

    Point3 getPosition();
    void   selectShape  (const Point3& pos);
    void   selectControl(const Point& pos);

    Mode               mode;
    Shape*             curShape;
    typename Shape::Id curControl;

    Point3 prevPosition;

    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;
    
    virtual void frame();
    virtual void buttonCallback(int deviceIndex, int buttonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
};


END_CRUSTA


#include <crusta/map/MapTool.hpp>

#endif //_MapTool_H_
