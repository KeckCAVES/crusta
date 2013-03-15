#ifndef _Vector3ui8_H_
#define _Vector3ui8_H_


#include <Geometry/Vector.h>

#include <crustacore/basics.h>


namespace crusta {


inline
Geometry::Vector<uint8_t,3> operator+(const Geometry::Vector<uint8_t,3>& tc, double d)
{
    return Geometry::Vector<uint8_t,3>(tc[0]+d, tc[1]+d, tc[2]+d);
}
inline
Geometry::Vector<uint8_t,3> operator+(double d, const Geometry::Vector<uint8_t,3>& tc)
{
    return tc + d;
}

inline
Geometry::Vector<uint8_t,3> operator*(const Geometry::Vector<uint8_t,3>& tc, double d)
{
    return Geometry::Vector<uint8_t,3>(tc[0]*d, tc[1]*d, tc[2]*d);
}
inline
Geometry::Vector<uint8_t,3> operator*(double d, const Geometry::Vector<uint8_t,3>& tc)
{
    return tc * d;
}

inline
std::ostream& operator<<(std::ostream& oss, Geometry::Vector<uint8_t,3>& tc)
{
    /* uint8_t is seen as a char not an integer; must, therefore, cast to int
       otherwise '\0' will be generated */
    for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension-1; ++i)
        oss << (int)tc[i] << " ";
    oss << (int)tc[Geometry::Vector<uint8_t,3>::dimension-1];
    return oss;
}

inline
std::istream& operator>>(std::istream& iss, Geometry::Vector<uint8_t,3>& tc)
{
    /* uint8_t is seen as a char not an integer; must, therefore, buffer to int
       otherwise 48 (ascii number for '0') will be generated */
    int temp;
    for (int i=0; i<Geometry::Vector<uint8_t,3>::dimension; ++i)
    {
        iss >> temp;
        tc[i] = temp;
    }
    return iss;
}


} //namespace crusta


#endif //_Vector3ui8_H_
