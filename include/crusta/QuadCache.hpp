#include <Math/Constants.h>
#include <Misc/ThrowStdErr.h>

#include <crusta/checkGl.h>


BEGIN_CRUSTA


template <typename BufferParam>
void Main2dCache<BufferParam>::
init(const std::string& iName, int size, int iTileSize)
{
    tileSize = iTileSize;
    CacheUnit<BufferParam>::init(iName, size);
}

template <typename BufferParam>
void Main2dCache<BufferParam>::
initData(typename BufferParam::DataType& data)
{
    data = new typename BufferParam::DataArrayType[tileSize*tileSize];
}



template <typename BufferParam>
Gpu2dAtlasCache<BufferParam>::
~Gpu2dAtlasCache()
{
    glDeleteTextures(1, &texture);
}


template <typename BufferParam>
void Gpu2dAtlasCache<BufferParam>::
init(const std::string& iName, int size, int tileSize,
     GLenum internalFormat, GLenum filterMode)
{
//- compute the 2D-layered layout
    int maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    int maxTextureLayers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &maxTextureLayers);

    int bestNum  = 0;
    int minExtra = Math::Constants<int>::max;
    for (int num=1; num*tileSize<=maxTextureSize; ++num)
    {
        int numPerPlane = num*num;
        int numLayers   = int(Math::ceil(float(size)/float(numPerPlane)));
        if (numLayers > maxTextureLayers)
            continue;
        int extra = numLayers*numPerPlane - size;
        if (extra < minExtra)
        {
            bestNum  = num;
            minExtra = extra;
        }
    }

    if (bestNum == 0)
    {
        Misc::throwStdErr("Gpu2dAtlasCache: could not accommodate cache of "
                          "size %d", size);
    }

    texSize   = bestNum*tileSize;
    texLayers = int(Math::ceil(float(size)/float(bestNum*bestNum)));

    pixSize   = Vector2f(1.0f / texSize, 1.0f / texSize);
    subSize   = Vector2f(tileSize*pixSize[0], tileSize*pixSize[1]);
    subOffset = Point3f(0.0f, 0.0f, 0.0f);

//- initialize the buffers
    size = texLayers*bestNum*bestNum;
    CacheUnit<BufferParam>::init(iName, size);

//- create the texture object storage
    CHECK_GL_CLEAR_ERROR;

    glGenTextures(1, &texture);

    glPushAttrib(GL_TEXTURE_BIT);

    glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, texture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, internalFormat,
                 texSize, texSize, texLayers, 0, GL_RGB, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, filterMode);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, filterMode);

    glPopAttrib();

    CHECK_GL_THROW_ERROR;
}

template <typename BufferParam>
void Gpu2dAtlasCache<BufferParam>::
initData(typename BufferParam::DataType& data)
{
    //loop back (row, layer) if the current subregion is out of bounds
    if (subOffset[0] >= 1.0f)
    {
        //move up a row and reset the column offset
        subOffset[1] += subSize[1];
        subOffset[0]  = 0.0f;
    }
    if (subOffset[1] >= 1.0f)
    {
        //move up a layer and reset the row offset
        subOffset[2] += 1.0f;
        subOffset[1]  = 0.0f;
    }

    //assign the subregion
    data.offset = subOffset;
    data.size   = subSize;

    //move to the next subregion (next column)
    subOffset[0] += subSize[0];
}


template <typename BufferParam>
void Gpu2dAtlasCache<BufferParam>::
bind() const
{
    glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, texture);
}

template <typename BufferParam>
void Gpu2dAtlasCache<BufferParam>::
stream(const SubRegion& sub, GLenum dataFormat, GLenum dataType, void* data)
{
    glPushAttrib(GL_TEXTURE_BIT);

    glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, texture);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, GLint(sub.offset[0]*texSize),
                    GLint(sub.offset[1]*texSize), GLint(sub.offset[2]),
                    GLsizei(sub.size[0]*texSize), GLsizei(sub.size[1]*texSize),
                    1, dataFormat, dataType, data);
    glPopAttrib();

    CHECK_GLA;
}



template <typename BufferParam>
Gpu2dRenderAtlasCache<BufferParam>::
~Gpu2dRenderAtlasCache()
{
    glDeleteFramebuffersEXT(1, &renderFbo);
}


