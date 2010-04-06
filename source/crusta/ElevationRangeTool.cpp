#include <crusta/ElevationRangeTool.h>

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


ElevationRangeTool::Factory* ElevationRangeTool::factory = NULL;
const Scalar ElevationRangeTool::markerSize              = 0.2;
const Scalar ElevationRangeTool::selectDistance          = 0.5;

ElevationRangeTool::ChangeCallbackData::
ChangeCallbackData(ElevationRangeTool* iTool, Scalar iMin, Scalar iMax) :
    tool(iTool), min(iMin), max(iMax)
{
}


ElevationRangeTool::
ElevationRangeTool(const Vrui::ToolFactory* iFactory,
                   const Vrui::ToolInputAssignment& inputAssignment) :
    Tool(iFactory, inputAssignment),
    controlPointsSet(0), controlPointsHover(0), controlPointsSelected(0)
{
}

ElevationRangeTool::
~ElevationRangeTool()
{
}


Misc::CallbackList& ElevationRangeTool::
getChangeCallbacks()
{
    return changeCallbacks;
}


Vrui::ToolFactory* ElevationRangeTool::
init(Vrui::ToolFactory* parent)
{
    Factory* erFactory = new Factory("ElevationRangeTool",
        "Elevation Range Tool", parent, *Vrui::getToolManager());

    erFactory->setNumDevices(1);
    erFactory->setNumButtons(0, 1);

    Vrui::getToolManager()->addClass(erFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    ElevationRangeTool::factory = erFactory;

    return ElevationRangeTool::factory;
}


const Vrui::ToolFactory* ElevationRangeTool::
getFactory() const
{
    return factory;
}

void ElevationRangeTool::
frame()
{
    //grab the current tool position and map it to the unscaled globe
    Point3 pos = getPosition();
    pos        = crusta->mapToUnscaledGlobe(pos);

    //compute selection distance
    Scalar scaleFac   = Vrui::getNavigationTransformation().getScaling();
    Scalar selectDist = selectDistance / scaleFac;

    switch (controlPointsSet)
    {
        case 1:
        {
            Scalar dist = Geometry::dist(pos, ends[0]);
            if (dist < selectDist)
                controlPointsHover = 1;
            else
                controlPointsHover = 0;
            break;
        }
        case 2:
        {
            Scalar dist[2] = { Geometry::dist(pos, ends[0]),
                               Geometry::dist(pos, ends[1]) };
            if (dist[0]<dist[1] && dist[0]<selectDist)
                controlPointsHover = 1;
            else if (dist[1]<dist[0] && dist[1]<selectDist)
                controlPointsHover = 2;
            else
                controlPointsHover = 0;
            break;
        }
        default:
            break;
    }

    if (controlPointsSelected != 0)
    {
        ends[controlPointsSelected-1] = pos;
        if (controlPointsSet == 2)
            notifyChange();
    }
}

void ElevationRangeTool::
display(GLContextData& contextData) const
{
    CHECK_GLA
    //don't draw anything if we don't have any control points yet
    if (controlPointsSet == 0)
        return;

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
    //draw the control points
    for (int i=0; i<controlPointsSet; ++i)
    {
        if (controlPointsHover == i+1)
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
            glVertex3f(ends[i][0]-size, ends[i][1],      ends[i][2]);
            glVertex3f(ends[i][0]+size, ends[i][1],      ends[i][2]);
            glVertex3f(ends[i][0],      ends[i][1]-size, ends[i][2]);
            glVertex3f(ends[i][0],      ends[i][1]+size, ends[i][2]);
            glVertex3f(ends[i][0],      ends[i][1],      ends[i][2]-size);
            glVertex3f(ends[i][0],      ends[i][1],      ends[i][2]+size);
        glEnd();
        CHECK_GLA
    }

    //restore coordinate system
    glPopMatrix();
    //restore GL state
    glPopAttrib();
    CHECK_GLA
}

void ElevationRangeTool::
buttonCallback(int deviceIndex, int buttonIndex,
               Vrui::InputDevice::ButtonCallbackData* cbData)
{
    //get the current position
    Point3 pos = getPosition();
    pos        = crusta->mapToUnscaledGlobe(pos);

    if (cbData->newButtonState)
    {
        switch (controlPointsSet)
        {
            case 0:
                ends[0]               = pos;
                controlPointsSet      = 1;
                controlPointsHover    = 1;
                controlPointsSelected = 1;
                break;
            case 1:
                ends[1]               = pos;
                controlPointsSet      = 2;
                controlPointsHover    = 2;
                controlPointsSelected = 2;
                break;
            case 2:
                ends[controlPointsHover-1] = pos;
                controlPointsSelected      = controlPointsHover;
                break;
            default:
                break;
        }
    }
    else
        controlPointsSelected = 0;
}


Point3 ElevationRangeTool::
getPosition()
{
    Vrui::NavTrackerState nav = Vrui::getDeviceTransformation(getDevice(0));
    Vrui::NavTrackerState::Vector trans = nav.getTranslation();
    return Point3(trans[0], trans[1], trans[2]);
}

void ElevationRangeTool::
notifyChange()
{
    assert(controlPointsSet == 2);
    Scalar min = Vector3(ends[0]).mag() - SPHEROID_RADIUS;
    Scalar max = Vector3(ends[1]).mag() - SPHEROID_RADIUS;

    ChangeCallbackData cb(this, min, max);
    changeCallbacks.call(&cb);
}


END_CRUSTA
