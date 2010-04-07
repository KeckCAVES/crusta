#ifndef _ColorHexagon_H_
#define _ColorHexagon_H_


#include <vector>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GLMotif/Types.h>
#include <GLMotif/Widget.h>


namespace GLMotif {
    
class ColorHexagon : public Widget
{
public:
    /** type for colors with opacity values */
    typedef GLColor<GLfloat, 3> Color;
    
    /** base class for callback data sent by color picker */
    class CallbackData : public Misc::CallbackData
    {
    public:
        /** pointer to the color map that sent the callback */
        ColorHexagon* ColorHexagon;
        
        CallbackData(ColorHexagon* iColorHexagon);
    };
    
    class ColorChangedCallbackData : public CallbackData
    {
    public:
        ColorChangedCallbackData(ColorHexagon* iColorHexagon,
            const Color& iOldColor, const Color& iNewColor);
        
        /** previously selected color */
        Color oldColor;
        /** new selected color */
        Color newColor;
    };
    
    ColorHexagon(const char* iName, Container* iParent,
                 bool iManageChild=true);
    virtual ~ColorHexagon();

    /** set the color value of the current hexagon */
    void setValue(Scalar newValue);
    
    /** changes the margin width */
    void setMarginWidth(GLfloat newMarginWidth);
    /** sets a new preferred size */
    void setPreferredSize(const Vector& newPreferredSize);
    
    /** returns list of color map change callbacks */
    Misc::CallbackList& getColorChangedCallbacks();
    
private:
    /** update the representation of the hexagon */
    void updateHexagon();

    /** non-value scaled corner colors */
    static const Color colors[6];

    /** color value of the current hexagon */
    Scalar value;

    /** width of margin around color map area */
    GLfloat marginWidth;
    /** the color picker's preferred size */
    Vector preferredSize;
    /** position and size of color hexagon area */
    Box hexagonAreaBox;
    
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


#endif //_ColorHexagon_H_
