#include <crusta/ElevationRangeTool.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include <Comm/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/GLColorMap.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Label.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/TextField.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/WidgetManager.h>
#include <Misc/CreateNumberedFileName.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <crusta/checkGl.h>
#include <crusta/Crusta.h>

BEGIN_CRUSTA


ElevationRangeTool::Factory* ElevationRangeTool::factory = NULL;
const Scalar ElevationRangeTool::markerSize              = 0.2;
const Scalar ElevationRangeTool::selectDistance          = 0.5;


ElevationRangeTool::
ElevationRangeTool(const Vrui::ToolFactory* iFactory,
                   const Vrui::ToolInputAssignment& inputAssignment) :
    Tool(iFactory, inputAssignment),
    mode(MODE_MIN_MAX), markersSet(0), markersHover(0), markersSelected(0),
    rangeSliderDragged(0)
{
    const GLMotif::StyleSheet* style =
            Vrui::getWidgetManager()->getStyleSheet();

    dialog = new GLMotif::PopupWindow("ElevationRangeDialog",
        Vrui::getWidgetManager(), "Elevation Range Settings");

    GLMotif::RowColumn* top = new GLMotif::RowColumn("ERtop", dialog, false);
    top->setNumMinorWidgets(1);

//- create the mode selection group
    GLMotif::RadioBox* modeBox = new GLMotif::RadioBox("Range Modes",top,false);
    modeBox->setNumMinorWidgets(2);
    modeBox->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
    modeBox->setPacking(GLMotif::RadioBox::PACK_GRID);

    GLMotif::ToggleButton* minMaxToggle =
        new GLMotif::ToggleButton("ERminMaxMode", modeBox, "Set Min/Max");
    new GLMotif::ToggleButton("ERshiftMode", modeBox, "Shift Min");

    modeBox->setSelectedToggle(minMaxToggle);
    modeBox->getValueChangedCallbacks().add(this,
                                            &ElevationRangeTool::modeCallback);

    modeBox->manageChild();

//- create the direct control group
    GLMotif::RowColumn* direct = new GLMotif::RowColumn("ERdirect", top, false);
    direct->setNumMinorWidgets(5);

    //create the two sliders for direct control of the range
    new GLMotif::Label("ERmaxLabelName", direct, "Max:");
    rangeLabels[1] = new GLMotif::TextField("ERmaxLabel", direct, 9);
    rangeLabels[1]->setFloatFormat(GLMotif::TextField::FIXED);
    rangeLabels[1]->setFieldWidth(9);
    rangeLabels[1]->setPrecision(2);

    GLMotif::Button* maxMinus = new GLMotif::Button("ERmaxMinusButton", direct,
                                                    "-");
    maxMinus->getSelectCallbacks().add(this,
                                       &ElevationRangeTool::plusMinusCallback);

    rangeSliders[1] = new GLMotif::Slider("ERmaxSlider", direct,
        GLMotif::Slider::HORIZONTAL, 10.0 * style->fontHeight);
    rangeSliders[1]->setValue(0.0);
    rangeSliders[1]->setValueRange(-2.0, 2.0, 0.00001);
    rangeSliders[1]->getDraggingCallbacks().add(this,
        &ElevationRangeTool::dragCallback);

    GLMotif::Button* maxPlus = new GLMotif::Button("ERmaxPlusButton", direct,
                                                   "+");
    maxPlus->getSelectCallbacks().add(this,
                                      &ElevationRangeTool::plusMinusCallback);


    new GLMotif::Label("ERminLabelName", direct, "Min:");
    rangeLabels[0] = new GLMotif::TextField("ERminLabel", direct, 9);
    rangeLabels[0]->setFloatFormat(GLMotif::TextField::FIXED);
    rangeLabels[0]->setFieldWidth(9);
    rangeLabels[0]->setPrecision(2);

    GLMotif::Button* minMinus = new GLMotif::Button("ERminMinusButton", direct,
                                                    "-");
    minMinus->getSelectCallbacks().add(this,
                                       &ElevationRangeTool::plusMinusCallback);

    rangeSliders[0] = new GLMotif::Slider("ERminSlider", direct,
        GLMotif::Slider::HORIZONTAL, 10.0 * style->fontHeight);
    rangeSliders[0]->setValue(0.0);
    rangeSliders[0]->setValueRange(-2.0, 2.0, 0.00001);
    rangeSliders[0]->getDraggingCallbacks().add(this,
        &ElevationRangeTool::dragCallback);

    GLMotif::Button* minPlus = new GLMotif::Button("ERminPlusButton", direct,
                                                   "+");
    minPlus->getSelectCallbacks().add(this,
                                      &ElevationRangeTool::plusMinusCallback);

    direct->manageChild();

//- create the save/load group
    GLMotif::RowColumn* io = new GLMotif::RowColumn("ERio", top, false);
    io->setNumMinorWidgets(2);

    GLMotif::Button* load = new GLMotif::Button("ERloadButton", io, "Load");
    load->getSelectCallbacks().add(this, &ElevationRangeTool::loadCallback);
    GLMotif::Button* save = new GLMotif::Button("ERsaveButton", io, "Save");
    save->getSelectCallbacks().add(this, &ElevationRangeTool::saveCallback);

    io->manageChild();

    top->manageChild();
}

