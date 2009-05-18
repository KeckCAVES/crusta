#include <crusta/CacheRequest.h>

BEGIN_CRUSTA

CacheRequest::
CacheRequest() :
    lod(0), index(~0,~0,~0,~0), demFile(NULL),
    demTile(DemFile::INVALID_TILEINDEX), isHeightLoaded(false),
    isColorLoaded(false)
{}

CacheRequest::
CacheRequest(float iLod, const TreeIndex& iIndex, const Scope& iScope,
             DemFile* fileDem, DemFile::TileIndex tileDem) :
    lod(iLod), index(iIndex), scope(iScope), demFile(fileDem),
    demTile(tileDem), isHeightLoaded(false), isColorLoaded(false)
{}

bool CacheRequest::
bool operator ==(const CacheRequest& other) const
{
    return index.index == other.index.index;
}
bool CacheRequest::
operator <(const CacheRequest& other) const
{
    return fabs(lod) < fabs(other.lod);
}

END_CRUSTA
