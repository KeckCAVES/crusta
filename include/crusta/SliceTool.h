#ifndef _Crusta_SliceTool_H_
#define _Crusta_SliceTool_H_


#include <GL/VruiGlew.h>

#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GLMotif/Button.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/Slider.h>
#include <GLMotif/TextField.h>
#include <Vrui/Tools/GenericToolFactory.h>

#include <crusta/SurfaceProjector.h>
#include <crusta/Tool.h>


namespace GLMotif {
    class PopupWindow;
}


BEGIN_CRUSTA


///\todo rename all the minMax to dual and all the shift to single
class SliceTool : public Vrui::Tool, public SurfaceProjector
{
    friend class Vrui::GenericToolFactory<SliceTool>;

public:
    typedef Vrui::GenericToolFactory<SliceTool> Factory;

    /** base class for callback data sent by the probe */
    class CallbackData : public Misc::CallbackData
    {
    public:
        /** pointer to the probe that sent the callback */
        SliceTool* probe;

        CallbackData(SliceTool* probe_);
    };

    /** callback for changes in the sample */
    class SampleCallbackData : public CallbackData
    {
    public:
        SampleCallbackData(SliceTool* probe_,
                           int sampleId_, int numSamples_,
                           const SurfacePoint& surfacePoint_);

        /** the sample's position in a sequence */
        int sampleId;
        /** the number of samples in a sequence */
        int numSamples;
        /** the surface point for the sample */
        SurfacePoint surfacePoint;
    };

    SliceTool(const Vrui::ToolFactory* iFactory,
                     const Vrui::ToolInputAssignment& inputAssignment);
    virtual ~SliceTool();

    static Vrui::ToolFactory* init();

protected:
    /** resets the state of the marker modes of the tool */
    void resetMarkers();

    /** callback registered listeners for sample picking */
    void callback();

    /** the currently probed surface point */
    SurfacePoint surfacePoint;

    /** keep track of which markers are being hovered over */
    int markersHover;
    /** keep track of which markers are selected and being dragged */
    int markersSelected;

    /** the size of the markers */
    static const Scalar markerSize;
    /** physical space distance within which markers should be selected */
    static const Scalar selectDistance;

    /** popup window for modifying the modes */
    GLMotif::PopupWindow* dialog;

    GLMotif::TextField* strikeAmountTextField;
    GLMotif::TextField* dipAmountTextField;
    GLMotif::TextField* slopeAngleTextField;
private:
    static Factory* factory;
    void updatePlaneParameters();

//- Inherited from Vrui::Tool
public:
    virtual const Vrui::ToolFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;

    virtual void strikeAmountSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
    virtual void dipAmountSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);

    virtual void slopeAngleSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
    virtual void falloffSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);

    virtual void buttonCallback(int deviceIndex, int deviceButtonIndex,
                                Vrui::InputDevice::ButtonCallbackData* cbData);

    void updateTextFields();

//- Inherited from CrustaComponent
public:
    virtual void setupComponent(Crusta* crusta);

    struct Plane {
        // fault line (segment) between points a and b with given slope
        Plane(const Vector3 &a, const Vector3 &b, double slopeAngleDegrees);
        // plane through 2 points and planet center (intersection with planet is great circle)
        Plane(const Vector3 &a, const Vector3 &b);
        double getPointDistance(const Vector3 &point) const;
        Vector3 getPlaneCenter() const;

        Vector3 strikeDirection;
        Vector3 dipDirection;

        // for drawing lines on surface etc.
        Vector3 startPoint, endPoint;
        // hessian normal form
        Vector3 normal;
        double distance;
    };

    // FIXME: Does not allow for multiple instances
    struct SliceParameters {
        double strikeAmount;
        double dipAmount;
        double slopeAngleDegrees;
        double falloffFactor;

        /** the tool's marker locations (control points of fault line) */
        std::vector<Point3> controlPoints;

        Vector3 faultCenter; // center of mass of control points

        std::vector<Plane> faultPlanes;
        std::vector<Plane> separatingPlanes;

        SliceParameters();
        // plane equations parameters (recalculated when values above change)
        void updatePlaneParameters();
        Vector3 getShiftVector(const Plane &p) const;
    };

    static SliceParameters _sliceParameters;
    static const SliceParameters &getParameters();

};

END_CRUSTA


#endif //_Crusta_SliceTool_H_
