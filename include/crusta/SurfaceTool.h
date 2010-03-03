#ifndef _SurfaceTool_H_
#define _SurfaceTool_H_


#include <Vrui/Tools/GenericToolFactory.h>
#include <Vrui/Tools/TransformTool.h>

#include <crusta/CrustaComponent.h>


BEGIN_CRUSTA


class SurfaceTool : public Vrui::TransformTool, public CrustaComponent
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
    bool projectionFailed;

//- Inherited from Vrui::Tool
public:
    virtual void initialize();
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;
};

END_CRUSTA


#endif //_SurfaceTool_H_
