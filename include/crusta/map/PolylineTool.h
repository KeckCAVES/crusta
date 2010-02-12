#ifndef _PolylineTool_H_
#define _PolylineTool_H_

#include <crusta/map/MapTool.h>


BEGIN_CRUSTA


class PolylineTool : public MapTool
{
    friend class Vrui::GenericToolFactory<PolylineTool>;

public:
    typedef Vrui::GenericToolFactory<PolylineTool> Factory;

    PolylineTool(const Vrui::ToolFactory* iFactory,
                 const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~PolylineTool();

    static Vrui::ToolFactory* init(Vrui::ToolFactory* parent);

private:
    static Factory* factory;

//- Inherited from MapTool
protected:
    virtual void createShape(Shape*& shape, Shape::Id& control,
                             const Point3& pos);
    virtual void deleteShape(Shape*& shape, Shape::Id& control);
    virtual void removeControl(Shape*& shape, Shape::Id& control);

    virtual ShapePtrs getShapes();

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;
};


END_CRUSTA


#endif //_PolylineTool_H_
