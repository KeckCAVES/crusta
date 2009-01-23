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
    ScopeCallback(void* callObject, ScopeCallbackFunc callMethod) :
        object(callObject), method(callMethod) {}

    void operator()(const ScopeData& scopeData) {
        method(object, scopeData);
    }

protected:
    void* object;
    ScopeCallbackFunc method;    
};

typedef std::vector<ScopeCallback> ScopeCallbacks;
    
}

END_CRUSTA

#endif //_gridProcessing_H_