ElevationRangeTool::
~ElevationRangeTool()
{
    //close the dialog
    Vrui::popdownPrimaryWidget(dialog);
}


Vrui::ToolFactory* ElevationRangeTool::
init(Vrui::ToolFactory* parent)
{
    Factory* erFactory = new Factory("CrustaElevationRangeTool",
        "Elevation Range Tool", parent, *Vrui::getToolManager());

    erFactory->setNumDevices(1);
    erFactory->setNumButtons(0, 1);

    Vrui::getToolManager()->addClass(erFactory,
        Vrui::ToolManager::defaultToolFactoryDestructor);

    ElevationRangeTool::factory = erFactory;

    return ElevationRangeTool::factory;
}


void ElevationRangeTool::
setupComponent(Crusta* nCrusta)
{
    //base setup
    Tool::setupComponent(nCrusta);

    //update the sliders with the proper min and max values
    GLColorMap* colorMap = crusta->getColorMap();
    Scalar min           = colorMap->getScalarRangeMin();
    Scalar max           = colorMap->getScalarRangeMax();

    updateLabels(min, max);

    //popup the dialog
    const Vrui::NavTransform& navXform = Vrui::getNavigationTransformation();
    Vrui::popupPrimaryWidget(dialog,
        navXform.transform(Vrui::getDisplayCenter()));
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
    Point3 pos;
    if (Vrui::isMaster())
    {
        pos = getPosition();
        pos = crusta->mapToUnscaledGlobe(pos);

        if (Vrui::getMainPipe() != NULL)
            Vrui::getMainPipe()->write<Point3>(pos);
    }
    else
        Vrui::getMainPipe()->read<Point3>(pos);

    switch (mode)
    {
        case MODE_MIN_MAX:
            frameMinMax(pos);
            break;

        case MODE_SHIFT:
            frameShift(pos);
            break;

        default:
            break;
    }
}

void ElevationRangeTool::
display(GLContextData& contextData) const
{
    CHECK_GLA
    //don't draw anything if we don't have any control points yet
    if (markersSet == 0)
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
    for (int i=0; i<markersSet; ++i)
    {
        Point3 m = crusta->mapToScaledGlobe(markers[i]);

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
            glVertex3f(m[0]-size, m[1],      m[2]);
            glVertex3f(m[0]+size, m[1],      m[2]);
            glVertex3f(m[0],      m[1]-size, m[2]);
            glVertex3f(m[0],      m[1]+size, m[2]);
            glVertex3f(m[0],      m[1],      m[2]-size);
            glVertex3f(m[0],      m[1],      m[2]+size);
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
buttonCallback(int, int, Vrui::InputDevice::ButtonCallbackData* cbData)
{
    //get the current position
    Point3 pos;
    if (Vrui::isMaster())
    {
        pos = getPosition();
        pos = crusta->mapToUnscaledGlobe(pos);

        if (Vrui::getMainPipe() != NULL)
            Vrui::getMainPipe()->write<Point3>(pos);
    }
    else
        Vrui::getMainPipe()->read<Point3>(pos);


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
                        markers[0]      = pos;
                        markersSet      = 1;
                        markersHover    = 1;
                        markersSelected = 1;
                    } break;

                    case 1:
                    {
                        markers[1]      = pos;
                        markersSet      = 2;
                        markersHover    = 2;
                        markersSelected = 2;
                        applyToColorMap(MANIP_MIN_MAX_MARKERS);
                    } break;

                    case 2:
                    {
                        if (markersHover == 0)
                        {
                            //resetting marker placement
                            markers[0]      = pos;
                            markersSet      = 1;
                            markersHover    = 1;
                            markersSelected = 1;
                        }
                        else
                        {
                            //modifying existing marker position
                            markers[markersHover-1] = pos;
                            markersSelected         = markersHover;
                            applyToColorMap(MANIP_MIN_MAX_MARKERS);
                        }
                    } break;

                    default:
                        break;
                }
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
                markers[0] = pos;
                applyToColorMap(MANIP_SHIFT_MARKER);
            }
            else
                markersSelected = 0;
        }
    }
}


void ElevationRangeTool::
resetMarkers()
{
    markersSet         = 0;
    markersHover       = 0;
    markersSelected    = 0;
}

