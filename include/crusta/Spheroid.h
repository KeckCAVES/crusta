#ifndef _Spheroid_H_
#define _Spheroid_H_

#include <crusta/basics.h>
#include <crusta/QuadTerrain.h>

BEGIN_CRUSTA

class Spheroid
{
public:
    Spheroid(const std::string& demBase, const std::string& colorBase);
    ~Spheroid();

    /** process a display update */
    void display(GLContextData& contextData) const;

protected:
    typedef std::vector<QuadTerrain*> TerrainPtrs;

    TerrainPtrs basePatches;
};

END_CRUSTA

#endif //_Spheroid_H_
