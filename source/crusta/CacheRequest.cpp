#include <crusta/CacheRequest.h>

BEGIN_CRUSTA

CacheRequest::
CacheRequest() :
    lod(0), parent(NULL), child(~0)
{}

CacheRequest::
CacheRequest(float iLod, MainCacheBuffer* iParent, uint8 iChild) :
    lod(iLod), parent(iParent), child(iChild)
{}

bool CacheRequest::
operator ==(const CacheRequest& other) const
{
/**\todo there is some danger here. Double check that I'm really only discarding
components of the request that are allowed to be different. So far, since
everything is fetched on a node basis the index is all that is needed to
coalesce requests */
    return parent->getData().index == other.parent->getData().index &&
           child == other.child;
}
bool CacheRequest::
operator <(const CacheRequest& other) const
{
    return Math::abs(lod) < Math::abs(other.lod);
}

END_CRUSTA
