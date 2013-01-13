#include <crusta/Misc/ColorMap.h>


#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>

#include <GL/GLColorMap.h>
#include <Math/Constants.h>


namespace Misc {


static const ColorMap::Point blackPoints[] = {
    ColorMap::Point(0.0, ColorMap::Color(0.0f,0.0f,0.0f,0.0f)),
    ColorMap::Point(1.0, ColorMap::Color(0.0f,0.0f,0.0f,0.0f)) };
static const ColorMap::Point greyScalePoints[] = {
    ColorMap::Point(0.0, ColorMap::Color(0.0f,0.0f,0.0f,0.0f)),
    ColorMap::Point(1.0, ColorMap::Color(1.0f,1.0f,1.0f,1.0f)) };
static const ColorMap::Point whitePoints[] = {
    ColorMap::Point(0.0, ColorMap::Color(1.0f,1.0f,1.0f,1.0f)),
    ColorMap::Point(1.0, ColorMap::Color(1.0f,1.0f,1.0f,1.0f)) };

const ColorMap ColorMap::black(2, blackPoints);
const ColorMap ColorMap::greyScale(2, greyScalePoints);
const ColorMap ColorMap::white(2, whitePoints);

    
struct PointValueLessThan
{
    bool operator() (const ColorMap::Point& left, const ColorMap::Value& right)
    { return left.value < right; }
    bool operator() (const ColorMap::Value& left, const ColorMap::Point& right)
    { return left < right.value; }
};

template <typename Iterator>
static Iterator
interpolateMap(const Iterator& begin, const Iterator& end,
            const ColorMap::Value& value, ColorMap::Color& color)
{
    //find the two control points on either side of the value
    Iterator upper = std::lower_bound(begin, end, value, PointValueLessThan());
    Iterator lower = upper;
    if (upper == begin)
        ++upper;
    else
        --lower;
    
    //interpolate the new control point
    ColorMap::Value invWidth = 1.0 / (upper->value - lower->value);
    GLfloat w2 = GLfloat((value - lower->value) * invWidth);
    GLfloat w1 = GLfloat((upper->value - value) * invWidth);
    
    for(int i=0; i<4; ++i)
        color[i] = w1*lower->color[i] + w2*upper->color[i];
    
    return upper;
}

    

const ColorMap::ValueRange ColorMap::ValueRange::
invalid(Math::Constants<Value>::max, -Math::Constants<Value>::max);

ColorMap::ValueRange::
ValueRange(Value min_, Value max_) :
    min(min_), max(max_)
{
}

ColorMap::Point::
Point(Value value_, Color color_) :
    value(value_), color(color_)
{
}


ColorMap::
ColorMap()
{
    *this = greyScale;
}

ColorMap::
ColorMap(int numPoints, const Point* initialPoints)
{
    points.resize(numPoints);
    for (int i=0; i<numPoints; ++i)
        points[i] = initialPoints[i];
}


ColorMap::Points::iterator ColorMap::
interpolate(Point& point)
{
    return interpolateMap(points.begin(), points.end(),
                          point.value, point.color);
}

const ColorMap::Points& ColorMap::
getPoints() const
{
    return points;
}

ColorMap::Points& ColorMap::
getPoints()
{
    return points;
}


ColorMap::ValueRange ColorMap::
getValueRange() const
{
    if (points.size()<2)
        return ValueRange::invalid;
    return ValueRange(points.front().value, points.back().value);
}

void ColorMap::
setValueRange(const ValueRange& newRange)
{
    //silently ignore setting the range of an invalid map
    if (points.size()<2)
        return;

    //remap the control points' values to the new range
    const ValueRange oldRange = getValueRange();
    Value remap = (newRange.max - newRange.min) /
                  (oldRange.max - oldRange.min);

    for(Points::iterator it=points.begin(); it!=points.end(); ++it)
        it->value = newRange.min + (it->value - oldRange.min) * remap;
}


void ColorMap::
discretize(DiscreteMap& discrete) const
{
    //silently ignore exporting an invalid map
    if (points.size()<2)
        return;

    //interpolate all colors in the color array
    int numEntries   = static_cast<int>(discrete.size());
    ValueRange range = getValueRange();
    Value remap      = (range.max - range.min) / Value(numEntries-1);
    for(int i=0; i<numEntries; ++i)
    {
        //calculate the map value for the color array entry
        Value value = range.min + remap*Value(i);
        //interpolate the color
        interpolateMap(points.begin(), points.end(), value, discrete[i]);
    }
}

void ColorMap::
load(const std::string& fileName)
{
    points.clear();
    std::ifstream file(fileName.c_str());

    Point p;
    std::string line;
    while (file)
    {
        getline(file, line);
        if (line[0]!='#' &&
            sscanf(line.c_str(), "%lf %f %f %f %f", &p.value,
                   &p.color[0], &p.color[1], &p.color[2], &p.color[3]) == 5)
        {
            points.push_back(p);
        }
    }

    assert(points.size()>2);
}

void ColorMap::
save(const std::string& fileName) const
{
    std::ofstream file(fileName.c_str());
///\todo proper error message here
    if (!file)
        return;

    for (Points::const_iterator it=points.begin(); it!=points.end(); ++it)
    {
        file << it->value;
        for (int i=0; i<4; ++i)
            file << " " << it->color[i];
        file << std::endl;
    }
    file.close();
}



} //end namespace Misc
