#ifndef _Crusta_ShaderMultiDataSource_H_
#define _Crusta_ShaderMultiDataSource_H_


#include <crusta/shader/ShaderDataSource.h>


namespace crusta {


class ShaderMultiDataSource : public ShaderDataSource
{
public:
    void clear();
    void addSource(ShaderDataSource* src);

protected:
    std::vector<ShaderDataSource*> sources;

//- inherited from ShaderFragment
public:
    virtual void reset();
    virtual void initUniforms(GLuint programObj);
    virtual bool update();
    virtual std::string getCode();
};


} //namespace crusta


#endif //_Crusta_ShaderMultiDataSource_H_
