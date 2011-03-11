#include <crusta/SliceTool.h>


#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/StyleSheet.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/Crusta.h>


BEGIN_CRUSTA


SliceTool::Factory* SliceTool::factory = NULL;
const Scalar SliceTool::markerSize            = 0.2;
const Scalar SliceTool::selectDistance        = 0.5;
SliceTool::SliceParameters SliceTool::_sliceParameters;

SliceTool::CallbackData::
CallbackData(SliceTool* probe_) :
    probe(probe_)
{
}

SliceTool::SampleCallbackData::
SampleCallbackData(SliceTool* probe_, int sampleId_, int numSamples_,
                   const SurfacePoint& surfacePoint_) :
    CallbackData(probe_), sampleId(sampleId_), numSamples(numSamples_),
    surfacePoint(surfacePoint_)
{
}

SliceTool::
SliceTool(const Vrui::ToolFactory* iFactory,
                 const Vrui::ToolInputAssignment& inputAssignment) :
    Tool(iFactory, inputAssignment), markersSet(0), markersHover(0), markersSelected(0),
    dialog(NULL)
{
    const GLMotif::StyleSheet *style = Vrui::getWidgetManager()->getStyleSheet();


    dialog = new GLMotif::PopupWindow("SliceDialog",
        Vrui::getWidgetManager(), "Crusta Slice Tool");

    GLMotif::RowColumn* top = new GLMotif::RowColumn("SPTtop", dialog, false);
    top->setNumMinorWidgets(2);

    new GLMotif::Label("Angle", top, "Displacement angle");
    GLMotif::Slider *angleSlider = new GLMotif::Slider("DipSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    angleSlider->setValueRange(0, 360.0, 1.0);
    angleSlider->setValue(0.0);
    angleSlider->getValueChangedCallbacks().add(this, &SliceTool::angleSliderCallback);



    new GLMotif::Label("DisplacementLabel", top, "Displacement magnitude");
    GLMotif::Slider *displacementSlider = new GLMotif::Slider("displacementSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    displacementSlider->setValueRange(0, 1.0, 0.01);
    displacementSlider->setValue(0.0);
    displacementSlider->getValueChangedCallbacks().add(this, &SliceTool::displacementSliderCallback);

    new GLMotif::Label("SlopeAngleLabel", top, "Slope angle");
    GLMotif::Slider *slopeAngleSlider = new GLMotif::Slider("slopeAngleSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    slopeAngleSlider->setValueRange(0.0 + 15.0, 180.0 - 15.0, 1.0);
    slopeAngleSlider->setValue(45.0);
    slopeAngleSlider->getValueChangedCallbacks().add(this, &SliceTool::slopeAngleSliderCallback);

    new GLMotif::Label("FalloffLabel", top, "Falloff");
    GLMotif::Slider *falloffSlider = new GLMotif::Slider("falloffSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    falloffSlider->setValueRange(0.0, 5.0, 0.025);
    falloffSlider->setValue(1.0);
    falloffSlider->getValueChangedCallbacks().add(this, &SliceTool::falloffSliderCallback);

    top->manageChild();
}
void SliceTool::angleSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.angle = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    Vrui::requestUpdate();
}

void SliceTool::displacementSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.displacementAmount = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    Vrui::requestUpdate();
}
void SliceTool::slopeAngleSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.slopeAngleDegrees = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    Vrui::requestUpdate();
}

void SliceTool::falloffSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.falloffFactor = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    Vrui::requestUpdate();
}

SliceTool::
~SliceTool()
{
    //close the dialog
    if (dialog != NULL)
        Vrui::popdownPrimaryWidget(dialog);
}


