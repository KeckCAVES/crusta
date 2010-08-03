#include <crusta/DebugTool.h>

#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

BEGIN_CRUSTA


DebugTool::Factory* DebugTool::factory = NULL;

bool DebugTool::bogusButton = false;


DebugTool::
DebugTool(const Vrui::ToolFactory* factory,
          const Vrui::ToolInputAssignment& inputAssignment) :
          Vrui::Tool(factory, inputAssignment), CrustaComponent(NULL)
{
    for (int i=0; i<numButtons; ++i)
    {
        buttons[i]     = false;
        buttonsLast[i] = false;
    }
}

DebugTool::
~DebugTool()
{
}

void DebugTool::
init()
{
    Factory* toolFactory = new Factory("DebugTool", "Debug Tool", NULL,
                                       *Vrui::getToolManager());

    toolFactory->setNumDevices(1);
    toolFactory->setNumButtons(0, numButtons);
    toolFactory->setNumValuators(0, 0);

    Vrui::getToolManager()->addClass(toolFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    DebugTool::factory = toolFactory;
}


int DebugTool::
getNumButtons()
{
    return numButtons;
}

bool DebugTool::
getButton(int i)
{
    return i<numButtons ? buttons[i] : false;
}

bool& DebugTool::
getButtonLast(int i)
{
    return i<numButtons ? buttonsLast[i] : bogusButton;
}


const Vrui::ToolFactory* DebugTool::
getFactory() const
{
    return factory;
}

void DebugTool::
buttonCallback(int, int deviceButtonIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
    buttons[deviceButtonIndex] = cbData->newButtonState;
}


END_CRUSTA
