#ifndef _Crusta_H_
#define _Crusta_H_

#include <crusta/Spheroid.h>

class GLContextData;

BEGIN_CRUSTA

/** Main crusta class */
class Crusta
{
protected:
    static double verticalScale;
    Spheroid spheroid;

public:
    Crusta(const std::string& demFileBase, const std::string& colorFileBase);

    /** set the vertical exaggeration. Make sure to set this value within a 
        frame callback so that it doesn't change during a rendering phase */
    static void setVerticalScale(double newVerticalScale);
    ///retrieve the vertical exaggeration factor
    static double getVerticalScale();

    void frame();
    void display(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_Crusta_H_
