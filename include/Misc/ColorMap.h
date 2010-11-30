#ifndef _Misc_ColorMap_H_
#define _Misc_ColorMap_H_


#include <vector>

#include <GL/GLColor.h>


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
    
    void clear();

    const Points& getPoints() const;
    Points& getPoints();
    
    ValueRange getValueRange() const;
    void setValueRange(const ValueRange& newRange);
    
    void discretize(DiscreteMap& discrete) const;
    
protected:
    Points points;
};


} //end namespace Misc


#endif //_Misc_ColorMap_H_
