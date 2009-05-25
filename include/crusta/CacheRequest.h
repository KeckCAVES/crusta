#ifndef _CacheRequest_H_
#define _CacheRequest_H_

#include <vector>

#include <crusta/QuadtreeFileSpecs.h>
#include <crusta/Scope.h>

BEGIN_CRUSTA

/** information required to process the fetch/generation of data */
class CacheRequest
{
    friend class MainCache;
    
public:
    CacheRequest();
    CacheRequest(float iLod, const TreeIndex& iIndex, const Scope& iScope,
                 DemFile* fileDem, DemFile::TileIndex tileDem);

    bool operator ==(const CacheRequest& other) const;
    bool operator <(const CacheRequest& other) const;
    
protected:
    /** lod value used for prioritizing the requests */
    float lod;
    /** index of the node */
    TreeIndex index;
    /** scope of the node requested */
    Scope scope;
    
    /** quadtree file from which to source the elevation data */
    DemFile* demFile;
    /** index of the tile that contains the elevation data to load */
    DemFile::TileIndex demTile;
    
    //- flags to manage lazy loading
    bool isHeightLoaded;
    bool isColorLoaded;
};

typedef std::vector<CacheRequest> CacheRequests;

END_CRUSTA

#endif //_CacheRequest_H_