Point3 ElevationRangeTool::
getPosition()
{
    Vrui::NavTrackerState nav = Vrui::getDeviceTransformation(getDevice(0));
    Vrui::NavTrackerState::Vector trans = nav.getTranslation();
    return Point3(trans[0], trans[1], trans[2]);
}


void ElevationRangeTool::
applyToColorMap(const ManipulationSource& manip)
{
    //compute the new min/max
    Scalar newMin, newMax;
    if (Vrui::isMaster())
    {
        switch (manip)
        {
            case MANIP_SLIDERS:
            {
                Scalar* minMax[2] = {&newMin, &newMax};
                //get the current range from the color map
                GLColorMap* colorMap = crusta->getColorMap();
                *(minMax[0])         = colorMap->getScalarRangeMin();
                *(minMax[1])         = colorMap->getScalarRangeMax();

                //grab the value from the proper slider
                assert(rangeSliderDragged==1 || rangeSliderDragged==2);
                GLMotif::Slider* slider = rangeSliders[rangeSliderDragged-1];
                Scalar sliderValue      = slider->getValue();

                //scale the slider value
                Scalar delta = pow(10.0, Math::abs(sliderValue)) - 1.0;
                delta        = sliderValue<0.0 ? -delta : delta;

                //apply the change to the corresponding limit
                *(minMax[rangeSliderDragged-1]) += delta;
                //don't allow the limits to cross over
                if (rangeSliderDragged == 1)
                    newMax = newMax<newMin ? newMin : newMax;
                else
                    newMin = newMin>newMax ? newMax : newMin;

                break;
            }
            case MANIP_MIN_MAX_MARKERS:
            {
                assert(markersSet == 2);
                newMin = Vector3(markers[0]).mag() -
                         crusta->getSettings().globeRadius;
                newMax = Vector3(markers[1]).mag() -
                         crusta->getSettings().globeRadius;
                break;
            }
            case MANIP_SHIFT_MARKER:
            {
                assert(markersSet == 1);
                //compute the min elevation from the marker
                newMin = Vector3(markers[0]).mag() -
                         crusta->getSettings().globeRadius;

                //get the current range from the color map
                GLColorMap* colorMap = crusta->getColorMap();
                Scalar min           = colorMap->getScalarRangeMin();
                Scalar max           = colorMap->getScalarRangeMax();
                Scalar range         = max - min;

                newMax = newMin + range;

                break;
            }
            default:
                newMin = newMax = 0.0;
        }
        //send the new values to the slaves
        if (Vrui::getMainPipe() != NULL)
        {
            Vrui::getMainPipe()->write<Scalar>(newMin);
            Vrui::getMainPipe()->write<Scalar>(newMax);
        }
    }
    else
    {
        //read the new values from the master
        Vrui::getMainPipe()->read<Scalar>(newMin);
        Vrui::getMainPipe()->read<Scalar>(newMax);
    }

    //set the new min max in the color map
    GLColorMap* colorMap = crusta->getColorMap();
    colorMap->setScalarRange(newMin, newMax);
    crusta->touchColorMap();

    //update the displayed min/max
    updateLabels(newMin, newMax);
}


void ElevationRangeTool::
frameMinMax(const Point3& pos)
{
    if (Vrui::isMaster())
    {
        //compute selection distance
        Scalar scaleFac   = Vrui::getNavigationTransformation().getScaling();
        Scalar selectDist = selectDistance / scaleFac;

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
        if (markersSet == 2)
            applyToColorMap(MANIP_MIN_MAX_MARKERS);
    }
}

void ElevationRangeTool::
frameShift(const Point3& pos)
{
    if (markersSelected == 1)
    {
        markers[0] = pos;
        applyToColorMap(MANIP_SHIFT_MARKER);
    }
}


void ElevationRangeTool::
updateLabels(const Scalar& min, const Scalar& max)
{
    rangeLabels[0]->setValue(min);
    rangeLabels[1]->setValue(max);
}

void ElevationRangeTool::
modeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
{
    resetMarkers();
    if (strcmp(cbData->newSelectedToggle->getName(), "ERminMaxMode") == 0)
        mode = MODE_MIN_MAX;
    else if (strcmp(cbData->newSelectedToggle->getName(), "ERshiftMode") == 0)
        mode = MODE_SHIFT;
}

