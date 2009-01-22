#ifndef _gridProcessing_H_
#define _gridProcessing_H_

#include <vector>

#include <basics.h>
#include <Scope.h>

BEGIN_CRUSTA

namespace gridProcessing {

struct ScopeData;

typedef uint Id;
typedef void (*RegistrationCallbackFunc)(void* object, const Id newId);
typedef void (*ScopeCallbackFunc)(void* object, const ScopeData& scopeData);
    
enum Phase
{
    UPDATE_PHASE,
    DISPLAY_PHASE
};

struct ScopeData
{
    Scope scope;
};

struct RegistrationCallback
{
    void* object;
    RegistrationCallbackFunc method;
    
    RegistrationCallback(void* object, RegistrationCallbackFunc method) {
        this->object = object; this->method = method;
    }
    void operator()(const Id newId) {
        method(object, newId);
    }
};

struct ScopeCallback {
    void* object;
    ScopeCallbackFunc method;

    ScopeCallback(void* object, ScopeCallbackFunc method) {
        this->object = object; this->method = method;
    }
    void operator()(const ScopeData& scopeData) {
        method(object, scopeData);
    }
};

typedef std::vector<ScopeCallback> ScopeCallbacks;
    
}

END_CRUSTA

#endif //_gridProcessing_H_
