#ifndef _GLMotif_ColorMapInOut_H_
#define _GLMotif_ColorMapInOut_H_


#include <GLMotif/Button.h>
#include <crusta/Misc/ColorMap.h>


namespace GLMotif {


class ColorMapInOut : public Button
{
public:
    /** base class for in/out events */
    class CallbackData : public Misc::CallbackData
    {
    public:
        CallbackData(ColorMapInOut* inout_);
        ColorMapInOut*  inout;
    };

    /** callback data produced when data is to be sent to the in/out */
    class InCallbackData : public CallbackData
    {
    public:
        InCallbackData(ColorMapInOut* inout_);
    };

    /** callback data produced when data is to be retrieved from the in/out */
    class OutCallbackData : public CallbackData
    {
    public:
        OutCallbackData(ColorMapInOut* inout_);
    };

    ColorMapInOut(const char* name_, Container* parent_, const char* label_,
                  const Misc::ColorMap* const initMap_=NULL,
                  bool manageChild_=true);

    const Misc::ColorMap& getColorMap();
    void setColorMap(const Misc::ColorMap& map);

    /** retrieve the in callback list */
    Misc::CallbackList& getInCallbacks();
    /** retrieve the out callback list */
    Misc::CallbackList& getOutCallbacks();

protected:
    typedef GLfloat           Stop;
    typedef std::vector<Stop> Stops;

    /** updates the widget positions of all stops */
    void updateStops();

    /** the value marks of the color map in widget coordinates */
    Stops stops;
    /** the color map data stored by this in/out */
    Misc::ColorMap colorMap;
    /** list of callbacks to be called for setting the color map */
    Misc::CallbackList inCallbacks;
    /** list of callbacks to be called for retrieving the color map */
    Misc::CallbackList outCallbacks;

//- inherited from Widget
public:
    virtual void resize(const Box& newExterior);
    virtual void draw(GLContextData& contextData) const;
    virtual void pointerButtonUp(Event& event);
};


} //end namespace GLMotif


#endif //_GLMotif_ColorMapInOut_H_
