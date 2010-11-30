#include <Misc/ColorMap.h>


#include <algorithm>

#include <GL/GLColorMap.h>
#include <Math/Constants.h>


namespace Misc {


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


void ColorMap::
clear()
{
    points.clear();
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


struct PointValueLessThan
{
    bool operator() (const ColorMap::Point& left, const ColorMap::Value& right)
    { return left.value < right; }
    bool operator() (const ColorMap::Value& left, const ColorMap::Point& right)
    { return left < right.value; }
};

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

        //find the two control points on either side of the value
        Points::const_iterator upper = std::lower_bound(
            points.begin(), points.end(), value, PointValueLessThan());
        Points::const_iterator lower = upper;
        if (upper == points.begin())
            ++upper;
        else
            --lower;

        //interpolate the new control point
        Value invWidth = 1.0 / (upper->value - lower->value);
        GLfloat w2 = GLfloat((value - lower->value) * invWidth);
        GLfloat w1 = GLfloat((upper->value - value) * invWidth);

        for(int j=0; j<4; ++j)
            discrete[i][j] = w1*lower->color[j] + w2*upper->color[j];
    }
}



} //end namespace Misc
