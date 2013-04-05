#include <crusta/LayerToggleTool.h>
#include <crusta/ColorMapper.h>

namespace crusta {

LayerToggleTool::Factory* LayerToggleTool::factory = NULL;

LayerToggleTool::LayerToggleTool(
  const Vrui::ToolFactory* factory,
  const Vrui::ToolInputAssignment& inputAssignment)
  : Vrui::Tool(factory, inputAssignment),
    state(true)
{}

LayerToggleTool::~LayerToggleTool()
{}

Vrui::ToolFactory* LayerToggleTool::init()
{
  Factory* factory = new Factory(
    "CrustaLayerToggleTool",
    "Layer Visibility Toggler",
    dynamic_cast<Vrui::ToolFactory*>(Vrui::getToolManager()->loadClass("CrustaTool")),
    *Vrui::getToolManager());
  factory->setNumButtons(1);
  factory->setButtonFunction(0, "Toggle layer visibility");
  Vrui::getToolManager()->addClass(factory, Vrui::ToolManager::defaultToolFactoryDestructor);
  LayerToggleTool::factory = factory;
  return factory;
}

void LayerToggleTool::buttonCallback(int, Vrui::InputDevice::ButtonCallbackData* cbData)
{
	if(!cbData->newButtonState) return;
  state = !state;
  COLORMAPPER->setVisibleAll(state);
}

const Vrui::ToolFactory* LayerToggleTool::getFactory() const
{
  return factory;
}

} //namespace crusta
