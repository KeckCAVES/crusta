#ifndef _gridProcessing_H_
#define _gridProcessing_H_

#include <vector>

#include <basics.h>
#include <Cache.h>
#include <Scope.h>

class GLContextData;

BEGIN_CRUSTA

namespace gridProcessing {

struct ScopeData;

typedef uint Id;
typedef void (*RegistrationCallbackFunc)(void* object, const Id newId);
typedef void (*ScopePrePostFunc)(void* object, GLContextData& contextData);
typedef void (*ScopeCallbackFunc)(void* object, const ScopeData& scopeData,
                                  GLContextData& contextData);
    
enum Phase
{
    UPDATE_PHASE,
    DISPLAY_PHASE
};

struct ScopeData
{
    Scope* scope;
    Cache::BufferPtrs* data;
};

class RegistrationCallback
{
public:
    RegistrationCallback(void* callObject, RegistrationCallbackFunc callMethod):
        object(callObject), method(callMethod) {}

    void operator()(const Id newId) {
        method(object, newId);
    }
    
protected:
    void* object;
    RegistrationCallbackFunc method;
};

class ScopeCallback {
public:
    ScopeCallback(void* callObject, ScopeCallbackFunc callMethod,
                  ScopePrePostFunc callPreMethod=NULL,
                  ScopePrePostFunc callPostMethod=NULL) :
        object(callObject), preMethod(callPreMethod),
        method(callMethod), postMethod(callPostMethod) {}

    void preTraversal(GLContextData& contextData) {
        if (preMethod != NULL)
            preMethod(object, contextData);
    }
    void traversal(const ScopeData& scopeData, GLContextData& contextData) {
        assert(method!=NULL && "ScopeCallback: calling bogus callback\n");
        method(object, scopeData, contextData);
    }
    void postTraversal(GLContextData& contextData) {
        if (postMethod != NULL)
            postMethod(object, contextData);
    }

protected:
    void* object;
    ScopePrePostFunc  preMethod;
    ScopeCallbackFunc method;
    ScopePrePostFunc  postMethod;
};

typedef std::vector<ScopeCallback> ScopeCallbacks;
    
}

END_CRUSTA

#endif //_gridProcessing_H_
