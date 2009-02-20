#include <TextureAllocator.h>

BEGIN_CRUSTA

TextureAllocator::
TextureAllocator(uint texBitDepth, GLint texInternalFormat, GLsizei texWidth,
                 GLsizei texHeight, GLint texMinificationFilter,
                 GLint texMagnificationFilter) :
    bitDepth(texBitDepth), internalFormat(texInternalFormat), width(texWidth),
    height(texHeight), minificationFilter(texMinificationFilter),
    magnificationFilter(texMagnificationFilter)
{}

uint TextureAllocator::
getAllocationSize()
{
    return width*height*bitDepth;
}

Cache::Buffer::DataHandle TextureAllocator::
allocate()
{
    Cache::Buffer::DataHandle retHandle;
    glGenTextures(1, reinterpret_cast<GLuint*>(&retHandle.id));
    
    glBindTexture(GL_TEXTURE_2D, retHandle.id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                 GL_RGB, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minificationFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magnificationFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return retHandle;
}

void TextureAllocator::
deallocate(Cache::Buffer::DataHandle& data)
{
    glDeleteTextures(1, reinterpret_cast<GLuint*>(&data.id));
    data.id = 0;
}


END_CRUSTA
