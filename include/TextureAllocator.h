#ifndef _TextureAllocator_H_
#define _TextureAllocator_H_

#include <Cache.h>

#include <GL/gl.h>

BEGIN_CRUSTA

class TextureAllocator : public Cache::Allocator
{
//- inherited from Cache::Allocator
public:
    TextureAllocator(uint texBitDepth, GLint texInternalFormat,
                     GLsizei texWidth, GLsizei texHeight,
                     GLint texMinificationFilter, GLint texMagnificationFilter);
    
    virtual uint getAllocationSize();
    virtual Cache::Buffer::DataHandle allocate();
    virtual void deallocate(Cache::Buffer::DataHandle& data);
    
protected:
    uint    bitDepth;
    GLint   internalFormat;
    GLsizei width;
    GLsizei height;
    GLint   minificationFilter;
    GLint   magnificationFilter;
};

END_CRUSTA

#endif //_TextureAllocator_H_
