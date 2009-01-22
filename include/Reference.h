#ifndef _Reference_H_
#define _Reference_H_

#include <basics.h>

BEGIN_CRUSTA

template<typename T>
class Reference
{
public:
    Reference() { object = NULL; }
    Reference(T*& target) { object = &target; }

    bool isValid() { return object!=NULL ? *object!=NULL : false; }
    T* operator->() { return *object; }
    
protected:
    T** object;
};

END_CRUSTA

#endif //_Reference_H_
