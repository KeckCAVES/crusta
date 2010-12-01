#include <GLMotif/RangeWidget.h>


#include <string>

#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/TextField.h>
#include <GLMotif/WidgetManager.h>


namespace GLMotif {


RangeWidget::CallbackData::
CallbackData(RangeWidget* widget) :
    rangeWidget(widget)
{
}

RangeWidget::RangeChangedCallbackData::
RangeChangedCallbackData(RangeWidget* widget, double minimum, double maximum) :
    CallbackData(widget), min(minimum), max(maximum)
{
}


RangeWidget::
RangeWidget(const char* name, Container* parent, bool childManaged) :
    RowColumn(name, parent, false)
{
    const StyleSheet* style = getManager()->getStyleSheet();

    setNumMinorWidgets(1);


//- create the min max labels group
    RowColumn* labels = new RowColumn("Rlabels", this, false);
    labels->setNumMinorWidgets(7);
    labels->setPacking(RowColumn::PACK_GRID);

    new Label("RminLabelName", labels, "Min:");

    rangeLabels[0] = new TextField("RminLabel", labels, 9);
    rangeLabels[0]->setFloatFormat(TextField::FIXED);
    rangeLabels[0]->setFieldWidth(9);
    rangeLabels[0]->setPrecision(2);

    new Blind("RminBlind", labels);

    rangeLabels[1] = new TextField("RcenterLabel", labels, 9);
    rangeLabels[1]->setFloatFormat(TextField::FIXED);
    rangeLabels[1]->setFieldWidth(9);
    rangeLabels[1]->setPrecision(2);

    new Blind("RmaxBlind", labels);

    rangeLabels[2] = new TextField("RmaxLabel", labels, 9);
    rangeLabels[2]->setFloatFormat(TextField::FIXED);
    rangeLabels[2]->setFieldWidth(9);
    rangeLabels[2]->setPrecision(2);

    new Label("RmaxLabelName", labels, ":Max");

    labels->manageChild();


//- create the min/max sliders group
    RowColumn* minmax = new RowColumn("Rminmax", this, false);
    minmax->setNumMinorWidgets(2);
    minmax->setPacking(RowColumn::PACK_GRID);

    RelativeSlider* slider;
    slider = new RelativeSlider("RminSlider", minmax,
                                RelativeSlider::HORIZONTAL,
                                8.0 * style->fontHeight);
    slider->setValue(0.0);
    slider->setValueRange(-2.0, 2.0, 1.0);
    slider->getValueCallbacks().add(this, &RangeWidget::sliderCallback);

    slider = new RelativeSlider("RmaxSlider", minmax,
                                RelativeSlider::HORIZONTAL,
                                8.0 * style->fontHeight);
    slider->setValue(0.0);
    slider->setValueRange(-2.0, 2.0, 1.0);
    slider->getValueCallbacks().add(this, &RangeWidget::sliderCallback);

    minmax->manageChild();


//- create the shift slider group
    RowColumn* shift = new RowColumn("Rshift", this, false);
    shift->setNumMinorWidgets(1);

    slider = new RelativeSlider("RshiftSlider", shift,
                                RelativeSlider::HORIZONTAL,
                                8.0 * style->fontHeight);
    slider->setValue(0.0);
    slider->setValueRange(-2.0, 2.0, 1.0);
    slider->getValueCallbacks().add(this, &RangeWidget::sliderCallback);

    shift->manageChild();


//- finalize
    if(childManaged)
        manageChild();
}

RangeWidget::
~RangeWidget()
{
}


void RangeWidget::
setRange(double newMin, double newMax, bool propagate)
{
    min = newMin;
    max = newMax;

    updateLabels();

    if (propagate)
        callback();
}


Misc::CallbackList& RangeWidget::
getRangeChangedCallbacks()
{
    return rangeChangedCallbacks;
}

void RangeWidget::
updateLabels()
{
    rangeLabels[0]->setValue(min);
    rangeLabels[1]->setValue((min+max)*0.5);
    rangeLabels[2]->setValue(max);
}

void RangeWidget::
callback()
{
    RangeChangedCallbackData cbData(this, min, max);
    rangeChangedCallbacks.call(&cbData);
}

void RangeWidget::
sliderCallback(GLMotif::RelativeSlider::ValueCallbackData* cbData)
{
    static const double EPSILON = 0.000001;

    std::string sliderName = std::string(cbData->slider->getName());
    if (sliderName == "RminSlider")
    {
        min += cbData->value;
        min  = std::min(min, max-EPSILON);
    }
    else if (sliderName == "RmaxSlider")
    {
        max += cbData->value;
        max  = std::max(max, min+EPSILON);
    }
    else if (sliderName == "RshiftSlider")
    {
        min += cbData->value;
        max += cbData->value;
    }

    updateLabels();
    callback();
}


} //end namespace GLMotif
