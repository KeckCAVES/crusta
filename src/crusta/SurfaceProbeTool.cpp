#include <crusta/SurfaceProbeTool.h>


#include <Cluster/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>


namespace crusta {


SurfaceProbeTool::Factory* SurfaceProbeTool::factory = NULL;
const Scalar SurfaceProbeTool::markerSize            = 0.2;
const Scalar SurfaceProbeTool::selectDistance        = 0.5;

SurfaceProbeTool::CallbackData::
CallbackData(SurfaceProbeTool* probe_) :
    probe(probe_)
{
}

SurfaceProbeTool::SampleCallbackData::
SampleCallbackData(SurfaceProbeTool* probe_, int sampleId_, int numSamples_,
                   const SurfacePoint& surfacePoint_) :
    CallbackData(probe_), sampleId(sampleId_), numSamples(numSamples_),
    surfacePoint(surfacePoint_)
{
}

SurfaceProbeTool::
SurfaceProbeTool(const Vrui::ToolFactory* iFactory,
                 const Vrui::ToolInputAssignment& inputAssignment) :
    Tool(iFactory, inputAssignment),
    mode(MODE_MIN_MAX), markersSet(0), markersHover(0), markersSelected(0),
    dialog(NULL)
{
    dialog = new GLMotif::PopupWindow("SurfaceProbeDialog",
        Vrui::getWidgetManager(), "Crusta Surface Probe");

    GLMotif::RowColumn* top = new GLMotif::RowColumn("SPTtop", dialog, false);
    top->setNumMinorWidgets(1);

//- create the mode selection group
    GLMotif::RadioBox* modeBox = new GLMotif::RadioBox("Sample Modes",top,false);
    modeBox->setNumMinorWidgets(2);
    modeBox->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
    modeBox->setPacking(GLMotif::RadioBox::PACK_GRID);

    GLMotif::ToggleButton* minMaxToggle =
        new GLMotif::ToggleButton("SPTminMaxMode", modeBox, "Dual");
    new GLMotif::ToggleButton("SPTshiftMode", modeBox, "Single");

    modeBox->setSelectedToggle(minMaxToggle);
    modeBox->getValueChangedCallbacks().add(
        this, &SurfaceProbeTool::modeCallback);

    modeBox->manageChild();

    top->manageChild();
}

SurfaceProbeTool::
~SurfaceProbeTool()
{
    //close the dialog
    if (dialog != NULL)
        Vrui::popdownPrimaryWidget(dialog);
}


Vrui::ToolFactory* SurfaceProbeTool::
init()
{
    Vrui::ToolFactory* crustaToolFactory = dynamic_cast<Vrui::ToolFactory*>(
            Vrui::getToolManager()->loadClass("CrustaTool"));

    Factory* surfaceFactory = new Factory(
        "CrustaSurfaceProbeTool", "Surface Probe",
        crustaToolFactory, *Vrui::getToolManager());

    surfaceFactory->setNumButtons(1);

    Vrui::getToolManager()->addClass(surfaceFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    SurfaceProbeTool::factory = surfaceFactory;

    return SurfaceProbeTool::factory;
}

Misc::CallbackList& SurfaceProbeTool::
getSampleCallbacks()
{
    return sampleCallbacks;
}

void SurfaceProbeTool::
resetMarkers()
{
    markersSet         = 0;
    markersHover       = 0;
    markersSelected    = 0;
}

void SurfaceProbeTool::
frameMinMax()
{
    const Geometry::Point<double,3>& pos = surfacePoint.position;

    if (Vrui::isMaster())
    {
        //compute selection distance
        double scaleFac   = Vrui::getNavigationTransformation().getScaling();
        double selectDist = selectDistance / scaleFac;

        switch (markersSet)
        {
            case 1:
            {
                Scalar dist = Geometry::dist(pos, markers[0]);
                if (dist < selectDist)
                    markersHover = 1;
                else
                    markersHover = 0;
                break;
            }
            case 2:
            {
                Scalar dist[2] = { Geometry::dist(pos, markers[0]),
                                   Geometry::dist(pos, markers[1]) };
                if (dist[0]<dist[1] && dist[0]<selectDist)
                    markersHover = 1;
                else if (dist[1]<dist[0] && dist[1]<selectDist)
                    markersHover = 2;
                else
                    markersHover = 0;
                break;
            }
            default:
                break;
        }

        if (Vrui::getMainPipe() != NULL)
            Vrui::getMainPipe()->write<int>(markersHover);
    }
    else
        Vrui::getMainPipe()->read<int>(markersHover);

    if (markersSelected != 0)
    {
        markers[markersSelected-1] = pos;
        callback();
    }
}

void SurfaceProbeTool::
frameShift()
{
    if (markersSelected == 1)
    {
        markers[0] = surfacePoint.position;
        callback();
    }
}


void SurfaceProbeTool::
callback()
{
    int numSamples = mode==MODE_MIN_MAX ? 2 : 1;
    SampleCallbackData cbData(this, markersSelected, numSamples, surfacePoint);
    sampleCallbacks.call(&cbData);
}


void SurfaceProbeTool::
modeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
{
    resetMarkers();
    if (strcmp(cbData->newSelectedToggle->getName(), "SPTminMaxMode") == 0)
        mode = MODE_MIN_MAX;
    else if (strcmp(cbData->newSelectedToggle->getName(), "SPTshiftMode") == 0)
        mode = MODE_SHIFT;
}


const Vrui::ToolFactory* SurfaceProbeTool::
getFactory() const
{
    return factory;
}

void SurfaceProbeTool::
frame()
{
    //project the device position
    surfacePoint = project(getButtonDevice(0), false);

    //no updates if the projection failed
    if (projectionFailed)
        return;

    switch (mode)
    {
        case MODE_MIN_MAX:
            frameMinMax();
            break;

        case MODE_SHIFT:
            frameShift();
            break;

        default:
            break;
    }
}

void SurfaceProbeTool::
display(GLContextData& contextData) const
{
//- render the surface projector stuff
    //transform the position back to physical space
    const Vrui::NavTransform& navXform = Vrui::getNavigationTransformation();
    Geometry::Point<double,3> position = navXform.transform(surfacePoint.position);

    Vrui::NavTransform devXform = getButtonDevice(0)->getTransformation();
    Vrui::NavTransform projXform(Geometry::Vector<double,3>(position), devXform.getRotation(),
                                 devXform.getScaling());

    SurfaceProjector::display(contextData, projXform, devXform);

//- render own stuff
    CHECK_GLA
    //don't draw anything if we don't have any control points yet
    if (markersSet == 0)
        return;

    //save relevant GL state and set state for marker rendering
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);

    //go to navigational coordinates
    std::vector<Geometry::Point<double,3> > markerPos(markersSet);
    for (int i=0; i<markersSet; ++i)
        markerPos[i] = crusta->mapToScaledGlobe(markers[i]);

    Geometry::Vector<double,3> centroid(0.0, 0.0, 0.0);
    for (int i=0; i<markersSet; ++i)
        centroid += Geometry::Vector<double,3>(markerPos[i]);
    centroid /= markersSet;

    //load the centroid relative translated navigation transformation
    glPushMatrix();

    Vrui::NavTransform nav =
        Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroid);
    glLoadMatrix(nav);

