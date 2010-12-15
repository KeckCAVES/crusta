#include <GLMotif/RangeEditor.h>


#include <string>

#include <GLMotif/Label.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/TextField.h>
#include <GLMotif/WidgetManager.h>


namespace GLMotif {


RangeEditor::CallbackData::
CallbackData(RangeEditor* editor) :
    rangeEditor(editor)
{
}

RangeEditor::RangeChangedCallbackData::
RangeChangedCallbackData(RangeEditor* editor, double minimum, double maximum) :
    CallbackData(editor), min(minimum), max(maximum)
{
}


RangeEditor::
RangeEditor(const char* name, Container* parent, bool childManaged) :
    RowColumn(name, parent, false)
{
///\todo allow changing the preferred size through API
const Vector preferredSize(4.0f, 0.0f, 0.0f);

    const StyleSheet* style = getManager()->getStyleSheet();

    setNumMinorWidgets(1);


///\todo fix the layout. Want filler spaces in between the range labels
//- create the min max labels group
    RowColumn* labels = new RowColumn("Rlabels", this, false);
    labels->setNumMinorWidgets(5);
    labels->setColumnWeight(2, 1.0f);

    new Label("RminLabelName", labels, "Min:");

    rangeLabels[0] = new TextField("RminLabel", labels, 9);
    rangeLabels[0]->setFloatFormat(TextField::FIXED);
    rangeLabels[0]->setFieldWidth(9);
    rangeLabels[0]->setPrecision(2);

    Margin* centerMargin = new Margin("RcenterMargin", labels, false);
    centerMargin->setAlignment(Alignment::HCENTER);
    rangeLabels[1] = new TextField("RcenterLabel", centerMargin, 9);
    rangeLabels[1]->setFloatFormat(TextField::FIXED);
    rangeLabels[1]->setFieldWidth(9);
    rangeLabels[1]->setPrecision(2);
    centerMargin->manageChild();

    rangeLabels[2] = new TextField("RmaxLabel", labels, 9);
    rangeLabels[2]->setHAlignment(GLFont::Left);
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
                                style->fontHeight*preferredSize[0]);
    slider->setValue(0.0);
    slider->setValueRange(-2.0, 2.0, 1.0);
    slider->getValueCallbacks().add(this, &RangeEditor::sliderCallback);

    slider = new RelativeSlider("RmaxSlider", minmax,
                                RelativeSlider::HORIZONTAL,
                                style->fontHeight*preferredSize[0]);
    slider->setValue(0.0);
    slider->setValueRange(-2.0, 2.0, 1.0);
    slider->getValueCallbacks().add(this, &RangeEditor::sliderCallback);

    minmax->manageChild();


//- create the shift slider group
    RowColumn* shift = new RowColumn("Rshift", this, false);
    shift->setNumMinorWidgets(1);

    slider = new RelativeSlider("RshiftSlider", shift,
                                RelativeSlider::HORIZONTAL,
                                style->fontHeight*2.0f*preferredSize[0]);
    slider->setValue(0.0);
    slider->setValueRange(-2.0, 2.0, 1.0);
    slider->getValueCallbacks().add(this, &RangeEditor::sliderCallback);

    shift->manageChild();

//- finalize
    if(childManaged)
        manageChild();
}

RangeEditor::
~RangeEditor()
{
}


void RangeEditor::
shift(double newMin, bool propagate)
{
    double trans = newMin - min;
    setRange(newMin, max+trans, propagate);
}

void RangeEditor::
setMin(double newMin, bool propagate)
{
    setRange(newMin, max, propagate);
}

void RangeEditor::
setMax(double newMax, bool propagate)
{
    setRange(min, newMax, propagate);
}

void RangeEditor::
setRange(double newMin, double newMax, bool propagate)
{
    min = newMin;
    max = newMax;

    updateLabels();

    if (propagate)
        callback();
}


Misc::CallbackList& RangeEditor::
getRangeChangedCallbacks()
{
    return rangeChangedCallbacks;
}

void RangeEditor::
updateLabels()
{
    rangeLabels[0]->setValue(min);
    rangeLabels[1]->setValue((min+max)*0.5);
    rangeLabels[2]->setValue(max);
}

void RangeEditor::
callback()
{
    RangeChangedCallbackData cbData(this, min, max);
    rangeChangedCallbacks.call(&cbData);
}

void RangeEditor::
sliderCallback(RelativeSlider::ValueCallbackData* cbData)
{
    static const double EPSILON = 0.000001;

    std::string sliderName = std::string(cbData->slider->getName());
    if (sliderName == "RminSlider")
    {
        min += cbData->value;
        min  = std::min(min, max-EPSILON);
        if (cbData->reason==RelativeSlider::ValueCallbackData::CLICKED_LEFT ||
            cbData->reason==RelativeSlider::ValueCallbackData::CLICKED_RIGHT)
        {
            int factor = int(min / cbData->value);
            min = factor * cbData->value;
        }
    }
    else if (sliderName == "RmaxSlider")
    {
        max += cbData->value;
        max  = std::max(max, min+EPSILON);
        if (cbData->reason==RelativeSlider::ValueCallbackData::CLICKED_LEFT ||
            cbData->reason==RelativeSlider::ValueCallbackData::CLICKED_RIGHT)
        {
            int factor = int(max / cbData->value);
            max = factor * cbData->value;
        }
    }
    else if (sliderName == "RshiftSlider")
    {
        min += cbData->value;
        max += cbData->value;
        if (cbData->reason==RelativeSlider::ValueCallbackData::CLICKED_LEFT ||
            cbData->reason==RelativeSlider::ValueCallbackData::CLICKED_RIGHT)
        {
            int factor = int(min / cbData->value);
            min = factor * cbData->value;
            factor = int(max / cbData->value);
            max = factor * cbData->value;
        }
    }

    updateLabels();
    callback();
}


} //end namespace GLMotif
