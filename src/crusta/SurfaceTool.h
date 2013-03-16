#ifndef _SurfaceTool_H_
#define _SurfaceTool_H_

#include <crusta/SurfaceProjector.h>

#include <crusta/vrui.h>


namespace crusta {


class SurfaceTool : public Vrui::TransformTool, public SurfaceProjector
{
    friend class Vrui::GenericToolFactory<SurfaceTool>;

public:
    typedef Vrui::GenericToolFactory<SurfaceTool> Factory;

    SurfaceTool(const Vrui::ToolFactory* iFactory,
                const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~SurfaceTool();

    static Vrui::ToolFactory* init();

private:
    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual void initialize();
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int buttonSlotIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
};

} //namespace crusta


#endif //_SurfaceTool_H_