Vrui::ToolFactory* SliceTool::
init()
{
    _sliceParameters.faultLine[0] = Point3(6371000,0,0);
    _sliceParameters.faultLine[1] = Point3(0,6371000,0);

    _sliceParameters.angle = 0.0;
    _sliceParameters.displacementAmount = 0.0;
    _sliceParameters.slopeAngleDegrees = 0.0;
    _sliceParameters.falloffFactor = 1.0;
    _sliceParameters.updatePlaneParameters();


    Vrui::ToolFactory* crustaToolFactory = dynamic_cast<Vrui::ToolFactory*>(
            Vrui::getToolManager()->loadClass("CrustaTool"));

    Factory* surfaceFactory = new Factory(
        "CrustaSliceTool", "Slice Tool",
        crustaToolFactory, *Vrui::getToolManager());

    surfaceFactory->setNumDevices(1);
    surfaceFactory->setNumButtons(0, 1);

    Vrui::getToolManager()->addClass(surfaceFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    SliceTool::factory = surfaceFactory;

    return SliceTool::factory;
}

void SliceTool::
resetMarkers()
{
    markersSet         = 0;
    markersHover       = 0;
    markersSelected    = 0;
}

void SliceTool::
callback()
{
    std::cout << "callback" << std::endl;
}

const Vrui::ToolFactory* SliceTool::
getFactory() const
{
    return factory;
}

void SliceTool::
frame()
{
    //project the device position
    surfacePoint = project(input.getDevice(0), false);

    //no updates if the projection failed
    if (projectionFailed)
        return;

    const Point3& pos = surfacePoint.position;

    if (Vrui::isMaster())
    {
        //compute selection distance
        double scaleFac   = Vrui::getNavigationTransformation().getScaling();
        double selectDist = selectDistance / scaleFac;

        switch (markersSet)
        {
            case 1:
            {
                Scalar dist = Geometry::dist(pos, _sliceParameters.faultLine[0]);
                if (dist < selectDist)
                    markersHover = 1;
                else
                    markersHover = 0;
                break;
            }
            case 2:
            {
                Scalar dist[2] = { Geometry::dist(pos, _sliceParameters.faultLine[0]),
                                   Geometry::dist(pos, _sliceParameters.faultLine[1]) };
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
        _sliceParameters.faultLine[markersSelected-1] = pos;
        _sliceParameters.updatePlaneParameters();
        callback();
    }
}

void SliceTool::
display(GLContextData& contextData) const
{
//- render the surface projector stuff
    //transform the position back to physical space
    const Vrui::NavTransform& navXform = Vrui::getNavigationTransformation();
    Point3 position = navXform.transform(surfacePoint.position);

    Vrui::NavTransform devXform = input.getDevice(0)->getTransformation();
    Vrui::NavTransform projXform(Vector3(position), devXform.getRotation(),
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
    std::vector<Point3> markerPos(markersSet);
    for (int i=0; i<markersSet; ++i)
        markerPos[i] = crusta->mapToScaledGlobe(_sliceParameters.faultLine[i]);

    Vector3 centroid(0.0, 0.0, 0.0);
    for (int i=0; i<markersSet; ++i)
        centroid += Vector3(markerPos[i]);
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

        markerPos[i] = Point3(Vector3(markerPos[i])-centroid);
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


void SliceTool::
buttonCallback(int, int, Vrui::InputDevice::ButtonCallbackData* cbData)
{
    //project the device position
    surfacePoint = project(input.getDevice(0), false);

    //disable any button callback if the projection has failed.
    if (projectionFailed)
        return;

    if (cbData->newButtonState)
    {
        switch (markersSet)
        {
        case 0:
            {
                _sliceParameters.faultLine[0]      = surfacePoint.position;
                markersSet      = 1;
                markersHover    = 1;
                markersSelected = 1;
            } break;

        case 1:
            {
            _sliceParameters.faultLine[1]      = surfacePoint.position;
            markersSet      = 2;
            markersHover    = 2;
            markersSelected = 2;
        } break;

        case 2:
            {
        if (markersHover == 0)
        {
            //resetting marker placement
            _sliceParameters.faultLine[0]      = surfacePoint.position;
            markersSet      = 1;
            markersHover    = 1;
            markersSelected = 1;
        }
        else
        {
            //modifying existing marker position
            _sliceParameters.faultLine[markersHover-1] = surfacePoint.position;
            markersSelected         = markersHover;
        }
    } break;

        default:
            break;
        }
        _sliceParameters.updatePlaneParameters();
        callback();
    }
    else
        markersSelected = 0;

}

void SliceTool::
setupComponent(Crusta* crusta)
{
    //base setup
    SurfaceProjector::setupComponent(crusta);

    //popup the dialog
    const Vrui::NavTransform& navXform = Vrui::getNavigationTransformation();
    Vrui::popupPrimaryWidget(dialog,
        navXform.transform(Vrui::getDisplayCenter()));
}


const SliceTool::SliceParameters &SliceTool::getParameters() {
    return _sliceParameters;
}

void SliceTool::SliceParameters::updatePlaneParameters() {
    Vector3 pA(faultLine[0]);
    Vector3 pB(faultLine[1]);

    // half of strike vector
    planeStrikeDirection = (pB - pA) * 0.5;
    // midpoint of fault line / plane

    // radius vector (center of planet to midpoint of fault line) = normal (because planet center is at (0,0,0))
    Vector3 n((pA + pB) * 0.5);

    // dip vector
    planeDipDirection = cross(planeStrikeDirection, n);
    planeDipDirection.normalize();

    // scale to same length as strike vector (for starters)
    planeDipDirection *=2 * planeStrikeDirection.mag();
    // rotate around strike vector
    planeDipDirection = Vrui::Rotation::rotateAxis(planeStrikeDirection, slopeAngleDegrees / 360.0 * 2 * M_PI).transform(planeDipDirection);



    planeNormal = normalize(cross(planeStrikeDirection,planeDipDirection)); // recalculate normal
    //double planeD = (pA - center) * n;  // plane distance from origin


}

double SliceTool::SliceParameters::getPlaneDistanceFrom(Vector3 p) const {
    return (faultLine[0] - p) * planeNormal;
}

Vector3 SliceTool::SliceParameters::getPlaneCenter() const {
    return getPlaneDistanceFrom(Vector3(0,0,0)) * planeNormal;
}

Vector3 SliceTool::SliceParameters::getShiftVector() const {
    double strikeComponent = cos(angle / 360.0 * 2 * M_PI);
    double dipComponent = sin(angle / 360.0 * 2 * M_PI);

    return Vector3(displacementAmount * strikeComponent * planeStrikeDirection +
                   displacementAmount * dipComponent * planeDipDirection);
}

Vector3 SliceTool::SliceParameters::getFaultCenter() const {
    return 0.5 * (Vector3(faultLine[0]) + Vector3(faultLine[1]));
}

double SliceTool::SliceParameters::getFalloff() const {
    return falloffFactor * (faultLine[0] - faultLine[1]).mag();
}

END_CRUSTA
