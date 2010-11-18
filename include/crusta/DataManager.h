#ifndef _DataManager_H_
#define _DataManager_H_

#include <crusta/CrustaComponent.h>
#include <crusta/GlobeFile.h>
#include <crusta/QuadCache.h>


BEGIN_CRUSTA

class Crusta;
class Polyhedron;

class DataManager : public CrustaComponent
{
public:
    typedef GlobeFile<DemHeight>    DemFile;
    typedef GlobeFile<TextureColor> ColorFile;
    
    DataManager(Crusta* iCrusta);
    ~DataManager();

    /** assign the given databases to the data manager */
    void load(const std::string& demPath, const std::string& colorPath);
    /** detach the data manager from the current databases */
    void unload();

    /** check if DEM data is available from the manager */
    bool hasDemData() const;
    /** check if color data is available from the manager */
    bool hasColorData() const;

    /** get the polyhedron that serves as the basis for the managed data */
    const Polyhedron* const getPolyhedron() const;
    /** get the value used to indicate the abscence of height data */
    const DemHeight& getDemNodata();
    /** get the value used to indicate the abscence of color data */
    const TextureColor& getColorNodata();
    
    /** load the root data of a patch */
    void loadRoot(TreeIndex rootIndex, const Scope& scope);
    /** load the data required for the child of the specified node */
    void loadChild(MainCacheBuffer* parent,uint8 which,MainCacheBuffer* child);

protected:
    /** produce the flat sphere cartesian space coordinates for a node */
    void generateGeometry(QuadNodeMainData& child);
    /** source the elevation data for a node */
    void sourceDem(QuadNodeMainData* parent, QuadNodeMainData& child);
    /** source the color data for a node */
    void sourceColor(QuadNodeMainData* parent, QuadNodeMainData& child);

    /** globe file from which to source data for the elevation */
    DemFile* demFile;
    /** globe file from which to source data for the color */
    ColorFile* colorFile;
    /** polyhedron serving as the basis for the managed data */
    Polyhedron* polyhedron;

    /** value for "no-data" elevations */
    DemHeight demNodata;
    /** value for "no-data" colors */
    TextureColor colorNodata;

    /** temporary storage for computing the high-precision surface geometry */
    double* geometryBuf;
};

END_CRUSTA

#endif //_DataManager_H_
