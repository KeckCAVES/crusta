#ifndef _Misc_ColorMap_H_
#define _Misc_ColorMap_H_


#include <string>
#include <vector>

#include <crustavrui/vrui.h>


namespace Misc {


class ColorMap
{
public:
    typedef GLColor<GLfloat,4> Color;
    typedef double             Value;
    typedef std::vector<Color> DiscreteMap;

    struct ValueRange
    {
        static const ValueRange invalid;

        ValueRange(Value min_=invalid.min, Value max_=invalid.max);
        Value min;
        Value max;
    };

    struct Point
    {
        Point(Value value_=0, Color color_=Color());
        Value value;
        Color color;
    };
    typedef std::vector<Point> Points;

    ColorMap();
    ColorMap(int numPoints, const Point* initialPoints);

    Points::iterator interpolate(Point& point);

    const Points& getPoints() const;
    Points& getPoints();

    ValueRange getValueRange() const;
    void setValueRange(const ValueRange& newRange);

    void discretize(DiscreteMap& discrete) const;

    void load(const std::string& fileName);
    void save(const std::string& fileName) const;

    static const ColorMap black;
    static const ColorMap greyScale;
    static const ColorMap white;

protected:
    Points points;
};


} //end namespace Misc


#endif //_Misc_ColorMap_H_
