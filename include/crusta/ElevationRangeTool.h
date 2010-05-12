#ifndef _ElevationRangeTool_H_
#define _ElevationRangeTool_H_


#include <GLMotif/Button.h>
#include <GLMotif/FileSelectionDialog.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/Slider.h>
#include <Misc/TimerEventScheduler.h>


#include <crusta/Tool.h>


namespace GLMotif
{
    class TextField;
    class PopupWindow;
}


BEGIN_CRUSTA


class ElevationRangeTool : public Tool
{
    friend class Vrui::GenericToolFactory<ElevationRangeTool>;

public:
    typedef Vrui::GenericToolFactory<ElevationRangeTool> Factory;

    ElevationRangeTool(const Vrui::ToolFactory* iFactory,
                const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~ElevationRangeTool();

    static Vrui::ToolFactory* init(Vrui::ToolFactory* parent);

protected:
    /** modes of the range tool */
    enum Mode
    {
        /** manipulate the min and max of the range */
        MODE_MIN_MAX,
        /** manipulate the origin of the range */
        MODE_SHIFT
    };
    /** names for manipulation sources */
    enum ManipulationSource
    {
        /** changed using the min/max sliders */
        MANIP_SLIDERS,
        /** changed by setting min/max markers */
        MANIP_MIN_MAX_MARKERS,
        /** changed by setting the range origin marker */
        MANIP_SHIFT_MARKER
    };

    /** resets the state of the marker modes of the tool */
    void resetMarkers();
    /** retrieve the tool position in navigation coordinates relative to the
        unscaled globe */
    Point3 getPosition();

    /** apply new min and max values derived from specified manipulation */
    void applyToColorMap(const ManipulationSource& manip);

    /** process a new frame for the min/max mode */
    void frameMinMax(const Point3& pos);
    /** process a new frame for the shifting mode */
    void frameShift(const Point3& pos);


    /** update the labels showing the min and max elevation */
    void updateLabels(const Scalar& min, const Scalar& max);
    /** callback to process change of range manipulation mode */
    void modeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData);
    /** callback to process dragging of the direct range sliders */
    void dragCallback(GLMotif::Slider::DraggingCallbackData* cbData);
    /** callback to process pressing the +/- buttons on the range sliders */
    void plusMinusCallback(GLMotif::Button::SelectCallbackData* cbData);
    /** callback to process timed manipulation of min/max through sliders */
    void tickCallback(Misc::TimerEventScheduler::CallbackData* cbData);
    /** callback to process loading an elevation range from file */
    void loadCallback(GLMotif::Button::SelectCallbackData* cbData);
    /** callback to process saving an elevation range to file */
    void saveCallback(GLMotif::Button::SelectCallbackData* cbData);

    /** ok callback for loading dialog */
    void loadFileOKCallback(
        GLMotif::FileSelectionDialog::OKCallbackData* cbData);
    /** cancel callback for loading dialog */
    void loadFileCancelCallback(
        GLMotif::FileSelectionDialog::CancelCallbackData* cbData);

    /** current editing mode of the range tool */
    int mode;

    /** keep track of which markers are set */
    int markersSet;
    /** keep track of which markers are being hovered over */
    int markersHover;
    /** keep track of which markers are selected and being dragged */
    int markersSelected;

    /** the tool's marker locations */
    Point3 markers[2];

    /** the size of the markers */
    static const Scalar markerSize;
    /** physical space distance within which markers should be selected */
    static const Scalar selectDistance;

    /** record if a range slider is being dragged */
    int rangeSliderDragged;

    /** popup window for modifying the modes and the ranges directly */
    GLMotif::PopupWindow* dialog;
    /** text fields displaying the current min/max values */
    GLMotif::TextField* rangeLabels[2];
    /** direct adjustment sliders */
    GLMotif::Slider* rangeSliders[2];

private:
    static Factory* factory;

//- Inherited from CrustaComponent
public:
    virtual void setupComponent(Crusta* nCrusta);

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int deviceIndex, int buttonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);
};

END_CRUSTA


#endif //_ElevationRangeTool_H_
