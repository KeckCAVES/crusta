#include <GLMotif/ColorMapInOut.h>


#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GLMotif/Event.h>


namespace GLMotif {


ColorMapInOut::CallbackData::
CallbackData(ColorMapInOut* inout_) :
    inout(inout_)
{
}

ColorMapInOut::InCallbackData::
InCallbackData(ColorMapInOut* inout_) :
    CallbackData(inout_)
{
}

ColorMapInOut::OutCallbackData::
OutCallbackData(ColorMapInOut* inout_) :
    CallbackData(inout_)
{
}

ColorMapInOut::
ColorMapInOut(const char* name_, Container* parent_, const char* label_,
              const Misc::ColorMap* const initMap_, bool manageChild_) :
    Button(name_, parent_, label_, manageChild_)
{
    if (initMap_ != NULL)
        colorMap = *initMap_;
    updateStops();
}


const Misc::ColorMap& ColorMapInOut::
getColorMap()
{
    return colorMap;
}

void ColorMapInOut::
setColorMap(const Misc::ColorMap& map)
{
    colorMap = map;
    updateStops();
}


Misc::CallbackList& ColorMapInOut::
getInCallbacks()
{
    return inCallbacks;
}

Misc::CallbackList& ColorMapInOut::
getOutCallbacks()
{
    return outCallbacks;
}


void ColorMapInOut::
updateStops()
{
//- make sure the control points match the managed color map
    const Misc::ColorMap::Points& points = colorMap.getPoints();
    const int numPoints                  = static_cast<int>(points.size());

    stops.resize(numPoints);

//- update the visual representation
    GLfloat x1     = getInterior().getCorner(0)[0];
    GLfloat x2     = getInterior().getCorner(1)[0];
    GLfloat width  = x2 - x1;

    const Misc::ColorMap::ValueRange range    = colorMap.getValueRange();
    const Misc::ColorMap::Value invRangeWidth = 1.0 / (range.max - range.min);

    for (int i=0; i<numPoints; ++i)
    {
        const Misc::ColorMap::Point& p = points[i];
        stops[i] = x1 + GLfloat((p.value - range.min)*invRangeWidth) * width;
    }
}


void ColorMapInOut::
resize(const Box& newExterior)
{
    //adjust the label
    Label::resize(newExterior);
    //adjust the stops
    updateStops();
}

void ColorMapInOut::
draw(GLContextData& contextData) const
{
    //y-value at the bottom of the map area
    GLfloat y1 = getInterior().getCorner(0)[1];
    //y-value at the top of the map area
    GLfloat y2 = getInterior().getCorner(2)[1];
    GLfloat z  = getInterior().getCorner(0)[2];

    //draw parent class decorations (from the point of view of a Label)
    Widget::draw(contextData);

    //draw the label margin (code from Label)
    glColor(backgroundColor);
    glBegin(GL_QUAD_STRIP);
    glNormal3f(0.0f,0.0f,1.0f);
    glVertex(labelBox.getCorner(0));
    glVertex(getInterior().getCorner(0));
    glVertex(labelBox.getCorner(1));
    glVertex(getInterior().getCorner(1));
    glVertex(labelBox.getCorner(3));
    glVertex(getInterior().getCorner(3));
    glVertex(labelBox.getCorner(2));
    glVertex(getInterior().getCorner(2));
    glVertex(labelBox.getCorner(0));
    glVertex(getInterior().getCorner(0));
    glEnd();

    //draw the color mapped interior
    const Misc::ColorMap::Points& points = colorMap.getPoints();
    glBegin(GL_QUAD_STRIP);
    const int numPoints = static_cast<int>(points.size());
    for (int i=0; i<numPoints; ++i)
    {
        glColor(points[i].color);
        glVertex3f(stops[i], y2, z);
        glVertex3f(stops[i], y1, z);
    }
    glEnd();

    //draw the label texture
    drawLabel(contextData);
}

void ColorMapInOut::
pointerButtonUp(Event& event)
{
    //only process events that are for us
    if (event.getTargetWidget() == this)
    {
        //figure out which side of the button we're on
        Point p = event.getWidgetPoint().getPoint();
        for (int i=0; i<2; ++i)
        {
            p[i]=(p[i] - getInterior().getCorner(1)[i]) /
                 (getInterior().getCorner(1)[i]-getInterior().getCorner(2)[i]);
        }

        if (p[0]<p[1])
        {
            InCallbackData cbData(this);
            inCallbacks.call(&cbData);
        }
        else
        {
            OutCallbackData cbData(this);
            outCallbacks.call(&cbData);
        }
    }

    //disarm the button
    setArmed(false);
}


} //end namespace GLMotif
