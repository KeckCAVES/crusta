#ifndef _Crusta_H_
#define _Crusta_H_

#include <crusta/Spheroid.h>

class GLContextData;

BEGIN_CRUSTA

/** Main crusta class */
class Crusta
{
protected:
    static double verticalExaggeration;
    Spheroid spheroid;

public:
    Crusta(const std::string& demFileBase, const std::string& colorFileBase);

    ///retrieve the vertical exaggeration factor
    static double getVerticalExaggeration() const;

    void frame();
    void display(GLContextData& contextData) const;
};

END_CRUSTA

#endif //_Crusta_H_
