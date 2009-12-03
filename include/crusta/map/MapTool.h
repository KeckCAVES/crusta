#ifndef _MapTool_H_
#define _MapTool_H_

#include <crusta/map/Shape.h>
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

    virtual Shape*    createShape();
    virtual ShapePtrs getShapes();

    Point3 getPosition();
    void   selectShape   (const Point3& pos);
    void   selectControl (const Point3& pos);
    void   addPointAtEnds(const Point3& pos);

    Mode      mode;
    Shape*    curShape;
    Shape::Id curControl;

    Point3 prevPosition;

private:
    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int deviceIndex, int buttonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
};


END_CRUSTA


#endif //_MapTool_H_
