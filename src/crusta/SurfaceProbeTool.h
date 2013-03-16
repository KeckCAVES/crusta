#ifndef _Crusta_SurfaceProbeTool_H_
#define _Crusta_SurfaceProbeTool_H_


#include <crustavrui/GL/VruiGlew.h>

#include <crusta/SurfaceProjector.h>
#include <crusta/Tool.h>

#include <crusta/vrui.h>


namespace GLMotif {
    class PopupWindow;
}


namespace crusta {


///\todo rename all the minMax to dual and all the shift to single
class SurfaceProbeTool : public Vrui::Tool, public SurfaceProjector
{
    friend class Vrui::GenericToolFactory<SurfaceProbeTool>;

public:
    typedef Vrui::GenericToolFactory<SurfaceProbeTool> Factory;

    /** base class for callback data sent by the probe */
    class CallbackData : public Misc::CallbackData
    {
    public:
        /** pointer to the probe that sent the callback */
        SurfaceProbeTool* probe;

        CallbackData(SurfaceProbeTool* probe_);
    };

    /** callback for changes in the sample */
    class SampleCallbackData : public CallbackData
    {
    public:
        SampleCallbackData(SurfaceProbeTool* probe_,
                           int sampleId_, int numSamples_,
                           const SurfacePoint& surfacePoint_);

        /** the sample's position in a sequence */
        int sampleId;
        /** the number of samples in a sequence */
        int numSamples;
        /** the surface point for the sample */
        SurfacePoint surfacePoint;
    };

    SurfaceProbeTool(const Vrui::ToolFactory* iFactory,
                     const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~SurfaceProbeTool();

    static Vrui::ToolFactory* init();

    /** returns list of sample callbacks */
    Misc::CallbackList& getSampleCallbacks();

protected:
    /** modes of the range tool */
    enum Mode
    {
        /** manipulate the min and max of the range */
        MODE_MIN_MAX,
        /** manipulate the origin of the range */
        MODE_SHIFT
    };

    /** resets the state of the marker modes of the tool */
    void resetMarkers();

    /** process a new frame for the min/max mode */
    void frameMinMax();
    /** process a new frame for the shifting mode */
    void frameShift();

    /** callback registered listeners for sample picking */
    void callback();

    /** callback to process change of range manipulation mode */
    void modeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData);


    /** the currently probed surface point */
    SurfacePoint surfacePoint;

    /** current editing mode of the range tool */
    int mode;

    /** keep track of which markers are set */
    int markersSet;
    /** keep track of which markers are being hovered over */
    int markersHover;
    /** keep track of which markers are selected and being dragged */
    int markersSelected;

    /** the tool's marker locations */
    Geometry::Point<double,3> markers[2];

    /** the size of the markers */
    static const Scalar markerSize;
    /** physical space distance within which markers should be selected */
    static const Scalar selectDistance;

    /** list of callbacks to be called when a sample has been picked */
    Misc::CallbackList sampleCallbacks;

    /** popup window for modifying the modes */
    GLMotif::PopupWindow* dialog;

private:
    static Factory* factory;

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void buttonCallback(int buttonSlotIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);

//- Inherited from CrustaComponent
public:
    virtual void setupComponent(Crusta* crusta);
};

} //namespace crusta


#endif //_Crusta_SurfaceProbeTool_H_