void ElevationRangeTool::
dragCallback(GLMotif::Slider::DraggingCallbackData* cbData)
{
    switch (cbData->reason)
    {
        case GLMotif::Slider::DraggingCallbackData::DRAGGING_STARTED:
        {
            rangeSliderDragged = cbData->dragWidget==rangeSliders[0] ? 1 : 2;
            //schedule a tick
            Misc::TimerEventScheduler* tes =
                Vrui::getWidgetManager()->getTimerEventScheduler();
            double nextTickTime = tes->getCurrentTime() + 0.05;
            tes->scheduleEvent(nextTickTime, this,
                               &ElevationRangeTool::tickCallback);
            //clear the current markers as they will no longer be representative
            resetMarkers();
            break;
        }

        case GLMotif::Slider::DraggingCallbackData::DRAGGING_STOPPED:
        {
            //record that dragging has stopped
            rangeSliderDragged = 0;
            //reset the affected slider
            if (cbData->dragWidget == rangeSliders[0])
                rangeSliders[0]->setValue(0.0);
            else
                rangeSliders[1]->setValue(0.0);
            break;
        }

        default:
            return;
    }
}

void ElevationRangeTool::
plusMinusCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    //get the current range from the color map
    GLColorMap* colorMap = crusta->getColorMap();
    Scalar newMin        = colorMap->getScalarRangeMin();
    Scalar newMax        = colorMap->getScalarRangeMax();

    if (strcmp(cbData->button->getName(), "ERmaxMinusButton") == 0)
        newMax = floor(newMax - 1.0);
    else if (strcmp(cbData->button->getName(), "ERmaxPlusButton") == 0)
        newMax = floor(newMax + 1.0);
    else if (strcmp(cbData->button->getName(), "ERminMinusButton") == 0)
        newMin = floor(newMin - 1.0);
    else if (strcmp(cbData->button->getName(), "ERminPlusButton") == 0)
        newMin = floor(newMin + 1.0);

    //set the new min max in the color map
    colorMap->setScalarRange(newMin, newMax);
    crusta->touchColorMap();

    //update the displayed min/max
    updateLabels(newMin, newMax);}

void ElevationRangeTool::
tickCallback(Misc::TimerEventScheduler::CallbackData* cbData)
{
    if (rangeSliderDragged != 0)
    {
        //apply modification
        applyToColorMap(MANIP_SLIDERS);

        //reschedule tick
        Misc::TimerEventScheduler* tes =
            Vrui::getWidgetManager()->getTimerEventScheduler();
        double nextTickTime = tes->getCurrentTime() + 0.05;
        tes->scheduleEvent(nextTickTime, this,
                           &ElevationRangeTool::tickCallback);
    }
}

void ElevationRangeTool::
loadCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    //create a file selection dialog and connect callbacks to process loading
    GLMotif::FileSelectionDialog* fileDialog =
        new GLMotif::FileSelectionDialog(Vrui::getWidgetManager(),
                                         "Load Range File", 0, ".rng");
    fileDialog->getOKCallbacks().add(this,
                                     &ElevationRangeTool::loadFileOKCallback);
    fileDialog->getCancelCallbacks().add(this,
        &ElevationRangeTool::loadFileCancelCallback);
    Vrui::getWidgetManager()->popupPrimaryWidget(fileDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(dialog));
}

void ElevationRangeTool::
saveCallback(GLMotif::Button::SelectCallbackData* cbData)
{
    //grab the current min and max values
    GLColorMap* colorMap = crusta->getColorMap();
    Scalar min           = colorMap->getScalarRangeMin();
    Scalar max           = colorMap->getScalarRangeMax();

    //generate a numbered file name
    std::string fileName("Crusta_ElevationRange.rng");
    fileName = Misc::createNumberedFileName(fileName, 4);

    //open the file and dump the current min and max values
    std::ofstream osf(fileName.c_str());
    if (osf.fail())
    {
        std::cerr << "ElevationRangeTool::save: error opening file " <<
                     fileName << " for output." << std::endl;
        return;
    }

    osf << "Range minimum: " << min << "\n" << "Range maximum: " << max;
    osf.close();
}


void ElevationRangeTool::
loadFileOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
{
    //load the range from the selected file
    std::ifstream ifs(cbData->selectedFileName.c_str());
    if (ifs.fail())
    {
        std::cerr << "ElevationRangeTool::load: error opening file " <<
                     cbData->selectedFileName << " for input." << std::endl;
        return;
    }

    GLColorMap* colorMap = crusta->getColorMap();
    Scalar newMin        = colorMap->getScalarRangeMin();
    Scalar newMax        = colorMap->getScalarRangeMax();

    std::string token;
    while (!ifs.eof())
    {
        std::getline(ifs, token, ':');
        if (token.compare("Range minimum") == 0)
        {
            ifs >> newMin;
            //read and discard the rest of the line
            std::getline(ifs, token);
        }
        else if (token.compare("Range maximum") == 0)
        {
            ifs >> newMax;
            //read and discard the rest of the line
            std::getline(ifs, token);
        }
    }

    //set the new min max in the color map
    colorMap->setScalarRange(newMin, newMax);
    crusta->touchColorMap();

    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void ElevationRangeTool::
loadFileCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}


END_CRUSTA
