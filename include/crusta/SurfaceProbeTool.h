#ifndef _Crusta_SurfaceProbeTool_H_
#define _Crusta_SurfaceProbeTool_H_


#include <Vrui/Tools/GenericToolFactory.h>
#include <Vrui/Tools/TransformTool.h>

#include <crusta/CrustaComponent.h>


BEGIN_CRUSTA


class SurfaceProbeTool : public Vrui::TransformTool, public CrustaComponent
{
    friend class Vrui::GenericToolFactory<SurfaceProbeTool>;

public:
    typedef Vrui::GenericToolFactory<SurfaceProbeTool> Factory;

    SurfaceProbeTool(const Vrui::ToolFactory* iFactory,
                const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~SurfaceProbeTool();

    static Vrui::ToolFactory* init();

private:
    static Factory* factory;
    bool projectionFailed;

//- Inherited from Vrui::Tool
public:
    virtual void initialize();
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int deviceIndex, int deviceButtonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
};

END_CRUSTA


#endif //_Crusta_SurfaceProbeTool_H_
