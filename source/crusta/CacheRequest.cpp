#include <crusta/CacheRequest.h>

#include <crusta/Node.h>

BEGIN_CRUSTA

CacheRequest::
CacheRequest() :
    lod(0), node(NULL)
{}

CacheRequest::
CacheRequest(float iLod, Node* iNode) :
    lod(iLod), node(iNode)
{}

bool CacheRequest::
operator ==(const CacheRequest& other) const
{
/**\todo there is some danger here. Double check that I'm really only discarding
components of the request that are allowed to be different. So far, since
everything is fetched on a node basis the index is all that is needed to
coalesce requests */
    return node == other.node;
}
bool CacheRequest::
operator <(const CacheRequest& other) const
{
    return Math::abs(lod) < Math::abs(other.lod);
}

END_CRUSTA
