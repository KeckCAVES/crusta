#ifndef _CacheRequest_H_
#define _CacheRequest_H_

#include <vector>

#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

/** information required to process the fetch/generation of data */
class CacheRequest
{
    friend class MainCache;
    
public:
    CacheRequest();
    CacheRequest(float iLod, MainCacheBuffer* iParent, uint8 iChild);

    bool operator ==(const CacheRequest& other) const;
    bool operator <(const CacheRequest& other) const;
    
protected:
    /** lod value used for prioritizing the requests */
    float lod;
    /** pointer to the parent of the requested */
    MainCacheBuffer* parent;
    /** index of the child to be loaded */
    uint8 child;
};

typedef std::vector<CacheRequest> CacheRequests;

END_CRUSTA

#endif //_CacheRequest_H_
