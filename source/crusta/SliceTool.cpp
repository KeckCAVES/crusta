#include <crusta/SliceTool.h>
#include <crusta/CrustaSettings.h>

#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/StyleSheet.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <limits>

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
    Tool(iFactory, inputAssignment), markersHover(0), markersSelected(0),
    dialog(NULL)
{
    const GLMotif::StyleSheet *style = Vrui::getWidgetManager()->getStyleSheet();


    dialog = new GLMotif::PopupWindow("SliceDialog",
        Vrui::getWidgetManager(), "Crusta Slice Tool");

    GLMotif::RowColumn* top = new GLMotif::RowColumn("SPTtop", dialog, false);
    top->setNumMinorWidgets(3);
    top->setColumnWeight(1, 1.0);

    new GLMotif::Label("StrikeAmount", top, "Strike-slip");
    GLMotif::RelativeSlider *strikeAmountSlider = new GLMotif::RelativeSlider("StrikeAmountSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    strikeAmountSlider->setValue(0.0);
    strikeAmountSlider->setValueRange(-2.0, 2.0, 0.01);
    strikeAmountSlider->getValueCallbacks().add(this, &SliceTool::strikeAmountSliderCallback);

    strikeAmountTextField = new GLMotif::TextField("strikeAmountTextField", top, 5);
    strikeAmountTextField->setFloatFormat(GLMotif::TextField::FIXED);
    strikeAmountTextField->setFieldWidth(6);
    strikeAmountTextField->setPrecision(3);

    new GLMotif::Label("SlopeAngleLabel", top, "Fault dip");
    GLMotif::Slider *slopeAngleSlider = new GLMotif::Slider("slopeAngleSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    slopeAngleSlider->setValue(90.0);
    slopeAngleSlider->setValueRange(0.0, 180.0, 1.0);
    slopeAngleSlider->getValueChangedCallbacks().add(this, &SliceTool::slopeAngleSliderCallback);

    slopeAngleTextField = new GLMotif::TextField("slopeAmountextField", top, 10);
    slopeAngleTextField->setFloatFormat(GLMotif::TextField::FIXED);
    slopeAngleTextField->setFieldWidth(3);
    slopeAngleTextField->setPrecision(0);

    new GLMotif::Label("dipAmountLabel", top, "Dip-slip");
    GLMotif::RelativeSlider *dipAmountSlider = new GLMotif::RelativeSlider("dipAmountSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    dipAmountSlider->setValue(0.0);
    dipAmountSlider->setValueRange(-2.0, 2.0, 0.01);
    dipAmountSlider->getValueCallbacks().add(this, &SliceTool::dipAmountSliderCallback);

    dipAmountTextField = new GLMotif::TextField("dipAmountextField", top, 5);
    dipAmountTextField->setFloatFormat(GLMotif::TextField::FIXED);
    dipAmountTextField->setFieldWidth(6);
    dipAmountTextField->setPrecision(3);


    new GLMotif::Label("FalloffLabel", top, "Falloff");
    GLMotif::Slider *falloffSlider = new GLMotif::Slider("falloffSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    falloffSlider->setValue(1.0);
    falloffSlider->setValueRange(0.1, 200.0, 0.1);
    falloffSlider->getValueChangedCallbacks().add(this, &SliceTool::falloffSliderCallback);

    GLMotif::ToggleButton *showFaultLinesButton = new GLMotif::ToggleButton("ShowFaultLinesButton", top, "Show Lines");
    showFaultLinesButton->setToggle(true);
    showFaultLinesButton->getValueChangedCallbacks().add(this, &SliceTool::showFaultLinesButtonCallback);

    new GLMotif::Label("coloringLabel", top, "Coloring");
    GLMotif::Slider *coloringSlider = new GLMotif::Slider("coloringSlider", top, GLMotif::Slider::HORIZONTAL, style->fontHeight*10.0f);
    coloringSlider->setValue(0.0);
    coloringSlider->setValueRange(0, 0.1, 0.001);
    coloringSlider->getValueChangedCallbacks().add(this, &SliceTool::coloringSliderCallback);

    GLMotif::Button *resetButton = new GLMotif::Button("ResetButton", top, "Reset");
    resetButton->getSelectCallbacks().add(this, &SliceTool::resetButtonCallback);

    top->manageChild();

    updateTextFields();
    _sliceParameters.updatePlaneParameters();
}


void SliceTool::resetButtonCallback(GLMotif::Button::SelectCallbackData *) {
    _sliceParameters.controlPoints.clear();
    markersHover    = 0;
    markersSelected = 0;
    _sliceParameters.updatePlaneParameters();
}

void SliceTool::updateTextFields() {
     //displacementTextField->setValue(_sliceParameters.getShiftVector().mag());
     strikeAmountTextField->setValue(_sliceParameters.strikeAmount);
     dipAmountTextField->setValue(_sliceParameters.dipAmount);
     slopeAngleTextField->setValue(_sliceParameters.slopeAngleDegrees);
}

void SliceTool::strikeAmountSliderCallback(GLMotif::RelativeSlider::ValueCallbackData* cbData)
{
    _sliceParameters.strikeAmount += Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    updateTextFields();
    Vrui::requestUpdate();
}

void SliceTool::dipAmountSliderCallback(GLMotif::RelativeSlider::ValueCallbackData* cbData)
{
    _sliceParameters.dipAmount += Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    updateTextFields();
    Vrui::requestUpdate();
}
void SliceTool::slopeAngleSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.slopeAngleDegrees = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    updateTextFields();
    Vrui::requestUpdate();
}

void SliceTool::falloffSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.falloffFactor = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    updateTextFields();
    Vrui::requestUpdate();
}

void SliceTool::coloringSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
{
    _sliceParameters.coloring = Vrui::Scalar(cbData->value);
    _sliceParameters.updatePlaneParameters();
    updateTextFields();
    Vrui::requestUpdate();
}

void SliceTool::showFaultLinesButtonCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData) {
    _sliceParameters.showFaultLines = cbData->set;
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
    //_sliceParameters.controlPoints.push_back(Point3(6371000,0,0));
    //_sliceParameters.controlPoints.push_back(Point3(0,6371000,0));

    _sliceParameters.strikeAmount = 0.0;
    _sliceParameters.dipAmount = 0.0;
    _sliceParameters.slopeAngleDegrees = 90.0;
    _sliceParameters.falloffFactor = 1.0;
    _sliceParameters.showFaultLines = true;
    _sliceParameters.coloring = 0.0;
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
    _sliceParameters.controlPoints.clear();
    markersHover       = 0;
    markersSelected    = 0;
    _sliceParameters.updatePlaneParameters();
}

void SliceTool::
callback()
{
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

        double minDist = std::numeric_limits<double>::max();
        markersHover = 0;

        for (size_t i=0; i < _sliceParameters.controlPoints.size(); ++i) {
            double dist = Geometry::dist(pos, _sliceParameters.controlPoints[i]);
            if (dist < selectDist && dist < minDist) {
                minDist = dist;
                markersHover = i+1;
            }
        }

        if (Vrui::getMainPipe() != NULL)
            Vrui::getMainPipe()->write<int>(markersHover);
    }
    else
        Vrui::getMainPipe()->read<int>(markersHover);

    if (markersSelected != 0)
    {
        _sliceParameters.controlPoints[markersSelected - 1] = pos;
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
    if (_sliceParameters.controlPoints.empty())
        return;

    //save relevant GL state and set state for marker rendering
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);

    //go to navigational coordinates
    std::vector<Point3> markerPos;
    for (size_t i=0; i< _sliceParameters.controlPoints.size(); ++i)
        markerPos.push_back(crusta->mapToScaledGlobe(_sliceParameters.controlPoints[i]));

    Vector3 centroid(0.0, 0.0, 0.0);
    for (size_t i=0; i < _sliceParameters.controlPoints.size(); ++i)
        centroid += Vector3(markerPos[i]);
    centroid /= _sliceParameters.controlPoints.size();

    //load the centroid relative translated navigation transformation
    glPushMatrix();

    Vrui::NavTransform nav =
        Vrui::getDisplayState(contextData).modelviewNavigational;
    nav *= Vrui::NavTransform::translate(centroid);
    glLoadMatrix(nav);

    //compute marker size
    Scalar scaleFac = Vrui::getNavigationTransformation().getScaling();
    Scalar size     = markerSize/scaleFac;

    for (size_t i=0; i < _sliceParameters.controlPoints.size(); ++i)
        markerPos[i] = Point3(Vector3(markerPos[i])-centroid);

    CHECK_GLA
    //draw the control points
    for (int i=0; i < static_cast<int>(_sliceParameters.controlPoints.size()); ++i)
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

        glBegin(GL_LINES);
            glVertex3f(markerPos[i][0]-size, markerPos[i][1], markerPos[i][2]);
            glVertex3f(markerPos[i][0]+size, markerPos[i][1], markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1]-size, markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1]+size, markerPos[i][2]);
            glVertex3f(markerPos[i][0], markerPos[i][1], markerPos[i][2]-size);
            glVertex3f(markerPos[i][0], markerPos[i][1], markerPos[i][2]+size);

            /*
            if (i <  _sliceParameters.controlPoints.size() - 1) {
                glVertex3f(markerPos[i][0], markerPos[i][1], markerPos[i][2]);
                glVertex3f(markerPos[i+1][0], markerPos[i+1][1], markerPos[i+1][2]);
            }
            */
        glEnd();

        CHECK_GLA
    }

   // glDisable(GL_DEPTH_TEST);

    GLdouble depthRange[2];
    glGetDoublev(GL_DEPTH_RANGE, depthRange);
    glDepthRange(0.0, 0.0);

    if (_sliceParameters.showFaultLines) {
        glLineWidth(8.0);
        glBegin(GL_LINES);

        for (size_t i=0; i < _sliceParameters.faultPlanes.size(); ++i) {
           // std::cout << "controlpoint0 = " << params.controlPoints[0][0] << ", " << params.controlPoints[0][1] << ", " << params.controlPoints[0][2] << std::endl;

            Vector3 a = Vector3(markerPos[i]);
            Vector3 b = Vector3(markerPos[i+1]);

            if (i == 0) {
                glColor3f(1,1,1);
            } else {
                Vector3 upDir = Vector3(_sliceParameters.controlPoints[i]);
                upDir.normalize();
                Vector3 faultLine = Vector3(b - a);
                Vector3 strikeDir = faultLine - (upDir * faultLine) * upDir;
                strikeDir.normalize();
                double compression = _sliceParameters.slopePlanes[0].normal * strikeDir;
                double sign = compression > 0 ? 1 : -1;
                compression = sign * sqrt(sqrt(fabs(compression)));
                if (_sliceParameters.strikeAmount < 0.0)
                    compression *= -1;

                if (compression < 0.0) {
                    double k = -compression;
                    //glColor3f(compression + 1, 1 + compression, 1.0);
                    glColor3f(1-k, 1-k, 1);
                } else {
                    double k = compression;
                    //glColor3f(1.0, 1 - compression, -compression);
                    glColor3f(1, 1-k + k * (165./255.), 1-k);
                }
            }

            glVertex3d(a[0], a[1], a[2]);
            glVertex3d(b[0], b[1], b[2]);

            /*
            double omega = acos(Vector3(a).normalize() * Vector3(b).normalize());

            for (size_t i=0; i < 2; ++i) {
                double t = i / 1.0;
                Vector3 pt = (1.0 / sin(omega)) * (sin((1-t)*omega) * a + sin(t * omega) * b);
                glVertex3d(pt[0], pt[1], pt[2]);
            }
*/
        }

        glEnd();
    }

    glDepthRange(depthRange[0], depthRange[1]);

    //glEnable(GL_DEPTH_TEST);

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
        if (markersHover > 0) {
            // cursor hovering above existing marker - drag it
            _sliceParameters.controlPoints[markersHover - 1]      = surfacePoint.position;
            markersSelected = markersHover;
        } else if (_sliceParameters.controlPoints.size() < 64) {
            // not all markers used - add new marker
            _sliceParameters.controlPoints.push_back(surfacePoint.position);
            markersHover = markersSelected = _sliceParameters.controlPoints.size();
        } else {
            // all markers in use - reset markers
            //_sliceParameters.controlPoints.clear();
            //_sliceParameters.controlPoints.push_back(surfacePoint.position);
            //markersHover    = 1;
            //markersSelected = 1;
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

SliceTool::Plane::Plane(const Vector3 &a, const Vector3 &b, double slopeAngleDegrees) : startPoint(a), endPoint(b) {
    strikeDirection = endPoint - startPoint;
    strikeDirection.normalize();

    // midpoint of fault line / plane

    // radius vector (center of planet to midpoint of fault line) = normal (because planet center is at (0,0,0))
    normal = cross(strikeDirection, startPoint); // normal of plane containing point and planet center
    normal.normalize();

    // rotate normal by 90 degrees around strike dir, now it's the tangential (horizon) plane
    // additionally, rotate by given slope angle

    normal = Vrui::Rotation::rotateAxis(strikeDirection, (90.0 + slopeAngleDegrees) / 360.0 * 2 * M_PI).transform(normal);
    normal.normalize();

    // dip vector
    dipDirection = cross(strikeDirection, normal);
    dipDirection.normalize(); // just to be sure :)

    distance = -startPoint * normal;
}

SliceTool::Plane::Plane(const Vector3 &a, const Vector3 &b) : startPoint(a), endPoint(b) {
    strikeDirection = endPoint - startPoint;
    strikeDirection.normalize();

    normal = -cross(a, b);
    normal.normalize();

    // dip vector
    dipDirection = Vector3(-0.5 * (a+b));
    dipDirection.normalize();

    distance = 0;
}

double SliceTool::Plane::getPointDistance(const Vector3 &point) const {
    return normal * point - distance;
}

Vector3 SliceTool::Plane::getPlaneCenter() const {
    return getPointDistance(Vector3(0,0,0)) * normal;
}

SliceTool::SliceParameters::SliceParameters() {
}


const SliceTool::SliceParameters &SliceTool::getParameters() {
    return _sliceParameters;
}

void SliceTool::SliceParameters::updatePlaneParameters() {
    faultPlanes.clear();
    separatingPlanes.clear();
    slopePlanes.clear();

    if (controlPoints.size() < 2)
        return;

    std::vector<Vector3> pts;
    // project all points to average sphere
    double radius = 0.0;
    for (size_t i=0; i < controlPoints.size(); ++i)
        radius += Vector3(controlPoints[i]).mag();

    radius /= controlPoints.size();
    for (size_t i=0; i < controlPoints.size(); ++i) {
        Vector3 v = Vector3(controlPoints[i]);
        v.normalize();
        pts.push_back(radius * v);
    }


    faultCenter = Vector3(0,0,0);
    if (!pts.empty()) {
        for (size_t i=0; i < pts.size(); ++i)
            faultCenter += Vector3(pts[i]);
        faultCenter *= 1.0 / pts.size();
    }


    size_t nPlanes = pts.size() - 1;

    for (size_t i=0; i < nPlanes; ++i) {
        faultPlanes.push_back(Plane(Vector3(pts[i]), Vector3(pts[i+1]), 90));
        slopePlanes.push_back(Plane(Vector3(pts[i]), Vector3(pts[i+1]), slopeAngleDegrees));
    }

    for (size_t i=0; i < pts.size(); ++i) {
        Vector3 n(0,0,0);
        if (i == 0)
            n = faultPlanes[i].normal;
        else if (i == pts.size() - 1)
            n = faultPlanes[i-1].normal;
        else
            n = 0.5 * (faultPlanes[i-1].normal + faultPlanes[i].normal);
        n.normalize();

        separatingPlanes.push_back(Plane(Vector3(pts[i]), Vector3(pts[i]) + 1e6 * n));
    }
}


Vector3 SliceTool::SliceParameters::getShiftVector(const Plane &p) const {
    return Vector3(strikeAmount * p.strikeDirection +
                   dipAmount * p.dipDirection);
}

Vector3 SliceTool::SliceParameters::getLinearTranslation() const {
    if (controlPoints.size() <= 1)
        return Vector3(0,0,0);

    Vector3 a = Vector3(controlPoints.front());
    Vector3 b = Vector3(controlPoints.back());

    Vector3 delta = b - a;
    delta.normalize();

    return strikeAmount * delta;
}

END_CRUSTA
