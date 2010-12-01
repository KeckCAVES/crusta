#include <GLMotif/RelativeSlider.h>


#include <GLMotif/Event.h>
#include <GLMotif/WidgetManager.h>


namespace GLMotif {



RelativeSlider::ValueCallbackData::
ValueCallbackData(RelativeSlider* relativeSlider,
                         ProductionReason productionReason,
                         float newValue) :
    slider(relativeSlider), reason(productionReason),
    value(newValue)
{
}


RelativeSlider::
RelativeSlider(const char* name, Container* parent, Orientation orientation,
               float shaftLength, bool manageChild) :
    Slider(name, parent, orientation, shaftLength, manageChild),
    isRepeating(false), nextRepeatEventTime(0.0)
{
}


void RelativeSlider::
pointerButtonDown(Event& event)
{
    //where inside the widget did the event hit?
    int dimension = orientation==HORIZONTAL ? 0 : 1 ;
    float picked = event.getWidgetPoint().getPoint()[dimension];

    //hit left of the slider?
    if(picked < sliderBox.origin[dimension])
    {
        value = -valueIncrement;
        positionSlider();

        callback(ValueCallbackData::CLICKED_LEFT);

        /* Schedule a timer event for click repeat: */
        isRepeating = true;
        Misc::TimerEventScheduler* tes = getManager()->getTimerEventScheduler();
        nextRepeatEventTime = tes->getCurrentTime() + 0.5;
        tes->scheduleEvent(nextRepeatEventTime, this,
                           &RelativeSlider::repeatTimerEventCallback);
    }
    //hit right of the slider?
    else if(picked > sliderBox.origin[dimension]+sliderBox.size[dimension])
    {
        value = valueIncrement;
        positionSlider();

        callback(ValueCallbackData::CLICKED_RIGHT);

        /* Schedule a timer event for click repeat: */
        isRepeating = true;
        Misc::TimerEventScheduler* tes = getManager()->getTimerEventScheduler();
        nextRepeatEventTime = tes->getCurrentTime() + 0.5;
        tes->scheduleEvent(nextRepeatEventTime, this,
                           &RelativeSlider::repeatTimerEventCallback);
    }
    //hit slider!
    else
    {
        /* Start dragging: */
        dragOffset = sliderBox.origin[dimension]-picked;
        startDragging(event);

        isRepeating = true;

        /* Schedule a timer event for drag repeat: */
        Misc::TimerEventScheduler* tes = getManager()->getTimerEventScheduler();
        nextRepeatEventTime = tes->getCurrentTime() + 0.05;
        tes->scheduleEvent(nextRepeatEventTime, this,
                           &RelativeSlider::repeatTimerEventCallback);
    }
}

void RelativeSlider::
pointerButtonUp(Event& event)
{
    value = 0.0f;
    positionSlider();

    isRepeating = false;
    stopDragging(event);

    /* Cancel any pending click-repeat events: */
    Misc::TimerEventScheduler* tes = getManager()->getTimerEventScheduler();
    tes->removeEvent(nextRepeatEventTime, this,
                     &RelativeSlider::repeatTimerEventCallback);
}

void RelativeSlider::
pointerMotion(Event& event)
{
    if (isDragging)
    {
        /* Update the slider value and position: */
        int dimension = orientation==HORIZONTAL ? 0 : 1 ;
        float newSliderPosition =
            event.getWidgetPoint().getPoint()[dimension] + dragOffset;

        /* Calculate the new slider value: */
        value = (newSliderPosition-shaftBox.origin[dimension]) *
                (valueMax-valueMin) /
                (shaftBox.size[dimension]-sliderLength) +
                valueMin;

        if (value < valueMin)
            value = valueMin;
        else if (value > valueMax)
            value = valueMax;

        positionSlider();
    }
}


Misc::CallbackList& RelativeSlider::
getValueCallbacks()
{
    return valueCallbacks;
}


void RelativeSlider::
callback(const ValueCallbackData::ProductionReason& reason)
{
    float relativeValue;
    if (value<0)
        relativeValue = -(pow(2.0f, Math::abs(value)) - 1.0f);
    else
        relativeValue =   pow(2.0f, Math::abs(value)) - 1.0f;

    ValueCallbackData cbData(this, reason, relativeValue);
    valueCallbacks.call(&cbData);
}

void RelativeSlider::
repeatTimerEventCallback(Misc::TimerEventScheduler::CallbackData* cbData)
{

    /* Only react to event if still in click-repeat mode: */
    if(isRepeating)
    {
        callback(ValueCallbackData::DRAGGED);

        Misc::TimerEventScheduler* tes = getManager()->getTimerEventScheduler();
        nextRepeatEventTime = tes->getCurrentTime() + 0.05;
        tes->scheduleEvent(nextRepeatEventTime, this,
                           &RelativeSlider::repeatTimerEventCallback);
    }
}


} //end namespace GLMotif
