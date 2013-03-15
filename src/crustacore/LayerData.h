#ifndef _LayerData_H_
#define _LayerData_H_


#include <string>

#include <crustacore/basics.h>


namespace crusta {


/// serves as a container for the semantics of a data layer value
template <typename TypeParam>
struct LayerData
{
    /// data type for values of the layer stored in the preprocessed hierarchies
    typedef TypeParam Type;

    /// get the default nodata value based on the type
    static float defaultNodata();
    /// suffix for the layer data type
    static std::string suffix();
};


template <>
struct LayerData<float>
{
    typedef float Type;

    static float defaultNodata()
    {
        return -4.294967296e+9;
    }
    static std::string suffix()
    {
        return std::string("f");
    }
};


typedef LayerData<float> LayerDataf;


} //namespace crusta


#endif //_LayerData_H_
