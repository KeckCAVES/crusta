#ifndef _DataManager_H_
#define _DataManager_H_

#include <crusta/CrustaComponent.h>
#include <crusta/QuadtreeFileSpecs.h>
#include <crusta/QuadCache.h>


BEGIN_CRUSTA

class Crusta;
class Polyhedron;

class DataManager : public CrustaComponent
{
public:
    typedef std::vector<DemFile*>   DemFiles;
    typedef std::vector<ColorFile*> ColorFiles;

    DataManager(Polyhedron* polyhedron, const std::string& demBase,
                const std::string& colorBase, Crusta* iCrusta);
    ~DataManager();

    /** check if DEM data is available from the manager */
    bool hasDemData() const;
    /** check if color data is available from the manager */
    bool hasColorData() const;

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

    /** quadtree file from which to source data for the elevation */
    DemFiles demFiles;
    /** quadtree file from which to source data for the color */
    ColorFiles colorFiles;

    /** value for "no-data" elevations */
    DemHeight demNodata;
    /** value for "no-data" colors */
    TextureColor colorNodata;

    /** temporary storage for computing the high-precision surface geometry */
    double* geometryBuf;
};

END_CRUSTA

#endif //_DataManager_H_
