#ifndef _ShaderFragment_H_
#define _ShaderFragment_H_


#include <algorithm>
#include <vector>

#include <crusta/basics.h>
#include <crusta/QuadNodeData.h> // for SubRegion


BEGIN_CRUSTA


///\todo comment!!
class ShaderFragment
{
public:
    ShaderFragment();
    virtual ~ShaderFragment();

    virtual bool update();

    virtual std::string getCode();
///\todo deprecate the individual gets
    virtual std::string getUniforms();
    virtual std::string getFunctions();

    virtual void initUniforms(GLuint programObj);
    virtual void reset();

protected:
    bool uniformsEmitted;
    bool functionsEmitted;

    std::string makeUniqueName(const std::string& baseName) const;

private:
///\todo use IdGenerator?
    static int nextSequenceNumber; // to generate unique uniform names
    int sequenceNumber;
};


END_CRUSTA


#endif //_ShaderFragment_H_
