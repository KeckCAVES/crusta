#include <crusta/GLMotif/ColorHexagon.h>

#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GLMotif/Container.h>
#include <GLMotif/Event.h>

#define LEFT_X  0.21132487
#define RIGHT_X 0.78867513

namespace GLMotif {

const int ColorHexagon::
wedges[6][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,5}, {0,5,6}, {0,6,1} };

const ColorHexagon::Color ColorHexagon::
baseColors[7] = { Color(1,1,1),
                  Color(1,0,0), Color(1,1,0), Color(0,1,0),
                  Color(0,1,1), Color(0,0,1), Color(1,0,1) };

ColorHexagon::CallbackData::
CallbackData(ColorHexagon* iHexagon) :
    hexagon(iHexagon)
{
}

ColorHexagon::ColorChangedCallbackData::
ColorChangedCallbackData(ColorHexagon* iHexagon, const Color& iColor) :
    CallbackData(iHexagon), color(iColor)
{
}

ColorHexagon::
ColorHexagon(const char* iName, Container* iParent, bool iManageChild) :
    Widget(iName, iParent, false),
    value(1.0f), curWedge(0),
    marginWidth(0.0), preferredSize(0.0, 0.0, 0.0), isDragging(false)
{
    /* Initialize corner vertices' z coordinate: */
    for(int i=0;i<7;++i)
        corners[i][2]=0.0f;
    curCoords[0] = 1.0f; curCoords[1] = 0.0f; curCoords[2] = 0.0f;

    updateHexagon();

    if (iManageChild)
        manageChild();
}

ColorHexagon::
~ColorHexagon()
{
}

void ColorHexagon::
setValue(const GLfloat& newValue)
{
    value = newValue;
    updateHexagon();
}

void ColorHexagon::
setMarginWidth(GLfloat newMarginWidth)
{
    /* Set the new margin width: */
    marginWidth = newMarginWidth;

    if (isManaged)
    {
        /* Try adjusting the widget size to accomodate the new margin width: */
        parent->requestResize(this, calcNaturalSize());
    }
    else
    {
        resize(Box(Vector(0.0f,0.0f,0.0f), calcNaturalSize()));
    }
}

void ColorHexagon::
setPreferredSize(const Vector& newPreferredSize)
{
    /* Set the new preferred size: */
    preferredSize = newPreferredSize;

    if (isManaged)
    {
        /* Try adjusting the widget size to accomodate the new preferred size:*/
        parent->requestResize(this, calcNaturalSize());
    }
    else
    {
        resize(Box(Vector(0.0f,0.0f,0.0f), calcNaturalSize()));
    }
}


Misc::CallbackList& ColorHexagon::
getColorChangedCallbacks()
{
    return colorChangedCallbacks;
}


ColorHexagon::Color ColorHexagon::
computeCurColor()
{
    Color c(0,0,0);
    for (int i=0; i<3; ++i)
    {
        for (int j=0; j<3; ++j)
        {
            c[i] += curCoords[j]*colors[wedges[curWedge][j]][i];
        }
    }
    return c;
}

void ColorHexagon::
updateHexagon()
{
    //compute the relevant x positions
    const GLfloat x0 = hexagonAreaBox.origin[0];
    const GLfloat x1 = x0 +  LEFT_X*hexagonAreaBox.size[0];
    const GLfloat x2 = x0 +     0.5*hexagonAreaBox.size[0];
    const GLfloat x3 = x0 + RIGHT_X*hexagonAreaBox.size[0];
    const GLfloat x4 = x0 + hexagonAreaBox.size[0];

    //compute the relevant y positions
    const GLfloat y0 = hexagonAreaBox.origin[1];
    const GLfloat y1 = y0 + 0.5 * hexagonAreaBox.size[1];
    const GLfloat y2 = y0 + hexagonAreaBox.size[1];

    //assign the proper x,y to the corner points
    corners[0][0] = x2; corners[0][1] = y1;
    corners[1][0] = x4; corners[1][1] = y1;
    corners[2][0] = x3; corners[2][1] = y2;
    corners[3][0] = x1; corners[3][1] = y2;
    corners[4][0] = x0; corners[4][1] = y1;
    corners[5][0] = x1; corners[5][1] = y0;
    corners[6][0] = x3; corners[6][1] = y0;

    //assign the proper scaled color to the corners
    for (int i=0; i<7; ++i)
    {
        for (int j=0; j<3; ++j)
        {
            colors[i][j] = value * baseColors[i][j];
        }
    }
}

