#include <crustavrui/GLMotif/ColorMapEditor.h>


#include <crustavrui/vrui.h>


namespace GLMotif {

ColorMapEditor::CallbackData::
CallbackData(ColorMapEditor* colorMapWidget) :
    editor(colorMapWidget)
{
}

ColorMapEditor::SelectedControlPointChangedCallbackData::
SelectedControlPointChangedCallbackData(ColorMapEditor* colorMapWidget,
                                        int oldControlPoint_,
                                        int newControlPoint_) :
    CallbackData(colorMapWidget), oldControlPoint(oldControlPoint_),
    newControlPoint(newControlPoint_)
{
    if (oldControlPoint==-1 && newControlPoint!=-1)
        reason = SELECT;
    else if (oldControlPoint!=-1 && newControlPoint!=-1)
        reason = SWITCH;
    else if (oldControlPoint!=-1 && newControlPoint==-1)
        reason = DESELECT;
}

ColorMapEditor::ColorMapChangedCallbackData::
ColorMapChangedCallbackData(ColorMapEditor* colorMapWidget) :
    CallbackData(colorMapWidget)
{
}


ColorMapEditor::
ColorMapEditor(const char* name, Container* parent, bool manageChild_) :
    Widget(name, parent, false),
    marginWidth(0.0f), preferredSize(0.0f,0.0f,0.0f),
    controlPointSize(marginWidth*0.5f),
    selectedControlPointColor(1.0f,0.0f,0.0f),
    selected(-1), isDragging(false)
{
    /* Initialize the control points: */
    updateControlPoints();

    /* Manage me: */
    if(manageChild_)
        manageChild();
}


void ColorMapEditor::
setMarginWidth(GLfloat newMarginWidth)
{
    //set the new margin width
    marginWidth = newMarginWidth;

    if (isManaged)
    {
        //try adjusting the widget size to accomodate the new margin width
        parent->requestResize(this, calcNaturalSize());
    }
    else
        resize(Box(Vector(0.0f,0.0f,0.0f), calcNaturalSize()));
}

void ColorMapEditor::
setPreferredSize(const Vector& newPreferredSize)
{
    //set the new preferred size
    preferredSize = newPreferredSize;

    if(isManaged)
    {
        //try adjusting the widget size to accomodate the new preferred size
        parent->requestResize(this, calcNaturalSize());
    }
    else
        resize(Box(Vector(0.0f,0.0f,0.0f), calcNaturalSize()));
}


void ColorMapEditor::
setControlPointSize(GLfloat newControlPointSize)
{
    controlPointSize = newControlPointSize;
}

void ColorMapEditor::
setSelectedControlPointColor(const Color& newSelectedControlPointColor)
{
    selectedControlPointColor = newSelectedControlPointColor;
}

void ColorMapEditor::
selectControlPoint(int controlPointIndex)
{
    if (controlPointIndex<static_cast<int>(controlPoints.size()))
    {
        SelectedControlPointChangedCallbackData cbData(this, selected,
                                                       controlPointIndex);
        selected = controlPointIndex;
        selectedControlPointChangedCallbacks.call(&cbData);
    }
}

void ColorMapEditor::
deleteSelectedControlPoint()
{
    if (selected!=-1 && selected!=0 &&
        selected!=static_cast<int>(controlPoints.size()))
    {
        //delete the entry
        Misc::ColorMap::Points& points = colorMap.getPoints();
        points.erase(points.begin()+selected);

        //unselect the control point
        SelectedControlPointChangedCallbackData selectCbData(this,selected,-1);
        selected = -1;
        selectedControlPointChangedCallbacks.call(&selectCbData);

        //update all control points
        updateControlPoints();

        //call the color map change callbacks
        ColorMapChangedCallbackData cbData(this);
        colorMapChangedCallbacks.call(&cbData);
    }
}

bool ColorMapEditor::
hasSelectedControlPoint() const
{
    return selected != -1;
}


int ColorMapEditor::
insertControlPoint(double value, bool propagate)
{
    //determine the color and entry location
    Misc::ColorMap::Point newPoint(value, Misc::ColorMap::Color());
    Misc::ColorMap::Points::iterator it = colorMap.interpolate(newPoint);

    //insert a new entry into the colormap
    it = colorMap.getPoints().insert(it, newPoint);

    //update all control points
    updateControlPoints(propagate);

    if (propagate)
    {
        //call the color map change callbacks
        ColorMapChangedCallbackData cbData(this);
        colorMapChangedCallbacks.call(&cbData);
    }

    return it - colorMap.getPoints().begin();
}


Misc::ColorMap::Point& ColorMapEditor::
getSelectedControlPoint()
{
    return colorMap.getPoints()[selected];
}
Misc::ColorMap& ColorMapEditor::
getColorMap()
{
    return colorMap;
}

void ColorMapEditor::
touch()
{
    //update all control points
    updateControlPoints();

    //call the color map change callbacks: */
    ColorMapChangedCallbackData cbData(this);
    colorMapChangedCallbacks.call(&cbData);
}

Misc::CallbackList& ColorMapEditor::
getSelectedControlPointChangedCallbacks()
{
    return selectedControlPointChangedCallbacks;
}
Misc::CallbackList& ColorMapEditor::
getColorMapChangedCallbacks()
{
    return colorMapChangedCallbacks;
}





void ColorMapEditor::
updateControlPoints(bool propagate)
{
//- make sure the control points match the managed color map
    Misc::ColorMap::Points& points = colorMap.getPoints();
    int numPoints                  = static_cast<int>(points.size());

    if (static_cast<int>(controlPoints.size()) != numPoints)
    {
        //deselect
        if (hasSelectedControlPoint())
        {
            if (propagate)
            {
                SelectedControlPointChangedCallbackData cbData(this,
                                                               selected, -1);
                selected = -1;
                selectedControlPointChangedCallbacks.call(&cbData);
            }
            else
                selected = -1;
        }

        //reassign the control points
        controlPoints.resize(numPoints);
    }

//- update the visual representation
    GLfloat x1     = colorMapAreaBox.getCorner(0)[0];
    GLfloat x2     = colorMapAreaBox.getCorner(1)[0];
    GLfloat width  = x2 - x1;
    GLfloat y1     = colorMapAreaBox.getCorner(0)[1];
    GLfloat y2     = colorMapAreaBox.getCorner(2)[1];
    GLfloat height = y2 - y1;

    Misc::ColorMap::ValueRange range    = colorMap.getValueRange();
    Misc::ColorMap::Value invRangeWidth = 1.0 / (range.max - range.min);

    for (int i=0; i<numPoints; ++i)
    {
        ControlPoint& cp               = controlPoints[i];
        const Misc::ColorMap::Point& p = points[i];
        cp[0] = x1 + GLfloat(invRangeWidth*(p.value - range.min)) * width;
        cp[1] = y1 + p.color[3] * height;
        cp[2] = colorMapAreaBox.getCorner(0)[2];
    }
}


Vector ColorMapEditor::
calcNaturalSize() const
{
    //return size of color map area plus margin
    Vector result = preferredSize;
    result[0] += 2.0f * marginWidth;
    result[1] += 2.0f * marginWidth;

    return calcExteriorSize(result);
}

void ColorMapEditor::
resize(const Box& newExterior)
{
    //resize the parent class widget
    Widget::resize(newExterior);

    //adjust the color map area position
    colorMapAreaBox = getInterior();
    colorMapAreaBox.doInset(Vector(marginWidth, marginWidth, 0.0f));

    //update the color map area positions of all control points
    updateControlPoints();
}

void ColorMapEditor::
draw(GLContextData& contextData) const
{
    const Misc::ColorMap::Points& points = colorMap.getPoints();
    int numPoints = static_cast<int>(points.size());

//- draw the parent class widget
    Widget::draw(contextData);

    //y-value at the bottom of the map area
    GLfloat y1 = colorMapAreaBox.getCorner(0)[1];
    //y-value at the top of the map area
    GLfloat y2 = colorMapAreaBox.getCorner(2)[1];
    GLfloat z  = colorMapAreaBox.getCorner(0)[2];

//- draw the margin area with the background color
    glColor(backgroundColor);
    glBegin(GL_QUADS);
    glNormal3f(0.0f,0.0f,1.0f);
        glVertex(getInterior().getCorner(2));
        glVertex(colorMapAreaBox.getCorner(2));
        glVertex(colorMapAreaBox.getCorner(3));
        glVertex(getInterior().getCorner(3));

        glVertex(getInterior().getCorner(0));
        glVertex(colorMapAreaBox.getCorner(0));
        glVertex(colorMapAreaBox.getCorner(2));
        glVertex(getInterior().getCorner(2));

        glVertex(getInterior().getCorner(1));
        glVertex(colorMapAreaBox.getCorner(1));
        glVertex(colorMapAreaBox.getCorner(0));
        glVertex(getInterior().getCorner(0));

        glVertex(getInterior().getCorner(3));
        glVertex(colorMapAreaBox.getCorner(3));
        glVertex(colorMapAreaBox.getCorner(1));
        glVertex(getInterior().getCorner(1));
    glEnd();

    //disable lighting for the color area
    GLboolean lightingEnabled = glIsEnabled(GL_LIGHTING);
    if (lightingEnabled)
        glDisable(GL_LIGHTING);

//- draw the color map area
    glBegin(GL_QUAD_STRIP);
    for (int i=0; i<numPoints; ++i)
    {
        glColor(points[i].color);
        glVertex3f(controlPoints[i][0], y2, z);
        glVertex3f(controlPoints[i][0], y1, z);
    }
    glEnd();

//- draw the control point interconnecting lines
    GLfloat lineWidth;
    glGetFloatv(GL_LINE_WIDTH,&lineWidth);

    //outer line
    glLineWidth(3.0f);
    glColor3f(0.0f,0.0f,0.0f);
    glBegin(GL_LINE_STRIP);
    for (int i=0; i<numPoints; ++i)
    {
        glVertex3f(controlPoints[i][0], controlPoints[i][1],
                   z + marginWidth*0.25f);
    }
    glEnd();

    //inner line
    glLineWidth(1.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_LINE_STRIP);
    for (int i=0; i<numPoints; ++i)
    {
        glVertex3f(controlPoints[i][0], controlPoints[i][1],
                   z + marginWidth*0.25f);
    }
    glEnd();

    //restore the line width
    glLineWidth(lineWidth);

    //restore the lighting flag
    if (lightingEnabled)
        glEnable(GL_LIGHTING);


//- draw a button for each control point
    GLfloat nl = 1.0f / Math::sqrt(3.0f);
    glBegin(GL_TRIANGLES);
    for (int i=0; i<numPoints; ++i)
    {
        const ControlPoint& cp = controlPoints[i];

        if (i == selected)
        {
            if (isDragging)
            {
                Color c(1.0f-selectedControlPointColor[0],
                        1.0f-selectedControlPointColor[1],
                        1.0f-selectedControlPointColor[2],
                        selectedControlPointColor[3]);
                glColor(c);
            }
            else
                glColor(selectedControlPointColor);
        }
        else
            glColor(foregroundColor);

        glNormal3f(-nl, nl, nl);
        glVertex3f(cp[0]-controlPointSize, cp[1], z);
        glVertex3f(cp[0], cp[1], z+controlPointSize);
        glVertex3f(cp[0], cp[1]+controlPointSize, z);
        glNormal3f(nl, nl, nl);
        glVertex3f(cp[0], cp[1]+controlPointSize, z);
        glVertex3f(cp[0], cp[1], z+controlPointSize);
        glVertex3f(cp[0]+controlPointSize, cp[1], z);
        glNormal3f(nl, -nl, nl);
        glVertex3f(cp[0]+controlPointSize,cp[1], z);
        glVertex3f(cp[0], cp[1], z+controlPointSize);
        glVertex3f(cp[0], cp[1]-controlPointSize, z);
        glNormal3f(-nl, -nl, nl);
        glVertex3f(cp[0], cp[1]-controlPointSize, z);
        glVertex3f(cp[0], cp[1], z+controlPointSize);
        glVertex3f(cp[0]-controlPointSize, cp[1], z);
    }
    glEnd();
}

bool ColorMapEditor::
findRecipient(Event& event)
{
    if (isDragging)
    {
        /* This event belongs to me! */
        return event.setTargetWidget(this,event.calcWidgetPoint(this));
    }
    else
        return Widget::findRecipient(event);
}

void ColorMapEditor::
pointerButtonDown(Event& event)
{
    //find the closest control point to the event's location
    GLfloat minDist2 = Math::sqr(controlPointSize*1.5f);
    int newSelected  = -1;

    GLfloat x1 = colorMapAreaBox.getCorner(0)[0];
    GLfloat x2 = colorMapAreaBox.getCorner(1)[0];

    int numPoints = static_cast<int>(controlPoints.size());
    for (int i=0; i<numPoints; ++i)
    {
        const Point& cp = controlPoints[i];
        GLfloat dist2 = Geometry::sqrDist(cp,event.getWidgetPoint().getPoint());
        if (minDist2 > dist2)
        {
            minDist2    = dist2;
            newSelected = i;
            for (int j=0; j<2; ++j)
            {
                dragOffset[j] = Point::Vector::Scalar(
                    event.getWidgetPoint().getPoint()[j] - cp[j]);
            }
            dragOffset[2] = Point::Vector::Scalar(0);
        }
    }

    //create a new control point if none was selected
    Misc::ColorMap::ValueRange vr = colorMap.getValueRange();
    if (newSelected == -1)
    {
        //compute the value of the current click-point
        double remap    = (vr.max - vr.min) / double(x2 - x1);
        double newValue = (event.getWidgetPoint().getPoint()[0] - double(x1));
        newValue        = vr.min + newValue * remap;
        //clamp it to the value range
        newValue        = std::max(newValue, vr.min);
        newValue        = std::min(newValue, vr.max);

        //select the inserted point
        selected = insertControlPoint(newValue);
    }
    else if(newSelected == selected)
    {
        //start dragging the selected control point
        isDragging = true;
    }
    else
    {
        //call callbacks if selected control point has changed
        SelectedControlPointChangedCallbackData cbData(this, selected,
                                                       newSelected);
        selected = newSelected;
        selectedControlPointChangedCallbacks.call(&cbData);
    }
}

void ColorMapEditor::
pointerButtonUp(Event&)
{
    isDragging=false;
}

void ColorMapEditor::
pointerMotion(Event& event)
{
    if (isDragging)
    {
        //calculate the new value and opacity
        GLfloat x1 = colorMapAreaBox.getCorner(0)[0];
        GLfloat x2 = colorMapAreaBox.getCorner(1)[0];
        GLfloat y1 = colorMapAreaBox.getCorner(0)[1];
        GLfloat y2 = colorMapAreaBox.getCorner(2)[1];

        Point p = event.getWidgetPoint().getPoint() - dragOffset;
        const Misc::ColorMap::ValueRange vr = colorMap.getValueRange();
        double remap = (vr.max-vr.min) / (x2-x1);
        double newValue = vr.min + (p[0] - double(x1)) * remap;

        //clamp the new value to the valid interval range
        Misc::ColorMap::Points& points = colorMap.getPoints();

        if (selected == 0)
            newValue = vr.min;
        else if(selected == static_cast<int>(controlPoints.size()-1))
            newValue = vr.max;
        else if(newValue < points[selected-1].value)
            newValue = points[selected-1].value;
        else if(newValue > points[selected+1].value)
            newValue = points[selected+1].value;

        //clamp the new opacity to the valid interval
        GLfloat newOpacity = (p[1]-y1) / (y2-y1);
        newOpacity = std::max(newOpacity, 0.0f);
        newOpacity = std::min(newOpacity, 1.0f);

        //assign the new properties
        points[selected].value    = newValue;
        points[selected].color[3] = newOpacity;

        //update all control points
        updateControlPoints();

        //call the color map change callbacks
        ColorMapChangedCallbackData cbData(this);
        colorMapChangedCallbacks.call(&cbData);
    }
}


} //end namespace GLMotif
