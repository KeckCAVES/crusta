#include <crusta/DebugTool.h>


#include <iostream>

#include <crusta/Crusta.h>
#include <crusta/SurfacePoint.h>

#include <Geometry/OrthogonalTransformation.h>
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

    toolFactory->setNumButtons(numButtons);

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
frame()
{
    if (!buttons[0])
        return;

    Vrui::InputDevice* device = getButtonDevice(0);

    //transform the physical frame to navigation space
    Vrui::NavTransform physicalFrame = device->getTransformation();
    Vrui::NavTransform modelFrame    =
        Vrui::getInverseNavigationTransformation() * physicalFrame;

    //align the model frame to the surface
    SurfacePoint surfacePoint;
    if (Vrui::isMaster())
    {
        Vrui::Vector rayDir = device->getRayDirection();
        rayDir = Vrui::getInverseNavigationTransformation().transform(
            rayDir);
        rayDir.normalize();

        Ray ray(modelFrame.getOrigin(), rayDir);
        surfacePoint = crusta->intersect(ray);

        if (surfacePoint.isValid())
        {
            std::cerr << "Node: " << surfacePoint.nodeIndex.med_str() << "   "<<\
                surfacePoint.cellIndex[0] << "x" << surfacePoint.cellIndex[1] <<
                "   " << surfacePoint.cellPosition[0] << "|" <<
                surfacePoint.cellPosition[1] << "   index " <<
                surfacePoint.nodeIndex.index() << "\n";
        }
    }
}

void DebugTool::
buttonCallback(int buttonSlotIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
    buttons[buttonSlotIndex] = cbData->newButtonState;
}


END_CRUSTA