    //compute marker size
    Scalar scaleFac = Vrui::getNavigationTransformation().getScaling();
    Scalar size     = markerSize/scaleFac;

    CHECK_GLA
    //draw the control points
    for (int i=0; i<markersSet; ++i)
    {
        if (markersHover == i+1)
        {
            glColor3f(0.3f, 0.9f, 0.5f);
            glLineWidth(2.0f);
        }
        else
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(1.0f);
        }

        markerPos[i] = Geometry::Point<double,3>(Geometry::Vector<double,3>(markerPos[i])-centroid);
        glBegin(GL_LINES);
            glVertex3f(markerPos[i][0]-size, markerPos[i][1], markerPos[i][2]);
            glVertex3f(markerPos[i][0]+size, markerPos[i][1], markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1]-size, markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1]+size, markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1], markerPos[i][2]-size);
            glVertex3f(markerPos[i][0], markerPos[i][1], markerPos[i][2]+size);
        glEnd();

        CHECK_GLA
    }

    //restore coordinate system
    glPopMatrix();
    //restore GL state
    glPopAttrib();
    CHECK_GLA
}


void SurfaceProbeTool::
buttonCallback(int, Vrui::InputDevice::ButtonCallbackData* cbData)
{
    //project the device position
    surfacePoint = project(getButtonDevice(0), false);

    //disable any button callback if the projection has failed.
    if (projectionFailed)
        return;

    switch (mode)
    {
        case MODE_MIN_MAX:
        {
            if (cbData->newButtonState)
            {
                switch (markersSet)
                {
                    case 0:
                    {
                        markers[0]      = surfacePoint.position;
                        markersSet      = 1;
                        markersHover    = 1;
                        markersSelected = 1;
                    } break;

                    case 1:
                    {
                        markers[1]      = surfacePoint.position;
                        markersSet      = 2;
                        markersHover    = 2;
                        markersSelected = 2;
                    } break;

                    case 2:
                    {
                        if (markersHover == 0)
                        {
                            //resetting marker placement
                            markers[0]      = surfacePoint.position;
                            markersSet      = 1;
                            markersHover    = 1;
                            markersSelected = 1;
                        }
                        else
                        {
                            //modifying existing marker position
                            markers[markersHover-1] = surfacePoint.position;
                            markersSelected         = markersHover;
                        }
                    } break;

                    default:
                        break;
                }
                callback();
            }
            else
                markersSelected = 0;

            break;
        }

        case MODE_SHIFT:
        {
            if (cbData->newButtonState)
            {
                markersSet = markersSelected = 1;
                markers[0] = surfacePoint.position;
                callback();
            }
            else
                markersSelected = 0;
        }
    }
}

void SurfaceProbeTool::
setupComponent(Crusta* crusta)
{
    //base setup
    SurfaceProjector::setupComponent(crusta);

    //popup the dialog
    const Vrui::NavTransform& navXform = Vrui::getNavigationTransformation();
    Vrui::popupPrimaryWidget(dialog,
        navXform.transform(Vrui::getDisplayCenter()));
}


} //namespace crusta
