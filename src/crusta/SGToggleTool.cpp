#include <crusta/SGToggleTool.h>
#include <crusta/CrustaSettings.h>

namespace crusta {

SGToggleTool::Factory* SGToggleTool::factory = NULL;

SGToggleTool::SGToggleTool(
  const Vrui::ToolFactory* factory,
  const Vrui::ToolInputAssignment& inputAssignment)
  : Vrui::Tool(factory, inputAssignment)
{}

SGToggleTool::~SGToggleTool()
{}

Vrui::ToolFactory* SGToggleTool::init()
{
  Factory* factory = new Factory(
    "CrustaSGToggleTool",
    "Scene Graph Visibility Toggler",
    dynamic_cast<Vrui::ToolFactory*>(Vrui::getToolManager()->loadClass("CrustaTool")),
    *Vrui::getToolManager());
  factory->setNumButtons(1);
  factory->setButtonFunction(0, "Toggle scene graph visibility");
  Vrui::getToolManager()->addClass(factory, Vrui::ToolManager::defaultToolFactoryDestructor);
  SGToggleTool::factory = factory;
  return factory;
}

void SGToggleTool::buttonCallback(int, Vrui::InputDevice::ButtonCallbackData* cbData)
{
	if(!cbData->newButtonState) return;
  SETTINGS->sceneGraphViewerEnabled = !SETTINGS->sceneGraphViewerEnabled;
}

const Vrui::ToolFactory* SGToggleTool::getFactory() const
{
  return factory;
}

} //namespace crusta
