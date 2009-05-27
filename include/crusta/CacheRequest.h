#ifndef _CacheRequest_H_
#define _CacheRequest_H_

#include <vector>

#include <crusta/QuadtreeFileSpecs.h>
#include <crusta/Scope.h>

BEGIN_CRUSTA

class Node;

/** information required to process the fetch/generation of data */
class CacheRequest
{
    friend class MainCache;
    
public:
    CacheRequest();
    CacheRequest(float iLod, Node* iNode);

    bool operator ==(const CacheRequest& other) const;
    bool operator <(const CacheRequest& other) const;
    
protected:
    /** lod value used for prioritizing the requests */
    float lod;
    /** pointer to the node for which the request was made */
    Node* node;
};

typedef std::vector<CacheRequest> CacheRequests;

END_CRUSTA

#endif //_CacheRequest_H_
