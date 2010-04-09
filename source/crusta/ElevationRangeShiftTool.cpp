#include <crusta/ElevationRangeShiftTool.h>

#include <cassert>

#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>

BEGIN_CRUSTA


ElevationRangeShiftTool::Factory* ElevationRangeShiftTool::factory = NULL;
const Scalar ElevationRangeShiftTool::markerSize                   = 0.2;

ElevationRangeShiftTool::ChangeCallbackData::
ChangeCallbackData(ElevationRangeShiftTool* iTool, Scalar iHeight) :
    tool(iTool), height(iHeight)
{
}


ElevationRangeShiftTool::
ElevationRangeShiftTool(const Vrui::ToolFactory* iFactory,
                   const Vrui::ToolInputAssignment& inputAssignment) :
    Tool(iFactory, inputAssignment), isDragging(false)
{
}

ElevationRangeShiftTool::
~ElevationRangeShiftTool()
{
}


Misc::CallbackList& ElevationRangeShiftTool::
getChangeCallbacks()
{
    return changeCallbacks;
}


Vrui::ToolFactory* ElevationRangeShiftTool::
init(Vrui::ToolFactory* parent)
{
    Factory* ersFactory = new Factory("ElevationRangeShiftTool",
        "Shift Elevation Range Tool", parent, *Vrui::getToolManager());

    ersFactory->setNumDevices(1);
    ersFactory->setNumButtons(0, 1);

    Vrui::getToolManager()->addClass(ersFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    ElevationRangeShiftTool::factory = ersFactory;

    return ElevationRangeShiftTool::factory;
}


const Vrui::ToolFactory* ElevationRangeShiftTool::
getFactory() const
{
    return factory;
}

void ElevationRangeShiftTool::
frame()
{
    //grab the current tool position and map it to the unscaled globe
    Point3 pos = getPosition();
    pos        = crusta->mapToUnscaledGlobe(pos);

    if (isDragging)
    {
        curPos = pos;
        notifyChange();
    }
}

void ElevationRangeShiftTool::
display(GLContextData& contextData) const
{
    CHECK_GLA
    //save relevant GL state and set state for marker rendering
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);

    glDisable(GL_LIGHTING);

    //go to navigational coordinates
    glPushMatrix();
    glLoadMatrix(Vrui::getDisplayState(contextData).modelviewNavigational);

    //compute marker size
    Scalar scaleFac = Vrui::getNavigationTransformation().getScaling();
    Scalar size     = markerSize/scaleFac;

    CHECK_GLA
    //draw the marker
    if (isDragging)
    {
        glColor3f(0.3f, 0.9f, 0.5f);
        glLineWidth(2.0f);
    }
    else
    {
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(1.0f);
    }

    glBegin(GL_LINES);
        glVertex3f(curPos[0]-size, curPos[1],      curPos[2]);
        glVertex3f(curPos[0]+size, curPos[1],      curPos[2]);
        glVertex3f(curPos[0],      curPos[1]-size, curPos[2]);
        glVertex3f(curPos[0],      curPos[1]+size, curPos[2]);
        glVertex3f(curPos[0],      curPos[1],      curPos[2]-size);
        glVertex3f(curPos[0],      curPos[1],      curPos[2]+size);
    glEnd();
    CHECK_GLA

    //restore coordinate system
    glPopMatrix();
    //restore GL state
    glPopAttrib();
    CHECK_GLA
}

void ElevationRangeShiftTool::
buttonCallback(int deviceIndex, int buttonIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
    //get the current position
    Point3 pos = getPosition();
    pos        = crusta->mapToUnscaledGlobe(pos);

    if (cbData->newButtonState)
    {
        isDragging = true;
        curPos = pos;
        notifyChange();
    }
    else
        isDragging = false;
}


Point3 ElevationRangeShiftTool::
getPosition()
{
    Vrui::NavTrackerState nav = Vrui::getDeviceTransformation(getDevice(0));
    Vrui::NavTrackerState::Vector trans = nav.getTranslation();
    return Point3(trans[0], trans[1], trans[2]);
}

void ElevationRangeShiftTool::
notifyChange()
{
    Scalar height = Vector3(curPos).mag() - SPHEROID_RADIUS;

    ChangeCallbackData cb(this, height);
    changeCallbacks.call(&cb);
}


END_CRUSTA
