#ifndef _ColorPicker_H_
#define _ColorPicker_H_


#include <vector>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GLMotif/Types.h>
#include <GLMotif/Widget.h>


namespace GLMotif {

class ColorPicker : public Widget
{
public:
    /** type for colors with opacity values */
    typedef GLColor<GLfloat,4> Color;

    /** base class for callback data sent by color picker */
    class CallbackData : public Misc::CallbackData
    {
    public:
        /** pointer to the color map that sent the callback */
        ColorPicker* colorPicker;

        CallbackData(ColorPicker* iColorPicker);
    };

    class ColorChangedCallbackData : public CallbackData
    {
    public:
        ColorChangedCallbackData(ColorPicker* iColorPicker,
            const Color& iOldColor, const Color& iNewColor);

        /** previously selected color */
        Color oldColor;
        /** new selected color */
        Color newColor;
    };

    ColorPicker(const char* iName, Container* iParent, bool iManageChild=true);
    virtual ~ColorPicker();

    /** set the current color of the picker */
    void setCurrentColor(const Color& currentColor);

    /** changes the margin width */
    void setMarginWidth(GLfloat newMarginWidth);
    /** sets a new preferred size */
    void setPreferredSize(const Vector& newPreferredSize);

    /** returns list of color map change callbacks */
    Misc::CallbackList& getColorChangedCallbacks();

private:
    /** the current color */
    Color color;
    /** the current value of the color */
    Scalar value;

    /** width of margin around color map area */
    GLfloat marginWidth;
    /** the color picker's preferred size */
    Vector preferredSize;
    /** position and size of color map area */
    Box colorMapAreaBox;

    /** list of callbacks to be called when the color changes */
    Misc::CallbackList colorChangedCallbacks;

    /** flag whether a color selector is being dragged */
    bool isDragging;


//- Inherited from Widget
public:
    virtual Vector calcNaturalSize() const;
    virtual void resize(const Box& newExterior);
    virtual void draw(GLContextData& contextData) const;
    virtual bool findRecipient(Event& event);
    virtual void pointerButtonDown(Event& event);
    virtual void pointerButtonUp(Event& event);
    virtual void pointerMotion(Event& event);
};

} //end namespace GLMotif


#endif //_ColorPicker_H_
