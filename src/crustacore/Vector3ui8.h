#ifndef _Vector3ui8_H_
#define _Vector3ui8_H_


#include <Geometry/Vector.h>

#include <crustacore/basics.h>


BEGIN_CRUSTA


typedef Geometry::Vector<uint8, 3> Vector3ui8;


inline
Vector3ui8 operator+(const Vector3ui8& tc, double d)
{
    return Vector3ui8(tc[0]+d, tc[1]+d, tc[2]+d);
}
inline
Vector3ui8 operator+(double d, const Vector3ui8& tc)
{
    return tc + d;
}

inline
Vector3ui8 operator*(const Vector3ui8& tc, double d)
{
    return Vector3ui8(tc[0]*d, tc[1]*d, tc[2]*d);
}
inline
Vector3ui8 operator*(double d, const Vector3ui8& tc)
{
    return tc * d;
}

inline
std::ostream& operator<<(std::ostream& oss, Vector3ui8& tc)
{
    /* uint8 is seen as a char not an integer; must, therefore, cast to int
       otherwise '\0' will be generated */
    for (int i=0; i<Vector3ui8::dimension-1; ++i)
        oss << (int)tc[i] << " ";
    oss << (int)tc[Vector3ui8::dimension-1];
    return oss;
}

inline
std::istream& operator>>(std::istream& iss, Vector3ui8& tc)
{
    /* uint8 is seen as a char not an integer; must, therefore, buffer to int
       otherwise 48 (ascii number for '0') will be generated */
    int temp;
    for (int i=0; i<Vector3ui8::dimension; ++i)
    {
        iss >> temp;
        tc[i] = temp;
    }
    return iss;
}


END_CRUSTA


#endif //_Vector3ui8_H_
