#ifndef _SurfaceTool_H_
#define _SurfaceTool_H_

#include <Vrui/Tools/TransformTool.h>

#include <crusta/basics.h>


BEGIN_CRUSTA


class SurfaceTool : public Vrui::TransformTool
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
    virtual const Vrui::ToolFacotry* getFactory() const;
    virtual void frame();
}

END_CRUSTA


#endif //_SurfaceTool_H_
