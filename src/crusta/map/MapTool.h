#ifndef _MapTool_H_
#define _MapTool_H_

#include <crusta/map/Shape.h>
#include <crusta/Tool.h>


namespace crusta {


class Shape;


class MapTool : public Tool
{
    friend class Vrui::GenericToolFactory<MapTool>;

public:
    typedef Vrui::GenericToolFactory<MapTool> Factory;

    MapTool(const Vrui::ToolFactory* iFactory,
            const Vrui::ToolInputAssignment& inputAssignment);
    ~MapTool();

    static Vrui::ToolFactory* init(Vrui::ToolFactory* parent);

protected:
    typedef std::vector<Shape*> ShapePtrs;

    enum Mode
    {
        MODE_DRAGGING,
        MODE_IDLE,
        MODE_SELECTING_CONTROL,
        MODE_SELECTING_SHAPE
    };

    virtual void createShape(Shape*& shape, Shape::ControlId& control,
                             const Geometry::Point<double,3>& pos);
    virtual void deleteShape(Shape*& shape, Shape::ControlId& control);
    virtual void addControlPoint(Shape*& shape, Shape::ControlId& control,
                                 const Geometry::Point<double,3>& pos);
    virtual void removeControl(Shape*& shape, Shape::ControlId& control);
    virtual void unselectShape(Shape*& shape, Shape::ControlId& control);

    virtual ShapePtrs getShapes();

    Geometry::Point<double,3> getPosition();
    void   selectShape   (const Geometry::Point<double,3>& pos);
    void   selectControl (const Geometry::Point<double,3>& pos);
    void   addPointAtEnds(const Geometry::Point<double,3>& pos);

    int              toolId;
    Mode             mode;
    Shape::ControlId curControl;

    Geometry::Point<double,3> prevPosition;

private:
    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int buttonSlotIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);

//- Inherited from Crusta::Component
public:
    virtual void setupComponent(Crusta* nCrusta);
};


} //namespace crusta


#endif //_MapTool_H_
