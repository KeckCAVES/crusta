#include <crusta/Tool.h>

BEGIN_CRUSTA


Tool::Factory* factory = NULL;


Tool::
Tool(const Vrui::ToolFactory* factory,
     const Vrui::ToolInputAssignment& inputAssignment) :
    Vrui::Tool(factory, inputAssignment)
{
}
Tool::
~Tool()
{
}

void Tool::
init()
{
    Factory* toolFactory = new Factory("CrustaTool", "Abstract Crusta Tool",
                                       NULL, Vrui::getToolManager());
    Vrui::getToolManager()->addClass(toolFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);
}


const Vrui::ToolFactory* Tool::
getFactory() const
{
    return factory;
}


END_CRUSTA
