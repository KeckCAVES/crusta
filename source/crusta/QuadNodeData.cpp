#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

QuadNodeMainData::
QuadNodeMainData(uint size)
{
    geometry = new Vertex[size*size];
    height   = new float[size*size];
    color    = new uint8[size*size*3];
}
QuadNodeMainData::
~QuadNodeMainData()
{
    delete geometry;
    delete height;
    delete color;
}

QuadNodeVideoData::
QuadNodeVideoData(uint size)
{
    createTexture(geometry, GL_RGB32F_ARB, size);
    createTexture(height, GL_INTENSITY32F_ARB, size);
    createTexture(color, GL_RGB, size);
}
QuadNodeVideoData::
~QuadNodeVideoData()
{
    glDeleteTextures(1, &geometry);
    glDeleteTextures(1, &height);
    glDeleteTextures(1, &color);
}

void QuadNodeVideoData::
createTexture(GLuint& texture, GLint internalFormat, uint size)
{
    glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size, size, 0,
                 GL_RGB, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

template <typename NodeDataType>
class CacheBuffer
{
public:
    CacheBuffer(uint size);

    /** retrieve the main memory node data from the buffer */
    const NodeDataType& getData() const;
    /** confirm use of the buffer for the current frame */
    void touch();
    /** pin the element in the cache such that it cannot be swaped out */
    void pin(bool wantPinned=true);
    /** query the frame number of the buffer */
    uint getFrameNumber() const;
    
protected:
    /** sequence number used to evaluate LRU prioritization */
    uint frameNumber;
    /** the actual node data */
    NodeDataType data;
};

typedef CacheBuffer<QuadNodeMainData>  MainCacheBuffer;
typedef CacheBuffer<QuadNodeVideoData> VideoCacheBuffer;

END_CRUSTA

#include <crusta/QuadNodeData.hpp>
