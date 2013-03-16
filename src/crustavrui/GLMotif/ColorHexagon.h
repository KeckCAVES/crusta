#ifndef _ColorHexagon_H_
#define _ColorHexagon_H_


#include <vector>

#include <crustavrui/vrui.h>


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
        CallbackData(ColorHexagon* iColorHexagon);

        /** pointer to the color map that sent the callback */
        ColorHexagon* hexagon;
    };

    class ColorChangedCallbackData : public CallbackData
    {
    public:
        ColorChangedCallbackData(ColorHexagon* iHexagon, const Color& iColor);

        /** new selected color */
        Color color;
    };

    ColorHexagon(const char* iName, Container* iParent,
                 bool iManageChild=true);
    virtual ~ColorHexagon();

    /** set the color value of the current hexagon */
    void setValue(const GLfloat& newValue);

    /** changes the margin width */
    void setMarginWidth(GLfloat newMarginWidth);
    /** sets a new preferred size */
    void setPreferredSize(const Vector& newPreferredSize);

    /** returns list of color map change callbacks */
    Misc::CallbackList& getColorChangedCallbacks();

private:
    /** compute the current color */
    Color computeCurColor();
    /** update the representation of the hexagon */
    void updateHexagon();
    /** generate the callback for a color change */
    void notifyColorChanged();
    /** determine the wedge and coordinates for a widget location */
    bool processPosition(const Point& pos);

    /** indices to the corners of the hexagon's wedges */
    static const int wedges[6][3];
    /** non-value scaled center and corner colors */
    static const Color baseColors[7];

    /** position of the hexagon center and corners */
    Vector corners[7];
    /** scaled color of the hexagon center and corners */
    Color colors[7];
    /** color value of the current hexagon */
    GLfloat value;

    /** wedge containing the current color */
    int curWedge;
    /** barycentric coordinates of the current color */
    GLfloat curCoords[3];

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
