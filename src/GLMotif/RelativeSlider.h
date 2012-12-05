#ifndef _GLMotif_RelativeSlider_H_
#define _GLMotif_RelativeSlider_H_


#include <GLMotif/Slider.h>
#include <Misc/TimerEventScheduler.h>


namespace GLMotif {


class RelativeSlider : public Slider
{
public:
    class ValueCallbackData : public Misc::CallbackData
    {
    public:
        /** enumerated type for different production reasons */
        enum ProductionReason
        {
            CLICKED_LEFT,
            CLICKED_RIGHT,
            DRAGGED
        };

        /** pointer to the slider widget causing the event */
        RelativeSlider* slider;
        /** reason for this value change */
        ProductionReason reason;
        /** current slider value */
        float value;

        ValueCallbackData(RelativeSlider* relativeSlider,
                          ProductionReason productionReason,
                          float valueNew);
    };

    RelativeSlider(const char* name, Container* parent, Orientation orientation,
                   float shaftLength, bool sManageChild =true);

    virtual void pointerButtonDown(Event& event);
    virtual void pointerButtonUp(Event& event);
    virtual void pointerMotion(Event& event);

    /** retrieve the list of registered value callbacks */
    Misc::CallbackList& getValueCallbacks();

protected:
    void callback(const ValueCallbackData::ProductionReason& reason);

    /** callback for repeat timer events */
    void repeatTimerEventCallback(
        Misc::TimerEventScheduler::CallbackData* cbData);

    /** list of callbacks registered for the value production */
    Misc::CallbackList valueCallbacks;

    /** flag continous value production */
    bool isRepeating;
    //time at which the next drag-repeat event was scheduled
    double nextRepeatEventTime;
};


} //end namespace GLMotif


#endif //_GLMotif_RelativeSlider_H_
