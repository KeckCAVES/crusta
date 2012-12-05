#include <crusta/Tool.h>

#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

BEGIN_CRUSTA


Tool::Factory* Tool::factory = NULL;


Tool::
Tool(const Vrui::ToolFactory* factory,
     const Vrui::ToolInputAssignment& inputAssignment) :
     Vrui::Tool(factory, inputAssignment), CrustaComponent(NULL)
{
}
Tool::
~Tool()
{
}

Vrui::ToolFactory* Tool::
init(Vrui::ToolFactory* parent)
{
    Factory* toolFactory = new Factory("CrustaTool", "Crusta Tool", parent,
                                       *Vrui::getToolManager());
    Vrui::getToolManager()->addClass(toolFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    return toolFactory;
}


const Vrui::ToolFactory* Tool::
getFactory() const
{
    return factory;
}


END_CRUSTA