template <typename BufferParam>
void Gpu2dRenderAtlasCache<BufferParam>::
init(const std::string& iName, int size, int tileSize,
     GLenum internalFormat, GLenum filterMode)
{
    Gpu2dAtlasCache<BufferParam>::init(iName, size, tileSize,
                                       internalFormat, filterMode);

    CHECK_GL_CLEAR_ERROR;
    //create the framebuffer to be used to attach and render the coverage maps
    glGenFramebuffers(1, &renderFbo);
    CHECK_GL_THROW_ERROR;
}

template <typename BufferParam>
void Gpu2dRenderAtlasCache<BufferParam>::
beginRender(const SubRegion& sub)
{
///\todo query this from the GL only once per frame, pass info along in glData
    //save the current viewport specification
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    //set the viewport to match the atlas entry
    glViewport(GLint(sub.offset[0]*this->texSize),
               GLint(sub.offset[1]*this->texSize),
               GLsizei(sub.size[0]*this->texSize),
               GLsizei(sub.size[1]*this->texSize));

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);
    glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuf);
    glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);

    //bind the coverage rendering framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, renderFbo);
    //attach the appropriate coverage map
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              this->texture, 0, GLint(sub.offset[2]));
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_NONE);
    CHECK_GLFBA;
}

template <typename BufferParam>
void Gpu2dRenderAtlasCache<BufferParam>::
endRender()
{
    //bind back the old framebuffer and restore buffer read/draw
    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    glDrawBuffer(oldDrawBuf);
    glReadBuffer(oldReadBuf);

    //restore the viewport
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
}


template <typename BufferParam>
void Gpu1dAtlasCache<BufferParam>::
init(const std::string& iName, int size, int tileSize,
     GLenum internalFormat, GLenum filterMode)
{
//- compute the 1+1D layout
    int maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    int bestNum  = 0;
    int minExtra = Math::Constants<int>::max;
    for (int num=1; num*tileSize<=maxTextureSize; ++num)
    {
        int numLayers = int(Math::ceil(float(size)/float(num)));
        if (numLayers > maxTextureSize)
            continue;
        int extra = numLayers*num - size;
        if (extra < minExtra)
        {
            bestNum  = num;
            minExtra = extra;
        }
    }

    if (bestNum == 0)
    {
        Misc::throwStdErr("Gpu1dAtlasCache: could not accommodate cache of "
                          "size %d", size);
    }

    texWidth  = bestNum*tileSize;
    texHeight = int(Math::ceil(float(size)/float(bestNum)));

    pixSize   = Vector2f(1.0f / texWidth, 1.0f / texHeight);
    subSize   = Vector2f(tileSize*pixSize[0], pixSize[1]);
    subOffset = Point3f(0.0f, 0.0f, 0.0f);

//- initialize the buffers
    size = texHeight*bestNum;
    CacheUnit<BufferParam>::init(iName, size);

//- create the texture object storage
    CHECK_GL_CLEAR_ERROR;

    glGenTextures(1, &texture);

    glPushAttrib(GL_TEXTURE_BIT);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texWidth, texHeight, 0,
                 GL_RGB, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);

    glPopAttrib();

    CHECK_GL_THROW_ERROR;
}

template <typename BufferParam>
void Gpu1dAtlasCache<BufferParam>::
initData(typename BufferParam::DataType& data)
{
    //loop back (row) if the current subregion is out of bounds
    if (subOffset[0] >= 1.0f)
    {
        //move up a row and reset the column offset
        subOffset[1] += subSize[1];
        subOffset[0]  = 0.0f;
    }

    //assign the subregion
    data.offset = subOffset;
    data.size   = subSize;

    //move to the next subregion (next column)
    subOffset[0] += subSize[0];
}


template <typename BufferParam>
Gpu1dAtlasCache<BufferParam>::
~Gpu1dAtlasCache()
{
    glDeleteTextures(1, &texture);
}


template <typename BufferParam>
void Gpu1dAtlasCache<BufferParam>::
bind() const
{
    glBindTexture(GL_TEXTURE_2D, texture);
}

template <typename BufferParam>
void Gpu1dAtlasCache<BufferParam>::
stream(const SubRegion& sub, GLenum dataFormat, GLenum dataType, void* data)
{
    CHECK_GL_CLEAR_ERROR;

    glPushAttrib(GL_TEXTURE_BIT);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(
        GL_TEXTURE_2D, 0,
        GLint(sub.offset[0]*texWidth), GLint(sub.offset[1]*texHeight),
        GLsizei(sub.size[0]*texWidth), 1,
        dataFormat, dataType, data);

    glPopAttrib();

    CHECK_GL;
}


END_CRUSTA
