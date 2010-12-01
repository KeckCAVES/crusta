#ifndef _GLMotif_RangeWidget_H_
#define _GLMotif_RangeWidget_H_


#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Misc/ColorMap.h>
#include <GLMotif/RelativeSlider.h>
#include <GLMotif/RowColumn.h>


namespace GLMotif {


class TextField;


class RangeWidget : public RowColumn
{
public:
    /** base class for callback data */
    class CallbackData : public Misc::CallbackData
    {
    public:
        CallbackData(RangeWidget* widget);
        RangeWidget* rangeWidget;
    };
    /** callback data for communicating a change in the range values */
    class RangeChangedCallbackData : public CallbackData
    {
    public:
        RangeChangedCallbackData(RangeWidget* widget,
                                 double minimum, double maximum);
        double min;
        double max;
    };


    RangeWidget(const char* name, Container* parent, bool manageChild=true);
    virtual ~RangeWidget();

    /** set the current range of the widget */
    void setRange(double min, double max, bool propagate=true);

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


#endif //_GLMotif_RangeWidget_H_