void ColorHexagon::
notifyColorChanged()
{
    ColorChangedCallbackData cb(this, computeCurColor());
    colorChangedCallbacks.call(&cb);
}

bool ColorHexagon::
processPosition(const Point& pos)
{
    static const GLfloat EPSILON = 0.0001;

    //go through all the wedges, find containing wedge and compute coords
    for (int w=0; w<6; ++w)
    {
        //alias the current wedge's corners
        const Vector& c0 = corners[wedges[w][0]];
        const Vector& c1 = corners[wedges[w][1]];
        const Vector& c2 = corners[wedges[w][2]];

        //see http://en.wikipedia.org/wiki/Barycentric_coordinates_(mathematics)
        //compute the determinant
        GLfloat det = (c1[0]-c0[0])*(c2[1]-c0[1]) - (c2[0]-c0[0])*(c1[1]-c0[1]);
        if (Math::abs(det) < EPSILON)
            continue;
        det = 1.0f / det;

        //compute coordinates and check interior of wedge
        GLfloat coords[3];
        coords[0] =(c1[0]-pos[0])*(c2[1]-pos[1])-(c2[0]-pos[0])*(c1[1]-pos[1]);
        coords[0]*= det;
        if (coords[0]<0.0f || coords[0]>1.0f)
            continue;
        coords[1] =(c2[0]-pos[0])*(c0[1]-pos[1])-(c0[0]-pos[0])*(c2[1]-pos[1]);
        coords[1]*= det;
        if (coords[1]<0.0f || coords[1]>1.0f)
            continue;
        coords[2] =(c0[0]-pos[0])*(c1[1]-pos[1])-(c1[0]-pos[0])*(c0[1]-pos[1]);
        coords[2]*= det;
        if (coords[2]<0.0f || coords[2]>1.0f)
            continue;

        //set the current wedge and coordinates
        curWedge = w;
        for (int i=0; i<3; ++i)
            curCoords[i] = coords[i];
        return true;
    }

    //not contained in any wedge
    return false;
}


Vector ColorHexagon::
calcNaturalSize() const
{
    /* Return size of color hexagon area plus margin: */
    Vector result = preferredSize;
    result[0] += 2.0 * marginWidth;
    result[1] += 2.0 * marginWidth;

    return calcExteriorSize(result);
}

void ColorHexagon::
resize(const Box& newExterior)
{
    /* Resize the parent class widget: */
    Widget::resize(newExterior);

    /* Adjust the color hexagon area position: */
    hexagonAreaBox = getInterior();
    hexagonAreaBox.doInset(Vector(marginWidth,marginWidth,0.0f));

    /* Update the color hexagon area positions of all control points: */
    updateHexagon();
}

void ColorHexagon::
draw(GLContextData& contextData) const
{
    //draw the parent class widget
    Widget::draw(contextData);

    //draw the background parts (not covered by the hexagon)
    glColor(backgroundColor);
    glBegin(GL_TRIANGLES);
        glVertex(corners[2]);
        glVertex(corners[1]);
        glVertex(getInterior().getCorner(3));

        glVertex(getInterior().getCorner(2));
        glVertex(corners[4]);
        glVertex(corners[3]);

        glVertex(corners[4]);
        glVertex(getInterior().getCorner(0));
        glVertex(corners[5]);

        glVertex(corners[6]);
        glVertex(getInterior().getCorner(1));
        glVertex(corners[1]);
    glEnd();

    //backup OpenGL state
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);

    //draw the hexagon
    glBegin(GL_TRIANGLE_FAN);

    for (int i=0; i<7; ++i)
    {
        glColor(colors[i]);
        glVertex(corners[i]);
    }
    glColor(colors[1]);
    glVertex(corners[1]);

    glEnd();

    //restore OpenGL state
    glPopAttrib();
}

bool ColorHexagon::
findRecipient(Event& event)
{
    if(isDragging)
    {
        /* This event belongs to me! */
        return event.setTargetWidget(this, event.calcWidgetPoint(this));
    }
    else
    {
        return Widget::findRecipient(event);
    }
}

void ColorHexagon::
pointerButtonDown(Event& event)
{
    Point pos = event.getWidgetPoint().getPoint();
    if (processPosition(pos))
    {
        notifyColorChanged();
        isDragging = true;
    }
}

void ColorHexagon::
pointerButtonUp(Event& event)
{
    isDragging = false;
}

void ColorHexagon::
pointerMotion(Event& event)
{
    Point pos = event.getWidgetPoint().getPoint();
    if (processPosition(pos))
        notifyColorChanged();
}

} //end namespace GLMotif

