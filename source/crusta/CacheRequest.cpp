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
operator ==(const CacheRequest& other) const
{
/**\todo there is some danger here. Double check that I'm really only discarding
         components of the request that are allowed to be different. So far,
         since everything is fetched on a node basis the index is all that is
         needed to coalesce requests */
    return index == other.index;
}
bool CacheRequest::
operator <(const CacheRequest& other) const
{
    return Math::abs(lod) < Math::abs(other.lod);
}

END_CRUSTA
