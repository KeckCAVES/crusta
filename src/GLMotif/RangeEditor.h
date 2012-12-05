#ifndef _GLMotif_RangeEditor_H_
#define _GLMotif_RangeEditor_H_


#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Misc/ColorMap.h>
#include <GLMotif/RelativeSlider.h>
#include <GLMotif/RowColumn.h>


namespace GLMotif {


class TextField;


class RangeEditor : public RowColumn
{
public:
    /** base class for callback data */
    class CallbackData : public Misc::CallbackData
    {
    public:
        CallbackData(RangeEditor* editor);
        RangeEditor* rangeEditor;
    };
    /** callback data for communicating a change in the range values */
    class RangeChangedCallbackData : public CallbackData
    {
    public:
        RangeChangedCallbackData(RangeEditor* editor,
                                 double minimum, double maximum);
        double min;
        double max;
    };


    RangeEditor(const char* name, Container* parent, bool manageChild=true);
    virtual ~RangeEditor();

    /**\{ set the current range of the editor */
    void  shift(double min, bool propagate=true);
    void setMin(double min, bool propagate=true);
    void setMax(double max, bool propagate=true);
    void setRange(double min, double max, bool propagate=true);
    /**\}*/

    /** retrieve the list of registered range change callbacks */
    Misc::CallbackList& getRangeChangedCallbacks();

protected:
    /** update the labels showing the min and max elevation */
    void updateLabels();
    /** generate a callback for value changed */
    void callback();

    /** callback to process dragging of the sliders */
    void sliderCallback(GLMotif::RelativeSlider::ValueCallbackData* cbData);

    double min;
    double max;

    /** list of callbacks registered for the change of the range */
    Misc::CallbackList rangeChangedCallbacks;

    /** text fields displaying the current min/max and center values */
    GLMotif::TextField* rangeLabels[3];
};


} //end namespace GLMotif


#endif //_GLMotif_RangeEditor_H_
